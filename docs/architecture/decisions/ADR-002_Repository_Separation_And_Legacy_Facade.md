# ARCHITECTURE DECISION RECORD (ADR)
## ADR-002: Phân Rã Repository Độc Lập & Bảo Toàn Legacy Facade

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-002
- **Tiêu đề:** Phân rã Repository Độc Lập & Bảo Toàn Legacy Facade cho MockDatabaseEngine
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Senior C++ Engineer
- **Phạm vi tác động:** `include/storage/MockDatabaseEngine.h`, `src/storage/MockDatabaseEngine.cpp`, các Concrete Repositories mới (`FileUserRepository`, `FileBookingRepository`, `FileReviewRepository`), và toàn bộ 16 test suites trong `tests/TestRunner.cpp`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong cấu trúc hiện tại, lớp `MockDatabaseEngine` (674 dòng lệnh trong `src/storage/MockDatabaseEngine.cpp`) là một khối cài đặt nguyên khối (Monolithic Repository) thực hiện cùng lúc 3 interfaces: `IUserRepository`, `IBookingRepository`, và `IReviewRepository`. 

Lớp này quản lý đồng thời:
1. Ba tệp lưu trữ vật lý độc lập: `users.dat`, `bookings.dat`, và `reviews.dat`.
2. Bốn cấu trúc dữ liệu tối ưu hoá bộ nhớ: `ramHashIndex` (`HashTable<int, std::streampos>`), `profileLruCache` (`LRUCache<int, std::string>`), `priceRateIndex` (`AVLTree<double, int>`), và `ratingIndex` (`AVLTree<double, int>`).
3. Logic sinh ID tự tăng cho cả 3 thực thể (`nextAutoIncrementId`, `nextBookingId`, `nextReviewId`).
4. Toàn bộ 16 bộ kiểm thử tự động tại `tests/TestRunner.cpp` đều khởi tạo và tương tác trực tiếp với `MockDatabaseEngine`.

Việc gom toàn bộ vào một lớp vi phạm nguyên lý Đơn trách nhiệm (Single Responsibility Principle - SRP) ở tầng Data Access Layer và gây khó khăn cho việc kiểm thử độc lập từng miền dữ liệu.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Phân rã thành 3 Concrete Repositories độc lập:**
   - **`FileUserRepository` (Hiện thực hoá `IUserRepository`):** Chịu trách nhiệm duy nhất về lưu trữ người dùng trên `users.dat`. Tiếp quản quyền sở hữu độc quyền của 4 cấu trúc dữ liệu tối ưu:
     - `ramHashIndex`: Quản lý byte offset của `userId` để tìm kiếm đĩa O(1).
     - `profileLruCache`: Đệm chuỗi hồ sơ người dùng để giảm tải I/O đĩa.
     - `priceRateIndex` & `ratingIndex`: Hai cây AVL phục vụ truy vấn theo dải giá (`findTutorIdsByPriceRange`) và điểm đánh giá (`findTutorIdsByMinRating`).
   - **`FileBookingRepository` (Hiện thực hoá `IBookingRepository`):** Chịu trách nhiệm duy nhất về lưu trữ đơn đặt lịch trên `bookings.dat`. Quản lý `nextBookingId` và cập nhật trạng thái đơn học nguyên tử.
   - **`FileReviewRepository` (Hiện thực hoá `IReviewRepository`):** Chịu trách nhiệm duy nhất về lưu trữ đánh giá nhận xét trên `reviews.dat`. Quản lý `nextReviewId` và xoá đánh giá vi phạm.
2. **Chuyển đổi `MockDatabaseEngine` thành một Pure Delegating Facade:**
   - Giữ nguyên toàn bộ giao diện công khai (public methods) của `MockDatabaseEngine` để đảm bảo tương thích ngược 100% với 16 bộ kiểm thử hiện hành và các thành phần phụ thuộc cũ.
   - Bên trong, `MockDatabaseEngine` nắm giữ 3 con trỏ `std::shared_ptr` tham chiếu đến 3 Repositories cụ thể trên và **uỷ quyền 100%** các lời gọi hàm xuống các repositories này.
   - Loại bỏ hoàn toàn mã đọc/ghi file và các biến trạng thái trùng lặp bên trong `MockDatabaseEngine`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Xoá bỏ hoàn toàn MockDatabaseEngine và sửa toàn bộ Test Suites
- **Mô tả:** Xoá bỏ hoàn toàn `MockDatabaseEngine`, chuyển đổi trực tiếp toàn bộ 16 bài kiểm thử trong `tests/TestRunner.cpp` và mã ứng dụng sang gọi trực tiếp `FileUserRepository`, `FileBookingRepository`, `FileReviewRepository`.
- **Lý do loại bỏ:**
  - Tiềm ẩn rủi ro phá vỡ các bài kiểm thử hồi quy học thuật vốn đang hoạt động ổn định (100% PASS).
  - Khối lượng chỉnh sửa quá lớn đồng thời tại một thời điểm, vi phạm nguyên tắc "Refactoring từng bước an toàn" (Step-by-step safe migration).

#### Phương án B: Giữ nguyên MockDatabaseEngine dạng Monolithic
- **Mô tả:** Tiếp tục sử dụng `MockDatabaseEngine` làm lớp cài đặt duy nhất cho cả 3 interfaces; không tạo các concrete repository riêng biệt.
- **Lý do loại bỏ:**
  - Vi phạm nguyên tắc SRP ở tầng lưu trữ;
  - Khiến mã nguồn tầng DAL tiếp tục phình to và không thể kiểm thử cô lập (isolated testing) từng file dữ liệu.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Phân tách trách nhiệm hoàn hảo:** Mỗi thực thể và tệp lưu trữ được quản lý bởi đúng một Repository chuyên trách.
- **Tương thích ngược 100% (Zero Breaking Changes):** Toàn bộ 16 bộ kiểm thử hiện tại trong `tests/TestRunner.cpp` tiếp tục biên dịch và vượt qua mà không cần viết lại toàn bộ mã kiểm thử.
- **Không có dữ liệu trùng lặp (No Split Brain):** Do `MockDatabaseEngine` uỷ quyền hoàn toàn xuống các concrete repository được tiêm vào, hệ thống chỉ có đúng một nguồn chân lý (Single Source of Truth).
- **Đáp ứng xuất sắc tiêu chí học thuật:** Đảm bảo toàn bộ các cấu trúc dữ liệu nâng cao (`HashTable`, `LRUCache`, `AVLTree`) được định vị chính xác tại nơi chúng phát huy giá trị kỹ thuật cao nhất (trong `FileUserRepository`).

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Tăng thêm một mức uỷ quyền (Indirection Overhead) khi gọi qua `MockDatabaseEngine`. Tuy nhiên chi phí gọi hàm nội tuyến C++ (inline method invocation) là không đáng kể ($O(1)$ CPU cycles).

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Nguy cơ bất đồng bộ nếu `MockDatabaseEngine` tự lưu một bản sao dữ liệu riêng biệt với các concrete repositories.
- **Biện pháp giảm thiểu:** Thiết kế `MockDatabaseEngine` không chứa bất kỳ container hoặc con trỏ dữ liệu tệp độc lập nào; 100% các phương thức đều return trực tiếp kết quả từ `userRepo->...`, `bookingRepo->...`, `reviewRepo->...`.

---

### 6. KẾT LUẬN
Quyết định tách thành 3 Concrete File Repositories kết hợp với Pure Delegating Facade là giải pháp hoàn hảo, dung hoà giữa tính thanh thoát của kiến trúc mới và tính ổn định bền vững của hệ thống kiểm thử hiện hành.
