# ARCHITECTURE DECISION RECORD (ADR)
## ADR-006: Định Vị GUIBridge và ConsoleApp Là Các Presentation Adapters Chuẩn Mực

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-006
- **Tiêu đề:** Định Vị GUIBridge và ConsoleApp Là Các Presentation Adapters Chuẩn Mực
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Qt/QML Specialist
- **Phạm vi tác động:** `include/gui/GUIBridge.h`, `src/gui/GUIBridge.cpp`, `include/ui/ConsoleApp.h`, `src/ui/ConsoleApp.cpp`, và các tệp QML Views.

---

### 1. BỐI CẢNH (CONTEXT)
Trong cấu trúc hiện nay của TutorMatch, tầng giao diện (Presentation Layer) có hai vấn đề kiến trúc nghiêm trọng:
1. **`GUIBridge` đóng vai trò "God Object" (1,894 dòng lệnh):**
   - Trực tiếp nắm giữ 3 con trỏ `userRepo`, `bookingRepo`, `reviewRepo` (`include/gui/GUIBridge.h`, dòng 43-45) và thực hiện hơn 20 truy vấn tệp dữ liệu trực tiếp trong `src/gui/GUIBridge.cpp`.
   - Trực tiếp sở hữu `MatchingEngine` và điều phối thuật toán tìm kiếm gia sư.
   - Trực tiếp nắm giữ biến cấu hình kinh doanh `platformCommissionRate`.
   - Vừa làm nhiệm vụ chuyển đổi kiểu dữ liệu cho Qt QML, vừa đóng vai trò điều khiển nghiệp vụ và truy xuất cơ sở dữ liệu.
2. **`ConsoleApp` hoàn toàn đi vòng qua Tầng Nghiệp Vụ (1,589 dòng lệnh):**
   - `ConsoleApp.h` khai báo trực tiếp `std::shared_ptr<MockDatabaseEngine> dbEngine;`.
   - Trong `src/ui/ConsoleApp.cpp`, có tới 24 phương thức gọi trực tiếp vào `dbEngine` để thực hiện thao tác CRUD tệp tin.
   - `ConsoleApp` hoàn toàn không sử dụng các dịch vụ `AuthService`, `BookingService`, `AdminService`, tự viết lại logic đăng nhập, đặt lịch và quản lý người dùng.
3. **QML tự tính toán chỉ số nghiệp vụ:**
   - Trong `StudentBookingsView.qml`, mã JavaScript tự duyệt mảng và tính tổng chi phí `weeklyCost += (b.hourlyRate || 0.0)`.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Thiết lập `GUIBridge` và `ConsoleApp` là hai Presentation Adapters song song, độc lập công nghệ:**
   - Cả hai lớp đều thuộc Tầng 1 (Presentation Layer).
   - Cả hai lớp **TUYỆT ĐỐI KHÔNG** được phép giữ bất kỳ con trỏ nào đến Repositories (`IUserRepository`, `MockDatabaseEngine`, v.v.) và không được thực hiện bất kỳ thao tác File I/O nào.
   - Cả hai lớp được tiêm phụ thuộc (Dependency Injection) duy nhất vào 6 Services thuộc Business Logic Layer:
     - `AuthService`
     - `UserService`
     - `BookingService`
     - `ReviewService` *(Mới)*
     - `MatchingService` *(Mới)*
     - `AdminService`
2. **Trách nhiệm duy nhất của `GUIBridge` (Thin QML Adapter):**
   - Tiếp nhận các lời gọi từ giao diện QML thông qua các phương thức `Q_INVOKABLE`.
   - Chuyển đổi tham số từ các kiểu dữ liệu Qt/QML (`QString`, `QVariantList`, `QVariantMap`, `int`, `double`) sang các kiểu dữ liệu C++ chuẩn (`std::string`, `std::vector`, DTOs) để chuyển tiếp sang các Services.
   - Nhận kết quả từ Services, đóng gói thành `QVariantMap` hoặc `QVariantList` và trả về cho QML.
   - Quản lý trạng thái phiên làm việc của giao diện người dùng (`currentUser`, cờ đăng nhập `isLoggedIn`, thông điệp trạng thái `statusMessage`).
   - Bảo toàn 100% chữ ký của hơn 45 phương thức `Q_INVOKABLE` hiện hữu để không làm vỡ bất kỳ màn hình nào trong số 28 tệp QML.
3. **Trách nhiệm duy nhất của `ConsoleApp` (Thin CLI Adapter):**
   - Đọc dữ liệu nhập từ bàn phím qua dòng lệnh (`std::cin`, `std::getline`).
   - Gọi các phương thức nghiệp vụ của 6 Services.
   - Định dạng kết quả và in ra màn hình console bằng bảng màu ANSI (`COLOR_GREEN`, `COLOR_RED`, v.v.).
   - Loại bỏ 100% lời gọi trực tiếp đến `dbEngine`.
4. **Loại bỏ rò rỉ nghiệp vụ trong QML:**
   - Bổ sung cấu trúc DTO `StudentBookingMetrics` trong BLL và cung cấp phương thức `qmlGetStudentBookingMetrics()` trên `GUIBridge`.
   - `StudentBookingsView.qml` chỉ hiển thị dữ liệu đã được tính toán sẵn từ backend, không tự tính toán tài chính bằng JavaScript.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Chỉ tái cấu trúc GUIBridge, giữ nguyên ConsoleApp
- **Mô tả:** Giữ nguyên `ConsoleApp` gọi trực tiếp `MockDatabaseEngine` vì cho rằng giao diện Console chỉ là công cụ phụ hoặc debug.
- **Lý do loại bỏ:**
  - Vi phạm tính nhất quán của hệ thống. Nếu quy tắc nghiệp vụ về booking (ví dụ: kiểm tra xung đột lịch) thay đổi trong `BookingService`, giao diện Console sẽ bị lỗi thời và cho phép đặt lịch sai quy tắc, làm hỏng tính toàn vẹn của dữ liệu tệp.
  - Cả hai giao diện đều phải chia sẻ chung một "Application Core" duy nhất.

#### Phương án B: Giữ lại một số Repository queries trong GUIBridge để "tối ưu hoá"
- **Mô tả:** Cho phép `GUIBridge` gọi trực tiếp các hàm tìm kiếm nhanh trên repository mà không qua Service.
- **Lý do loại bỏ:** Phá vỡ ranh giới của Kiến trúc Ba Lớp; tạo tiền lệ xấu cho việc rò rỉ phụ thuộc tầng.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Phân tách ranh giới triệt để (Separation of Concerns):** Presentation Layer hoàn toàn "mù" về tầng lưu trữ; chỉ biết đến các ca sử dụng (Use Cases) của Tầng Nghiệp Vụ.
- **Tính nhất quán nghiệp vụ tuyệt đối (Single Source of Truth):** Dù người dùng tương tác qua giao diện đồ họa Qt Quick hay giao diện dòng lệnh ANSI Terminal, các quy tắc bảo vệ lịch học, kiểm duyệt gia sư, tính rating và kiểm tra bảo mật đều được áp dụng đồng nhất 100%.
- **Bảo toàn 100% giao diện QML:** Không gây lỗi hồi quy (zero breakages) trên 28 file QML; người dùng cuối có trải nghiệm hoàn toàn mượt mà.
- **Khả năng kiểm thử độc lập:** Có thể dễ dàng viết Mock Services để kiểm thử riêng `GUIBridge` mà không cần chạm vào đĩa cứng.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Cần viết lại phần tương tác backend của `src/ui/ConsoleApp.cpp` (khoảng 24 vị trí gọi `dbEngine->...`).
- Hàm `initialize()` của `GUIBridge` và `ConsoleApp` nhận nhiều tham số service hơn (6 services).

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Khi chuyển đổi các hàm `qmlGet...` sang dùng Service, cấu trúc khoá của `QVariantMap` trả về bị sai khác một ký tự (ví dụ: `"hourlyRate"` thành `"rate"`), khiến QML không hiển thị được dữ liệu.
- **Biện pháp giảm thiểu:** Lập bảng kiểm toán đối chiếu chi tiết (Verification Contract) cho từng phương thức `Q_INVOKABLE` và đối soát với thuộc tính binding trong từng file QML trước khi hoàn tất Bước 10.

---

### 6. KẾT LUẬN
Việc chuẩn hoá `GUIBridge` và `ConsoleApp` thành các Presentation Adapters thuần túy là mảnh ghép hoàn thiện mô hình Kiến trúc Ba Lớp chuẩn mực, bảo đảm dự án TutorMatch đạt chuẩn cao nhất về thiết kế phần mềm hướng đối tượng.
