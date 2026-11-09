# ARCHITECTURE DECISION RECORD (ADR)
## ADR-005: Quyền Sở Hữu và Cơ Chế Lưu Trữ Tỷ Lệ Hoa Hồng Nền Tảng (Commission State Ownership & Persistence)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-005
- **Tiêu đề:** Quyền Sở Hữu và Cơ Chế Lưu Trữ Tỷ Lệ Hoa Hồng Nền Tảng (Commission State Ownership & Persistence)
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Financial Domain Specialist
- **Phạm vi tác động:** `include/services/AdminService.h`, `src/services/AdminService.cpp`, `include/gui/GUIBridge.h`, `src/gui/GUIBridge.cpp`, `include/ui/ConsoleApp.h`, và `src/ui/ConsoleApp.cpp`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong kiến trúc hiện hành của TutorMatch:
- Biến tỷ lệ hoa hồng nền tảng (`platformCommissionRate`) đang được khai báo là thành viên dữ liệu của `GUIBridge` (`include/gui/GUIBridge.h`, dòng 53):
  ```cpp
  double platformCommissionRate;
  ```
- Biến này được khởi tạo mặc định bằng `0.25` (25%) trong hàm dựng của `GUIBridge` (`src/gui/GUIBridge.cpp`, dòng 43, 48).
- Khi Quản trị viên cập nhật tỷ lệ hoa hồng qua giao diện QML (`AdminCommissionConfigView.qml`), hàm `setPlatformCommissionRate()` chỉ thay đổi biến nội bộ trong RAM của `GUIBridge`.
- Khi tính toán chỉ số thống kê, `GUIBridge` phải truyền biến này dưới dạng tham số vào tầng nghiệp vụ: `adminService->calculateMetrics(platformCommissionRate)`.
- Tương tự, `ConsoleApp` (`include/ui/ConsoleApp.h`, dòng 75) cũng tự khai báo một biến `double platformCommissionRate;` riêng biệt trong RAM của giao diện dòng lệnh.

**Vấn đề kiến trúc:**
- Rò rỉ quyền sở hữu quy tắc kinh doanh (Business Configuration State) ra Presentation Layer.
- Tầng nghiệp vụ (`AdminService`) bị động, không tự quản lý được thông số tài chính của chính mình mà phải chờ giao diện "bơm" vào khi tính toán.
- Trùng lặp trạng thái giữa GUI và Console.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Chuyển quyền sở hữu nghiệp vụ vào `AdminService`:**
   - Di dời hoàn toàn biến `platformCommissionRate` khỏi `GUIBridge` và `ConsoleApp`, đưa vào làm thành viên dữ liệu `private` bên trong `AdminService` (`include/services/AdminService.h`):
     ```cpp
     class AdminService {
     private:
         double platformCommissionRate = 0.25; // 25% default
         mutable std::mutex adminMutex;
         // ...
     ```
   - Cung cấp các phương thức quản lý trạng thái an toàn đa luồng trên `AdminService`:
     ```cpp
     double getPlatformCommissionRate() const;
     void setPlatformCommissionRate(double newRate);
     ```
   - Phương thức tính toán chỉ số tài chính `AdminService::calculateMetrics()` không còn bắt buộc nhận tham số ngoài, mà tự động sử dụng biến thành viên nội tại `this->platformCommissionRate` để tính toán doanh thu hoa hồng:
     $$\text{platformCommissionRevenue} = \text{initialConnectionGMV} \times \text{platformCommissionRate}$$
2. **Duy trì nguyên tắc lưu trữ trong bộ nhớ RAM (RAM-Only Persistence):**
   - Tuân thủ nghiêm ngặt Quyết định Kiến trúc số 4 đã được duyệt: **Không thay đổi cơ chế lưu trữ bền vững trong giai đoạn này**.
   - Biến `platformCommissionRate` tiếp tục được lưu trữ trong bộ nhớ RAM. Khi ứng dụng tắt hoặc khởi động lại, giá trị sẽ quay trở về mặc định là 25% (0.25).
   - Tuyệt đối không tự ý tạo thêm bất kỳ tệp cấu hình mới nào (`.ini`, `.json`, `.dat`) ngoài phạm vi được phê duyệt.
3. **`GUIBridge` và `ConsoleApp` trở thành các bộ điều phối mỏng:**
   - Các phương thức `qmlGetCommissionRate()` và `qmlSetCommissionRate()` trong `GUIBridge` chỉ đơn thuần uỷ quyền (delegate) sang gọi `adminService->getPlatformCommissionRate()` và `adminService->setPlatformCommissionRate()`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Tự động lưu tỷ lệ hoa hồng vào file cấu hình mới
- **Mô tả:** Tạo một file `data/platform_config.dat` hoặc `config.json` để lưu giá trị hoa hồng xuống đĩa, giúp giá trị này tồn tại qua các lần khởi động lại.
- **Lý do loại bỏ:** Vi phạm ràng buộc của đồ án: "Không thay đổi schema dữ liệu và không tự ý sinh tệp lưu trữ mới khi chưa có phê duyệt kiến trúc riêng biệt". Hành vi này cần được tách thành một đề xuất nâng cấp tính năng riêng ở giai đoạn sau.

#### Phương án B: Giữ nguyên biến trong GUIBridge
- **Mô tả:** Tiếp tục để `GUIBridge` sở hữu biến hoa hồng và truyền vào `AdminService::calculateMetrics()`.
- **Lý do loại bỏ:** Vi phạm trực tiếp ranh giới của Kiến trúc Ba Lớp (`VIO-05`), biến lớp giao diện thành nơi lưu trữ dữ liệu nghiệp vụ tài chính.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Đúng đắn về mặt miền nghiệp vụ (Domain Purity):** Toàn bộ chính sách tài chính và tính toán doanh thu được đóng gói trọn vẹn trong `AdminService` thuộc Business Logic Layer.
- **Đồng bộ tuyệt đối giữa GUI và CLI:** Bất kể quản trị viên thay đổi tỷ lệ hoa hồng qua giao diện QML hay dòng lệnh Console, giá trị được cập nhật trực tiếp vào duy nhất một thực thể `AdminService` trong bộ nhớ ứng dụng.
- **Tuân thủ nguyên tắc an toàn đa luồng:** Việc truy xuất và cập nhật giá trị được bảo vệ bởi mutex trong `AdminService`.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Giá trị cấu hình hoa hồng sẽ bị khôi phục về 25% sau mỗi lần tắt ứng dụng (hành vi được xác nhận là đồng nhất với hiện trạng ban đầu).

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Một số đoạn mã kiểm thử cũ hoặc lời gọi hàm trong `TestRunner.cpp` đang truyền tham số vào `calculateMetrics(rate)`.
- **Biện pháp giảm thiểu:** Trong `AdminService`, cung cấp thêm một hàm nạp chồng (overload) `calculateMetrics(double overrideRate)` giữ nguyên chữ ký cũ để tương thích 100% với các test hiện tại.

---

### 6. KẾT LUẬN
Quyết định đưa quyền sở hữu `platformCommissionRate` vào `AdminService` và giữ nguyên tính chất lưu trữ RAM giải quyết triệt để vi phạm `VIO-05`, đồng thời đảm bảo an toàn tuyệt đối cho hệ thống dữ liệu tệp tin.
