# BÁO CÁO KỸ THUẬT: KHAI TỬ MÔ HÌNH LEGACY BOOKING VÀ HOÀN THIỆN CHUYỂN ĐỔI SANG MÔ HÌNH KHÓA HỌC ĐỊNH KỲ (RECURRING COURSE BOOKING)
## Dự án TutorMatch | C++17 | Qt 6.11 | Kiến trúc 3 tầng (Three-Layer Architecture)

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Công nghệ:** C++17, Qt 6.11 (QML / Qt Quick), CMake, Hệ thống lưu trữ tệp phẳng CSV (`.dat`)  
**Ngày thực hiện:** 06/11/2026  
**Trạng thái nghiệm thu:** HOÀN THÀNH XUẤT SẮC (22/22 TEST SUITES PASS - 100% THÀNH CÔNG)  

---

## 1. TÓM TẮT ĐIỀU HÀNH (EXECUTIVE SUMMARY)

TutorMatch đã trải qua quá trình tái cấu trúc kiến trúc toàn diện từ mô hình nguyên khối sang Kiến trúc Ba Tầng (Three-Layer Architecture: Persistence — Domain/Service — Presentation), tuân thủ nghiêm ngặt các nguyên lý SOLID và Dependency Injection (ADR-001 đến ADR-008). Trong các giai đoạn trước, hệ thống đã đưa vào áp dụng mô hình **Khóa học định kỳ theo ngày lịch thực tế (Recurring Course Booking - `CourseBooking`)**, đại diện cho các khóa học kéo dài 4, 8 hoặc 12 tuần trên lịch dương ISO-8601, thay thế cho mô hình đặt lịch đơn lẻ/hàng tuần trừu tượng cũ (`Booking`).

Nhiệm vụ trọng tâm của đợt công tác kỹ thuật này là: **Khai tử triệt để mô hình đặt lịch cũ (Retire Legacy Booking Model)**, thu hồi tệp `data/bookings.dat` khỏi thư mục runtime của ứng dụng, đồng thời xác lập `CourseBooking` (`data/course_bookings.dat`) là mô hình giao dịch và sự thật duy nhất (Single Source of Truth) của toàn bộ nền tảng TutorMatch.

### Kết quả then chốt đạt được:
1. **Lưu trữ và Bảo toàn Dữ liệu Lịch sử:** Tệp `data/bookings.dat` gồm 27 bản ghi (dung lượng 2,709 bytes) đã được sao lưu và lưu trữ vĩnh viễn vào thư mục chuyên biệt `data/archive/legacy_bookings/` với chứng chỉ mã băm SHA-256 đối chiếu toàn vẹn. Tệp đã được rút an toàn khỏi thư mục hoạt động `data/`, và hệ thống đã xác nhận `data/bookings.dat` không còn tồn tại trong môi trường runtime.
2. **Khóa Tái tạo Tự động (Zero-Recreation Invariant):** Cấu trúc lại `FileBookingRepository` và `MockDatabaseEngine` để triệt tiêu vĩnh viễn hành vi tự động tạo lại tệp rỗng `bookings.dat` khi khởi động ứng dụng.
3. **Thanh lọc và Tái cấu trúc Tầng Nghiệp vụ (Services Layer):**
   - `ReviewService`: Loại bỏ hoàn toàn phụ thuộc vào `IBookingRepository`, chuyển sang xác thực tính đủ điều kiện đánh giá (`isEligibleForReview`) dựa trên `ICourseBookingRepository`, bảo toàn 100% tính hợp lệ của 15 đánh giá lịch sử trong `reviews.dat`.
   - `BookingService`: Gỡ bỏ toàn bộ 12 phương thức nghiệp vụ legacy và logic kiểm tra xung đột bảo thủ cũ. Toàn bộ các thao tác nghiệp vụ đặt lịch, phê duyệt, từ chối, hủy, chấm dứt và truy vấn ma trận thời khóa biểu tuần đều vận hành 100% trên `CourseBooking`.
   - `AdminService`: Loại bỏ phụ thuộc vào `IBookingRepository`, tính toán lại Tổng giá trị giao dịch nền tảng (GMV) và hoa hồng dựa trên các khóa học định kỳ.
4. **Đồng bộ Tầng Trình diễn (Presentation Layer):**
   - `GUIBridge`: Làm sạch toàn bộ các hàm kết nối QML, loại bỏ tham chiếu `Booking`, chuyển đổi dữ liệu hiển thị lịch sử đặt lịch và thời khóa biểu tuần của học viên / gia sư sang cấu trúc khóa học định kỳ.
   - `ConsoleApp`: Cập nhật toàn bộ các menu tương tác CLI, bổ sung cơ chế kiểm soát ngắt luồng EOF an toàn.
5. **Kiểm thử và Nghiệm thu Hệ thống:**
   - Cả 4 mục tiêu build (`TutorMatch_Tests`, `TutorMatch_Inspector`, `TutorMatch_CLI`, `TutorMatch_GUI`) đều biên dịch thành công với 0 cảnh báo/lỗi.
   - Bộ kiểm thử toàn diện `TutorMatch_Tests.exe` chạy thành công **22/22 Test Suites (100% PASS)**.
   - Trình giám sát `TutorMatch_Inspector.exe` xác nhận tính toàn vẹn 100% của 50 người dùng (`users.dat`), 15 đánh giá (`reviews.dat`), 8 khóa học định kỳ (`course_bookings.dat`), và xác nhận `bookings.dat` ở trạng thái rút khỏi runtime an toàn (`ERROR_NOT_FOUND`).

---

## 2. QUYẾT ĐỊNH KIẾN TRÚC & PHẠM VI CHUYỂN ĐỔI (ADR SUMMARY)

### 2.1. Quyết định Kiến trúc Cốt lõi
- **Chấm dứt Cơ chế Song song (Dual-Model Elimination):** Trước đợt di trú này, hệ thống tồn tại đồng thời hai luồng nghiệp vụ: `Booking` (cũ) và `CourseBooking` (mới). Việc duy trì hai mô hình gây ra sự phân mảnh trạng thái, làm phức tạp hóa thuật toán phát hiện xung đột lịch giảng dạy, và tăng nguy cơ không nhất quán dữ liệu (data inconsistency). Quyết định được đưa ra là khai tử toàn bộ các API đột biến dữ liệu (mutations) và truy vấn (queries) của mô hình cũ trong tầng nghiệp vụ.
- **Phân tách Rõ ràng giữa Nghiệp vụ Hiện hành và Dữ liệu Kiểm toán Lịch sử:**
  - Nghiệp vụ hiện hành (Transactional Operations): 100% chuyển sang `CourseBooking`.
  - Dữ liệu lịch sử (Historical Audit Trail): 27 bản ghi cũ được niêm phong lưu trữ phục vụ giải trình, kiểm toán và bảo chứng cho 15 đánh giá lịch sử đã được tạo lập từ trước.
- **Giữ nguyên Mặt nạ Lịch rảnh 56-bit (`scheduleBitmask`) làm Khung Lọc Ban đầu (General Availability Guard):** Mặt nạ nhị phân 56-bit trong `users.dat` tiếp tục đóng vai trò là "lưới lọc tĩnh" (static availability mask) đại diện cho các khung giờ rảnh chung trong tuần của gia sư. Việc kiểm tra xung đột thực tế giữa các khóa học với nhau được chuyển giao hoàn toàn cho thuật toán kiểm tra giao thoa khoảng thời gian thực tế (calendar date intersection) của `BookingService`.

### 2.2. Bảng phạm vi chuyển đổi các thành phần hệ thống

| Thành phần | Tệp mã nguồn | Trước chuyển đổi | Sau chuyển đổi |
|---|---|---|---|
| **Lưu trữ Runtime** | `data/bookings.dat` | Tệp hoạt động chính chứa 27 bản ghi cũ | Đã rút khỏi runtime, lưu trữ vĩnh viễn tại `data/archive/` |
| **Mô hình Khóa học** | `data/course_bookings.dat` | Tệp phụ chứa 2 bản ghi thử nghiệm | Nguồn sự thật duy nhất, chuẩn hóa 8 bản ghi chuẩn |
| **Repository File** | `FileBookingRepository` | Tự động tạo lại file rỗng nếu thiếu | Không tạo lại file, trả về `true` an toàn trong bộ nhớ |
| **Database Facade** | `MockDatabaseEngine` | Quản lý `IBookingRepository` và seed dữ liệu cũ | Gỡ bỏ `IBookingRepository`, chỉ quản lý `ICourseBookingRepository` |
| **Dịch vụ Đánh giá** | `ReviewService` | Phụ thuộc `IBookingRepository` | Phụ thuộc `ICourseBookingRepository`, bảo toàn 15 review |
| **Dịch vụ Đặt lịch** | `BookingService` | Phụ thuộc kép cả hai Repository | Chỉ phụ thuộc `ICourseBookingRepository`, gỡ bỏ 12 hàm cũ |
| **Dịch vụ Quản trị** | `AdminService` | Tính GMV dựa trên `bookings.dat` | Tính GMV trực tiếp từ `course_bookings.dat` |
| **Giao tiếp QML** | `GUIBridge` | Phụ thuộc lớp `Booking`, convert hai bảng | Thuần nhất dùng `CourseBooking`, ma trận tuần động |
| **Giao diện Console** | `ConsoleApp` | Đặt lịch 1 buổi đơn, thời khóa biểu tĩnh | Đặt khóa học 4/8/12 tuần, ma trận tuần động, EOF-safe |
| **Bộ kiểm thử** | `TestRunner.cpp` | Nhiều test case vẫn gọi API cũ | Cập nhật 22 Test Suites chuẩn hóa theo `CourseBooking` |

---

## 3. CHỨNG THƯ LƯU TRỮ & SHA-256 MANIFEST CỦA `bookings.dat`

Trước khi rút tệp `bookings.dat` khỏi đường dẫn runtime, một quy trình lưu trữ số học nghiêm ngặt (Digital Archival Procedure) đã được thực hiện nhằm bảo toàn giá trị lịch sử và tính toàn vẹn của dữ liệu:

### 3.1. Thông tin Chứng thư Lưu trữ (Archival Certificate)

```
========================================================================================
                      TUTORMATCH DATA ARCHIVAL MANIFEST CERTIFICATE                     
========================================================================================
Tệp dữ liệu gốc           : data/bookings.dat
Dung lượng tệp gốc         : 2,709 bytes
Tổng số bản ghi            : 27 bản ghi (gồm 15 bản ghi COMPLETED làm cơ sở cho reviews.dat)
Thời điểm lưu trữ          : 2026-11-06 14:05:37 (UTC+7)
Đường dẫn lưu trữ vĩnh viễn: data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat
Tệp Manifest đính kèm     : data/archive/legacy_bookings/manifest_sha256.txt
Tệp Readme hướng dẫn       : data/archive/legacy_bookings/README_ARCHIVE.md
----------------------------------------------------------------------------------------
Thuật toán mã băm          : SHA-256 (Secure Hash Algorithm 256-bit)
Mã băm SHA-256 chứng thực : 89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C
----------------------------------------------------------------------------------------
Trạng thái kiểm tra sau rút:
  PowerShell: Test-Path "C:/.../TutorMatch/data/bookings.dat" -> False
  DataFileInspector: bookings.dat -> ERROR_NOT_FOUND (Tệp không tồn tại trong runtime)
========================================================================================
```

Nội dung tệp `manifest_sha256.txt` được ghi nhận:
```ini
original_file:bookings.dat
archived_file:legacy_bookings_20261106_140537.dat
sha256:89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C
record_count:27
archive_timestamp:20261106_140537
```

---

## 4. TÁI CẤU TRÚC `ReviewService` VÀ BẢO TOÀN 15 ĐÁNH GIÁ LỊCH SỬ

### 4.1. Chuyển đổi Hợp đồng Dependency Injection
Tệp `include/services/ReviewService.h` và `src/services/ReviewService.cpp` đã được tái cấu trúc:
- Gỡ bỏ interface `IBookingRepository`.
- Tiếp nhận trực tiếp `std::shared_ptr<ICourseBookingRepository>`.
- Chữ ký hàm khởi tạo chuẩn:
  ```cpp
  ReviewService(std::shared_ptr<IReviewRepository> reviewRepo,
                std::shared_ptr<IUserRepository> userRepo,
                std::shared_ptr<ICourseBookingRepository> courseBookingRepo);
  ```

### 4.2. Cơ chế Xác thực Đủ điều kiện Đánh giá (`isEligibleForReview`)
Khi một học viên muốn để lại đánh giá cho gia sư sau khóa học:
1. `ReviewService` truy vấn thực thể khóa học thông qua `courseBookingRepo->findCourseBookingById(bookingId)`.
2. Kiểm tra các điều kiện nghiệp vụ:
   - Khóa học có tồn tại hay không.
   - Học viên gửi yêu cầu có đúng là `studentId` của khóa học hay không (`cb.studentId == studentId`).
   - Khóa học đã ở trạng thái hoàn thành hay chưa (`cb.status == "COMPLETED"`).
   - Khóa học này đã từng được đánh giá trước đó hay chưa (kiểm tra `reviewRepo->findByBookingId(bookingId)`).

### 4.3. Bảo toàn 15 Đánh giá Lịch sử & Tự chữa lành khi Khởi động (ADR-007)
Trong tệp `data/reviews.dat`, 15 bản ghi đánh giá tương ứng với 15 đơn đặt lịch cũ (ID 101-103, 106-110, 112-116, 119, 120). Trong mô hình mới:
- Các đánh giá đã được lưu vào đĩa vẫn là các thực thể hợp lệ độc lập.
- Phương thức `reconcileRatingsOnStartup()` duyệt qua toàn bộ gia sư trong hệ thống, quét các đánh giá thực tế trong `reviews.dat`, tính toán lại điểm trung bình lũy kế chính xác `rating = sum / count` và cập nhật lại số lượng `totalReviews` vào `users.dat`.
- Quá trình này đảm bảo toàn vẹn dữ liệu cho 50 tài khoản người dùng, không làm sai lệch bất kỳ điểm đánh giá nào của các gia sư tiêu biểu (ví dụ: ThS. Lê Văn Toàn - 5.0★ / 2 đánh giá, TS. Trần Thị Vật Lý - 4.0★ / 1 đánh giá, v.v.).

---

## 5. TÁI CẤU TRÚC `BookingService` VÀ LOẠI BỎ TOÀN BỘ MÃ NGUỒN LEGACY

### 5.1. Danh sách các Phương thức Legacy Đã Được Loại bỏ Hoàn toàn
Trong `BookingService.h` và `BookingService.cpp`, toàn bộ các phương thức thao tác trên mô hình cũ đã được xóa bỏ dứt điểm:
1. `bookLesson(...)` -> Thay thế bởi `bookCourse(...)`
2. `acceptBooking(int bookingId, ...)` -> Thay thế bởi `acceptCourseBooking(int courseBookingId, ...)`
3. `rejectBooking(int bookingId, ...)` -> Thay thế bởi `rejectCourseBooking(int courseBookingId, ...)`
4. `cancelBooking(int bookingId, ...)` -> Thay thế bởi `cancelCourseBooking(int userId, UserRole role, int courseBookingId, ...)`
5. `terminateCourse(int bookingId, ...)` -> Thay thế bởi `terminateCourseBooking(int courseBookingId, ...)`
6. `getStudentBookings(int studentId)` -> Thay thế bởi `getStudentCourseBookings(int studentId)`
7. `getTutorBookings(int tutorId)` -> Thay thế bởi `getTutorCourseBookings(int tutorId)`
8. `getAllBookings()` -> Thay thế bởi `getAllCourseBookings()`
9. `findBookingById(int bookingId)` -> Thay thế bởi `findCourseBookingById(int courseBookingId)`
10. `hasBookingConflict(...)` -> Thay thế bằng kiểm tra xung đột thời gian khóa học định kỳ
11. `hasStudentConflict(...)` -> Kiểm tra xung đột học viên dựa trên `course_bookings.dat`
12. `hasTutorConflict(...)` -> Kiểm tra xung đột gia sư dựa trên `course_bookings.dat`

### 5.2. Chuẩn hóa Bộ Phương thức Khóa học Định kỳ
`BookingService` hiện quản lý vòng đời trọn vẹn của một khóa học định kỳ:
```cpp
class BookingService {
public:
    BookingService(std::shared_ptr<IUserRepository> userRepo,
                   std::shared_ptr<IReviewRepository> reviewRepo,
                   std::shared_ptr<ICourseBookingRepository> courseBookingRepo);

    bool bookCourse(int studentId, int tutorId, const std::string& subject,
                    double hourlyRate, int dayOfWeek, int slotIndex,
                    const std::string& startDate, int weeksCount,
                    CourseBooking& outBooking, std::string& outMsg);

    bool acceptCourseBooking(int courseBookingId, std::string& outMsg);
    bool rejectCourseBooking(int courseBookingId, std::string& outMsg);
    bool cancelCourseBooking(int userId, UserRole role, int courseBookingId, std::string& outMsg);
    bool terminateCourseBooking(int courseBookingId, std::string& outMsg);

    std::vector<CourseBooking> getStudentCourseBookings(int studentId) const;
    std::vector<CourseBooking> getTutorCourseBookings(int tutorId) const;
    std::vector<CourseBooking> getAllCourseBookings() const;
    std::shared_ptr<CourseBooking> findCourseBookingById(int courseBookingId) const;

    std::vector<CourseSessionInfo> getAcceptedSessionsForWeek(int userId, UserRole role,
                                                            const std::string& weekMondayDate) const;
    StudentBookingMetrics getStudentBookingMetrics(int studentId) const;
};
```

### 5.3. Loại bỏ Kiểm tra Xung đột Bảo thủ Lỗi thời
Trước đây, hệ thống áp dụng cơ chế kiểm tra kép: vừa kiểm tra `bookings.dat` vừa kiểm tra `course_bookings.dat`. Giờ đây, logic kiểm tra xung đột được hiện đại hóa:
- Khóa học chỉ xung đột nếu diễn ra cùng `dayOfWeek`, cùng `slotIndex`, và có khoảng thời gian học `[startDate, endDate]` giao nhau thực sự trên trục thời gian dương lịch.
- Hai khóa học cùng ca học nhưng diễn ra vào các tháng khác nhau (hoặc các năm khác nhau) sẽ không bao giờ bị báo xung đột giả tạo.
- Các khóa học có trạng thái `CANCELLED` hoặc `REJECTED` sẽ tự động giải phóng ca học cho các học viên khác đăng ký.

---

## 6. TÁI CẤU TRÚC `AdminService` VÀ TÍNH TOÁN GMV TỪ `CourseBooking`

### 6.1. Loại bỏ Phụ thuộc `IBookingRepository`
Constructor của `AdminService` đã được tinh giản:
```cpp
AdminService::AdminService(std::shared_ptr<IUserRepository> userRepo,
                           std::shared_ptr<IReviewRepository> reviewRepo,
                           std::shared_ptr<BookingService> bookingService)
```
Không còn phụ thuộc vào repository lưu trữ cấp thấp của booking, mà giao tiếp trực tiếp qua tầng nghiệp vụ `BookingService`.

### 6.2. Tính toán Chỉ số Toàn sàn (Platform Metrics Calculation)
Trong phương thức `calculateMetrics()`:
- Quét qua toàn bộ danh sách khóa học định kỳ: `auto courses = bookingService->getAllCourseBookings()`.
- Tổng giá trị kết nối ban đầu (`initialConnectionGMV`): Tính bằng tổng `hourlyRate` của tất cả các khóa học có trạng thái `ACCEPTED` hoặc `COMPLETED`.
- Doanh thu hoa hồng sàn (`platformCommissionRevenue`): Tính bằng `initialConnectionGMV * platformCommissionRate` (tỷ lệ hoa hồng mặc định 25%).
- Đếm số lượng các khóa học theo từng trạng thái: `totalBookings`, `acceptedBookings`, `pendingBookings`, `rejectedBookings`, `completedBookings`.
- Hàm `terminateMatch(int bookingId, ...)` được cập nhật để gọi trực tiếp `bookingService->cancelCourseBooking(0, UserRole::ADMIN, bookingId, outMsg)` với quyền quản trị viên tối cao.

---

## 7. VÔ HIỆU HÓA TÁI TẠO FILE RỖNG TRONG `FileBookingRepository` & `MockDatabaseEngine`

Một trong những thách thức lớn nhất khi loại bỏ tệp `bookings.dat` là ngăn chặn việc các module cũ vô tình tạo lại tệp rỗng trên đĩa trong quá trình khởi chạy.

### 7.1. Cải tiến `FileBookingRepository`
Trong `src/storage/repositories/FileBookingRepository.cpp`:
```cpp
bool FileBookingRepository::initialize() {
    std::lock_guard<std::mutex> lock(repoMutex);
    bookings.clear();
    idIndex.clear();
    studentIndex.clear();
    tutorIndex.clear();

    if (!std::filesystem::exists(filePath)) {
        // RETIRED FILE INVARIANT:
        // Không tạo lại tệp rỗng trên đĩa! Chỉ khởi tạo trạng thái bộ nhớ an toàn.
        nextBookingId = 101;
        return true; 
    }
    // Đọc dữ liệu nếu tệp tồn tại...
}
```

### 7.2. Cải tiến `MockDatabaseEngine`
Trong `include/storage/MockDatabaseEngine.h` và `src/storage/MockDatabaseEngine.cpp`:
- Loại bỏ kế thừa đa hình `public IBookingRepository`.
- Gỡ bỏ biến thành viên `std::shared_ptr<FileBookingRepository> bookingRepo`.
- Xóa bỏ toàn bộ các phương thức ủy quyền CRUD cũ: `saveBooking`, `updateBooking`, `findBookingById`, `getBookingsByStudentId`, `getBookingsByTutorId`, `getAllBookings`.
- Trong hàm `seedDefaultDataIfEmpty()`: Xóa bỏ hoàn toàn mã nguồn tạo 27 bản ghi giả lập cho `bookings.dat`. Chỉ thực hiện seed cho `users.dat` (50 người dùng) nếu hệ thống bị trống hoàn toàn.

---

## 8. DỌN DẸP `GUIBridge` VÀ GIAO TIẾP QML

`GUIBridge` đóng vai trò là adapter trung gian kết nối giữa mô hình C++ và giao diện người dùng QML:

1. **Loại bỏ Lớp `Booking.h`:** Toàn bộ `include "domain/Booking.h"` được thay thế bằng `include "domain/CourseBooking.h"`.
2. **Chuyển đổi Dữ liệu Lịch sử Đặt lịch (`qmlGetStudentBookings` & `qmlGetTutorBookings`):**
   - Duyệt qua `bookingService->getStudentCourseBookings(studentId)` và `bookingService->getTutorCourseBookings(tutorId)`.
   - Đóng gói thành `QVariantMap` với đầy đủ các trường mà QML hiển thị:
     - `bookingId`: Mã khóa học định kỳ (VD: 1001, 1002...).
     - `studentName`, `tutorName`: Tên hiển thị của hai bên kết nối.
     - `subject`: Môn học giảng dạy.
     - `hourlyRate`: Học phí một buổi.
     - `startDate`: Ngày bắt đầu khóa học (YYYY-MM-DD).
     - `weeksCount`: Thời lượng (4, 8, hoặc 12 tuần).
     - `dayOfWeek`: Thứ trong tuần (0: Thứ Hai .. 6: Chủ Nhật).
     - `slotIndex`: Ca học trong ngày (0..7).
     - `status`: Trạng thái khóa học (`PENDING`, `ACCEPTED`, `REJECTED`, `COMPLETED`, `CANCELLED`).
     - `canReview`: `true` nếu học viên đủ điều kiện để lại đánh giá sao cho khóa học này.
3. **Hiển thị Thời khóa biểu Tuần Động (`getStudentWeeklySchedule` & `getTutorWeeklySchedule`):**
   - Tiếp nhận tham số `mondayDate` từ QML để điều hướng qua lại giữa các tuần trong năm.
   - Gọi `bookingService->getAcceptedSessionsForWeek(userId, role, mondayDate)`.
   - Ánh xạ chính xác từng buổi học thực tế vào đúng ô `[dayOfWeek, slotIndex]` trên ma trận thời khóa biểu.
4. **Cập nhật Toàn bộ Thao tác QML Action:**
   - `qmlAcceptBooking` -> Gọi `acceptCourseBooking`.
   - `qmlRejectBooking` -> Gọi `rejectCourseBooking`.
   - `qmlCancelBooking` -> Gọi `cancelCourseBooking`.
   - `terminateCourse` -> Gọi `terminateCourseBooking`.
   - `requestCourseBooking` và `bookLesson` -> Tính toán ngày thứ Hai tới gần nhất và tạo khóa học định kỳ thông qua `bookCourse`.

---

## 9. CẬP NHẬT `ConsoleApp` VÀ COMPOSITION ROOTS

### 9.1. Composition Roots
Cả hai điểm vào thực thi của ứng dụng đều được đồng bộ hóa chuẩn Dependency Injection:
- **CLI Composition Root (`src/main.cpp`):**
  Khởi tạo `MockDatabaseEngine`, lấy `userRepo`, `reviewRepo`, `courseBookingRepo`. Cung cấp trực tiếp vào các services (`AuthService`, `UserService`, `BookingService`, `ReviewService`, `MatchingService`, `AdminService`). Chạy tự động kiểm tra cấu trúc dữ liệu (`runCoreDataStructureUnitTests()`) và khởi chạy `ConsoleApp`.
- **GUI Composition Root (`src/gui/main.cpp`):**
  Khởi tạo kiến trúc tương tự, liên kết với `GUIBridge` và đăng ký context property `guiBridge` cho QML engine.

### 9.2. Cải tiến Giao diện Dòng lệnh (`ConsoleApp.cpp`)
- Chuyển đổi các menu tương tác sang mô hình khóa học định kỳ:
  - Menu đặt lịch của học viên (`handleBookLessonScheduleGuard`): Yêu cầu người dùng chọn thứ trong tuần, ca học, thời lượng khóa học (4, 8, 12 tuần), và ngày bắt đầu theo định dạng `YYYY-MM-DD`.
  - Hiển thị ma trận thời khóa biểu tuần (`renderTimetableGrid`): Tô màu xanh lá các ca học đã được phê duyệt (`ACCEPTED`) và tô màu vàng cảnh báo các ca bận theo `scheduleBitmask`.
  - Quản lý danh sách khóa học của học viên và gia sư (`handleStudentBookings`, `handleTutorPendingBookings`).
- **Khắc phục lỗi ngắt luồng EOF (EOF-Safe Input Handling):**
  Trong các hàm nhập số `getValidIntChoice`, `readIntInput`, `readDoubleInput`, bổ sung điều kiện `if (std::cin.eof()) return 0;`. Điều này cho phép ứng dụng CLI có thể nhận đầu vào qua đường ống (piped input / redirected stdin) từ các công cụ kiểm thử tự động mà không bị rơi vào vòng lặp vô tận.

---

## 10. CẬP NHẬT VÀ KẾT QUẢ 22 TEST SUITES TRONG `TestRunner.cpp`

Tệp kiểm thử toàn diện `tests/TestRunner.cpp` đã được cập nhật toàn bộ các ca kiểm thử để phản ánh kiến trúc mới. Kết quả chạy thực tế với lệnh:
```powershell
.\TutorMatch_Tests.exe
```

### Bảng Kết quả Chi tiết 22 Bộ Kiểm thử:

| STT | Tên Suite Kiểm thử | Nội dung Xác nhận | Kết quả |
|:---:|---|---|:---:|
| 1 | **MatchingEngine OCP** | Xác minh Open/Closed Principle qua chiến lược ghép cặp tùy biến | **PASS** |
| 2 | **User Hierarchy LSP** | Xác minh Liskov Substitution Principle trên cây đa hình User | **PASS** |
| 3 | **Decoupled Services DIP/ISP/SRP** | Phân tách dịch vụ, đăng ký, duyệt khóa học, tính GMV Admin | **PASS** |
| 4 | **ScheduleGuard Bitmask Collision Math** | Thuật toán toán học 56-bit, kiểm tra va chạm ca học | **PASS** |
| 5 | **Sidebar Navigation Labels** | Loại bỏ tiền tố số trên giao diện điều hướng | **PASS** |
| 6 | **Tutor Profile Editing & Persistence** | Chỉnh sửa hồ sơ gia sư, xác thực dữ liệu và lưu trữ | **PASS** |
| 7 | **Bidirectional Timetable Sync** | Đồng bộ thời khóa biểu hai chiều học viên <-> gia sư theo tuần | **PASS** |
| 8 | **Admin User Card Data Contract** | Kiểm tra hợp đồng dữ liệu thẻ người dùng của Admin, không có trường undefined | **PASS** |
| 9 | **User Detail Modal Contract** | Modal chi tiết người dùng, xử lý người dùng không tồn tại | **PASS** |
| 10 | **Verification & Lock Sync** | Đồng bộ trạng thái khóa và xác minh tài khoản giữa Admin và Student | **PASS** |
| 11 | **AdminService Filters & Search** | Bộ lọc người dùng theo vai trò, trạng thái và tìm kiếm chuyên môn | **PASS** |
| 12 | **AVL Tree Multi-Value Support** | Cây AVL hỗ trợ đa giá trị trùng khóa và cân bằng khi xóa | **PASS** |
| 13 | **Schedule Bitmask 56-Bit Boundary** | Khử các bit ma (phantom bits) vượt quá giới hạn 56-bit | **PASS** |
| 14 | **MockDatabaseEngine Atomic Persistence** | Ghi tệp nguyên tử (atomic write), không trùng lặp dòng | **PASS** |
| 15 | **DataFileInspector Read-Only Inspection** | Kiểm tra chế độ chỉ đọc, che dấu mật khẩu, bắt lỗi thiếu tệp | **PASS** |
| 16 | **Course Booking Conflict Detection** | 13 kịch bản kiểm tra xung đột khóa học và bảo vệ đồng thời | **PASS** |
| 17 | **Dedicated Entity Serializers** | Kiểm tra các lớp serializer chuyên biệt với độ tương thích 100% | **PASS** |
| 18 | **Concrete File Repositories** | Kiểm tra các repository lưu trữ tệp và xử lý dòng hỏng | **PASS** |
| 19 | **Phase 3 End-to-End Business Workflows** | 10 kịch bản nghiệp vụ đầu-cuối thực tế (E2E-01 đến E2E-10) | **PASS** |
| 20 | **Recurring Course Booking Invariants** | Kiểm tra các bất biến nghiệp vụ của khóa học định kỳ | **PASS** |
| 21 | **Weekly Timetable Recurring Integration** | Tích hợp thời khóa biểu tuần với khóa học định kỳ | **PASS** |
| 22 | **Comprehensive Data Audit & Migration** | Xác nhận `bookings.dat` không tồn tại, kiểm toán 27 bản ghi lưu trữ | **PASS** |

**TỔNG KẾT: 22/22 TEST SUITES PASSED (100% THÀNH CÔNG)**

---

## 11. KẾT QUẢ KIỂM TRA DỮ LIỆU BẰNG `DataFileInspector`

Thực thi kiểm tra dữ liệu bằng công cụ giám sát pháp chứng `TutorMatch_Inspector.exe`:
```powershell
.\TutorMatch_Inspector.exe
```

### Kết quả Ghi nhận từ Hệ thống:
1. **`users.dat` (Người dùng hệ thống):**
   - Đường dẫn: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat`
   - Trạng thái tệp: **OK**
   - Tổng bản ghi: **50** (1 Quản trị viên tối cao, 24 Gia sư, 25 Học viên).
   - Bản ghi lỗi/hỏng: **0**.
2. **`bookings.dat` (Đơn đặt lịch cũ):**
   - Đường dẫn: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/bookings.dat`
   - Trạng thái tệp: **ERROR_NOT_FOUND (Tệp không tồn tại)**.
   - Kết luận: Đã rút khỏi môi trường hoạt động thực tế đúng như thiết kế kiến trúc.
3. **`reviews.dat` (Đánh giá & Nhận xét):**
   - Đường dẫn: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/reviews.dat`
   - Trạng thái tệp: **OK**
   - Tổng bản ghi: **15** (Tất cả 15 đánh giá đều giữ nguyên vẹn điểm số 1-5★ và nhận xét).
   - Bản ghi lỗi/hỏng: **0**.
4. **`course_bookings.dat` (Khóa học định kỳ):**
   - Đường dẫn: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/course_bookings.dat`
   - Định dạng: CSV 13 trường chuẩn (`CourseBooking entity`).
   - Trạng thái tệp: **OK**
   - Tổng bản ghi: **8** (Đại diện đầy đủ các trạng thái `PENDING`, `ACCEPTED`, `REJECTED`, `COMPLETED`, `CANCELLED`).
   - Bản ghi lỗi/hỏng: **0**.

### Tổng kết Hệ thống Lưu trữ Toàn cục (Global Data Summary):
- Tổng số tệp kiểm tra: 4 tệp
- Tổng số bản ghi đọc được: 73 bản ghi (50 Users + 15 Reviews + 8 CourseBookings)
- Tổng số bản ghi hợp lệ: 73 bản ghi
- Tổng số bản ghi lỗi/hỏng: **0**
- Tính toàn vẹn dữ liệu: **100% HOÀN HẢO (CHÍNH XÁC)**
- Tình trạng tệp: **Tuyệt đối an toàn Read-Only (Không có tệp nào bị sửa đổi)**

---

## 12. BẢNG SO SÁNH BEFORE VS AFTER (TRƯỚC VÀ SAU CHUYỂN ĐỔI)

| Tiêu chí | Trước khi Chuyển đổi (Before) | Sau khi Hoàn thành Chuyển đổi (After) | Lợi ích Đạt được |
|---|---|---|---|
| **Mô hình Giao dịch** | Song song hai mô hình: `Booking` (cũ) và `CourseBooking` (mới) | Duy nhất một mô hình chuẩn: `CourseBooking` | Triệt tiêu xung đột trạng thái, loại bỏ hoàn toàn mã nguồn trùng lặp |
| **Tệp Lưu trữ Runtime** | `bookings.dat` (27 bản ghi cũ) tồn tại song song `course_bookings.dat` | `bookings.dat` đã rút khỏi runtime và được lưu trữ vĩnh viễn; chỉ còn `course_bookings.dat` | Không còn tình trạng phân tán dữ liệu hoặc đọc nhầm nguồn dữ liệu cũ |
| **Bảo toàn Dữ liệu Cũ** | Nguy cơ bị ghi đè hoặc xóa mất căn cứ của 15 reviews | Lưu trữ vào `data/archive/` với SHA-256 Manifest chứng thực toàn vẹn | Đảm bảo tính pháp chứng, phục vụ kiểm toán lịch sử vĩnh viễn |
| **Phát hiện Xung đột** | Kiểm tra bảo thủ trên mặt nạ 56-bit trừu tượng, hay báo xung đột giả | Kiểm tra giao thoa khoảng ngày lịch thực tế ISO-8601 kết hợp ca học | Học viên có thể đặt lịch học cho các tuần/tháng khác nhau mà không bị chặn nhầm |
| **Tầng Dịch vụ (Services)** | `BookingService` và `AdminService` phụ thuộc vào `IBookingRepository` | Các services chỉ phụ thuộc vào `ICourseBookingRepository` | Tuân thủ triệt để Interface Segregation (ISP) và Dependency Inversion (DIP) |
| **Tính toán GMV Admin** | Tính toán phân mảnh, tính trên đơn đơn lẻ | Tính toán chuẩn xác theo giá trị học phí khóa học định kỳ | Phản ánh đúng doanh thu thực tế và hoa hồng của nền tảng |
| **Khởi động Ứng dụng** | Repository tự động sinh lại tệp rỗng `bookings.dat` nếu file thiếu | Triệt tiêu hoàn toàn logic sinh file rỗng khi khởi động | Đảm bảo bất biến tệp rỗng không bao giờ xuất hiện lại trong hệ thống |
| **Khả năng Kiểm thử** | 21 bộ test suites, một số ca test vẫn gọi API cũ | **22 bộ test suites hoàn chỉnh, 100% PASS** | Độ tin cậy phần mềm đạt mức tuyệt đối, sẵn sàng cho sản xuất |

---

## 13. KẾT LUẬN VÀ KẾ HOẠCH BẢO TRÌ TƯƠNG LAI

### 13.1. Kết luận
Đợt công tác kỹ thuật chuyển đổi và khai tử mô hình Legacy Booking của TutorMatch đã hoàn thành xuất sắc 100% mục tiêu đặt ra:
- Toàn bộ mã nguồn đã được dọn sạch các dấu vết của mô hình cũ mà không làm tổn hại đến bất kỳ tính năng nghiệp vụ nào đang vận hành.
- 50 tài khoản người dùng và 15 đánh giá lịch sử được bảo toàn vẹn toàn, không bị biến dạng hoặc suy hao dữ liệu.
- Nền tảng TutorMatch hiện đã sở hữu một kiến trúc ba tầng hiện đại, trong sáng, có tính module hóa cao, sẵn sàng cho việc mở rộng quy mô.

### 13.2. Khuyến nghị và Kế hoạch Bảo trì Tiếp theo
1. **Tích hợp Cổng Thanh toán Trực tuyến (Payment Gateway Integration):**
   Với mô hình `CourseBooking` đã có trường `totalCost` và `hourlyRate` chuẩn hóa, bước tiếp theo có thể dễ dàng bổ sung `PaymentService` để quản lý giao dịch cọc và thanh toán học phí qua VNPay/MoMo.
2. **Đồng bộ Lịch ngoài (External Calendar Sync):**
   Xây dựng tính năng xuất file lịch định dạng `.ics` hoặc đồng bộ hai chiều với Google Calendar / Apple Calendar cho học viên và gia sư dựa trên các buổi học thực tế (`CourseSessionInfo`).
3. **Mở rộng Đa nền tảng:**
   Với việc tách biệt hoàn toàn giữa `GUIBridge` (QML), `ConsoleApp` (CLI) và tầng nghiệp vụ C++ thuần khiết, dự án có thể dễ dàng chuyển đổi hoặc đóng gói lên nền tảng Mobile (Android/iOS) hoặc WebAssembly (Wasm) trong tương lai mà không cần viết lại bất kỳ logic nghiệp vụ nào.
