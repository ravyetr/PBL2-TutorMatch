# BÁO CÁO TOÀN DIỆN: KIỂM TOÁN DỮ LIỆU, CHUẨN HÓA VÀ DI TRÚ HỆ THỐNG LƯU TRỮ TUTORMATCH
## Chuyển đổi mô hình đặt lịch từ đơn lẻ/hàng tuần sang khóa học định kỳ theo ngày lịch thực tế

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Công nghệ:** C++17, Qt 6.11, Lưu trữ tệp phẳng CSV (`.dat`)  
**Ngày thực hiện:** 06/11/2026  
**Trạng thái thực hiện:** HOÀN THÀNH TOÀN DIỆN (100% PASSED)

---

## 1. TỔNG KIỂM KÊ CÁC TỆP DỮ LIỆU (COMPLETE DATA INVENTORY)

Toàn bộ hệ thống lưu trữ của TutorMatch dựa trên các tệp văn bản thuần định dạng CSV với đuôi `.dat` đặt trong thư mục `data/`. Quá trình kiểm toán toàn diện đã rà soát tất cả các tệp tham gia vào luồng dữ liệu của ứng dụng:

| Tên tệp | Đường dẫn thực tế | Serializer đảm nhiệm | Repository quản lý | Vai trò nghiệp vụ | Số bản ghi trước | Số bản ghi sau | Trạng thái toàn vẹn |
|---|---|---|---|---|:---:|:---:|:---:|
| `users.dat` | `data/users.dat` | `UserSerializer` | `FileUserRepository` | Lưu trữ tài khoản, thông tin xác thực, phân quyền (Admin/Tutor/Student), hồ sơ năng lực và mặt nạ lịch rảnh tuần hoàn 56-bit | 50 | 50 | Hợp lệ 100% |
| `bookings.dat` | `data/bookings.dat` | `BookingSerializer` | `FileBookingRepository` | Lưu trữ đơn đặt lịch học cũ (legacy model: 1 buổi hoặc tuần hoàn đơn giản với 56-slot mask), làm cơ sở lịch sử cho các đánh giá đã hoàn thành | 27 | 27 | Hợp lệ 100% |
| `course_bookings.dat` | `data/course_bookings.dat` | `CourseBookingSerializer` | `FileCourseBookingRepository` | Lưu trữ khóa học định kỳ mới theo ngày lịch thực tế (4, 8, 12 tuần) với thứ cố định, ca học, ngày bắt đầu chuẩn ISO-8601 | 2 | 8 | Hợp lệ 100% |
| `reviews.dat` | `data/reviews.dat` | `ReviewSerializer` | `FileReviewRepository` | Lưu trữ nhận xét và đánh giá sao (1-5★) của học viên dành cho gia sư sau khi hoàn thành khóa học | 15 | 15 | Hợp lệ 100% |

Ngoài ra, hệ thống lưu trữ các bản sao lưu lịch sử tại:
- `data/backup_pre_fix/`: Bản sao lưu lịch sử từ ngày 23/10/2026.
- `data/backup_migration_20261106_134710/`: Bản sao lưu được kiểm chứng ngay trước phiên di trú hiện tại, kèm mã băm SHA-256 cho từng tệp.

---

## 2. CẤU TRÚC LƯỢC ĐỒ THỰC TẾ (ACTUAL SCHEMAS)

Kiểm tra trực tiếp từ mã nguồn thực thi C++ trong `include/storage/serializers/` và `src/storage/serializers/`:

### 2.1. Lược đồ `users.dat` (`UserSerializer`)
Tệp áp dụng kỹ thuật serialize đa hình (polymorphic serialization) theo vai trò `UserRole`:
- **Trường cơ sở (Base User - 7 tokens):**
  1. `id` (`int`): Mã định danh duy nhất (Primary Key, 1..50).
  2. `username` (`string`): Tên đăng nhập duy nhất.
  3. `password` (`string`): Mật khẩu xác thực (được bảo vệ/che dấu khi hiển thị forensic).
  4. `email` (`string`): Địa chỉ thư điện tử.
  5. `fullName` (`string`): Họ và tên hiển thị đầy đủ.
  6. `role` (`string`): Vai trò (`ADMIN`, `TUTOR`, `STUDENT`).
  7. `isActive` (`int/bool`): Trạng thái kích hoạt tài khoản (`1` = hoạt động, `0` = bị khóa).
- **Phần mở rộng vai trò ADMIN (+1 token, tổng 8 tokens):**
  8. `permissionLevel` (`int`): Cấp độ quyền hạn quản trị (9 = Quản trị viên tối cao).
- **Phần mở rộng vai trò TUTOR (+8 tokens, tổng 15 tokens):**
  8. `bio` (`string`): Tiểu sử & kinh nghiệm giảng dạy.
  9. `subjectSpecialty` (`string`): Chuyên môn môn học chính.
  10. `hourlyRate` (`double`): Học phí tiêu chuẩn mỗi buổi học (VNĐ).
  11. `rating` (`double`): Điểm đánh giá trung bình tích lũy (1.0 - 5.0).
  12. `totalReviews` (`int`): Tổng số lượt đánh giá đã nhận từ học viên.
  13. `isVerified` (`int/bool`): Trạng thái xác minh hồ sơ từ Admin (`1`/`0`).
  14. `totalEarnings` (`double`): Tổng thu nhập giảng dạy tích lũy (VNĐ).
  15. `scheduleBitmask` (`uint64_t`): Mặt nạ nhị phân lịch rảnh tuần hoàn (56 ca, $0 \le \text{mask} < 2^{56}$).
- **Phần mở rộng vai trò STUDENT (+4 tokens, tổng 11 tokens):**
  8. `gradeLevel` (`string`): Khối lớp đang học (ví dụ: "12", "11").
  9. `school` (`string`): Môn học ưu tiên / nhu cầu học tập ("Toán 12", "Vật lý 12").
  10. `walletBalance` (`double`): Số dư ví tài khoản học viên (VNĐ).
  11. `scheduleBitmask` (`uint64_t`): Mặt nạ nhị phân thời gian rảnh học tập 56 ca.

### 2.2. Lược đồ `bookings.dat` (`BookingSerializer`)
Gồm 10 trường ngăn cách bằng dấu phẩy:
1. `id` (`int`): Mã đơn đặt lịch (Primary Key, bắt đầu từ 101).
2. `studentId` (`int`): Mã học viên (Foreign Key -> `User.id`).
3. `tutorId` (`int`): Mã gia sư (Foreign Key -> `User.id`).
4. `studentName` (`string`): Họ tên học viên tại thời điểm tạo đơn.
5. `tutorName` (`string`): Họ tên gia sư tại thời điểm tạo đơn.
6. `subject` (`string`): Môn học yêu cầu.
7. `hourlyRate` (`double`): Học phí thỏa thuận.
8. `slotMask` (`uint64_t`): Mặt nạ bit ca học trong tuần (đơn ca).
9. `status` (`string`): Trạng thái đơn (`PENDING`, `ACCEPTED`, `REJECTED`, `COMPLETED`, `CANCELLED`).
10. `createdAt` (`string`): Ngày tạo đơn (định dạng ISO YYYY-MM-DD).

### 2.3. Lược đồ `course_bookings.dat` (`CourseBookingSerializer`)
Gồm 13 trường ngăn cách bằng dấu phẩy theo chuẩn mô hình khóa học định kỳ:
1. `id` (`int`): Mã khóa học định kỳ (Primary Key, bắt đầu từ 1001).
2. `studentId` (`int`): Mã học viên (Foreign Key -> `User.id`).
3. `tutorId` (`int`): Mã gia sư (Foreign Key -> `User.id`).
4. `studentName` (`string`): Họ tên học viên.
5. `tutorName` (`string`): Họ tên gia sư.
6. `subject` (`string`): Môn học đăng ký.
7. `hourlyRate` (`double`): Học phí mỗi buổi học (VNĐ).
8. `dayOfWeek` (`int`): Thứ trong tuần cố định (quy ước `DateUtils`: `0 = Thứ 2` .. `6 = Chủ Nhật`).
9. `slotIndex` (`int`): Chỉ số ca học cố định trong ngày (`0` = 07:30-09:00 .. `7` = 20:30-22:00).
10. `startDate` (`string`): Ngày bắt đầu khóa học theo lịch thực tế (ISO YYYY-MM-DD).
11. `weeksCount` (`int`): Thời lượng khóa học (chỉ chấp nhận `4`, `8`, hoặc `12` tuần).
12. `status` (`string`): Trạng thái khóa học (`PENDING`, `ACCEPTED`, `REJECTED`, `COMPLETED`, `CANCELLED`).
13. `createdAt` (`string`): Ngày gửi yêu cầu đăng ký (ISO YYYY-MM-DD).

### 2.4. Lược đồ `reviews.dat` (`ReviewSerializer`)
Gồm 8 trường ngăn cách bằng dấu phẩy:
1. `id` (`int`): Mã đánh giá nhận xét (Primary Key, bắt đầu từ 1).
2. `bookingId` (`int`): Mã đơn học/khóa học tương ứng (Foreign Key -> `Booking.id` hoặc `CourseBooking.id`).
3. `studentId` (`int`): Mã học viên viết đánh giá (Foreign Key -> `User.id`).
4. `tutorId` (`int`): Mã gia sư được đánh giá (Foreign Key -> `User.id`).
5. `studentName` (`string`): Họ tên học viên đánh giá.
6. `ratingScore` (`int`): Điểm số đánh giá từ `1` đến `5` sao.
7. `comment` (`string`): Nội dung nhận xét chi tiết.
8. `createdAt` (`string`): Ngày đăng nhận xét (ISO YYYY-MM-DD).

---

## 3. BẢN ĐỒ QUAN HỆ PHỤ THUỘC DỮ LIỆU (DEPENDENCY MAP)

```mermaid
graph TD
    User["User (users.dat)<br/>Primary Key: id<br/>Role: ADMIN / TUTOR / STUDENT"]
    LegacyBooking["Booking (bookings.dat)<br/>Primary Key: id<br/>FK: studentId, tutorId -> User.id"]
    CourseBooking["CourseBooking (course_bookings.dat)<br/>Primary Key: id<br/>FK: studentId, tutorId -> User.id"]
    Review["Review (reviews.dat)<br/>Primary Key: id<br/>FK: bookingId -> Booking.id / CourseBooking.id<br/>FK: studentId, tutorId -> User.id"]

    User -->|1 : N (Học viên / Gia sư)| LegacyBooking
    User -->|1 : N (Học viên / Gia sư)| CourseBooking
    User -->|1 : N (Người nhận / Người gửi)| Review
    LegacyBooking -->|1 : 1 (Đơn COMPLETED)| Review
    CourseBooking -.->|1 : 1 (Khóa COMPLETED)| Review
```

### Phân tích quan hệ khóa ngoại (Foreign Key Constraints):
1. Mọi `studentId` trong `bookings.dat` và `course_bookings.dat` đều trỏ chính xác về tài khoản người dùng có `role == STUDENT` trong `users.dat`.
2. Mọi `tutorId` trong `bookings.dat` và `course_bookings.dat` đều trỏ chính xác về tài khoản người dùng có `role == TUTOR` trong `users.dat`.
3. 15 bản ghi trong `reviews.dat` liên kết 1:1 với 15 bản ghi `COMPLETED` trong `bookings.dat`. Bộ ba `(bookingId, studentId, tutorId)` hoàn toàn trùng khớp giữa hai tệp.
4. Điểm trung bình cộng các đánh giá của từng gia sư trong `reviews.dat` khớp chính xác 100% với thuộc tính `rating` và `totalReviews` của gia sư đó trong `users.dat`.

---

## 4. SỐ LƯỢNG BẢN GHI TRƯỚC DI TRÚ (PRE-MIGRATION COUNTS)

Trước khi thực hiện chuẩn hóa và di trú, kiểm toán dữ liệu cho kết quả chi tiết:

- **`users.dat`:** 50 bản ghi.
  - Quản trị viên (`ADMIN`): 3 tài khoản (IDs: 1, 8, 9).
  - Gia sư (`TUTOR`): 12 tài khoản (IDs: 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 21).
  - Học viên (`STUDENT`): 35 tài khoản (IDs: 6, 7, 17, 18, 19, 20, 22..50).
- **`bookings.dat`:** 27 bản ghi.
  - `COMPLETED`: 15 bản ghi (IDs: 101, 102, 103, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 119, 120).
  - `ACCEPTED`: 6 bản ghi (IDs: 104, 105, 118, 122, 123, 124).
  - `CANCELLED`: 2 bản ghi (IDs: 111, 117).
  - `REJECTED`: 1 bản ghi (ID: 121).
  - `PENDING`: 3 bản ghi (IDs: 125, 126, 127).
- **`course_bookings.dat`:** 2 bản ghi.
  - `ACCEPTED`: 2 bản ghi (IDs: 1001, 1002).
- **`reviews.dat`:** 15 bản ghi.
  - Tương ứng 15 đơn `COMPLETED` trong `bookings.dat`.
- **Tổng cộng toàn hệ thống:** 94 bản ghi trước di trú.

---

## 5. PHÂN LOẠI BẢN GHI CŨ VÀ MỚI (CLASSIFICATION OF RECORDS)

| Mã định danh | Tệp nguồn | Phân loại | Tình trạng kỹ thuật | Hành động phê duyệt |
|---|---|---|---|---|
| 1 .. 50 | `users.dat` | **VALID** | Tài khoản, mật khẩu, hồ sơ, bitmask hợp lệ $0 \le \text{mask} < 2^{56}$, email định dạng chuẩn | **KEEP** (Giữ nguyên toàn bộ) |
| 101..103, 106..110, 112..116, 119, 120 | `bookings.dat` | **LEGACY_HISTORICAL** | 15 đơn học đã hoàn thành (`COMPLETED`), đóng vai trò cơ sở dữ liệu khóa ngoại cho 15 nhận xét trong `reviews.dat` | **KEEP / ARCHIVE** (Giữ nguyên làm căn cứ tham chiếu lịch sử) |
| 104, 105, 118, 122..124 | `bookings.dat` | **LEGACY_ACTIVE** | 6 đơn học đã được chấp nhận (`ACCEPTED`) theo cơ chế cũ | **KEEP** (Lưu trữ tương thích ngược) |
| 111, 117 | `bookings.dat` | **LEGACY_AUDIT** | 2 đơn đã hủy (`CANCELLED`) | **KEEP** (Lưu vết kiểm toán) |
| 121 | `bookings.dat` | **LEGACY_AUDIT** | 1 đơn bị từ chối (`REJECTED`) | **KEEP** (Lưu vết kiểm toán) |
| 125 .. 127 | `bookings.dat` | **LEGACY_PENDING** | 3 đơn chờ duyệt (`PENDING`) mô hình cũ | **KEEP** (Lưu trữ tương thích ngược) |
| 1 .. 15 | `reviews.dat` | **VALID** | 15 đánh giá chất lượng thực tế từ 4★ đến 5★, khớp dữ liệu học viên và gia sư | **KEEP** (Bảo toàn 100%) |
| 1001 | `course_bookings.dat` | **VALID** | Khóa học 4 tuần môn Toán 12, Thứ 6 ca 0, bắt đầu 2026-11-06 (Thứ 6) | **KEEP** (Giữ nguyên chuẩn tắc) |
| 1002 | `course_bookings.dat` | **VALID** | Khóa học 4 tuần môn Toán 12, CN ca 0, bắt đầu 2026-11-08 (Chủ Nhật) | **KEEP** (Giữ nguyên chuẩn tắc) |
| 1003 .. 1008 | `course_bookings.dat` | **CANONICAL_NEW** | Bổ sung 6 khóa học định kỳ chuẩn tắc đại diện cho 8 tuần, 12 tuần, PENDING, REJECTED, CANCELLED, COMPLETED | **MIGRATE / ADD** (Bổ sung bộ demo chuẩn) |

---

## 6. ĐIỀU TRA CHUYÊN SÂU BẢN GHI ID 1001 (FORENSIC ANALYSIS OF RECORD 1001)

### Vấn đề đặt ra ban đầu:
Bản ghi dòng thô:
`1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,4,0,2026-11-06,4,ACCEPTED,2026-11-06`

Trước đây, có nghi vấn rằng Token thứ 7 và 8 bị đảo lộn (`slotIndex: 4, dayOfWeek: 0`), dẫn đến nghi vấn `2026-11-06` là Thứ 6 nhưng dữ liệu lại ghi `dayOfWeek = 0` (Thứ 2).

### Kết quả điều tra đối chiếu mã nguồn C++:
Kiểm tra trực tiếp tại `src/storage/serializers/CourseBookingSerializer.cpp` (dòng 80-90):
```cpp
    int dayOfWeek;
    if (!SerializerUtils::safeStringToInt(tokens[7], dayOfWeek) || dayOfWeek < 0 || dayOfWeek > 6) { ... }

    int slotIndex;
    if (!SerializerUtils::safeStringToInt(tokens[8], slotIndex) || slotIndex < 0 || slotIndex > 7) { ... }

    std::string startDate = tokens[9];
```

Đối chiếu chính xác từng token theo chỉ mục mảng `tokens`:
- `tokens[0] = 1001` (`id`)
- `tokens[1] = 6` (`studentId` = Nguyễn Văn Nam, Học sinh lớp 12)
- `tokens[2] = 2` (`tutorId` = ThS. Lê Văn Toàn, Gia sư môn Toán)
- `tokens[3] = Nguyễn Văn Nam` (`studentName`)
- `tokens[4] = ThS. Lê Văn Toàn` (`tutorName`)
- `tokens[5] = Toán 12 - Luyện thi Đại học` (`subject`)
- `tokens[6] = 150000` (`hourlyRate`)
- **`tokens[7] = 4`** -> **`dayOfWeek = 4` (Thứ Sáu)** theo quy ước `DateUtils` (`0 = Thứ 2, 1 = Thứ 3, ..., 4 = Thứ 6, 6 = Chủ Nhật`).
- **`tokens[8] = 0`** -> **`slotIndex = 0` (Ca 1: 07:30 - 09:00)**.
- **`tokens[9] = 2026-11-06`** -> **`startDate = 2026-11-06`**.
- `tokens[10] = 4` (`weeksCount = 4` tuần).
- `tokens[11] = ACCEPTED` (`status`).
- `tokens[12] = 2026-11-06` (`createdAt`).

### Kiểm tra toán học lịch Gregory:
Áp dụng thuật toán Sakamoto trong `DateUtils::dayOfWeek(2026, 11, 6)`:
- Ngày 06/11/2026 rơi chính xác vào **Thứ Sáu (Friday)**.
- Chỉ số `dayOfWeek` tính được là **4**.
- `weekday(startDate) == dayOfWeek` ($4 == 4$).

**Kết luận dứt khoát:** Bản ghi 1001 **hoàn toàn chính xác, hợp lệ 100% và tuân thủ nghiêm ngặt mọi bất biến nghiệp vụ**. Nghi vấn trước đây xuất phát từ việc đọc sai thứ tự token trong báo cáo cũ, chứ không phải lỗi dữ liệu trong tệp. Tương tự, bản ghi 1002 (`dayOfWeek = 6`, `startDate = 2026-11-08` là Chủ Nhật) cũng hoàn toàn hợp lệ.

---

## 7. CHÍNH SÁCH DỌN DẸP VÀ LÝ DO KỸ THUẬT (CLEANUP POLICY & RATIONALE)

1. **Tuyệt đối không xóa đánh giá (`reviews.dat`):**
   Mỗi đánh giá gắn liền với lịch sử giảng dạy và uy tín (sao trung bình) của gia sư trong `users.dat`. Nếu xóa review, điểm trung bình tích lũy và số lượt đánh giá của gia sư sẽ bị sai lệch nghiêm trọng.
2. **Không bịa đặt ngày học từ `createdAt` cho đơn cũ:**
   Trong mô hình `Booking` cũ, chỉ có `slotMask` và `createdAt`. Trường `createdAt` chỉ biểu thị ngày gửi đơn, không đại diện cho ngày học thực tế. Do đó, không tự ý chuyển đổi 27 đơn cũ thành `CourseBooking` khi không có ngày bắt đầu tường minh, mà giữ nguyên `bookings.dat` làm tệp lưu vết tương thích ngược (Backward Compatibility Archive).
3. **Không can thiệp vào `User.scheduleBitmask` để bù đắp đơn đặt lịch:**
   `User.scheduleBitmask` thuần túy biểu diễn khung giờ rảnh chung trong tuần (Weekly Availability). Lịch đặt các buổi học cụ thể theo ngày lịch thực tế được quản lý độc lập bởi danh sách `CourseBooking` có trạng thái `ACCEPTED`.
4. **Bảo toàn thông tin xác thực:**
   Không thay đổi định dạng mật khẩu, tên đăng nhập hay thông tin tài khoản người dùng thực tế.

---

## 8. CÁC THAO TÁC DI TRÚ ĐÃ THỰC HIỆN (MIGRATION OPERATIONS)

1. **Phát triển công cụ kiểm toán và di trú độc lập:**
   Xây dựng script `scripts/migrate_and_audit.py` hỗ trợ đầy đủ các cờ lệnh:
   - `--backup`: Tự động tạo thư mục sao lưu kèm mã băm SHA-256.
   - `--dry-run`: Chạy thử nghiệm trên thư mục staging cách ly `data/staging_migration/`.
   - `--commit`: Ghi đè tệp chính thức và tự động kiểm toán lại toàn bộ.
2. **Tạo bản sao lưu có kiểm chứng:**
   Sao lưu toàn bộ thư mục `data/` vào `data/backup_migration_20261106_134710/` kèm tệp kê băm `manifest_sha256.txt`.
3. **Thực thi Dry-run:**
   Chạy mô phỏng di trú trên thư mục staging. Kết quả kiểm toán staging đạt 100% (0 lỗi, 0 cảnh báo).
4. **Chuẩn hóa bộ dữ liệu Khóa học định kỳ (`course_bookings.dat`):**
   Bảo lưu 2 khóa học ban đầu (1001, 1002) và bổ sung 6 khóa học chuẩn tắc (1003..1008) thỏa mãn trọn vẹn 10 tiêu chí trình diễn nghiệp vụ:
   - Khóa 1001: 4 tuần, `ACCEPTED`, Thứ 6 (07:30 - 09:00), bắt đầu 2026-11-06 (Học viên 6, Gia sư 2).
   - Khóa 1002: 4 tuần, `ACCEPTED`, Chủ Nhật (07:30 - 09:00), bắt đầu 2026-11-08 (Học viên 6, Gia sư 2).
   - Khóa 1003: 8 tuần, `ACCEPTED`, Thứ 3 (09:15 - 10:45), bắt đầu 2026-11-10 (Học viên 7, Gia sư 3).
   - Khóa 1004: 12 tuần, `ACCEPTED`, Thứ 4 (11:00 - 12:30), bắt đầu 2026-11-11 (Học viên 27, Gia sư 14).
   - Khóa 1005: 4 tuần, `PENDING` (không chiếm chỗ), Thứ 5 (13:30 - 15:00), bắt đầu 2026-11-12 (Học viên 18, Gia sư 10).
   - Khóa 1006: 4 tuần, `REJECTED` (không chiếm chỗ), Thứ 2 (17:00 - 18:30), bắt đầu 2026-11-09 (Học viên 46, Gia sư 4).
   - Khóa 1007: 4 tuần, `CANCELLED` (đã giải phóng chỗ), Thứ 7 (18:45 - 20:15), bắt đầu 2026-11-07 (Học viên 45, Gia sư 3).
   - Khóa 1008: 4 tuần, `COMPLETED` (quá khứ, không chặn tương lai), Thứ 2 (09:15 - 10:45), bắt đầu 2026-09-07 (Học viên 33, Gia sư 12).
5. **Nâng cấp công cụ chẩn đoán `TutorMatch_Inspector`:**
   Mở rộng `DataFileInspector.h`, `DataFileInspector.cpp` và `src/tools/inspect_main.cpp` để hỗ trợ kiểm tra tệp thứ 4 (`course_bookings.dat`) với tùy chọn `--course-bookings` và kiểm tra toàn diện cả 4 tệp bằng `--all`.

---

## 9. BẢNG PHÂN LOẠI CHI TIẾT BẢN GHI (PRESERVED / ARCHIVED / MIGRATED)

```
Tổng cộng: 100 bản ghi
├── Preserved (Bảo tồn nguyên trạng): 94 bản ghi
│   ├── users.dat: 50 bản ghi
│   ├── reviews.dat: 15 bản ghi
│   ├── bookings.dat: 27 bản ghi
│   └── course_bookings.dat (gốc): 2 bản ghi (1001, 1002)
├── Migrated / Normalized (Bổ sung chuẩn tắc): 6 bản ghi
│   └── course_bookings.dat: 6 bản ghi (1003, 1004, 1005, 1006, 1007, 1008)
├── Removed (Xóa bỏ): 0 bản ghi
└── Quarantined (Cách ly nghi vấn): 0 bản ghi
```

---

## 10. ĐỐI CHIẾU SỐ LƯỢNG BẢN GHI TRƯỚC VÀ SAU DI TRÚ

| Tên tệp dữ liệu | Số bản ghi trước | Số bản ghi sau | Chênh lệch | Ghi chú |
|---|:---:|:---:|:---:|---|
| `users.dat` | 50 | 50 | 0 | Không thay đổi |
| `bookings.dat` | 27 | 27 | 0 | Không thay đổi (Lưu trữ lịch sử) |
| `reviews.dat` | 15 | 15 | 0 | Không thay đổi |
| `course_bookings.dat` | 2 | 8 | +6 | Bổ sung 6 khóa học định kỳ canonical |
| **Tổng cộng** | **94** | **100** | **+6** | **Hệ thống dữ liệu đạt 100 bản ghi chuẩn hóa** |

---

## 11. BẢNG KÊ MÃ BĂM SHA-256 TRƯỚC VÀ SAU (SHA-256 MANIFEST)

| Tên tệp | SHA-256 Trước di trú | SHA-256 Sau di trú | Trạng thái thay đổi |
|---|---|---|:---:|
| `users.dat` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | **KHÔNG ĐỔI** |
| `bookings.dat` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | **KHÔNG ĐỔI** |
| `reviews.dat` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | **KHÔNG ĐỔI** |
| `course_bookings.dat` | `A77EE016C6096DEC7C9828D6F8D4ACFAC76AF03002E497CBEAC0072EB8DA34E7` | `DF526F4CC8ADF4F1CD6B0C7CC4D8C9CED58CD20AC03D6778588690987C5A6904` | **CẬP NHẬT CHUẨN TẮC** |

---

## 12. KIỂM CHỨNG TÍNH TOÀN VẸN LIÊN TỆP (CROSS-FILE INTEGRITY VALIDATION)

Kiểm toán đã xác nhận thành công 100% các bất biến hệ thống:
1. **Toàn vẹn tham chiếu khóa ngoại:**
   - 100% bản ghi trong `bookings.dat` và `course_bookings.dat` tham chiếu đến học viên và gia sư thực tế tồn tại trong `users.dat`.
   - 100% bản ghi trong `reviews.dat` tham chiếu đến đơn học `COMPLETED` hợp lệ và khớp thông tin học viên, gia sư.
2. **Bất biến lịch khóa học:**
   - Với mọi bản ghi trong `course_bookings.dat`: $\text{DateUtils::dayOfWeek}(\text{startDate}) \equiv \text{dayOfWeek}$.
   - Danh sách ngày học được tạo ra: $\text{sessionDate}[i] = \text{startDate} + 7 \times i$ (ngày), khoảng cách luôn chính xác 7 ngày.
3. **Phát hiện và ngăn chặn xung đột (Zero Collisions):**
   - 4 khóa học `ACCEPTED` (1001, 1002, 1003, 1004) chiếm dụng tổng cộng 28 buổi học thực tế trên lịch.
   - Không có bất kỳ sự trùng lặp nào về thời gian (cùng ngày, cùng ca học) giữa các gia sư hoặc giữa các học viên.
4. **Các trạng thái không chiếm chỗ:**
   - Khóa học `PENDING` (1005), `REJECTED` (1006), `CANCELLED` (1007), `COMPLETED` (1008) không gây chiếm dụng hay xung đột ca học.

---

## 13. KẾT QUẢ KIỂM THỬ HỒI QUY TỰ ĐỘNG (REGRESSION TEST SUITE)

Đã bổ sung bộ kiểm thử thứ 22: `run_test_data_audit_cleanup_and_migration()` vào `tests/TestRunner.cpp`. Toàn bộ 22 bộ kiểm thử thực thi thành công mỹ mãn:

```text
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine...
  -> PASS: MatchingEngine seamlessly adopted CustomRatingOnlyMatchingStrategy!
[TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy...
  -> PASS: User subclasses honor polymorphic contract seamlessly!
[TEST] 3. Verifying DIP, ISP & SRP across decoupled Services...
  -> PASS: AuthService isolated registration & authentication successfully!
  -> PASS: BookingService coordinated lifecycle and maintained schedule invariants!
  -> PASS: AdminService accurately calculated GMV, commission and moderation status!
[TEST] 4. Verifying ScheduleGuard Bitmask Collision Math...
  -> PASS: ScheduleGuard 56-slot mathematical invariants & sanitization verified!
[TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes...
  -> PASS: All sidebar labels verified clean of numeric prefixes with correct routing!
[TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence...
  -> PASS: Tutor Profile Editing passed validation, persistence, and authorization tests!
[TEST] 7. Verifying Bidirectional Timetable Synchronization (Student <-> Tutor)...
  -> PASS: Bidirectional timetable synchronization verified across all conditions!
[TEST] 8. Verifying Admin User Card Data Contract & No Undefined Fields...
  -> PASS: All admin user cards meet data contract with zero undefined fields!
[TEST] 9. Verifying User Detail Modal Contract & Non-Existent User Handling...
  -> PASS: User detail modal contract verified, error state prevents false locked status!
[TEST] 10. Verifying Verification & Lock Sync Across Admin and Student Views...
  -> PASS: Verification and lock state synchronized across Admin and Student views seamlessly!
[TEST] 11. Verifying AdminService Filters & Subject Keyword Search...
  -> PASS: AdminService filters and subject keyword search verified!
[TEST] 12. Verifying AVL Tree Multi-Value Support and Key/Value Deletion...
  -> PASS: AVL Tree multi-value indexing & rebalancing deletion verified!
[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary & Phantom Bit Sanitization...
  -> PASS: 56-bit boundary and phantom bit sanitization verified across models and deserializers!
[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence & Zero-Duplication Invariants...
  -> PASS: MockDatabaseEngine atomic rewrite, zero duplicate lines & AVL synchronization verified!
[TEST] 15. Verifying DataFileInspector Read-Only Inspection & Error Handling...
  -> PASS: DataFileInspector read-only inspection, credential redaction & error handling verified!
[TEST] 16. Verifying Phase 1 Booking Conflict Detection & Synchronization...
  -> PASS: All 15 Booking Conflict Detection scenarios verified successfully!
[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)...
  -> PASS: UserSerializer, BookingSerializer, and ReviewSerializer verified with 100% legacy parity!
[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)...
  -> PASS: FileUserRepository, FileBookingRepository, and FileReviewRepository verified successfully!
[TEST] 19. Verifying Phase 3 End-to-End Business Workflows (E2E-01 to E2E-10)...
  -> PASS: All 10 End-to-End Scenarios (E2E-01 to E2E-10) verified successfully!
[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)...
  -> PASS: All Recurring Course Booking Scenarios verified successfully!
[TEST] 21. Verifying Weekly Timetable Integration for Recurring Course Booking...
  -> PASS: All Weekly Timetable Recurring Course Scenarios verified successfully!
[TEST] 22. Verifying Comprehensive Data Audit, Parsing & Migration Invariants...
  -> PASS: All Comprehensive Data Audit, Parsing & Migration Invariants verified successfully!
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

Chạy lặp lại lần thứ hai xác nhận tính ổn định tuyệt đối (Deterministic, Zero Flakes).

---

## 14. KẾT QUẢ BIÊN DỊCH TẤT CẢ CÁC TARGETS (BUILD RESULTS)

Toàn bộ 4 target trong CMakeLists.txt được biên dịch và kiểm tra smoke test hoàn hảo:

| Target | Loại thực thi | Trạng thái biên dịch | Kết quả Smoke Test |
|---|---|:---:|---|
| `TutorMatch_Tests` | Console Executable | **100% BUILT** | Chạy toàn bộ 22 test suites, exit code 0 |
| `TutorMatch_Inspector` | Forensic Tool | **100% BUILT** | Quét 4 tệp dữ liệu, hiển thị chi tiết 100 bản ghi hợp lệ |
| `TutorMatch_CLI` | Console Menu | **100% BUILT** | Khởi chạy giao diện dòng lệnh tương tác thành công |
| `TutorMatch_GUI` | Desktop Qt/QML | **100% BUILT** | Khởi chạy tiến trình GUI (PID 13884) tải giao diện thành công |

---

## 15. VỊ TRÍ SAO LƯU VÀ HƯỚNG DẪN PHỤC HỒI (BACKUP & RESTORATION)

### 15.1. Vị trí sao lưu:
- Đường dẫn thư mục: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/backup_migration_20261106_134710/`
- Tệp manifest kiểm tra tính toàn vẹn: `manifest_sha256.txt`

### 15.2. Hướng dẫn phục hồi nguyên trạng (Rollback Procedure):
Nếu muốn khôi phục về trạng thái chính xác trước khi di trú, chỉ cần thực hiện lệnh sau trong PowerShell:

```powershell
Set-Location "C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch"
Copy-Item "data\backup_migration_20261106_134710\*" -Destination "data\" -Force
python scripts/migrate_and_audit.py --audit
```

---

## 16. HẠN CHẾ VÀ ĐIỂM LƯU Ý KỸ THUẬT (LIMITATIONS & NOTES)

1. **Cơ chế lưu trữ tệp phẳng CSV:**
   Hệ thống sử dụng các tệp `.dat` ngăn cách bằng dấu phẩy. Mặc dù `SerializerUtils::escapeCsv` đã hỗ trợ xử lý dấu phẩy trong chuỗi (ví dụ: họ tên có dấu phẩy hoặc tiểu sử), các trường nhập liệu tương lai nên tiếp tục tuân thủ quy tắc escape chuẩn.
2. **Khác biệt ngữ nghĩa giữa Availability Bitmask và Course Booking:**
   - `User.scheduleBitmask` (56 bits): Đại diện cho **thời gian rảnh thường lệ hàng tuần**.
   - `CourseBooking` (`startDate` + `weeksCount`): Đại diện cho **các buổi học theo ngày tháng cụ thể**.
   Hệ thống thời khóa biểu (Timetable) hiện tại đã tích hợp hoàn hảo cả hai lớp dữ liệu này vào giao diện tuần của học viên và gia sư.
3. **Tính nhất quán giữa `MockDatabaseEngine` và tệp đĩa:**
   Mã nguồn `MockDatabaseEngine::seedDefaultDataIfEmpty()` đã được đồng bộ để tự động sinh 3 khóa học mẫu chuẩn tắc nếu hệ thống chạy ở môi trường cơ sở dữ liệu trống.

---
**Chữ ký kỹ thuật:**  
*Lead Software Architect & Senior C++/Qt Engineer — TutorMatch Core Team*
