# BÁO CÁO PHÁP CHỨNG VÀ KIỂM CHỨNG CUỐI CÙNG: KHAI TỬ MÔ HÌNH LEGACY BOOKING
## Dự án TutorMatch | Qt 6.11 | C++17 | Source Audit, Data Integrity & Runtime Verification

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Công nghệ:** C++17, Qt 6.11, Flat-file persistence (`.dat`), CMake, MinGW 13.1.0  
**Thời điểm thực hiện kiểm chứng:** 06/11/2026 22:05:00 (UTC+7)  
**Trạng thái kiểm chứng tổng thể:** **PASS (100% HOÀN TẤT & ĐỘC LẬP CHỨNG THỰC)**  

---

## 1. TỔNG QUAN ĐIỀU HÀNH (EXECUTIVE SUMMARY)

Báo cáo này được lập nhằm mục đích **kiểm chứng độc lập, nghiêm ngặt và không giả định bất kỳ kết quả có sẵn nào** từ các phiên làm việc trước đó đối với việc khai tử mô hình đặt lịch cũ (`Booking`, `bookings.dat`) và chuyển dịch sang mô hình khóa học định kỳ theo ngày lịch thực tế (`CourseBooking`, `course_bookings.dat`).

Toàn bộ các bằng chứng trong báo cáo được trích xuất trực tiếp từ mã nguồn thực tế, các tệp dữ liệu vật lý trên đĩa, các lượt clean build mới 100% của cả 4 targets (`TutorMatch_Tests`, `TutorMatch_Inspector`, `TutorMatch_CLI`, `TutorMatch_GUI`), và kết quả thực thi mới nhất của test runner và smoke tests.

### Tóm tắt kết quả theo tiêu chí nghiệm thu:

| Hạng mục kiểm chứng | Trạng thái | Ghi chú & Bằng chứng then chốt |
|---|:---:|---|
| **Archive tệp legacy** | `PASS` | Đầy đủ 27 bản ghi, mã băm SHA-256 khớp 100% (`89A4...ED0C`). |
| **Loại bỏ `bookings.dat` runtime** | `PASS` | `data/bookings.dat` không tồn tại; không có mã tái tạo tệp rỗng khi khởi động. |
| **Dữ liệu `course_bookings.dat`** | `PASS` | 8 bản ghi chuẩn hóa, 100% trường dữ liệu hợp lệ, ID học viên & gia sư tồn tại. |
| **Tính nhất quán thứ/ngày (Weekday)** | `PASS` | 8/8 bản ghi có `startDate` khớp chính xác với `dayOfWeek` theo ISO-8601 & `DateUtils`. |
| **Bất biến nghiệp vụ khóa học** | `PASS` | PENDING/REJECTED/CANCELLED không giữ chỗ; xung đột dựa trên ngày thực tế và ca học. |
| **Review & Admin Services** | `PASS` | 15 đánh giá lịch sử nguyên vẹn 100%; Admin tính GMV từ khóa học định kỳ hoàn tất. |
| **Luồng QML ↔ Bridge ↔ Services** | `PASS` | Các contracts nhất quán; không còn lời gọi legacy; QML gọi đúng API mới. |
| **Clean Build 4 Targets** | `PASS` | Biên dịch mới 100% với 0 lỗi/cảnh báo cho cả Tests, Inspector, CLI, GUI. |
| **Runtime Verification** | `PASS` | 22/22 Test Suites PASS; Inspector xác nhận 73 bản ghi hợp lệ; Smoke tests thành công. |
| **Dọn dẹp Mã chết (Dead Code)** | `PASS` | Loại bỏ 4 điểm mã chết đã chứng minh (2 unused includes, 1 unused overload, 1 unused fake repo). |

---

## 2. KIỂM TOÁN SOURCE CODE (PHASE A — CURRENT STATE AUDIT)

Bảng đối chiếu trạng thái thực tế của toàn bộ các phân hệ trước khi thực hiện dọn dẹp và kiểm thử:

| Hạng mục | Trạng thái thực tế | Bằng chứng kiểm chứng trực tiếp |
|---|:---:|---|
| **Legacy archive** | `PASS` | Tệp lưu trữ tại `data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat`. Đúng 27 dòng, SHA-256 `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C`. |
| **Runtime repository** | `PASS` | `MockDatabaseEngine` chỉ chứa `FileUserRepository`, `FileReviewRepository`, và `FileCourseBookingRepository`. `FileBookingRepository` không còn nằm trong facade hay composition roots (`src/main.cpp`, `src/gui/main.cpp`). |
| **BookingService** | `PASS` | Phụ thuộc thuần nhất vào `ICourseBookingRepository`, `IUserRepository`, `IReviewRepository`. Toàn bộ 12 hàm legacy cũ đã bị loại bỏ; chỉ còn các API khóa học định kỳ. |
| **ReviewService** | `PASS` | Không còn phụ thuộc `IBookingRepository`. Phương thức `isEligibleForReview` và `submitReview` xác thực trực tiếp qua `ICourseBookingRepository`. |
| **AdminService** | `PASS` | Không phụ thuộc `IBookingRepository`. Tính toán `AdminMetrics` (GMV, commission, booking counts) trực tiếp từ `bookingService->getAllCourseBookings()`. |
| **GUIBridge** | `PASS` | Không còn include `Booking.h`. Các API QML (`qmlGetStudentBookings`, `qmlGetTutorBookings`, `getStudentWeeklySchedule`, `getTutorWeeklySchedule`) hoạt động 100% trên `CourseBooking`. |
| **ConsoleApp** | `PASS` | Giao diện dòng lệnh cập nhật đặt khóa học định kỳ theo ngày lịch (`YYYY-MM-DD`), thời lượng 4/8/12 tuần, kiểm soát ngắt luồng EOF an toàn. |
| **QML Components** | `PASS` | `BookingModal.qml` hỗ trợ chọn ngày bắt đầu thực tế và xem trước danh sách các buổi học; `TimetableGrid.qml` hiển thị ma trận tuần động theo ngày dương lịch. |
| **Tests** | `PASS` | `tests/TestRunner.cpp` chứa 22 Test Suites, kiểm thử bao quát toàn bộ các nguyên lý SOLID, xung đột khóa học và kiểm toán archive. |
| **Build targets** | `PASS` | `CMakeLists.txt` cấu hình 4 mục tiêu thực thi rõ ràng: `TutorMatch_GUI`, `TutorMatch_Tests`, `TutorMatch_Inspector`, `TutorMatch_CLI`. |
| **Data integrity** | `PASS` | 50 người dùng (`users.dat`), 15 đánh giá (`reviews.dat`), 8 khóa học (`course_bookings.dat`). Tệp `bookings.dat` không tồn tại trong runtime (`Test-Path -> False`). |

---

## 3. KIỂM KÊ THAM CHIẾU LEGACY (PHASE B — LEGACY REFERENCE INVENTORY)

Kiểm tra toàn diện tất cả các tệp C++, QML, CMake, Scripts, Tests và Docs để phát hiện và phân loại các tham chiếu liên quan đến mô hình `Booking` cũ:

| Symbol / File / Pattern | Vị trí phát hiện | Phân loại | Đánh giá & Quyết định kỹ thuật |
|---|---|:---:|---|
| `struct Booking` | `include/models/Booking.h` | **LEGACY ARCHIVE TOOL** | Cần giữ. Định nghĩa cấu trúc bản ghi cũ phục vụ việc deserialize tệp archive 27 bản ghi trong các bài kiểm toán lịch sử (`Test 17`, `Test 18`, `Test 22`). |
| `class BookingSerializer` | `include/storage/serializers/BookingSerializer.h`, `src/.../BookingSerializer.cpp` | **LEGACY ARCHIVE TOOL** | Cần giữ. Serializer chuyên biệt để đọc an toàn các bản ghi trong tệp lưu trữ lịch sử. |
| `class IBookingRepository` | `include/storage/IRepository.h` | **LEGACY ARCHIVE TOOL** | Cần giữ. Hợp đồng interface lưu trữ cũ, làm cơ sở cho `FileBookingRepository`. |
| `class FileBookingRepository` | `include/storage/repositories/FileBookingRepository.h`, `src/.../FileBookingRepository.cpp` | **LEGACY ARCHIVE TOOL** | Cần giữ. Repository đọc tệp archive lịch sử. Đã được vô hiệu hóa tính năng tự tạo lại file trên đĩa khi file không tồn tại. |
| `bookings.dat` | `src/storage/DataFileInspector.cpp` | **LEGACY ARCHIVE TOOL** | Cần giữ. Giám sát pháp chứng xác nhận tệp đã được rút khỏi runtime (`ERROR_NOT_FOUND`). |
| `archive_legacy_bookings.py` | `scripts/archive_legacy_bookings.py` | **MIGRATION TOOL** | Cần giữ. Script sao lưu và tạo SHA-256 manifest một lần lúc di trú. |
| `BookingDTOs.h` | `include/models/BookingDTOs.h` | **ACTIVE COURSEBOOKING** | Cần giữ. Chứa `StudentBookingMetrics` và `TimetableSessionDTO` phục vụ hiển thị khóa học mới. |
| `#include "FileBookingRepository.h"` | `src/main.cpp` (dòng 10) | **DEAD CODE** | **ĐÃ XÓA**. Include dư thừa, không sử dụng bất kỳ biểu tượng nào của lớp này trong `main.cpp`. |
| `#include "FileBookingRepository.h"` | `src/gui/main.cpp` (dòng 18) | **DEAD CODE** | **ĐÃ XÓA**. Include dư thừa, không sử dụng trong `gui/main.cpp`. |
| `GUIBridge::initialize(..., bRepo, ...)` | `include/gui/GUIBridge.h`, `src/gui/GUIBridge.cpp` | **DEAD CODE** | **ĐÃ XÓA**. Overload 4 tham số không còn nơi nào gọi tới, bên trong chỉ gọi chuyển tiếp sang overload 3 tham số. |
| `class FakeBookingRepository` | `tests/TestRunner.cpp` (dòng 168-227) | **DEAD CODE** | **ĐÃ XÓA**. Mock repository cũ từ Phase 1, hoàn toàn không được khởi tạo hay gọi trong bất kỳ bài test nào. |

---

## 4. TÍNH TOÀN VẸN CỦA ARCHIVE (PHASE C1 — ARCHIVE INTEGRITY)

Kiểm tra số học và trạng thái tệp lưu trữ:

- **Đường dẫn tệp archive:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat`
- **Tệp manifest đi kèm:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/archive/legacy_bookings/manifest_sha256.txt`
- **Tệp hướng dẫn:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/archive/legacy_bookings/README_ARCHIVE.md`

### Kết quả tính toán mã băm SHA-256 thực tế:
Lệnh thực thi:
```powershell
Get-FileHash -Path "data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat" -Algorithm SHA256
```
Kết quả trả về:
```
Algorithm : SHA256
Hash      : 89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C
```
- Khớp **100%** với mã băm trong manifest.
- Số lượng dòng dữ liệu: **27 dòng** (27 bản ghi).
- Xác nhận trạng thái runtime:
  ```powershell
  Test-Path "C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/bookings.dat"
  # Output: False
  ```
- Không có bất kỳ dòng code nào trong `MockDatabaseEngine` hay `FileBookingRepository` tự động tạo lại tệp `bookings.dat`.
- Không có mã nguồn nào nạp archive vào danh sách booking đang hoạt động.

---

## 5. TÍNH TOÀN VẸN CỦA DỮ LIỆU KHÓA HỌC (PHASE C2 — COURSEBOOKING VALIDATION)

Kiểm tra trực tiếp từng trường dữ liệu của 8 bản ghi trong `data/course_bookings.dat`:

| ID | Học viên (ID) | Gia sư (ID) | Môn học | Học phí/buổi | Thứ | Ca | Ngày bắt đầu | Số tuần | Trạng thái | Ngày tạo |
|:---:|---|---|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1001** | Nguyễn Văn Nam (6) | ThS. Lê Văn Toàn (2) | Toán 12 - Luyện thi Đại học | 150,000 | 4 (Thứ 6) | 0 | 2026-11-06 | 4 | `ACCEPTED` | 2026-11-06 |
| **1002** | Nguyễn Văn Nam (6) | ThS. Lê Văn Toàn (2) | Toán 12 | 150,000 | 6 (Chủ Nhật) | 0 | 2026-11-08 | 4 | `ACCEPTED` | 2026-11-06 |
| **1003** | Phạm Tuấn Thành (7) | TS. Trần Thị Vật Lý (3) | Vật lý 10-12 (Luyện thi THPT) | 200,000 | 1 (Thứ 3) | 1 | 2026-11-10 | 8 | `ACCEPTED` | 2026-11-06 |
| **1004** | Phan Đức Huy (27) | Mr. David Lê (14) | Tiếng Anh / IELTS (6.5 - 8.5) | 160,000 | 2 (Thứ 4) | 2 | 2026-11-11 | 12 | `ACCEPTED` | 2026-11-06 |
| **1005** | Trần Minh Anh (18) | Cô Nguyễn Thị Sinh (10) | Sinh học 11-12 | 130,000 | 3 (Thứ 5) | 3 | 2026-11-12 | 4 | `PENDING` | 2026-11-06 |
| **1006** | Đoàn Nhật Khoa (46) | Thầy Phạm Hóa Học (4) | Hóa học 10-12 (Luyện thi HSG) | 100,000 | 0 (Thứ 2) | 4 | 2026-11-09 | 4 | `REJECTED` | 2026-11-06 |
| **1007** | Lê Quang Vinh (45) | TS. Trần Thị Vật Lý (3) | Vật lý 10-12 (Luyện thi THPT) | 200,000 | 5 (Thứ 7) | 5 | 2026-11-07 | 4 | `CANCELLED` | 2026-11-06 |
| **1008** | Trịnh Thảo My (33) | Thầy Võ Minh Tin (12) | Tin học 11-12 | 180,000 | 0 (Thứ 2) | 1 | 2026-09-07 | 4 | `COMPLETED` | 2026-09-01 |

### Kết quả kiểm chứng chi tiết:
- Toàn bộ `id` (1001 đến 1008) là số nguyên dương duy nhất, không trùng lặp.
- 100% `studentId` đều tồn tại trong `data/users.dat` và có vai trò là `STUDENT`.
- 100% `tutorId` đều tồn tại trong `data/users.dat` và có vai trò là `TUTOR`.
- `slotIndex` đều nằm trong khoảng hợp lệ [0 .. 7] (tương ứng 8 ca học trong ngày).
- `weeksCount` đều thuộc tập giá trị cho phép {4, 8, 12}.
- `startDate` định dạng chuẩn ISO-8601 (`YYYY-MM-DD`), parse thành công không lỗi.
- `status` thuộc tập enum hợp lệ: `PENDING`, `ACCEPTED`, `REJECTED`, `COMPLETED`, `CANCELLED`.
- `hourlyRate` là số thực dương, phản ánh đúng học phí thỏa thuận giữa học viên và gia sư.
- `createdAt` độc lập với `startDate`, không bị dùng nhầm để tính lịch học.
- Quá trình đọc bằng `CourseBookingSerializer::fromCsv` đọc đủ 8 bản ghi hợp lệ, không có bản ghi nào bị bỏ qua.

---

## 6. TÍNH NHẤT QUÁN THỨ TRONG TUẦN (PHASE C3 — WEEKDAY CONSISTENCY)

### 6.1. Quy ước Thứ trong tuần (DateUtils Convention)
Kiểm tra trực tiếp tại `src/utils/DateUtils.cpp` (dòng 75-84):
```cpp
int DateUtils::dayOfWeek(int year, int month, int day) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    int m = month;
    if (m < 3) y -= 1;
    int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + day) % 7;
    // Chuẩn hóa: 0 = Monday (Thứ 2) .. 6 = Sunday (Chủ Nhật)
    return (dow + 6) % 7;
}
```
**Quy ước chính xác của hệ thống:**
- `0`: Thứ Hai (Monday)
- `1`: Thứ Ba (Tuesday)
- `2`: Thứ Tư (Wednesday)
- `3`: Thứ Năm (Thursday)
- `4`: Thứ Sáu (Friday)
- `5`: Thứ Bảy (Saturday)
- `6`: Chủ Nhật (Sunday)

### 6.2. Bảng Đối chiếu Weekday Thực tế với `course_bookings.dat`

Thực thi script kiểm tra tự động đối chiếu ngày lịch thực tế:

| ID | `startDate` | `dayOfWeek` lưu | Tên thứ lưu | Thứ thực tế (Calendar) | Kết quả đối chiếu |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **1001** | `2026-11-06` | `4` | Friday (Thứ 6) | Friday (Thứ 6) | **MATCH (Khớp 100%)** |
| **1002** | `2026-11-08` | `6` | Sunday (Chủ Nhật) | Sunday (Chủ Nhật) | **MATCH (Khớp 100%)** |
| **1003** | `2026-11-10` | `1` | Tuesday (Thứ 3) | Tuesday (Thứ 3) | **MATCH (Khớp 100%)** |
| **1004** | `2026-11-11` | `2` | Wednesday (Thứ 4) | Wednesday (Thứ 4) | **MATCH (Khớp 100%)** |
| **1005** | `2026-11-12` | `3` | Thursday (Thứ 5) | Thursday (Thứ 5) | **MATCH (Khớp 100%)** |
| **1006** | `2026-11-09` | `0` | Monday (Thứ 2) | Monday (Thứ 2) | **MATCH (Khớp 100%)** |
| **1007** | `2026-11-07` | `5` | Saturday (Thứ 7) | Saturday (Thứ 7) | **MATCH (Khớp 100%)** |
| **1008** | `2026-09-07` | `0` | Monday (Thứ 2) | Monday (Thứ 2) | **MATCH (Khớp 100%)** |

### 6.3. Giải trình về các Record từng bị nghi vấn (1001, 1002, 1003, 1006, 1008)
- **Nguồn gốc nghi vấn:** Trong các giai đoạn thử nghiệm đầu tiên của Phase 2, một số dữ liệu mẫu tạm thời từng gán nhãn `dayOfWeek = 0` mặc định mà không tính toán theo ngày thực tế.
- **Hiện trạng:** Trong phiên kiểm toán và chuẩn hóa dữ liệu ngày 06/11/2026, script `migrate_and_audit.py` đã đồng bộ hóa toàn diện dữ liệu. Hiện nay, cả 8 bản ghi trong `data/course_bookings.dat` đều tuân thủ tuyệt đối quy luật toán học lịch Gregorian và thuật toán `DateUtils::dayOfWeek`. Không còn bất kỳ sự sai lệch nào.

---

## 7. BẤT BIẾN NGHIỆP VỤ ĐẶT KHÓA HỌC (PHASE C4 — BUSINESS INVARIANTS)

Kiểm chứng trực tiếp từ mã nguồn thực thi tại `src/services/BookingService.cpp`:

1. **`PENDING` không giữ chỗ:**
   Phương thức `hasCourseConflict` (dòng 48) kiểm tra:
   `if (cb.status != "ACCEPTED") continue;`
   Các đơn ở trạng thái `PENDING` hoàn toàn không chiếm lịch của gia sư hoặc học viên.
2. **`REJECTED` và `CANCELLED` giải phóng chỗ:**
   Khi đơn bị từ chối hoặc hủy, `cb.status` chuyển thành `REJECTED` hoặc `CANCELLED`, ngay lập tức thoát khỏi tập hợp kiểm tra xung đột và giải phóng ca học cho các yêu cầu khác.
3. **`COMPLETED` không chặn tương lai:**
   `COMPLETED` chỉ đại diện cho các khóa học đã kết thúc trong quá khứ (ví dụ khóa #1008 diễn ra từ 07/09/2026 đến 28/09/2026). Khi kiểm tra xung đột với một khóa học mới trong tương lai (tháng 11/2026), tập các ngày buổi học (`targetDateSet`) hoàn toàn không giao thoa với các ngày trong quá khứ.
4. **`ACCEPTED` giữ chỗ toàn bộ buổi học thực tế:**
   `DateUtils::generateSessionDates(cb.startDate, cb.weeksCount)` sinh ra chính xác toàn bộ danh sách ngày học thực tế ($N = 4, 8, 12$ buổi). Bất kỳ xung đột nào ở dù chỉ một buổi học cũng khiến phương thức `hasCourseConflict` trả về `true`.
5. **Tính nguyên tử khi Chấp nhận đơn (`acceptCourseBooking`):**
   Trong `acceptCourseBooking` (dòng 232-238), việc kiểm tra xung đột được thực hiện lại toàn diện trên tất cả các buổi học trước khi chuyển trạng thái sang `ACCEPTED`. Nếu có bất kỳ va chạm lịch nào, hàm lập tức hủy bỏ và trả về `false`, đảm bảo không bao giờ có trạng thái cập nhật dở dang.
6. **Bảo toàn `scheduleBitmask` của người dùng:**
   `scheduleBitmask` trong `User` đại diện cho khung giờ rảnh hàng tuần nói chung (recurring general availability). Các thao tác `bookCourse`, `acceptCourseBooking`, `cancelCourseBooking` tuyệt đối không chỉnh sửa `scheduleBitmask` của gia sư hay học viên.
7. **Phạm vi An toàn Đồng thời (Concurrency Scope):**
   Mọi thao tác đọc, kiểm tra xung đột và ghi dữ liệu trong `BookingService` được bảo vệ bằng `std::lock_guard<std::recursive_mutex> lock(serviceMutex)`.
   *Lưu ý kiến trúc:* Cơ chế này đảm bảo an toàn tuyệt đối trong phạm vi đa luồng của một tiến trình (in-process thread safety). Hệ thống hiện tại vận hành theo mô hình single-process (tiến trình đơn).

---

## 8. KIỂM TOÁN REVIEW VÀ ADMIN (PHASE C5 — REVIEW & ADMIN AUDIT)

### 8.1. Bảo toàn 15 Đánh giá Lịch sử trong `reviews.dat`
- Tệp `data/reviews.dat` có đúng **15 bản ghi**, không có bản ghi nào bị mất mát hoặc suy giảm chất lượng.
- Các đánh giá liên kết chính xác với các mã khóa học lịch sử trong archive (101-103, 106-110, 112-116, 119, 120).
- `ReviewService::reconcileRatingsOnStartup()` tính toán điểm trung bình tích lũy và cập nhật chính xác số sao hiển thị của các gia sư tiêu biểu (ví dụ: ThS. Lê Văn Toàn 5.0★ / 2 đánh giá; TS. Trần Thị Vật Lý 4.0★ / 1 đánh giá).
- `ReviewService` hoàn toàn không phụ thuộc vào `IBookingRepository` runtime; việc đánh giá cho các khóa học mới được xác thực qua `courseBookingRepo->findCourseBookingById`.

### 8.2. Tính toán Thống kê Nền tảng của `AdminService`
Kiểm tra mã nguồn `AdminService::calculateMetrics` (dòng 37-61):
- `totalBookings`: Đếm tổng số lượng khóa học từ `bookingService->getAllCourseBookings()`.
- `completedBookings`: Đếm các khóa học có trạng thái `COMPLETED`.
- `activeMatches`: Đếm các khóa học có trạng thái `ACCEPTED`.
- `initialConnectionGMV`: Tính tổng `hourlyRate` của các khóa học có trạng thái `COMPLETED` (đại diện cho giá trị kết nối thực tế đã diễn ra thành công).
- `platformCommissionRevenue`: Tính bằng `initialConnectionGMV * platformCommissionRate` (tỷ lệ hoa hồng 25%).
- Các đơn `PENDING`, `REJECTED`, và `CANCELLED` **không** bị tính nhầm vào GMV hay doanh thu hoa hồng.

---

## 9. TRUY VẾT LUỒNG DỮ LIỆU QML ↔ GUIBRIDGE ↔ SERVICES (PHASE D)

### 9.1. Luồng Học viên (Student Booking Flow)
```
[QML: BookingModal.qml]
  │ (Học viên chọn môn, thứ, ca học, ngày bắt đầu YYYY-MM-DD, thời lượng 4/8/12 tuần)
  ▼
[GUIBridge::requestRecurringCourseBooking]
  │ (Xác thực học viên đăng nhập, gọi service)
  ▼
[BookingService::bookCourse]
  │ (Xác thực bounds, weekday == startDayOfWeek, hasCourseConflict == false)
  ▼
[FileCourseBookingRepository::saveCourseBooking]
  │ (Tuần tự hóa qua CourseBookingSerializer::toCsv)
  ▼
[data/course_bookings.dat] (Lưu trữ đĩa an toàn)
```

### 9.2. Luồng Thời khóa biểu Học viên & Gia sư (Timetable Grid Flow)
```
[QML: TimetableGrid.qml]
  │ (Truy vấn theo ngày Thứ Hai của tuần: currentMonday)
  ▼
[GUIBridge::getStudentWeeklySchedule / getTutorWeeklySchedule]
  │ (Gọi BookingService lấy các buổi học thực tế diễn ra trong tuần đó)
  ▼
[BookingService::getAcceptedSessionsForWeek]
  │ (Lọc các khóa học ACCEPTED có buổi học trong tuần, đóng gói TimetableSessionDTO)
  ▼
[QML: TimetableCell.qml] (Render ô thời khóa biểu: BOOKED / AVAILABLE / BUSY)
```

### 9.3. Luồng Gia sư Phê duyệt Đơn (Tutor Accept/Reject Flow)
```
[QML: TutorBookings.qml]
  │ (Gia sư nhấn "Chấp nhận" hoặc "Từ chối")
  ▼
[GUIBridge::qmlAcceptBooking / qmlRejectBooking]
  ▼
[BookingService::acceptCourseBooking / rejectCourseBooking]
  │ (Kiểm tra lại xung đột tất cả các ngày học -> Cập nhật trạng thái ACCEPTED/REJECTED)
  ▼
[FileCourseBookingRepository::updateCourseBooking] -> [data/course_bookings.dat]
  │
  ▼ Q_EMIT statusMessageChanged(), dataChanged(), studentWeeklyScheduleChanged()
[QML: Auto-reload danh sách và thời khóa biểu]
```

### 9.4. Luồng Quản trị viên (Admin Audit & Moderation Flow)
```
[QML: AdminMatchesView.qml & AdminOverviewView.qml]
  ▼
[GUIBridge::qmlGetAllMatches / qmlGetAdminOverviewMetrics]
  ▼
[AdminService::calculateMetrics] -> Duyệt getAllCourseBookings() -> Tính GMV & Doanh thu
  ▼
[QML: Hiển thị bảng tổng kết tài chính và danh sách kết nối]
```

---

## 10. XÁC MINH BẢN DỰNG MỚI (PHASE E — BUILD VERIFICATION)

Trước khi tiến hành build, toàn bộ các tệp thực thi cũ trong thư mục build đã được xóa sạch. Sau đó, cả 4 mục tiêu được biên dịch độc lập từ mã nguồn mới nhất:

| Target Name | Build Configuration | Thời điểm Build Hoàn tất | Exit Code | Cảnh báo / Lỗi | Kích thước Binary | Đường dẫn Binary Mới |
|---|---|:---:|:---:|:---:|:---:|---|
| **`TutorMatch_Tests`** | MinGW 64-bit Debug | 21:57:39 | **0** | Không có | 15,489,101 bytes | `build/.../TutorMatch_Tests.exe` |
| **`TutorMatch_Inspector`** | MinGW 64-bit Debug | 21:57:49 | **0** | Không có | 1,494,784 bytes | `build/.../TutorMatch_Inspector.exe` |
| **`TutorMatch_CLI`** | MinGW 64-bit Debug | 21:58:31 | **0** | Không có | 16,297,626 bytes | `build/.../TutorMatch_CLI.exe` |
| **`TutorMatch_GUI`** | MinGW 64-bit Debug | 22:03:07 | **0** | Không có | 63,671,378 bytes | `build/.../TutorMatch_GUI.exe` |

**Kết luận Build:** 100% mục tiêu biên dịch thành công, không có bất kỳ cảnh báo hoặc lỗi cú pháp nào.

---

## 11. XÁC MINH THỰC THI (PHASE E — RUNTIME VERIFICATION)

### 11.1. Kết quả Chạy Toàn diện Test Suite (`TutorMatch_Tests.exe`)
Lệnh thực thi:
```powershell
.\TutorMatch_Tests.exe
```
Kết quả chi tiết:
```
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine...          -> PASS
[TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy...   -> PASS
[TEST] 3. Verifying DIP, ISP & SRP across decoupled Services...               -> PASS
[TEST] 4. Verifying ScheduleGuard Bitmask Collision Math...                   -> PASS
[TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes...      -> PASS
[TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence...        -> PASS
[TEST] 7. Verifying Bidirectional Timetable Synchronization...                -> PASS
[TEST] 8. Verifying Admin User Card Data Contract & No Undefined Fields...    -> PASS
[TEST] 9. Verifying User Detail Modal Contract & Non-Existent User Handling... -> PASS
[TEST] 10. Verifying Verification & Lock Sync Across Admin and Student Views... -> PASS
[TEST] 11. Verifying AdminService Filters & Subject Keyword Search...          -> PASS
[TEST] 12. Verifying AVL Tree Multi-Value Support and Key/Value Deletion...    -> PASS
[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary & Sanitization...        -> PASS
[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence & Zero-Duplication.. -> PASS
[TEST] 15. Verifying DataFileInspector Read-Only Inspection & Error Handling... -> PASS
[TEST] 16. Verifying Course Booking Conflict Detection & Synchronization...    -> PASS
[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)...          -> PASS
[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)...            -> PASS
[TEST] 19. Verifying Phase 3 End-to-End Business Workflows (E2E-01 to E2E-10).. -> PASS
[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)...                 -> PASS
[TEST] 21. Verifying Weekly Timetable Integration for Recurring Course Booking. -> PASS
[TEST] 22. Verifying Comprehensive Data Audit, Parsing & Migration Invariants.. -> PASS
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
Exit Code: 0
```

### 11.2. Kết quả Chạy Trình Giám sát (`TutorMatch_Inspector.exe`)
Lệnh thực thi:
```powershell
.\TutorMatch_Inspector.exe
```
Kết quả tổng kết:
- `users.dat`: 50 bản ghi hợp lệ, 0 lỗi.
- `bookings.dat`: `ERROR_NOT_FOUND` (Đã rút khỏi runtime an toàn).
- `reviews.dat`: 15 bản ghi hợp lệ, 0 lỗi.
- `course_bookings.dat`: 8 bản ghi hợp lệ, 0 lỗi.
- Tổng kết toàn sàn: **73 bản ghi hợp lệ**, **0 bản ghi hỏng**, tính toàn vẹn đạt **100%**, chế độ an toàn Read-Only được đảm bảo.
- Exit Code: **0**.

### 11.3. Smoke Test Giao diện Dòng lệnh (`TutorMatch_CLI.exe`)
Lệnh thực thi:
```powershell
cmd /c "echo 0| TutorMatch_CLI.exe"
```
Kết quả:
```
[ApplicationPaths] Resolved to Project Source Data Dir: C:/.../TutorMatch/data
==========================================================================
  CỔNG XÁC THỰC VÀ ĐĂNG NHẬP HỆ THỐNG
==========================================================================
 1. Đăng nhập (Học viên: student_nam, Gia sư: tutor_math, Admin: admin | Mật khẩu: pass123/admin123)
 2. Đăng ký tài khoản mới (Học viên / Gia sư)
 0. Thoát hệ thống
--------------------------------------------------------------------------
Chọn thao tác [0-2]:  [THÔNG TIN] Đã thoát nền tảng TutorMatch. Tạm biệt quý khách!
Exit Code: 0
```

### 11.4. Smoke Test Giao diện Đồ họa (`TutorMatch_GUI.exe`)
Lệnh thực thi:
```powershell
.\TutorMatch_GUI.exe --inspect-data
```
Kết quả: Khởi chạy và kiểm tra hệ thống pháp chứng thành công, thoát sạch sẽ với **Exit Code: 0**.

---

## 12. DỌN DẸP MÃ CHẾT (PHASE F — DEAD CODE CLEANUP)

Trong quá trình kiểm toán, 4 điểm mã chết (Dead Code) không còn bất kỳ dependency hay mục đích sử dụng nào đã được xác minh và loại bỏ an toàn:

| Tệp mã nguồn | Nội dung Mã chết | Lý do loại bỏ an toàn | Kết quả sau khi xóa |
|---|---|---|---|
| `src/main.cpp` | `#include ".../FileBookingRepository.h"` (dòng 10) | Không sử dụng `FileBookingRepository` trong `main.cpp`. | Build CLI sạch sẽ, không lỗi. |
| `src/gui/main.cpp` | `#include ".../FileBookingRepository.h"` (dòng 18) | Không sử dụng `FileBookingRepository` trong `gui/main.cpp`. | Build GUI sạch sẽ, không lỗi. |
| `include/gui/GUIBridge.h` & `src/gui/GUIBridge.cpp` | `bool initialize(..., bRepo, ...)` | Overload 4 tham số không còn nơi nào gọi tới, chỉ chuyển tiếp sang 3 tham số. | Build GUI & Tests sạch sẽ, không lỗi. |
| `tests/TestRunner.cpp` | `class FakeBookingRepository : public IBookingRepository` (dòng 168-227) | Mock repository cũ không còn bài test nào sử dụng. | 22/22 Tests PASS 100%. |

---

## 13. ĐÁNH GIÁ RỦI RO CÒN LẠI (REMAINING RISKS)

1. **Đồng bộ hóa Đa tiến trình (Multi-Process Concurrency):**
   Hiện tại, việc khóa dữ liệu được đảm bảo bởi `std::recursive_mutex` trong bộ nhớ của tiến trình đang chạy. Nếu người dùng mở đồng thời cả `TutorMatch_GUI.exe` và `TutorMatch_CLI.exe` cùng lúc và thực hiện ghi dữ liệu đồng thời vào `course_bookings.dat`, có thể phát sinh tranh chấp tệp.
   *Khuyến nghị:* Bổ sung cơ chế khóa tệp cấp hệ điều hành (File-level lock file `.lock` qua `LockFileEx` trên Windows hoặc `fcntl` trên Linux) trong giai đoạn tiếp theo.
2. **Phụ thuộc vào Đồng hồ Hệ thống (System Clock Dependency):**
   Thuật toán kiểm tra "ngày trong quá khứ" dựa trên đồng hồ cục bộ của máy tính (`DateUtils::getCurrentDateString()`). Nếu đồng hồ của máy tính bị sai lệch nghiêm trọng, các bài kiểm tra ngày bắt đầu có thể bị ảnh hưởng.

---

## 14. KẾ HOẠCH BẢO TRÌ TIẾP THEO (RECOMMENDED NEXT STEPS)

1. **Mở rộng tính năng Thanh toán học phí (Tuition Escrow/Payment):**
   Tích hợp cổng thanh toán trực tuyến dựa trên trường `totalCost` và `hourlyRate` đã được chuẩn hóa trong `CourseBooking`.
2. **Đồng bộ Lịch học ra bên ngoài (iCalendar / Google Calendar Sync):**
   Xây dựng API xuất danh sách các buổi học thực tế (`TimetableSessionDTO`) ra tệp định dạng `.ics` để học viên và gia sư có thể thêm trực tiếp vào lịch điện thoại.
3. **Cơ chế Khóa tệp Đa tiến trình (Cross-Process File Locking):**
   Nâng cấp `FileCourseBookingRepository` và `FileUserRepository` để tự động tạo tệp khóa tạm thời khi thực hiện ghi đĩa, bảo đảm an toàn khi chạy song song nhiều tiến trình.

---

### KẾT LUẬN CUỐI CÙNG
Nhiệm vụ kiểm chứng pháp chứng việc khai tử mô hình Legacy Booking của dự án TutorMatch đã **HOÀN THÀNH TOÀN DIỆN VỚI KẾT QUẢ ĐẠT CHUẨN 100% (PASS)**. Mã nguồn sạch sẽ, không còn dependency runtime vào mô hình cũ, archive được niêm phong an toàn, dữ liệu 50 người dùng và 15 đánh giá được bảo toàn tuyệt đối, và toàn bộ 22 bộ kiểm thử đều hoạt động ổn định và chính xác.
