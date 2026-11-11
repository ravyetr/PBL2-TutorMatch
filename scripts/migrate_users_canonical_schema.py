#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Production Migration Script: users.dat Canonical Schema Migration
Migrates users.dat by removing obsolete targetSubject and maximumTuition from Student profiles.
Ensures zero legacy records remain in active production users.dat.
"""

import os
import sys
import shutil
import hashlib
import tempfile

sys.stdout.reconfigure(encoding='utf-8')

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_DIR = os.path.join(PROJECT_DIR, 'data')
USERS_DAT = os.path.join(DATA_DIR, 'users.dat')
BACKUP_DIR = os.path.join(DATA_DIR, 'archive', 'legacy_users')
BACKUP_DAT = os.path.join(BACKUP_DIR, 'users_legacy_11fields_pre_migration.dat')
BUILD_USERS_DAT = os.path.join(PROJECT_DIR, 'build', 'Desktop_Qt_6_11_2_MinGW_64_bit_Debug', 'data', 'users.dat')

def compute_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def inspect_file(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath, 'rb') as f:
        raw = f.read()
    sha = hashlib.sha256(raw).hexdigest()
    lines = [l.strip() for l in raw.decode('utf-8').splitlines() if l.strip()]
    counts = {'ADMIN': 0, 'TUTOR': 0, 'STUDENT': 0}
    token_lens = {'ADMIN': set(), 'TUTOR': set(), 'STUDENT': set()}
    for line in lines:
        parts = line.split(',')
        role = parts[5]
        counts[role] = counts.get(role, 0) + 1
        token_lens[role].add(len(parts))
    return {
        'path': filepath,
        'size': len(raw),
        'sha256': sha,
        'total': len(lines),
        'counts': counts,
        'token_lens': token_lens,
        'lines': lines
    }

def run_migration():
    print("=" * 70)
    print(" TUTORMATCH: COMPLETE users.dat MIGRATION & AUDIT")
    print("=" * 70)

    # 1. Check backup
    os.makedirs(BACKUP_DIR, exist_ok=True)
    if not os.path.exists(BACKUP_DAT):
        print(f"[PRE-FLIGHT] Creating verified backup at: {BACKUP_DAT}")
        shutil.copy2(USERS_DAT, BACKUP_DAT)
    else:
        print(f"[PRE-FLIGHT] Verified backup already exists at: {BACKUP_DAT}")

    backup_info = inspect_file(BACKUP_DAT)
    print(f"  Backup SHA256: {backup_info['sha256']}")
    print(f"  Backup Size  : {backup_info['size']} bytes")
    print(f"  Backup Records: {backup_info['total']} (Admin: {backup_info['counts']['ADMIN']}, Tutor: {backup_info['counts']['TUTOR']}, Student: {backup_info['counts']['STUDENT']})")
    print(f"  Backup Token Lengths: {backup_info['token_lens']}")

    # 2. Parse legacy backup lines and generate canonical 9-field student lines
    migrated_lines = []
    migrated_count = 0
    student_count = 0
    tutor_count = 0
    admin_count = 0
    failed_count = 0

    seen_ids = set()

    for idx, line in enumerate(backup_info['lines']):
        tokens = line.split(',')
        if len(tokens) < 7:
            print(f"[ERROR] Malformed line {idx+1}: {line}")
            failed_count += 1
            continue

        u_id = int(tokens[0])
        if u_id in seen_ids:
            print(f"[ERROR] Duplicate user ID {u_id} at line {idx+1}")
            failed_count += 1
            continue
        seen_ids.add(u_id)

        role = tokens[5]
        if role == 'ADMIN':
            assert len(tokens) == 8, f"Admin record must have 8 fields, got {len(tokens)}"
            migrated_lines.append(line)
            admin_count += 1
            migrated_count += 1
        elif role == 'TUTOR':
            assert len(tokens) == 15, f"Tutor record must have 15 fields, got {len(tokens)}"
            migrated_lines.append(line)
            tutor_count += 1
            migrated_count += 1
        elif role == 'STUDENT':
            # Legacy: id, user, pass, email, name, role, active, grade, prefSubj, maxBudget, mask (11 fields)
            # Canonical: id, user, pass, email, name, role, active, grade, mask (9 fields)
            if len(tokens) == 11:
                # Remove tokens[8] (prefSubj) and tokens[9] (maxBudget)
                canonical_line = f"{tokens[0]},{tokens[1]},{tokens[2]},{tokens[3]},{tokens[4]},{tokens[5]},{tokens[6]},{tokens[7]},{tokens[10]}"
            elif len(tokens) == 9:
                canonical_line = line
            else:
                print(f"[ERROR] Unrecognized Student format ({len(tokens)} fields) at line {idx+1}: {line}")
                failed_count += 1
                continue
            migrated_lines.append(canonical_line)
            student_count += 1
            migrated_count += 1
        else:
            print(f"[ERROR] Unknown role '{role}' at line {idx+1}")
            failed_count += 1

    if failed_count > 0:
        print(f"[ABORT] Migration aborted due to {failed_count} errors. Original file untouched.")
        sys.exit(1)

    print(f"[TRANSFORM] Successfully converted {migrated_count} records (Admin: {admin_count}, Tutor: {tutor_count}, Student: {student_count}).")

    # 3. Write to temporary file first (atomic replacement guarantee)
    temp_fd, temp_path = tempfile.mkstemp(prefix='users_migrated_', suffix='.dat', dir=DATA_DIR)
    try:
        with os.fdopen(temp_fd, 'wb') as tf:
            content_bytes = ('\n'.join(migrated_lines) + '\n').encode('utf-8')
            tf.write(content_bytes)

        # 4. Validate temporary file
        temp_info = inspect_file(temp_path)
        assert temp_info['total'] == backup_info['total'], f"Record count mismatch: temp {temp_info['total']} vs orig {backup_info['total']}"
        assert temp_info['counts'] == backup_info['counts'], f"Role counts mismatch: temp {temp_info['counts']} vs orig {backup_info['counts']}"
        assert temp_info['token_lens']['ADMIN'] == {8}, "Admin token length error"
        assert temp_info['token_lens']['TUTOR'] == {15}, "Tutor token length error"
        assert temp_info['token_lens']['STUDENT'] == {9}, "Student token length must be exactly 9 (canonical)"

        # 5. Atomically replace production file
        shutil.move(temp_path, USERS_DAT)
        print(f"[SAFE REPLACE] Successfully replaced production file: {USERS_DAT}")

        # Sync to build dir if exists
        if os.path.exists(os.path.dirname(BUILD_USERS_DAT)):
            shutil.copy2(USERS_DAT, BUILD_USERS_DAT)
            print(f"[SYNC] Synced migrated data to build directory: {BUILD_USERS_DAT}")

    except Exception as e:
        if os.path.exists(temp_path):
            os.remove(temp_path)
        print(f"[ERROR] Exception during migration: {e}")
        # Rollback from backup if needed
        shutil.copy2(BACKUP_DAT, USERS_DAT)
        print(f"[ROLLBACK] Restored original users.dat from backup.")
        sys.exit(1)

    # 6. Post-migration verification
    prod_info = inspect_file(USERS_DAT)
    print("\n" + "=" * 70)
    print(" POST-MIGRATION INDEPENDENT VERIFICATION")
    print("=" * 70)
    print(f"Production File       : {prod_info['path']}")
    print(f"File Size (Original)  : {backup_info['size']} bytes")
    print(f"File Size (Migrated)  : {prod_info['size']} bytes (Δ = {prod_info['size'] - backup_info['size']} bytes)")
    print(f"SHA-256 (Original)    : {backup_info['sha256']}")
    print(f"SHA-256 (Migrated)    : {prod_info['sha256']}")
    print(f"Total Record Count    : {prod_info['total']} (Original: {backup_info['total']}) -> EXACT MATCH")
    print(f"  - Admin Records     : {prod_info['counts']['ADMIN']} (Tokens: {list(prod_info['token_lens']['ADMIN'])[0]})")
    print(f"  - Tutor Records     : {prod_info['counts']['TUTOR']} (Tokens: {list(prod_info['token_lens']['TUTOR'])[0]})")
    print(f"  - Student Records   : {prod_info['counts']['STUDENT']} (Tokens: {list(prod_info['token_lens']['STUDENT'])[0]})")
    print(f"Legacy Records Left   : 0 (ZERO legacy 11-field records remaining)")
    print(f"Failed / Skipped      : 0")

    # Verify ID sequence and individual fields
    prod_ids = []
    for idx, (orig_line, new_line) in enumerate(zip(backup_info['lines'], prod_info['lines'])):
        o_tokens = orig_line.split(',')
        n_tokens = new_line.split(',')
        # ID, username, passwordHash, email, fullName, role, isActive
        for f in range(7):
            assert o_tokens[f] == n_tokens[f], f"Mismatch at record {idx+1} field {f}: orig '{o_tokens[f]}' != new '{n_tokens[f]}'"
        prod_ids.append(int(n_tokens[0]))

        role = n_tokens[5]
        if role == 'STUDENT':
            assert len(n_tokens) == 9, f"Student record {idx+1} has {len(n_tokens)} fields instead of 9"
            assert o_tokens[7] == n_tokens[7], "Grade level mismatch"
            assert o_tokens[10] == n_tokens[8], "Schedule bitmask mismatch"
        elif role == 'TUTOR':
            assert len(n_tokens) == 15
            for f in range(7, 15):
                assert o_tokens[f] == n_tokens[f]
        elif role == 'ADMIN':
            assert len(n_tokens) == 8
            assert o_tokens[7] == n_tokens[7]

    assert len(prod_ids) == len(set(prod_ids)) == 50, "IDs must be unique and count 50"
    assert prod_ids == list(range(1, 51)), "IDs must be consecutive 1..50"

    print("Field-by-Field Parity : 100% VERIFIED (All IDs, credentials, names, bitmasks preserved)")
    print("Acceptance Condition  : FULLY SATISFIED")
    print("=" * 70)

if __name__ == '__main__':
    run_migration()
