import sys
import os
import shutil
import hashlib
import datetime
import argparse

sys.stdout.reconfigure(encoding='utf-8')

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_DIR = os.path.join(PROJECT_DIR, 'data')

def compute_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest().upper()

def day_of_week(y, m, d):
    # Sakamoto's algorithm normalized to 0=Mon .. 6=Sun
    t = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4]
    if m < 3:
        y -= 1
    dow = (y + y // 4 - y // 100 + y // 400 + t[m - 1] + d) % 7
    return (dow + 6) % 7

def parse_iso_date(date_str):
    parts = date_str.split('-')
    if len(parts) != 3:
        return None
    try:
        y, m, d = int(parts[0]), int(parts[1]), int(parts[2])
        dt = datetime.date(y, m, d)
        return dt
    except ValueError:
        return None

def audit_dataset(dir_path):
    print(f"\n============================================================")
    print(f" AUDITING DATASET IN: {dir_path}")
    print(f"============================================================")

    users_file = os.path.join(dir_path, 'users.dat')
    bookings_file = os.path.join(dir_path, 'bookings.dat')
    courses_file = os.path.join(dir_path, 'course_bookings.dat')
    reviews_file = os.path.join(dir_path, 'reviews.dat')

    errors = []
    warnings = []

    # 1. Audit users.dat
    users = {}
    if not os.path.exists(users_file):
        errors.append(f"users.dat not found in {dir_path}")
    else:
        with open(users_file, 'r', encoding='utf-8') as f:
            for line_no, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                tokens = line.split(',')
                if len(tokens) < 7:
                    errors.append(f"users.dat line {line_no}: requires at least 7 fields, got {len(tokens)}")
                    continue
                try:
                    uid = int(tokens[0])
                except ValueError:
                    errors.append(f"users.dat line {line_no}: invalid user id {tokens[0]}")
                    continue

                if uid in users:
                    errors.append(f"users.dat line {line_no}: duplicate user id {uid}")

                role = tokens[5]
                users[uid] = {
                    'id': uid,
                    'username': tokens[1],
                    'email': tokens[3],
                    'fullName': tokens[4],
                    'role': role,
                    'isActive': tokens[6] == '1'
                }

                if role == 'TUTOR':
                    if len(tokens) < 15:
                        errors.append(f"users.dat line {line_no}: Tutor requires 15 fields, got {len(tokens)}")
                    else:
                        users[uid]['bio'] = tokens[7]
                        users[uid]['subject'] = tokens[8]
                        users[uid]['hourlyRate'] = float(tokens[9])
                        users[uid]['rating'] = float(tokens[10])
                        users[uid]['totalReviews'] = int(tokens[11])
                        users[uid]['isVerified'] = tokens[12] == '1'
                        users[uid]['totalEarnings'] = float(tokens[13])
                        try:
                            bm = int(tokens[14])
                            if bm >= (1 << 56):
                                errors.append(f"users.dat line {line_no}: scheduleBitmask exceeds 56 bits")
                            users[uid]['scheduleBitmask'] = bm
                        except ValueError:
                            errors.append(f"users.dat line {line_no}: invalid scheduleBitmask {tokens[14]}")
                elif role == 'STUDENT':
                    if len(tokens) < 11:
                        errors.append(f"users.dat line {line_no}: Student requires 11 fields, got {len(tokens)}")
                    else:
                        users[uid]['gradeLevel'] = tokens[7]
                        users[uid]['school'] = tokens[8]
                        users[uid]['walletBalance'] = float(tokens[9])
                        try:
                            bm = int(tokens[10])
                            if bm >= (1 << 56):
                                errors.append(f"users.dat line {line_no}: scheduleBitmask exceeds 56 bits")
                            users[uid]['scheduleBitmask'] = bm
                        except ValueError:
                            errors.append(f"users.dat line {line_no}: invalid scheduleBitmask {tokens[10]}")
                elif role == 'ADMIN':
                    if len(tokens) < 8:
                        errors.append(f"users.dat line {line_no}: Admin requires 8 fields, got {len(tokens)}")

    print(f"  [Users] Total users loaded: {len(users)} (Errors: {len(errors)})")

    # 2. Audit bookings.dat (Legacy)
    bookings = {}
    if not os.path.exists(bookings_file):
        warnings.append(f"bookings.dat not found in {dir_path}")
    else:
        with open(bookings_file, 'r', encoding='utf-8') as f:
            for line_no, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                tokens = line.split(',')
                if len(tokens) < 10:
                    errors.append(f"bookings.dat line {line_no}: requires 10 fields, got {len(tokens)}")
                    continue
                try:
                    bid = int(tokens[0])
                    sid = int(tokens[1])
                    tid = int(tokens[2])
                    rate = float(tokens[6])
                    mask = int(tokens[7])
                except ValueError as ve:
                    errors.append(f"bookings.dat line {line_no}: numeric conversion error: {ve}")
                    continue

                if bid in bookings:
                    errors.append(f"bookings.dat line {line_no}: duplicate booking id {bid}")

                # FK checks
                if sid not in users:
                    errors.append(f"bookings.dat line {line_no}: studentId {sid} does not exist in users")
                elif users[sid]['role'] != 'STUDENT':
                    errors.append(f"bookings.dat line {line_no}: studentId {sid} is not a STUDENT")

                if tid not in users:
                    errors.append(f"bookings.dat line {line_no}: tutorId {tid} does not exist in users")
                elif users[tid]['role'] != 'TUTOR':
                    errors.append(f"bookings.dat line {line_no}: tutorId {tid} is not a TUTOR")

                status = tokens[8]
                if status not in ["PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED"]:
                    errors.append(f"bookings.dat line {line_no}: invalid status {status}")

                bookings[bid] = {
                    'id': bid, 'studentId': sid, 'tutorId': tid, 'status': status,
                    'createdAt': tokens[9]
                }

    print(f"  [Legacy Bookings] Total bookings loaded: {len(bookings)}")

    # 3. Audit course_bookings.dat (Recurring)
    course_bookings = {}
    accepted_reservations = [] # list of (date_str, slotIndex, tutorId, studentId, courseId)

    if not os.path.exists(courses_file):
        errors.append(f"course_bookings.dat not found in {dir_path}")
    else:
        with open(courses_file, 'r', encoding='utf-8') as f:
            for line_no, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                tokens = line.split(',')
                if len(tokens) < 13:
                    errors.append(f"course_bookings.dat line {line_no}: requires 13 fields, got {len(tokens)}")
                    continue
                try:
                    cid = int(tokens[0])
                    sid = int(tokens[1])
                    tid = int(tokens[2])
                    rate = float(tokens[6])
                    dow = int(tokens[7])
                    slot = int(tokens[8])
                    weeks = int(tokens[10])
                except ValueError as ve:
                    errors.append(f"course_bookings.dat line {line_no}: numeric conversion error: {ve}")
                    continue

                if cid in course_bookings:
                    errors.append(f"course_bookings.dat line {line_no}: duplicate course id {cid}")

                # FK checks
                if sid not in users:
                    errors.append(f"course_bookings.dat line {line_no}: studentId {sid} does not exist in users")
                elif users[sid]['role'] != 'STUDENT':
                    errors.append(f"course_bookings.dat line {line_no}: studentId {sid} is not a STUDENT")

                if tid not in users:
                    errors.append(f"course_bookings.dat line {line_no}: tutorId {tid} does not exist in users")
                elif users[tid]['role'] != 'TUTOR':
                    errors.append(f"course_bookings.dat line {line_no}: tutorId {tid} is not a TUTOR")

                # Field validations
                if dow < 0 or dow > 6:
                    errors.append(f"course_bookings.dat line {line_no}: dayOfWeek {dow} out of range 0..6")
                if slot < 0 or slot > 7:
                    errors.append(f"course_bookings.dat line {line_no}: slotIndex {slot} out of range 0..7")
                if weeks not in [4, 8, 12]:
                    warnings.append(f"course_bookings.dat line {line_no}: weeksCount {weeks} is not standard (4/8/12)")

                start_date_str = tokens[9]
                dt = parse_iso_date(start_date_str)
                if not dt:
                    errors.append(f"course_bookings.dat line {line_no}: invalid startDate format '{start_date_str}'")
                else:
                    computed_dow = day_of_week(dt.year, dt.month, dt.day)
                    if computed_dow != dow:
                        errors.append(f"course_bookings.dat line {line_no}: startDate {start_date_str} weekday is {computed_dow}, but dayOfWeek is {dow}!")

                status = tokens[11]
                if status not in ["PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED"]:
                    errors.append(f"course_bookings.dat line {line_no}: invalid status {status}")

                course_bookings[cid] = {
                    'id': cid, 'studentId': sid, 'tutorId': tid, 'dayOfWeek': dow,
                    'slotIndex': slot, 'startDate': start_date_str, 'weeksCount': weeks,
                    'status': status
                }

                # Collision check for ACCEPTED courses
                if status == "ACCEPTED" and dt:
                    for i in range(weeks):
                        session_dt = dt + datetime.timedelta(days=7 * i)
                        session_str = session_dt.isoformat()
                        # Check collision with other ACCEPTED courses
                        for res in accepted_reservations:
                            r_date, r_slot, r_tid, r_sid, r_cid = res
                            if r_date == session_str and r_slot == slot:
                                if r_tid == tid:
                                    errors.append(f"Collision: Tutor {tid} booked in both course {r_cid} and {cid} on {session_str} slot {slot}")
                                if r_sid == sid:
                                    errors.append(f"Collision: Student {sid} booked in both course {r_cid} and {cid} on {session_str} slot {slot}")
                        accepted_reservations.append((session_str, slot, tid, sid, cid))

    print(f"  [Course Bookings] Total recurring courses loaded: {len(course_bookings)}")
    print(f"  [Course Bookings] Total ACCEPTED session dates reserved: {len(accepted_reservations)}")

    # 4. Audit reviews.dat
    reviews = {}
    if not os.path.exists(reviews_file):
        warnings.append(f"reviews.dat not found in {dir_path}")
    else:
        with open(reviews_file, 'r', encoding='utf-8') as f:
            for line_no, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                tokens = line.split(',')
                if len(tokens) < 8:
                    errors.append(f"reviews.dat line {line_no}: requires 8 fields, got {len(tokens)}")
                    continue
                try:
                    rid = int(tokens[0])
                    bid = int(tokens[1])
                    sid = int(tokens[2])
                    tid = int(tokens[3])
                    rating = int(tokens[5])
                except ValueError as ve:
                    errors.append(f"reviews.dat line {line_no}: numeric conversion error: {ve}")
                    continue

                if rid in reviews:
                    errors.append(f"reviews.dat line {line_no}: duplicate review id {rid}")

                if sid not in users:
                    errors.append(f"reviews.dat line {line_no}: studentId {sid} does not exist in users")
                if tid not in users:
                    errors.append(f"reviews.dat line {line_no}: tutorId {tid} does not exist in users")

                if rating < 1 or rating > 5:
                    errors.append(f"reviews.dat line {line_no}: ratingScore {rating} outside 1..5 range")

                # Booking link check (must exist in legacy bookings OR course bookings)
                if bid in bookings:
                    b = bookings[bid]
                    if b['status'] != "COMPLETED":
                        errors.append(f"reviews.dat line {line_no}: referenced booking {bid} has status {b['status']} (not COMPLETED)")
                    if b['studentId'] != sid:
                        errors.append(f"reviews.dat line {line_no}: studentId {sid} does not match booking studentId {b['studentId']}")
                    if b['tutorId'] != tid:
                        errors.append(f"reviews.dat line {line_no}: tutorId {tid} does not match booking tutorId {b['tutorId']}")
                elif bid in course_bookings:
                    cb = course_bookings[bid]
                    if cb['status'] != "COMPLETED":
                        errors.append(f"reviews.dat line {line_no}: referenced course {bid} has status {cb['status']} (not COMPLETED)")
                    if cb['studentId'] != sid:
                        errors.append(f"reviews.dat line {line_no}: studentId {sid} does not match course studentId {cb['studentId']}")
                    if cb['tutorId'] != tid:
                        errors.append(f"reviews.dat line {line_no}: tutorId {tid} does not match course tutorId {cb['tutorId']}")
                else:
                    errors.append(f"reviews.dat line {line_no}: bookingId {bid} does not exist in bookings.dat nor course_bookings.dat")

                reviews[rid] = {
                    'id': rid, 'bookingId': bid, 'studentId': sid, 'tutorId': tid,
                    'ratingScore': rating
                }

    print(f"  [Reviews] Total reviews loaded: {len(reviews)}")

    # 5. Check Tutor Rating Aggregations
    tutor_reviews = {}
    for r in reviews.values():
        tutor_reviews.setdefault(r['tutorId'], []).append(r['ratingScore'])

    for tid, scores in tutor_reviews.items():
        if tid in users:
            avg_calc = round(sum(scores) / len(scores), 1)
            cnt_calc = len(scores)
            user_rating = users[tid].get('rating', 0.0)
            user_reviews = users[tid].get('totalReviews', 0)
            if abs(avg_calc - user_rating) > 0.01:
                errors.append(f"Tutor {tid} rating mismatch: user record has {user_rating}, calculated is {avg_calc}")
            if cnt_calc != user_reviews:
                errors.append(f"Tutor {tid} totalReviews mismatch: user record has {user_reviews}, calculated is {cnt_calc}")

    print(f"------------------------------------------------------------")
    print(f"AUDIT SUMMARY:")
    print(f"  Total Errors   : {len(errors)}")
    print(f"  Total Warnings : {len(warnings)}")
    for e in errors:
        print(f"   [ERROR] {e}")
    for w in warnings:
        print(f"   [WARN]  {w}")

    return len(errors) == 0

def create_backup(data_dir):
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = os.path.join(data_dir, f"backup_migration_{timestamp}")
    os.makedirs(backup_dir, exist_ok=True)

    manifest_lines = []
    files_to_backup = ['users.dat', 'bookings.dat', 'course_bookings.dat', 'reviews.dat']

    print(f"\n[BACKUP] Creating timestamped backup at: {backup_dir}")
    for fname in files_to_backup:
        src = os.path.join(data_dir, fname)
        dst = os.path.join(backup_dir, fname)
        if os.path.exists(src):
            shutil.copy2(src, dst)
            sha = compute_sha256(dst)
            manifest_lines.append(f"{fname}:{sha}")
            print(f"  Backed up {fname} -> SHA256: {sha}")
        else:
            print(f"  [WARN] File not found to backup: {fname}")

    manifest_file = os.path.join(backup_dir, 'manifest_sha256.txt')
    with open(manifest_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(manifest_lines) + '\n')

    print(f"[BACKUP] Manifest created at: {manifest_file}")
    return backup_dir

def prepare_canonical_staging(data_dir, staging_dir):
    if os.path.exists(staging_dir):
        shutil.rmtree(staging_dir)
    os.makedirs(staging_dir, exist_ok=True)

    # 1. Copy users.dat, bookings.dat, reviews.dat verbatim
    for fname in ['users.dat', 'bookings.dat', 'reviews.dat']:
        shutil.copy2(os.path.join(data_dir, fname), os.path.join(staging_dir, fname))

    # 2. Write normalized canonical course_bookings.dat
    # Covering:
    # 1001: 4w ACCEPTED (Student 6, Tutor 2, Fri slot 0, 2026-11-06)
    # 1002: 4w ACCEPTED (Student 6, Tutor 2, Sun slot 0, 2026-11-08)
    # 1003: 8w ACCEPTED (Student 7, Tutor 3, Tue slot 1, 2026-11-10)
    # 1004: 12w ACCEPTED (Student 27, Tutor 14, Wed slot 2, 2026-11-11)
    # 1005: 4w PENDING (Student 18, Tutor 10, Thu slot 3, 2026-11-12)
    # 1006: 4w REJECTED (Student 46, Tutor 4, Mon slot 4, 2026-11-09)
    # 1007: 4w CANCELLED (Student 45, Tutor 3, Sat slot 5, 2026-11-07)
    # 1008: 4w COMPLETED (Student 33, Tutor 12, Mon slot 1, 2026-09-07)
    canonical_courses = [
        "1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,4,0,2026-11-06,4,ACCEPTED,2026-11-06",
        "1002,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12,150000,6,0,2026-11-08,4,ACCEPTED,2026-11-06",
        "1003,7,3,Phạm Tuấn Thành,TS. Trần Thị Vật Lý,Vật lý 10-12 (Luyện thi THPT),200000,1,1,2026-11-10,8,ACCEPTED,2026-11-06",
        "1004,27,14,Phan Đức Huy,Mr. David Lê,Tiếng Anh / IELTS (6.5 - 8.5),160000,2,2,2026-11-11,12,ACCEPTED,2026-11-06",
        "1005,18,10,Trần Minh Anh,Cô Nguyễn Thị Sinh,Sinh học 11-12,130000,3,3,2026-11-12,4,PENDING,2026-11-06",
        "1006,46,4,Đoàn Nhật Khoa,Thầy Phạm Hóa Học,Hóa học 10-12 (Luyện thi HSG),100000,0,4,2026-11-09,4,REJECTED,2026-11-06",
        "1007,45,3,Lê Quang Vinh,TS. Trần Thị Vật Lý,Vật lý 10-12 (Luyện thi THPT),200000,5,5,2026-11-07,4,CANCELLED,2026-11-06",
        "1008,33,12,Trịnh Thảo My,Thầy Võ Minh Tin,Tin học 11-12,180000,0,1,2026-09-07,4,COMPLETED,2026-09-01"
    ]

    target_course_file = os.path.join(staging_dir, 'course_bookings.dat')
    with open(target_course_file, 'w', encoding='utf-8') as f:
        for c in canonical_courses:
            f.write(c + '\n')

    print(f"\n[STAGING] Staging copy created at: {staging_dir}")
    print(f"  course_bookings.dat written with {len(canonical_courses)} canonical records.")

def commit_migration(staging_dir, data_dir):
    print(f"\n[COMMIT] Committing staging dataset to production: {data_dir}")
    # Atomically copy over course_bookings.dat
    src_cb = os.path.join(staging_dir, 'course_bookings.dat')
    dst_cb = os.path.join(data_dir, 'course_bookings.dat')
    shutil.copy2(src_cb, dst_cb)
    print(f"  Successfully committed course_bookings.dat to production!")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="TutorMatch Data Audit, Cleanup & Migration Tool")
    parser.add_argument('--audit', action='store_true', help="Audit production dataset")
    parser.add_argument('--backup', action='store_true', help="Create verified timestamped backup")
    parser.add_argument('--dry-run', action='store_true', help="Perform dry-run migration on staging")
    parser.add_argument('--commit', action='store_true', help="Commit migration to production data")
    args = parser.parse_args()

    staging_dir = os.path.join(DATA_DIR, 'staging_migration')

    if args.backup:
        create_backup(DATA_DIR)

    if args.audit:
        passed = audit_dataset(DATA_DIR)
        sys.exit(0 if passed else 1)

    if args.dry_run:
        print("\n>>> STARTING DRY-RUN MIGRATION <<<")
        prepare_canonical_staging(DATA_DIR, staging_dir)
        passed = audit_dataset(staging_dir)
        if passed:
            print("\n>>> DRY-RUN AUDIT PASSED 100%! DATA IS READY FOR COMMIT. <<<")
        else:
            print("\n>>> DRY-RUN AUDIT FAILED! DO NOT COMMIT. <<<")
            sys.exit(1)

    if args.commit:
        print("\n>>> COMMITTING MIGRATION <<<")
        # 1. Audit staging first
        if not os.path.exists(staging_dir):
            print("Staging directory does not exist! Running dry-run first...")
            prepare_canonical_staging(DATA_DIR, staging_dir)

        if not audit_dataset(staging_dir):
            print("Staging validation failed! Aborting commit.")
            sys.exit(1)

        # 2. Make backup of current production
        backup_path = create_backup(DATA_DIR)

        # 3. Commit
        commit_migration(staging_dir, DATA_DIR)

        # 4. Final audit of production
        final_pass = audit_dataset(DATA_DIR)
        if final_pass:
            print("\n>>> PRODUCTION COMMIT AND FINAL AUDIT COMPLETED SUCCESSFULLY! <<<")
        else:
            print("\n>>> FINAL AUDIT OF PRODUCTION FAILED! Restoring backup... <<<")
            # Rollback
            shutil.copy2(os.path.join(backup_path, 'course_bookings.dat'), os.path.join(DATA_DIR, 'course_bookings.dat'))
            print("Rollback completed.")
            sys.exit(1)
