# BÁO CÁO TRIỂN KHAI HOÀN THÀNH (IMPLEMENTATION REPORT)
## PHASE 2, STEP 2: DEDICATED ENTITY SERIALIZERS

**Dự án:** TutorMatch — Qt 6.11.2 / C++17 Three-Layer Architecture Refactoring  
**Giai đoạn:** Phase 2 — Tách tầng Data Access Layer (DAL)  
**Bước thực hiện:** Step 2 — Dedicated Entity Serializers (Bộ tuần tự hóa thực thể chuyên biệt)  
**Ngày hoàn thành:** 2026-11-06  
**Trạng thái cổng chất lượng:** SẴN SÀNG ĐỂ KIỂM TOÁN / PHÊ DUYỆT (READY FOR AUDIT)  

---

### 1. TỔNG QUAN VÀ MỤC TIÊU BƯỚC 2

Trong mô hình kiến trúc cũ (Phase 0), logic chuyển đổi dữ liệu CSV (serialization / deserialization) bị ghép chặt trực tiếp vào các Domain Entities thông qua interface `ISerializable` (`User.cpp`, `User.h`, `Booking.h`, `Review.h`). Điều này vi phạm nguyên lý Single Responsibility Principle (SRP) khi Domain Entity vừa phải quản lý dữ liệu nghiệp vụ, vừa phải biết cách định dạng tệp phẳng trên đĩa.

Mục tiêu của **Phase 2, Step 2** là:
1. **Tách biệt hoàn toàn serialization/deserialization khỏi Domain Entities** bằng cách xây dựng 3 Serializer chuyên biệt: `UserSerializer`, `BookingSerializer`, `ReviewSerializer` và bộ tiện ích `SerializerUtils`.
2. **Bảo toàn tuyệt đối định dạng CSV và dữ liệu cũ (Zero Format Drift):** Đảm bảo đầu ra CSV của các Serializer mới giống chính xác 100% từng ký tự so với phương thức `serialize()` cũ.
3. **Bảo toàn đa hình (Polymorphism Preservation):** Khử tuần tự hóa `User` sinh ra đúng đối tượng con (`Student`, `Tutor`, `Admin`) thông qua con trỏ `std::shared_ptr<User>`, triệt tiêu nguy cơ Object Slicing.
4. **Không can thiệp ngoài phạm vi (Regression Safety):** Chưa xóa `ISerializable`, chưa thay đổi `MockDatabaseEngine`, chưa tạo File Repositories (Step 3), không chỉnh sửa các tệp dữ liệu `.dat`.

---

### 2. DANH MỤC CÁC TỆP TIN ĐÃ TẠO VÀ CHỈNH SỬA

#### 2.1. Các tệp tin tạo mới (Created Files)
1. **`include/storage/serializers/SerializerUtils.h`**: Tiện ích dùng chung cho các Serializer: thay thế dấu phẩy nội dung (`escapeCsv`), tách token CSV an toàn (`splitCsvLine`), và chuyển đổi kiểu chuỗi an toàn không ném ngoại lệ (`safeStringToInt`, `safeStringToDouble`, `safeStringToUll`).
2. **`src/storage/serializers/SerializerUtils.cpp`**: Cài đặt chi tiết các tiện ích xử lý chuỗi và số.
3. **`include/storage/serializers/UserSerializer.h`**: Khai báo hợp đồng tuần tự hóa và khử tuần tự hóa đa hình cho phân cấp người dùng (`User`, `Student`, `Tutor`, `Admin`).
4. **`src/storage/serializers/UserSerializer.cpp`**: Cài đặt chi tiết `UserSerializer`, kết nối với `ScheduleGuard::sanitizeMask` để đảm bảo bất biến 56-slot bitmask.
5. **`include/storage/serializers/BookingSerializer.h`**: Khai báo hợp đồng tuần tự hóa cho thực thể kết nối học tập (`Booking`).
6. **`src/storage/serializers/BookingSerializer.cpp`**: Cài đặt chi tiết `BookingSerializer`, xác thực 10 trường dữ liệu và 5 trạng thái vòng đời chuẩn.
7. **`include/storage/serializers/ReviewSerializer.h`**: Khai báo hợp đồng tuần tự hóa cho thực thể đánh giá (`Review`).
8. **`src/storage/serializers/ReviewSerializer.cpp`**: Cài đặt chi tiết `ReviewSerializer`, xác thực 8 trường dữ liệu và ràng buộc điểm sao 1..5.

#### 2.2. Các tệp tin chỉnh sửa (Modified Files)
1. **`CMakeLists.txt`**:
   - Đăng ký 4 file `.h` mới vào danh sách `HEADERS`.
   - Đăng ký 4 file `.cpp` mới vào danh sách `CORE_SOURCES` (dùng cho `TutorMatch_GUI`).
   - Đăng ký 4 file `.cpp` mới vào danh sách nguồn của target `TutorMatch_Tests`.
2. **`tests/TestRunner.cpp`**:
   - Bổ sung Suite kiểm thử thứ 17: `run_test_entity_serializers()` nhằm kiểm tra toàn diện round-trip, an toàn kiểu, trường hợp biên và tính tương thích ngược 100% từng ký tự.

---

### 3. ĐẶC TẢ GIAO DIỆN LẬP TRÌNH (API OF DEDICATED SERIALIZERS)

#### 3.1. `UserSerializer`
```cpp
namespace TutorMatch {

class UserSerializer {
public:
    // Tuần tự hóa theo loại cụ thể (Concrete Overloads)
    static std::string toCsv(const Student& student);
    static std::string toCsv(const Tutor& tutor);
    static std::string toCsv(const Admin& admin);

    // Tuần tự hóa đa hình (Polymorphic Serialization Dispatch)
    static std::string toCsv(const User& user);

    // Khử tuần tự hóa đa hình (Polymorphic Deserialization Factory)
    // Trả về std::shared_ptr<User> chứa instance thực tế là Student, Tutor, hoặc Admin.
    // Trả về nullptr nếu định dạng không hợp lệ hoặc dữ liệu lỗi.
    static std::shared_ptr<User> fromCsv(const std::string& csvLine, std::string* outError = nullptr);

    // Các hàm trợ giúp định kiểu tường minh (Strongly-typed Deserializers)
    static std::shared_ptr<Student> studentFromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static std::shared_ptr<Tutor> tutorFromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static std::shared_ptr<Admin> adminFromCsv(const std::string& csvLine, std::string* outError = nullptr);
};

} // namespace TutorMatch
```

#### 3.2. `BookingSerializer`
```cpp
namespace TutorMatch {

class BookingSerializer {
public:
    // Tuần tự hóa Booking thành chuỗi CSV 10 trường
    static std::string toCsv(const Booking& booking);

    // Khử tuần tự hóa chuẩn C++17 sử dụng std::optional
    static std::optional<Booking> fromCsv(const std::string& csvLine, std::string* outError = nullptr);

    // Khử tuần tự hóa trả về boolean status (không phát sinh cấp phát heap)
    static bool fromCsv(const std::string& csvLine, Booking& outBooking, std::string* outError = nullptr);

    // Kiểm tra tính hợp lệ của trạng thái ("PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED")
    static bool isValidStatus(const std::string& status);
};

} // namespace TutorMatch
```

#### 3.3. `ReviewSerializer`
```cpp
namespace TutorMatch {

class ReviewSerializer {
public:
    // Tuần tự hóa Review thành chuỗi CSV 8 trường
    static std::string toCsv(const Review& review);

    // Khử tuần tự hóa chuẩn C++17 sử dụng std::optional
    static std::optional<Review> fromCsv(const std::string& csvLine, std::string* outError = nullptr);

    // Khử tuần tự hóa trả về boolean status
    static bool fromCsv(const std::string& csvLine, Review& outReview, std::string* outError = nullptr);

    // Kiểm tra bất biến điểm số đánh giá (1..5 sao)
    static bool isValidRatingScore(int ratingScore);
};

} // namespace TutorMatch
```

---

### 4. CƠ CHẾ XỬ LÝ ĐA HÌNH VÀ AN TOÀN KIỂU CỦA USER

Để giải quyết bài toán đa hình của phân cấp `User` mà không gây hiện tượng **Object Slicing**:
1. **Khi Serialization:**
   - Hàm `toCsv(const User& user)` kiểm tra vai trò `user.getRole()`.
   - Sử dụng `dynamic_cast` an toàn để chuyển đổi sang `const Student*`, `const Tutor*`, hoặc `const Admin*`, sau đó gọi overload tương ứng.
   - Kết quả xuất ra đúng số lượng trường của từng subtype:
     - `Student`: **11 trường**.
     - `Tutor`: **15 trường**.
     - `Admin`: **8 trường**.
     - Base `User` fallback: **7 trường**.
2. **Khi Deserialization:**
   - Tokenize dòng CSV thành mảng `tokens`.
   - Kiểm tra trường thứ 6 (`tokens[5]`), đại diện cho `UserRole`:
     - Nếu `tokens[5] == "STUDENT"`: Kiểm tra `tokens.size() >= 11`, parse các trường, gọi `ScheduleGuard::sanitizeMask` và khởi tạo `std::make_shared<Student>(...)`.
     - Nếu `tokens[5] == "TUTOR"`: Kiểm tra `tokens.size() >= 15`, parse các trường, gọi `ScheduleGuard::sanitizeMask` và khởi tạo `std::make_shared<Tutor>(...)`.
     - Nếu `tokens[5] == "ADMIN"`: Kiểm tra `tokens.size() >= 8`, parse các trường và khởi tạo `std::make_shared<Admin>(...)`.
     - Bất kỳ giá trị nào khác hoặc dữ liệu thiếu trường/sai kiểu số: Báo lỗi và trả về `nullptr`.
3. **An toàn bộ nhớ và không cắt ngọn đối tượng:**
   - Giá trị trả về luôn là `std::shared_ptr<User>`, trỏ tới instance thực tế trên heap của lớp dẫn xuất.
   - Thử nghiệm trong test suite xác nhận: `dynamic_cast<Student*>(ptr.get())`, `dynamic_cast<Tutor*>(ptr.get())`, `dynamic_cast<Admin*>(ptr.get())` hoạt động hoàn hảo 100%.

---

### 5. BẰNG CHỨNG TƯƠNG THÍCH ĐỊNH DẠNG CSV CŨ (100% LEGACY PARITY)

Trong Suite kiểm thử 17 (Part D: Backward Compatibility), kiểm toán đối chiếu trực tiếp giữa output của `Serializer::toCsv()` và output của phương thức `serialize()` cũ trên cùng một instance:

1. **Đối với Student mẫu:**
   ```cpp
   Student s(6, "student_nam", "pass123", "nam.student@gmail.com", "Nguyễn Văn Nam",
             12, "Toán 12", 180000.0, 71777214294589695ULL, true);
   assert(UserSerializer::toCsv(s) == s.serialize()); // PASS
   ```
2. **Đối với Tutor mẫu:**
   ```cpp
   Tutor t(2, "tutor_math", "pass123", "le.math@tutormatch.vn", "ThS. Lê Văn Toàn",
           "Chuyên gia luyện thi Đại học môn Toán với 10 năm kinh nghiệm.", "Toán 12 - Luyện thi Đại học",
           150000.0, 5.0, 2, true, 0.0, 71777214294589694ULL, true);
   assert(UserSerializer::toCsv(t) == t.serialize()); // PASS
   ```
3. **Đối với Admin mẫu:**
   ```cpp
   Admin a(1, "admin", "admin123", "admin@tutormatch.vn", "System Administrator", 9, true);
   assert(UserSerializer::toCsv(a) == a.serialize()); // PASS
   ```
4. **Đối với Booking mẫu:**
   ```cpp
   Booking b(101, 6, 2, "Nguyễn Văn Nam", "ThS. Lê Văn Toàn", "Toán 12",
             150000.0, 1ULL, "COMPLETED", "2026-08-15");
   assert(BookingSerializer::toCsv(b) == b.serialize()); // PASS
   ```
5. **Đối với Review mẫu:**
   ```cpp
   Review r(1, 101, 6, 2, "Nguyễn Văn Nam", 5,
            "Thầy Toàn giảng Toán 12 rất dễ hiểu  phương pháp giải trắc nghiệm tốc độ cao.", "2026-08-15");
   assert(ReviewSerializer::toCsv(r) == r.serialize()); // PASS
   ```
6. **Xử lý dấu phẩy trong chuỗi (`escapeCsv`):**
   - Cả hai cơ chế đều chuyển dấu phẩy `,` thành dấu cách ` ` (khoảng trắng).
   - Ví dụ: `"Toán, Lý, Hóa"` $\rightarrow$ `"Toán  Lý  Hóa"`.
   - Kết quả: `Serializer::toCsv()` và `entity.serialize()` cho ra chuỗi trùng khớp chính xác từng ký tự (Exact match).
7. **Bất biến tái tuần tự hóa (Re-serialization Invariance):**
   - Với mọi thực thể hợp lệ $E$:
     $$\text{toCsv}(\text{fromCsv}(\text{toCsv}(E))) \equiv \text{toCsv}(E)$$
   - Đã được kiểm chứng thành công trên cả 3 loại entity.

---

### 6. BẰNG CHỨNG BIÊN DỊCH VÀ KIỂM THỬ THỰC TẾ

#### 6.1. Biên dịch mục tiêu `TutorMatch_Tests`
- **Lệnh thực thi:**
  ```powershell
  Set-Location "C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug"
  $env:PATH = "D:\CODE\Tools\CMake_64\bin;D:\CODE\6.11.2\mingw_64\bin;D:\CODE\Tools\mingw1310_64\bin;" + $env:PATH
  cmake --build . --target TutorMatch_Tests
  ```
- **Kết quả:** `Exit code 0`. Không có lỗi, không có cảnh báo.

#### 6.2. Biên dịch mục tiêu `TutorMatch_GUI`
- **Lệnh thực thi:**
  ```powershell
  Set-Location "C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug"
  $env:PATH = "D:\CODE\Tools\CMake_64\bin;D:\CODE\6.11.2\mingw_64\bin;D:\CODE\Tools\mingw1310_64\bin;" + $env:PATH
  cmake --build . --target TutorMatch_GUI
  ```
- **Kết quả:** `Exit code 0`. Cả 4 file `.cpp` của Serializer đã được biên dịch thành object files và liên kết thành công vào `TutorMatch_GUI.exe`.

#### 6.3. Thực thi Bộ kiểm thử `TutorMatch_Tests.exe`
- **Lệnh thực thi:**
  ```powershell
  .\TutorMatch_Tests.exe
  ```
- **Kết quả:** `Exit code 0`.
- **Thống kê:** **17/17 test suites PASS (100% PASS)**.
- **Log kiểm thử chi tiết:**
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
  [DB Seed Engine] Database is empty or corrupted. Auto-recovering default dataset...
  [DB Seed Engine] Seed completed cleanly!
    -> PASS: MockDatabaseEngine atomic rewrite, zero duplicate lines & AVL synchronization verified!
  [TEST] 15. Verifying DataFileInspector Read-Only Inspection & Error Handling...
  [ApplicationPaths] Resolved to Project Source Data Dir: C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data
  [ApplicationPaths] Resolved to Project Source Data Dir: C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data
  [ApplicationPaths] Resolved to Project Source Data Dir: C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data
    -> PASS: DataFileInspector read-only inspection, credential redaction & error handling verified!
  [TEST] 16. Verifying Phase 1 Booking Conflict Detection & Synchronization...
    -> PASS: All 15 Booking Conflict Detection scenarios verified successfully!
  [TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)...
    -> PASS: UserSerializer, BookingSerializer, and ReviewSerializer verified with 100% legacy parity!
  ============================================================
    >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
  ============================================================
  ```

---

### 7. XÁC NHẬN TOÀN VẸN DỮ LIỆU VÀ GIỚI HẠN PHẠM VI

#### 7.1. Trạng thái các tệp dữ liệu `.dat`
Kiểm tra metadata trực tiếp trên hệ thống tệp tin:
- `data/users.dat`: **6,958 bytes** (LastWriteTime: 2026-10-30 07:53:06) — **Nguyên vẹn 100%**.
- `data/bookings.dat`: **2,709 bytes** (LastWriteTime: 2026-10-30 07:51:40) — **Nguyên vẹn 100%**.
- `data/reviews.dat`: **1,650 bytes** (LastWriteTime: 2026-10-30 07:45:13) — **Nguyên vẹn 100%**.

#### 7.2. Giới hạn phạm vi (Boundary Adherence)
- Các Domain Entity (`User.h`, `User.cpp`, `Booking.h`, `Review.h`), interface `ISerializable`, `MockDatabaseEngine`, các lớp `Service` và `GUIBridge` **hoàn toàn không bị chỉnh sửa hay thay thế trong Step 2**.
- Các Dedicated Serializers hoạt động hoàn toàn độc lập, sẵn sàng để được tích hợp vào các File Repositories (`FileUserRepository`, `FileBookingRepository`, `FileReviewRepository`) ở **Phase 2, Step 3**.

---

### 8. VẤN ĐỀ TỒN TẠI VÀ LƯU Ý KỸ THUẬT

1. **Nợ kỹ thuật mật khẩu thô (Plaintext Password):**
   - Trường `passwordHash` trong `UserSerializer` tiếp tục bảo toàn nguyên vẹn giá trị chuỗi văn bản thô theo đúng định dạng `users.dat` và cơ chế xác thực của `AuthService`. Tuyệt đối không tự ý áp dụng hàm băm khi chưa có quy trình di trú dữ liệu độc lập.
2. **Kế hoạch cho Step 3:**
   - Trong Step 3, các File Repositories sẽ nhận các Serializer tương ứng thông qua cơ chế Dependency Injection (hoặc composition) để thực hiện đọc/ghi các dòng tệp `.dat`, thay thế dần các truy cập trực tiếp từ `MockDatabaseEngine`.

---

### 9. KẾT LUẬN VÀ DỪNG BƯỚC (STOP CONDITION)
Phase 2 — Step 2 đã được triển khai hoàn chỉnh, sạch sẽ, đạt chuẩn C++17 và bảo toàn 100% tương thích ngược.

**Hệ thống dừng tại đây và sẵn sàng chờ chỉ thị / phê duyệt trước khi chuyển sang Phase 2, Step 3 (Concrete File Repositories).**
