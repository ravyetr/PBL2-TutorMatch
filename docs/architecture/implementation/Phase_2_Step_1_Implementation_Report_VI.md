# BÁO CÁO THỰC THI KIẾN TRÚC PHASE 2 — BƯỚC 1 (STEP 1 IMPLEMENTATION REPORT)
## Dự án: TutorMatch — Chuẩn Hóa Hợp Đồng Repository & DTO Contracts (C++17 / Qt 6.11)
### Mã tài liệu: `Phase_2_Step_1_Implementation_Report_VI.md`

---

## 1. KẾT QUẢ BUILD VÀ TEST BASELINE (TRƯỚC KHI THỰC HIỆN BƯỚC 1)

Trước khi thực hiện bất kỳ chỉnh sửa nào trên mã nguồn, môi trường build và bộ kiểm thử đã được thiết lập và ghi nhận kết quả cơ sở (Baseline):

- **Môi trường biên dịch:** Windows MinGW 13.1.0 64-bit (`D:/CODE/Tools/mingw1310_64/bin/g++.exe`), CMake 3.x (`D:/CODE/Tools/CMake_64/bin/cmake.exe`), Qt 6.11.2 (`D:/CODE/6.11.2/mingw_64`).
- **Build Targets Baseline:**
  - `TutorMatch_Tests`: Build thành công (100% clean).
  - `TutorMatch_GUI`: Build thành công (100% clean).
- **Kết quả kiểm thử Baseline:** Chạy `TutorMatch_Tests.exe` với toàn bộ 16 test suites:
  ```
  ============================================================
      TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
  ============================================================
  [TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine... -> PASS
  [TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy... -> PASS
  [TEST] 3. Verifying DIP, ISP & SRP across decoupled Services... -> PASS
  [TEST] 4. Verifying ScheduleGuard Bitmask Collision Math... -> PASS
  [TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes... -> PASS
  [TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence... -> PASS
  [TEST] 7. Verifying Bidirectional Timetable Synchronization... -> PASS
  [TEST] 8. Verifying Admin User Card Data Contract & No Undefined Fields... -> PASS
  [TEST] 9. Verifying User Detail Modal Contract... -> PASS
  [TEST] 10. Verifying Verification & Lock Sync Across Admin and Student Views... -> PASS
  [TEST] 11. Verifying AdminService Filters & Subject Keyword Search... -> PASS
  [TEST] 12. Verifying AVL Tree Multi-Value Support and Key/Value Deletion... -> PASS
  [TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary & Sanitization... -> PASS
  [TEST] 14. Verifying MockDatabaseEngine Atomic Persistence & Zero-Duplication... -> PASS
  [TEST] 15. Verifying DataFileInspector Read-Only Inspection... -> PASS
  [TEST] 16. Verifying Phase 1 Booking Conflict Detection & Synchronization... -> PASS
  ============================================================
    >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
  ============================================================
  ```
- **Tình trạng Workspace:** Dự án là standalone workspace tại `C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch` (không dùng Git repository).

---

## 2. DANH MỤC CÁC TỆP ĐÃ KHẢO SÁT

Để đảm bảo không làm gãy mã nguồn hiện có, toàn bộ các tệp liên quan đến `IRepository` và các consumer đã được kiểm toán chi tiết:
1. `include/storage/IRepository.h` (Khảo sát các interface và generic template).
2. `include/storage/MockDatabaseEngine.h` & `src/storage/MockDatabaseEngine.cpp` (Khảo sát 24 phương thức ghi đè).
3. `include/services/AuthService.h` & `src/services/AuthService.cpp` (Khảo sát các lệnh gọi `userRepo->`).
4. `include/services/UserService.h` & `src/services/UserService.cpp` (Khảo sát các lệnh gọi `userRepo->`).
5. `include/services/BookingService.h` & `src/services/BookingService.cpp` (Khảo sát các lệnh gọi `bookingRepo->`, `userRepo->`, `reviewRepo->`).
6. `include/services/AdminService.h` & `src/services/AdminService.cpp` (Khảo sát các lệnh gọi `userRepo->`, `bookingRepo->`, `reviewRepo->`).
7. `include/gui/GUIBridge.h` & `src/gui/GUIBridge.cpp` (Khảo sát các lệnh gọi repo hiện thời).
8. `ui/qml/StudentBookingsView.qml` (Khảo sát logic tính toán `computeMetrics()` để chuẩn hóa DTO).
9. `tests/TestRunner.cpp` (Khảo sát `FakeUserRepository`, `FakeBookingRepository`, `FakeReviewRepository`).
10. `CMakeLists.txt` (Khảo sát danh sách headers và sources).

---

## 3. DANH MỤC TỆP ĐÃ TẠO MỚI & THAY ĐỔI TRONG BƯỚC 1

Trong Bước 1, tuân thủ nghiêm ngặt nguyên tắc thay đổi tối thiểu, chỉ có:
1. **Tệp tạo mới:**
   - [`include/models/BookingDTOs.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/BookingDTOs.h): Định nghĩa DTO `StudentBookingMetrics`.
2. **Tệp chuẩn hóa hợp đồng:**
   - [`include/storage/IRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/storage/IRepository.h): Bổ sung tài liệu chuẩn mực về DIP/ISP, quyền sở hữu bộ nhớ, hợp đồng lỗi; đánh dấu `@deprecated` template generic thừa; chuẩn hoá các giao diện `IUserRepository`, `IBookingRepository`, `IReviewRepository`.
3. **Tệp cấu hình build (Minimal Safe Adjustment theo ADR-008):**
   - [`CMakeLists.txt`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/CMakeLists.txt): Bổ sung `include/models/BookingDTOs.h` vào danh sách `HEADERS`.

*Tất cả các tệp `.cpp`, `.qml`, và tệp dữ liệu `.dat` đều được giữ nguyên vẹn 100%.*

---

## 4. ĐẶC TẢ CHI TIẾT CÁC HỢP ĐỒNG REPOSITORY CHUẨN HÓA

### 4.1. Hợp đồng `IUserRepository`
```cpp
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual int getNextUserId() = 0;
    virtual bool saveUser(const User& user) = 0;
    virtual std::shared_ptr<User> findUserById(int userId) = 0;
    virtual std::shared_ptr<User> findUserByUsername(const std::string& username) = 0;
    virtual bool updateUser(const User& user) = 0;
    virtual bool deleteUserSoft(int userId) = 0;

    virtual std::vector<std::shared_ptr<Tutor>> getAllTutors() = 0;
    virtual std::vector<std::shared_ptr<Student>> getAllStudents() = 0;
    virtual std::vector<std::shared_ptr<User>> getAllUsers() = 0;

    virtual std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) = 0;
    virtual std::vector<int> findTutorIdsByMinRating(double minRating) = 0;
};
```
- **Ranh giới trách nhiệm:** Quản lý trọn vẹn vòng đời tài khoản và dữ liệu đa hình (`Student`, `Tutor`, `Admin`). Cung cấp 2 truy vấn dải giá và dải rating phục vụ tìm kiếm gia sư tận dụng chỉ mục AVL.
- **Tính an toàn đa hình:** Trả về `std::shared_ptr<User>` (hoặc vector các con trỏ `shared_ptr<Tutor>`, `shared_ptr<Student>`), loại bỏ hoàn toàn nguy cơ Cắt lát đối tượng (Object Slicing).

### 4.2. Hợp đồng `IBookingRepository`
```cpp
class IBookingRepository {
public:
    virtual ~IBookingRepository() = default;

    virtual int getNextBookingId() = 0;
    virtual bool saveBooking(const Booking& booking) = 0;
    virtual bool updateBooking(const Booking& booking) = 0;
    virtual std::shared_ptr<Booking> findBookingById(int bookingId) = 0;
    virtual std::vector<Booking> getAllBookings() = 0;
    virtual std::vector<Booking> getBookingsByStudentId(int studentId) = 0;
    virtual std::vector<Booking> getBookingsByTutorId(int tutorId) = 0;
};
```
- **Ranh giới trách nhiệm:** Quản lý lưu trữ bản ghi đơn kết nối và trạng thái đơn (`PENDING`, `ACCEPTED`, `COMPLETED`, `CANCELLED`, `REJECTED`).
- **Ngữ nghĩa kiểu:** `Booking` là một Value Object nhẹ, trả về bản sao theo giá trị hoặc `std::shared_ptr<Booking>` khi cần ngữ nghĩa có thể null.

### 4.3. Hợp đồng `IReviewRepository`
```cpp
class IReviewRepository {
public:
    virtual ~IReviewRepository() = default;

    virtual int getNextReviewId() = 0;
    virtual bool saveReview(const Review& review) = 0;
    virtual bool deleteReview(int reviewId) = 0;
    virtual std::vector<Review> getReviewsByTutorId(int tutorId) = 0;
    virtual std::vector<Review> getAllReviews() = 0;
};
```
- **Ranh giới trách nhiệm:** Là Nguồn Chân Lý duy nhất (Authoritative Source of Truth) cho các bản ghi đánh giá (`reviews.dat`).
- **Không chứa quy tắc nghiệp vụ:** Điều kiện đánh giá (đơn phải `COMPLETED`, chính chủ học viên) và tính toán lại điểm rating trung bình cộng của gia sư là trách nhiệm nghiệp vụ của `ReviewService` (BLL), không đặt vào repository.

---

## 5. ĐẶC TẢ HỢP ĐỒNG DTO `StudentBookingMetrics`

DTO được định nghĩa độc lập tại [`include/models/BookingDTOs.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/BookingDTOs.h):

```cpp
namespace TutorMatch {

struct StudentBookingMetrics {
    int acceptedCount = 0;   ///< Số lượng đơn đang học (status == "ACCEPTED")
    int pendingCount = 0;    ///< Số lượng đơn chờ duyệt (status == "PENDING")
    int historyCount = 0;    ///< Số lượng đơn đã kết thúc (status in {"COMPLETED", "CANCELLED", "REJECTED"})
    double weeklyCost = 0.0; ///< Tổng học phí hàng tuần (tổng hourlyRate của các đơn ACCEPTED)

    constexpr StudentBookingMetrics() = default;
    constexpr StudentBookingMetrics(int accepted, int pending, int history, double cost)
        : acceptedCount(accepted), pendingCount(pending), historyCount(history), weeklyCost(cost) {}

    bool operator==(const StudentBookingMetrics& other) const { ... }
    bool operator!=(const StudentBookingMetrics& other) const { ... }
};

} // namespace TutorMatch
```

### Phân tích ngữ nghĩa trường và kiểu dữ liệu:
- `acceptedCount` (int): Đếm số lượng đơn học đang kết nối hoạt động (`ACCEPTED`).
- `pendingCount` (int): Đếm số lượng đơn học đang chờ gia sư phản hồi (`PENDING`).
- `historyCount` (int): Đếm toàn bộ đơn trong lịch sử (`COMPLETED`, `CANCELLED`, hoặc `REJECTED`).
- `weeklyCost` (double): Tổng chi phí học phí mỗi tuần của học viên. Trong mô hình lịch học lặp lại theo tuần của TutorMatch, mỗi ca học `ACCEPTED` đại diện cho 1 buổi học/tuần với học phí `hourlyRate`.
- **Vị trí kiến trúc:** Được đặt tại `include/models/BookingDTOs.h` vì đây là hợp đồng truyền dữ liệu miền (Domain DTO), không phụ thuộc vào bất kỳ framework giao diện nào, độc lập với Qt QML và File I/O, an toàn tuyệt đối khi truyền dữ liệu qua ranh giới BLL $\rightarrow$ PL.

---

## 6. QUYẾT ĐỊNH VỀ QUYỀN SỞ HỮU & VÒNG ĐỜI (OWNERSHIP & LIFETIME)

1. **Không rò rỉ cấu trúc dữ liệu lưu trữ:** Hợp đồng `IRepository.h` hoàn toàn không để lộ `HashTable`, `LRUCache`, `AVLTree`, byte offset, luồng tệp tin `<fstream>` hay chuỗi CSV. Tầng BLL chỉ nhìn thấy các kiểu dữ liệu trừu tượng và con trỏ thực thể miền.
2. **Quản lý bộ nhớ:**
   - Các thực thể đa hình `User` được quản lý bằng `std::shared_ptr<User>` để hỗ trợ đa hình an toàn mà không cần quản lý bộ nhớ thủ công bằng raw pointer.
   - Các thực thể `Booking` và `Review` là các cấu trúc dữ liệu miền nhẹ, được truyền bằng giá trị (`std::vector<Booking>`) hoặc tham chiếu hằng (`const Booking&`), tránh phát sinh cấp phát động không cần thiết trên heap.
   - DTO `StudentBookingMetrics` là một Plain Old C++ Object (POCO) nhẹ có kích thước cố định (khoảng 24 bytes), được truyền nhận theo giá trị (Pass-by-value).

---

## 7. KIỂM TRA CHIỀU PHỤ THUỘC (DEPENDENCY DIRECTION VERIFICATION)

Kiểm toán chiều phụ thuộc của các tệp sau Bước 1:
- `include/storage/IRepository.h`:
  - Chỉ `#include <vector>`, `<memory>`, `<string>`.
  - Chỉ forward-declare `class User; class Student; class Tutor; struct Booking; struct Review;`.
  - **Không phụ thuộc vào bất kỳ tệp cụ thể nào của DAL, Services hay GUI.**
- `include/models/BookingDTOs.h`:
  - Không include bất kỳ thư viện ngoài nào (thuần C++17).
  - Không phụ thuộc vào QML, Qt, Services hay Repositories.
- Chiều phụ thuộc: **Presentation $\rightarrow$ Business Logic $\rightarrow$ Repository Contracts $\leftarrow$ Data Access Layer** được bảo đảm 100%.

---

## 8. QUAN SÁT VỀ BẢO MẬT & DỮ LIỆU ĐĂNG NHẬP (SECURITY OBSERVATION)

Theo chỉ dẫn tại Mục 7 của yêu cầu Bước 1, chúng tôi đã rà soát cơ chế xác thực và nhận thấy:
- **Hiện trạng:** Tên biến trong `User.h` là `passwordHash`, nhưng thực tế giá trị lưu trong `data/users.dat` là mật khẩu dạng văn bản thô (Plaintext passwords như `pass123`, `admin123`).
- **Thực tế xác thực:** Trong `src/services/AuthService.cpp` (dòng 28), lệnh kiểm tra đăng nhập so sánh trực tiếp chuỗi thô:
  ```cpp
  if (user->getPasswordHash() != password) { ... }
  ```
- **Cam kết an toàn Bước 1:**
  - Không in mật khẩu ra log.
  - Không sửa đổi dữ liệu đăng nhập trong `users.dat`.
  - Không thay đổi hành vi xác thực của `AuthService`.
  - Ghi nhận đây là một món nợ kỹ thuật bảo mật (Security Debt) cần giải quyết trong một nhiệm vụ chuyên biệt sau này.

---

## 9. KẾT QUẢ BIÊN DỊCH VÀ KIỂM THỬ HỒI QUY (POST-STEP 1 VERIFICATION)

Sau khi tạo `BookingDTOs.h`, cập nhật `IRepository.h`, và đăng ký header vào `CMakeLists.txt`:

1. **Biên dịch Target `TutorMatch_Tests`:**
   ```
   [100%] Built target TutorMatch_Tests (Exit code: 0)
   ```
2. **Biên dịch Target `TutorMatch_GUI`:**
   ```
   [100%] Built target TutorMatch_GUI (Exit code: 0)
   ```
3. **Chạy hồi quy toàn bộ 16 Test Suites (`TutorMatch_Tests.exe`):**
   ```
   ============================================================
       TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
   ============================================================
   [TEST] 1. Verifying Open/Closed Principle (OCP)... -> PASS
   [TEST] 2. Verifying Liskov Substitution Principle (LSP)... -> PASS
   [TEST] 3. Verifying DIP, ISP & SRP across decoupled Services... -> PASS
   [TEST] 4. Verifying ScheduleGuard Bitmask Collision Math... -> PASS
   [TEST] 5. Verifying Sidebar Navigation Labels... -> PASS
   [TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence... -> PASS
   [TEST] 7. Verifying Bidirectional Timetable Synchronization... -> PASS
   [TEST] 8. Verifying Admin User Card Data Contract... -> PASS
   [TEST] 9. Verifying User Detail Modal Contract... -> PASS
   [TEST] 10. Verifying Verification & Lock Sync... -> PASS
   [TEST] 11. Verifying AdminService Filters & Subject Keyword Search... -> PASS
   [TEST] 12. Verifying AVL Tree Multi-Value Support & Deletion... -> PASS
   [TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary... -> PASS
   [TEST] 14. Verifying MockDatabaseEngine Atomic Persistence... -> PASS
   [TEST] 15. Verifying DataFileInspector Read-Only Inspection... -> PASS
   [TEST] 16. Verifying Phase 1 Booking Conflict Detection... -> PASS
   ============================================================
     >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
   ============================================================
   ```
4. **Kiểm tra tính toàn vẹn của tệp dữ liệu (`data/*.dat`):**
   - `users.dat`: 6,958 bytes (Nguyên vẹn, không đổi).
   - `bookings.dat`: 2,709 bytes (Nguyên vẹn, không đổi).
   - `reviews.dat`: 1,650 bytes (Nguyên vẹn, không đổi).

---

## 10. XÁC NHẬN PHẠM VI & DỪNG BƯỚC 1 (STOP CONDITION CONFIRMATION)

Tôi xin xác nhận nghiêm túc:
- **ĐÃ HOÀN TẤT TRỌN VẸN BƯỚC 1.**
- **TUYỆT ĐỐI CHƯA TRIỂN KHAI CÁC BƯỚC 2 ĐẾN 14:**
  - Chưa tạo Dedicated Serializers (Bước 2).
  - Chưa tạo Concrete File Repositories (Bước 3).
  - Chưa chuyển đổi `MockDatabaseEngine` thành Facade (Bước 4).
  - Chưa gỡ bỏ `ISerializable` khỏi Domain Entities (Bước 5).
  - Chưa trích xuất `ReviewService` (Bước 6) và `MatchingService` (Bước 7).
  - Chưa tái cấu trúc `GUIBridge` (Bước 10) hay `ConsoleApp` (Bước 11).
  - Chưa sửa đổi QML (Bước 12) hay Composition Root `main.cpp` (Bước 13).

**HỆ THỐNG ĐÃ SẴN SÀNG VÀ DỪNG LẠI TẠI ĐÂY ĐỂ CHỜ BẠN REVIEW VÀ PHÊ DUYỆT BƯỚC 1 TRƯỚC KHI BẮT ĐẦU BƯỚC 2!**
