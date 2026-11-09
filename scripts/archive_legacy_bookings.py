import os
import shutil
import hashlib
import datetime

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_DIR = os.path.join(PROJECT_DIR, 'data')
ARCHIVE_DIR = os.path.join(DATA_DIR, 'archive', 'legacy_bookings')
os.makedirs(ARCHIVE_DIR, exist_ok=True)

bookings_path = os.path.join(DATA_DIR, 'bookings.dat')
with open(bookings_path, 'rb') as f:
    data = f.read()

sha = hashlib.sha256(data).hexdigest().upper()
print('Original data/bookings.dat SHA256:', sha)
print('Total bytes:', len(data))
lines = [l for l in data.decode('utf-8').splitlines() if l.strip()]
print('Total records:', len(lines))

timestamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
archived_filename = f'legacy_bookings_{timestamp}.dat'
archived_path = os.path.join(ARCHIVE_DIR, archived_filename)
shutil.copy2(bookings_path, archived_path)

# Verify archived copy
with open(archived_path, 'rb') as f:
    archived_data = f.read()
archived_sha = hashlib.sha256(archived_data).hexdigest().upper()
assert sha == archived_sha, 'SHA mismatch in archive!'

# Write manifest and readme
manifest_path = os.path.join(ARCHIVE_DIR, 'manifest_sha256.txt')
with open(manifest_path, 'w', encoding='utf-8') as f:
    f.write(f'original_file:bookings.dat\n')
    f.write(f'archived_file:{archived_filename}\n')
    f.write(f'sha256:{sha}\n')
    f.write(f'record_count:{len(lines)}\n')
    f.write(f'archive_timestamp:{timestamp}\n')

readme_path = os.path.join(ARCHIVE_DIR, 'README_ARCHIVE.md')
with open(readme_path, 'w', encoding='utf-8') as f:
    f.write('# ARCHIVE: LEGACY BOOKINGS DATA\n\n')
    f.write(f'- File goc: `data/bookings.dat`\n')
    f.write(f'- Ten luu tru: `{archived_filename}`\n')
    f.write(f'- SHA-256: `{sha}`\n')
    f.write(f'- So luong ban ghi: {len(lines)}\n')
    f.write(f'- Thoi diem luu tru: {timestamp}\n\n')
    f.write('## Muc dich luu tru:\n')
    f.write('Tep nay chua 27 ban ghi cua mo hinh dat lich cu (legacy single/weekly booking model).\n')
    f.write('Trong do, 15 ban ghi COMPLETED (IDs 101-103, 106-110, 112-116, 119, 120) la can cu lich su cua 15 danh gia trong reviews.dat.\n')
    f.write('He thong TutorMatch da chuyen doi hoan toan sang mo hinh khoa hoc dinh ky (course_bookings.dat), do do tep nay duoc rut khoi duong dan chay thuc te va bao toan vinh vien tai day de phuc vu kiem toan lich su.\n')

print('Archive completed successfully at:', ARCHIVE_DIR)
