# ARCHITECTURE DECISION RECORD (ADR)
## ADR-003: Trích Xuất ReviewService và MatchingService Khỏi BookingService & Presentation Layer

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-003
- **Tiêu đề:** Trích xuất ReviewService và MatchingService Khỏi BookingService & Presentation Layer
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & OO Design Specialist
- **Phạm vi tác động:** `include/services/BookingService.h`, `src/services/BookingService.cpp`, `include/services/ReviewService.h`, `src/services/ReviewService.cpp`, `include/services/MatchingService.h`, `src/services/MatchingService.cpp`, `GUIBridge`, và `ConsoleApp`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong kiến trúc hiện hành của TutorMatch, có hai điểm bất hợp lý nghiêm trọng về phân công trách nhiệm:
1. **`BookingService` bị quá tải trách nhiệm (SRP Violation):**
   - Lớp này vừa quản lý vòng đời đơn đặt lịch (`bookLesson`, `acceptBooking`, `rejectBooking`, `cancelBooking`, kiểm tra xung đột thời khoá biểu 56-bit), vừa kiêm nhiệm quy trình đánh giá nhận xét (`submitReview`, `getTutorReviews`, `getAllReviews`) và tự tính toán điểm đánh giá trung bình cộng của gia sư (`sum / allRevs.size()`).
   - Việc này buộc `BookingService` phải phụ thuộc vào cả 3 interfaces: `IBookingRepository`, `IUserRepository`, và `IReviewRepository`.
2. **Nghiệp vụ Ghép nối thông minh bị rò rỉ ra Presentation Layer:**
   - Cả `GUIBridge.cpp` (dòng 795-802) và `ConsoleApp.cpp` đều tự mình truy xuất danh sách gia sư từ repository, tự lọc tiêu chí, tự quản lý `MatchingEngine`, và tự gọi thuật toán tìm kiếm Top-K.
   - Điều này biến Presentation Layer thành nơi điều phối nghiệp vụ, dẫn đến trùng lặp mã nguồn giữa giao diện đồ hoạ và giao diện dòng lệnh.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định trích xuất hai Service chuyên trách mới trong Business Logic Layer:

#### 1. Trích xuất `ReviewService`
- **Vị trí tệp:** `include/services/ReviewService.h`, `src/services/ReviewService.cpp`.
- **Trách nhiệm chuyên biệt:**
  - Quản lý điều kiện đánh giá (Review Eligibility): Đảm bảo chỉ học viên sở hữu đơn đặt lịch đã ở trạng thái `COMPLETED` và chưa từng đánh giá trước đó mới được phép tạo đánh giá mới.
  - Tiếp nhận và xác thực đánh giá (`submitReview`).
  - Xoá đánh giá vi phạm (`deleteReview`).
  - Tự động tính toán lại điểm đánh giá trung bình của gia sư dựa trên toàn bộ các đánh giá hợp lệ và cập nhật đồng bộ vào `IUserRepository`.
- **Phụ thuộc:** `IReviewRepository`, `IBookingRepository`, `IUserRepository`.
- **Hệ quả đối với `BookingService`:** Gỡ bỏ hoàn toàn `IReviewRepository` và toàn bộ mã liên quan đến đánh giá khỏi `BookingService`. `BookingService` chỉ còn tập trung 100% vào vòng đời đặt lịch học và các ràng buộc xung đột lịch.

#### 2. Trích xuất `MatchingService`
- **Vị trí tệp:** `include/services/MatchingService.h`, `src/services/MatchingService.cpp`.
- **Trách nhiệm chuyên biệt:**
  - Điều phối toàn bộ use case ghép nối gia sư và tìm kiếm gia sư theo tiêu chí.
  - Truy xuất danh sách gia sư hợp lệ (đang hoạt động `isActive` và đã xác thực `isVerified`) từ `IUserRepository`.
  - Phối hợp với `MatchingEngine` (sở hữu qua `std::unique_ptr<MatchingEngine>`) để thực thi thuật toán chấm điểm đa tiêu chuẩn (`WeightedScoreMatchingStrategy`) và trích xuất Top-K qua hàng đợi ưu tiên `MaxHeap`.
- **Phụ thuộc:** `IUserRepository`, `MatchingEngine`.
- **Hệ quả đối với Presentation Layer:** `GUIBridge` và `ConsoleApp` không còn sở hữu `MatchingEngine`, không tự điều phối tìm kiếm; cả hai chỉ đơn thuần gọi `matchingService->findTopMatchesForStudent()`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Giữ nguyên Review trong BookingService, chỉ tạo MatchingService
- **Mô tả:** Giữ nguyên các hàm `submitReview` trong `BookingService` để tránh phải tạo thêm file mới.
- **Lý do loại bỏ:** Vi phạm trực tiếp nguyên lý Single Responsibility Principle (SRP). Khái niệm "Booking" (Giao dịch lịch học) và "Review" (Đánh giá chất lượng) là hai miền khái niệm khác nhau trong nghiệp vụ giáo dục.

#### Phương án B: Biến MatchingEngine thành một Service
- **Mô tả:** Gộp `MatchingService` vào `MatchingEngine`, để `MatchingEngine` trực tiếp nhận `IUserRepository`.
- **Lý do loại bỏ:** Vi phạm việc tách biệt giữa **Algorithmic Engine** (thuật toán tính toán thuần túy, độc lập I/O) và **Application Service** (dịch vụ điều phối use case, tương tác với kho lưu trữ). `MatchingEngine` cần được giữ là một Domain/Algorithmic Engine thuần túy để dễ dàng kiểm thử đơn vị độc lập.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Phân tách trách nhiệm mạch lạc (High Cohesion):** `BookingService`, `ReviewService`, và `MatchingService` mỗi lớp chỉ phục vụ đúng một mục đích nghiệp vụ duy nhất.
- **Loại bỏ trùng lặp mã nguồn (DRY):** Cả GUI và Console dùng chung một quy trình nghiệp vụ ghép nối và đánh giá trong BLL.
- **An toàn đa luồng:** Mỗi service tự quản lý khoá đồng bộ (`serviceMutex`, `reviewMutex`) cho phạm vi dữ liệu mình phụ trách.
- **Bảo toàn tính toàn vẹn của Rating gia sư:** Điểm rating của gia sư luôn được cập nhật nguyên tử ngay khi có đánh giá mới hoặc khi xoá đánh giá.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Số lượng lớp và tệp tin trong `services/` tăng thêm 2 cặp tệp (`ReviewService.h/.cpp` và `MatchingService.h/.cpp`).
- Cần đăng ký thêm 2 service mới vào Composition Root trong `main.cpp`.

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Khi tính điểm rating gia sư mới trong `ReviewService`, thao tác ghi vào `IUserRepository` có thể bị lỗi làm điểm rating hiển thị không khớp với bảng đánh giá.
- **Biện pháp giảm thiểu:** Bọc thao tác tính toán và cập nhật trong `reviewMutex`, kiểm tra kết quả trả về của `userRepo->updateUser(*tutor)`, ghi log cảnh báo rõ ràng nếu có lỗi I/O.

---

### 6. KẾT LUẬN
Việc trích xuất `ReviewService` và `MatchingService` là quyết định kiến trúc bắt buộc để đạt được một hệ thống chuẩn SRP, giúp mã nguồn trong sáng, dễ mở rộng và dễ bảo trì.
