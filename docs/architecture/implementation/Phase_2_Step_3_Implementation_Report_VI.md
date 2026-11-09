# BÁO CÁO TRIỂN KHAI HOÀN THÀNH (IMPLEMENTATION REPORT)
## PHASE 2, STEP 3: CONCRETE FILE REPOSITORIES

**Dự án:** TutorMatch — Qt 6.11.2 / C++17 Three-Layer Architecture Refactoring  
**Giai đoạn:** Phase 2 — Tách tầng Data Access Layer (DAL)  
**Bước thực hiện:** Step 3 — Concrete File Repositories (Xây dựng các Kho lưu trữ tệp chuyên biệt)  
**Ngày hoàn thành:** 2026-11-06  
**Trạng thái cổng chất lượng:** SẴN SÀNG ĐỂ KIỂM TOÁN / PHÊ DUYỆT (READY FOR AUDIT)  

---

### 1. TỔNG QUAN VÀ MỤC TIÊU BƯỚC 3

Trong kiến trúc tổng thể của TutorMatch, tầng Data Access Layer (DAL) chịu trách nhiệm duy nhất về lưu trữ vật lý, quản lý truy xuất tệp và duy trì các cấu trúc dữ liệu tối ưu hóa trên bộ nhớ RAM. 

Sau khi Step 1 đã chuẩn hóa các Interface Contracts (`IUserRepository`, `IBookingRepository`, `IReviewRepository`) và Step 2 đã đóng gói thành công các bộ tuần tự hóa chuyên biệt (`UserSerializer`, `BookingSerializer`, `ReviewSerializer`), mục tiêu cốt lõi của **Step 3** là:
1. **Hiện thực hóa 3 Concrete Repositories độc lập:**
   - `FileUserRepository` (quản lý `users.dat` và các chỉ mục O(1) RAM Hash Index, O(log N) AVL Trees, O(1) LRU Cache).
   - `FileBookingRepository` (quản lý `bookings.dat`, truy vấn đặt lịch theo học viên/gia sư).
   - `FileReviewRepository` (quản lý `reviews.dat`, lưu và xóa đánh giá vi phạm).
2. **Tách biệt và tái sử dụng Serializers:** Sử dụng 100% các serializer từ Step 2 để chuyển đổi qua lại giữa Domain Entities và định dạng CSV của tệp phẳng.
3. **Bảo toàn toàn vẹn dữ liệu và an toàn lỗi I/O:** Áp dụng cơ chế ghi đè an toàn nguyên tử (Atomic file rewrite qua file `.tmp` và đổi tên), kiểm soát trạng thái stream, cô lập lỗi và kiểm thử trên thư mục sandbox riêng biệt để bảo vệ 100% ba file `.dat` gốc.
4. **Tuân thủ giới hạn phạm vi (Boundary Adherence):** Chưa chuyển đổi `MockDatabaseEngine` thành Delegating Facade (thuộc Step 4), chưa xóa `ISerializable` (thuộc Step 5), chưa can thiệp vào Services hay GUIBridge.

---

### 2. DANH MỤC CÁC TỆP TIN ĐÃ TẠO VÀ CHỈNH SỬA

#### 2.1. Các tệp tin tạo mới (Created Files)
1. **`include/storage/repositories/FileUserRepository.h`**: Khai báo lớp `FileUserRepository`, kế thừa hợp đồng `IUserRepository`, tích hợp `HashTable<int, std::streampos>`, `LRUCache<int, std::string>`, `AVLTree<double, int>`.
2. **`src/storage/repositories/FileUserRepository.cpp`**: Cài đặt chi tiết `FileUserRepository`, quản lý file `users.dat`, tìm kiếm O(1) disk seek qua `std::streampos`, cập nhật chỉ mục đồng bộ với đĩa và ghi đè nguyên tử.
3. **`include/storage/repositories/FileBookingRepository.h`**: Khai báo lớp `FileBookingRepository`, kế thừa hợp đồng `IBookingRepository`.
4. **`src/storage/repositories/FileBookingRepository.cpp`**: Cài đặt chi tiết `FileBookingRepository`, quản lý file `bookings.dat`, ghi append luồng học tập và cập nhật trạng thái vòng đời lớp học nguyên tử.
5. **`include/storage/repositories/FileReviewRepository.h`**: Khai báo lớp `FileReviewRepository`, kế thừa hợp đồng `IReviewRepository`.
6. **`src/storage/repositories/FileReviewRepository.cpp`**: Cài đặt chi tiết `FileReviewRepository`, quản lý file `reviews.dat`, hỗ trợ ghi đánh giá mới và xóa đánh giá vi phạm an toàn.

#### 2.2. Các tệp tin chỉnh sửa (Modified Files)
1. **`CMakeLists.txt`**:
   - Đăng ký 3 file `.h` repository mới vào danh sách `HEADERS`.
   - Đăng ký 3 file `.cpp` repository mới vào `CORE_SOURCES` (dành cho `TutorMatch_GUI`).
   - Đăng ký 3 file `.cpp` repository mới vào target `TutorMatch_Tests`.
2. **`tests/TestRunner.cpp`**:
   - Bổ sung Test Suite 18: `run_test_concrete_file_repositories()`, kiểm thử toàn diện toàn bộ các chức năng CRUD, chỉ mục, cache, và các trường hợp lỗi (failure cases) trong môi trường thư mục tạm cô lập (isolated sandbox).

---

### 3. CHI TIẾT THIẾT KẾ VÀ HIỆN THỰC HÓA 3 REPOSITORIES

#### 3.1. `FileUserRepository` (Triển khai `IUserRepository`)
- **Quản lý tệp:** `users.dat` (mặc định thông qua `ApplicationPaths::resolveDataDirectory("data") + "/users.dat"`, có thể nhận đường dẫn tệp/thư mục tùy biến phục vụ kiểm thử).
- **Phân tách trách nhiệm tuần tự hóa:** Toàn bộ việc ghi ra chuỗi CSV giao cho `UserSerializer::toCsv(*user)`, việc đọc vào giao cho `UserSerializer::fromCsv(line)`.
- **Bảo toàn đa hình:** Phương thức `findUserById` và `findUserByUsername` trả về `std::shared_ptr<User>` trỏ đến instance thực tế `Student`, `Tutor`, hoặc `Admin`. Các phép `dynamic_cast` hoạt động hoàn hảo, không xảy ra Object Slicing.
- **Quản lý cấu trúc dữ liệu tối ưu:**
  - `ramHashIndex` (`HashTable<int, std::streampos>`): Lưu trữ vị trí byte offset của từng `userId` trong tệp, cho phép thực hiện `seekg(offset)` và đọc đúng dòng dữ liệu trong thời gian $O(1)$ mà không cần quét tuần tự toàn bộ file.
  - `profileLruCache` (`LRUCache<int, std::string>`): Đệm 50 dòng hồ sơ CSV gần nhất. Khi có cache hit, deserialize ngay mà không chạm xuống đĩa.
  - `priceRateIndex` & `ratingIndex` (`AVLTree<double, int>`): Cây nhị phân tìm kiếm cân bằng, lập chỉ mục mức học phí và điểm rating của các gia sư đang hoạt động (`isActive == true`). Hỗ trợ tìm kiếm theo dải giá (`rangeSearch`) trong $O(\log N + K)$.
- **Cơ chế đồng bộ hóa và an toàn đa luồng:**
  - Bảo vệ bởi `mutable std::recursive_mutex userRepoMutex`.
  - Khi `updateUser()` hoặc `deleteUserSoft()` được gọi, repository xóa bỏ chỉ mục cũ trên cây AVL, ghi đè an toàn tệp, và chỉ cập nhật lại `ramHashIndex`/`profileLruCache`/`AVLTree` sau khi tệp tin đã ghi đè thành công xuống đĩa.

#### 3.2. `FileBookingRepository` (Triển khai `IBookingRepository`)
- **Quản lý tệp:** `bookings.dat`.
- **Phân tách trách nhiệm tuần tự hóa:** Sử dụng `BookingSerializer::toCsv(booking)` khi ghi và `BookingSerializer::fromCsv(line)` khi đọc.
- **Quản lý ID tự tăng:** Khởi tạo `nextBookingId` từ giá trị lớn nhất trong tệp (bắt đầu tối thiểu từ 101).
- **Thao tác lưu trữ:**
  - `saveBooking()`: Mở tệp ở chế độ `std::ios::app`, ghi dòng CSV kết thúc bằng `\n`.
  - `updateBooking()`: Đọc toàn bộ danh sách, thay thế bản ghi mang ID tương ứng, ghi đè nguyên tử qua tệp `.tmp`, sau đó đổi tên sang `bookings.dat`. Nếu ID không tồn tại, trả về `false` và không làm biến đổi file.
  - `getBookingsByStudentId()`, `getBookingsByTutorId()`: Lọc theo thuộc tính tương ứng từ danh sách đầy đủ.
- **Ranh giới trách nhiệm (Architectural Boundary):** Không chứa bất kỳ logic kiểm tra trùng lịch (ScheduleGuard collision check) hay chính sách chấp thuận nào. Repository chỉ thuần túy lưu trữ và truy vấn dữ liệu.

#### 3.3. `FileReviewRepository` (Triển khai `IReviewRepository`)
- **Quản lý tệp:** `reviews.dat`.
- **Phân tách trách nhiệm tuần tự hóa:** Sử dụng `ReviewSerializer::toCsv(review)` khi ghi và `ReviewSerializer::fromCsv(line)` khi đọc.
- **Quản lý ID tự tăng:** Bắt đầu từ 1, tự động đồng bộ theo `maxId + 1` khi đọc tệp.
- **Thao tác lưu trữ:**
  - `saveReview()`: Ghi nối đuôi (append) vào tệp `reviews.dat`.
  - `deleteReview(int reviewId)`: Lọc bỏ review mang ID bị xóa (kiểm duyệt nội dung vi phạm), ghi đè an toàn tệp `reviews.dat`.
  - `getReviewsByTutorId(int tutorId)`: Trả về đầy đủ các đánh giá của gia sư để Service tính toán lại điểm số `rating` và `ratingCount`. Repository tuyệt đối không tự ý cập nhật sang `users.dat`.

---

### 4. BẢO TOÀN DỮ LIỆU VÀ XỬ LÝ LỖI I/O (I/O SAFETY & ATOMICITY)

1. **Ghi đè nguyên tử chống hỏng hóc (Atomic Persistence):**
   - Khi ghi đè (`updateUser`, `updateBooking`, `deleteReview`), dữ liệu được ghi trước vào tệp tạm thời `*.tmp`.
   - Kiểm tra `fail()`, gọi `flush()`, và kiểm tra `good()` của luồng ghi.
   - Kiểm tra kích thước file tạm qua `std::filesystem::file_size`.
   - Sử dụng `std::filesystem::rename` (trên Windows có fallback sao chép luồng nhị phân an toàn) để tráo đổi tệp tạm thành tệp chính thức.
   - Nếu xảy ra lỗi tại bất kỳ bước nào, tệp tạm bị xóa bỏ ngay lập tức, tệp gốc giữ nguyên trạng 100%, và RAM index/cache không bị thay đổi.
2. **Xử lý tệp không tồn tại và tệp rỗng:**
   - Trong `initialize()`, repository tự động kiểm tra và tạo thư mục cha nếu chưa có, tạo tệp rỗng nếu tệp chưa tồn tại.
   - Khi tệp rỗng, các phương thức truy vấn trả về vector rỗng hoặc `nullptr`, không gây crash hoặc ngoại lệ.
3. **Khả năng phục hồi khi gặp dòng dữ liệu bị hỏng (Malformed Record Resilience):**
   - Nếu trong tệp xuất hiện một dòng bị cắt cụt, thiếu trường, hoặc sai kiểu dữ liệu, Serializer trả về `nullptr`/`nullopt`.
   - Repository ghi log cảnh báo chi tiết (không log mật khẩu) và bỏ qua dòng lỗi đó, tiếp tục nạp các dòng dữ liệu hợp lệ còn lại mà không làm sập ứng dụng.

---

### 5. KẾT QUẢ BIÊN DỊCH VÀ BẰNG CHỨNG KIỂM THỬ THỰC TẾ

#### 5.1. Biên dịch mục tiêu `TutorMatch_Tests`
- **Lệnh thực thi:**
  ```powershell
  Set-Location "C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug"
  $env:PATH = "D:\CODE\Tools\CMake_64\bin;D:\CODE\6.11.2\mingw_64\bin;D:\CODE\Tools\mingw1310_64\bin;" + $env:PATH
  cmake --build . --target TutorMatch_Tests
  ```
- **Kết quả:** `Exit code 0`. Không có lỗi biên dịch, không có cảnh báo.

#### 5.2. Biên dịch mục tiêu `TutorMatch_GUI`
- **Lệnh thực thi:**
  ```powershell
  Set-Location "C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug"
  $env:PATH = "D:\CODE\Tools\CMake_64\bin;D:\CODE\6.11.2\mingw_64\bin;D:\CODE\Tools\mingw1310_64\bin;" + $env:PATH
  cmake --build . --target TutorMatch_GUI
  ```
- **Kết quả:** `Exit code 0`. Biên dịch và liên kết thành công vào `TutorMatch_GUI.exe`.

#### 5.3. Thực thi Bộ kiểm thử tự động `TutorMatch_Tests.exe`
- **Lệnh thực thi:**
  ```powershell
  .\TutorMatch_Tests.exe
  ```
- **Kết quả:** `Exit code 0`.
- **Thống kê:** **18/18 test suites PASS (100% PASS)**.
- **Nhật ký kiểm thử thực tế (Verbatim Output Log):**
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
  [TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)...
  [FileUserRepository Warning] Skipping corrupted line in C:\Users\DELL\AppData\Local\Temp\tutormatch_step3_test_sandbox\users_test.dat: User CSV requires at least 7 fields, got 5
  [FileUserRepository Notice] Auto-compacting users: detected 5 lines, deduplicating to 4 records.
  [FileBookingRepository Warning] Skipping corrupted booking line: Booking CSV requires at least 10 fields, got 3
  [FileReviewRepository Warning] Skipping corrupted review line: Review CSV requires at least 8 fields, got 3
    -> PASS: FileUserRepository, FileBookingRepository, and FileReviewRepository verified successfully!
  ============================================================
    >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
  ============================================================
  ```

---

### 6. XÁC NHẬN TOÀN VẸN DỮ LIỆU GỐC VÀ RANH GIỚI KIẾN TRÚC

#### 6.1. Tính bất biến của các tệp dữ liệu phẳng gốc (`data/*.dat`)
Kiểm toán độc lập hệ thống tệp tin xác nhận:
- `data/users.dat`: **6,958 bytes** (Timestamp: 2026-10-30 07:53:06) — **Nguyên vẹn 100% (Untouched)**.
- `data/bookings.dat`: **2,709 bytes** (Timestamp: 2026-10-30 07:51:40) — **Nguyên vẹn 100% (Untouched)**.
- `data/reviews.dat`: **1,650 bytes** (Timestamp: 2026-10-30 07:45:13) — **Nguyên vẹn 100% (Untouched)**.

Toàn bộ quá trình kiểm thử tự động của Suite 18 được thực hiện trên thư mục tạm `tutormatch_step3_test_sandbox` trong `temp_directory_path()`, tuyệt đối không ghi đè hay can thiệp vào thư mục `data/` thật.

#### 6.2. Tuân thủ giới hạn phạm vi (Boundary Adherence)
- Lớp `MockDatabaseEngine` giữ nguyên 100% cấu trúc ban đầu, chưa chuyển đổi thành Facade (chờ Step 4).
- Các Domain Entity (`User`, `Student`, `Tutor`, `Admin`, `Booking`, `Review`) và `ISerializable` giữ nguyên trạng (chờ Step 5).
- Các lớp Service (`AuthService`, `BookingService`, `UserService`, `AdminService`) và lớp giao diện `GUIBridge` giữ nguyên trạng, chưa bị thay đổi dependency.

---

### 7. VẤN ĐỀ TỒN TẠI VÀ CHUẨN BỊ CHO BƯỚC 4

1. **Chuẩn bị chuyển đổi Facade (Step 4):**
   - Ba Concrete Repositories hiện tại đã hoàn chỉnh, hoạt động độc lập và đạt độ tin cậy tuyệt đối.
   - Trong Step 4, `MockDatabaseEngine` sẽ được chuyển đổi thành một Pure Delegating Facade chứa 3 `std::shared_ptr<FileUserRepository>`, `std::shared_ptr<FileBookingRepository>`, `std::shared_ptr<FileReviewRepository>` để loại bỏ hoàn toàn mã I/O trùng lặp mà vẫn giữ vững 100% tính tương thích cho 18 bài kiểm thử.
2. **Nợ kỹ thuật mật khẩu:**
   - Trường `passwordHash` trong `FileUserRepository` tiếp tục bảo toàn định dạng chuỗi văn bản thô theo đúng thiết kế hiện hữu của dự án.

---

### 8. ĐIỀU KIỆN DỪNG (STOP CONDITION)
Mọi yêu cầu kỹ thuật của **Phase 2 — Step 3** đã hoàn thành trọn vẹn, không có lỗi, không có regression.

**Hệ thống dừng tại đây và sẵn sàng chờ phê duyệt trước khi chuyển sang Phase 2, Step 4 (MockDatabaseEngine Compatibility Facade).**
