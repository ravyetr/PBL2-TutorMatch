# ARCHITECTURE DECISION RECORD (ADR)
## ADR-008: Điều Chỉnh Tối Thiểu và An Toàn Danh Sách Tệp Trong Hệ Thống Build CMake

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-008 (Supplementary to ADR-001)
- **Tiêu đề:** Điều Chỉnh Tối Thiểu và An Toàn Danh Sách Tệp Trong Hệ Thống Build CMake (Minimal Safe CMake Build Graph Adjustment)
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Build Systems Specialist
- **Phạm vi tác động:** `CMakeLists.txt`, các target `TutorMatch_GUI` và `TutorMatch_Tests`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong Quyết định Kiến trúc số 1 (ADR-001) về *Logical In-Place Layering*, một trong các nguyên tắc được nêu ra là: "Bảo toàn cấu trúc CMake target trừ khi có yêu cầu thiết kế cụ thể chứng minh việc thay đổi là bắt buộc". 

Qua kết quả thẩm định kỹ thuật độc lập (Finding **F-03**):
- Tệp `CMakeLists.txt` hiện tại quản lý mã nguồn thông qua các danh sách tệp tường minh:
  ```cmake
  set(CORE_SOURCES
      src/models/User.cpp
      src/models/SubjectTaxonomy.cpp
      src/ds/ScheduleGuard.cpp
      src/storage/ApplicationPaths.cpp
      src/storage/MockDatabaseEngine.cpp
      src/storage/DataFileInspector.cpp
      src/algorithms/MatchingEngine.cpp
      src/services/AuthService.cpp
      src/services/UserService.cpp
      src/services/BookingService.cpp
      src/services/AdminService.cpp
      src/gui/GUIBridge.cpp
  )
  ```
- CMake **hoàn toàn không sử dụng cơ chế wildcard/globbing (`file(GLOB ...)`)**.
- Quá trình tái cấu trúc sang Kiến trúc 3 Lớp sinh ra các lớp chuyên biệt mới cần thiết cho SRP và tính module hóa:
  - 3 Serializers: `UserSerializer.cpp`, `BookingSerializer.cpp`, `ReviewSerializer.cpp`.
  - 3 Concrete Repositories: `FileUserRepository.cpp`, `FileBookingRepository.cpp`, `FileReviewRepository.cpp`.
  - 2 Services trích xuất: `ReviewService.cpp`, `MatchingService.cpp`.

Nếu giữ nguyên tuyệt đối 100% không chỉnh sửa từng ký tự của `CMakeLists.txt`, các tệp `.cpp` mới này sẽ không bao giờ được trình biên dịch (MinGW GCC/Clang) biên dịch, dẫn đến lỗi Unresolved External Symbol (Linker Error LNK2019) tại giai đoạn liên kết.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Cho phép điều chỉnh tối thiểu, có kiểm soát trên `CMakeLists.txt` (Minimal Safe Adjustment):**
   - **Bảo toàn 100% cấu trúc các Target:**
     - `TutorMatch_GUI` (Desktop Application executable).
     - `TutorMatch_Tests` (Unit & Integration test runner executable).
     - `TutorMatch_Inspector` (Standalone diagnostic tool executable).
   - **Bảo toàn 100% cấu hình Qt QML Module:** Giữ nguyên hàm `qt_add_qml_module(TutorMatch_GUI URI TutorMatch VERSION 1.0 QML_FILES ${QML_FILES})`.
   - **Bảo toàn 100% các cờ biên dịch và thư viện liên kết:** C++17 standard, `Qt6::Core`, `Qt6::Gui`, `Qt6::Qml`, `Qt6::Quick`.
2. **Phạm vi thay đổi duy nhất được phép:**
   Chỉ bổ sung tên các tệp mã nguồn mới vào danh sách `CORE_SOURCES` và `HEADERS`:
   - Bổ sung vào `CORE_SOURCES`:
     ```cmake
     src/storage/UserSerializer.cpp
     src/storage/BookingSerializer.cpp
     src/storage/ReviewSerializer.cpp
     src/storage/FileUserRepository.cpp
     src/storage/FileBookingRepository.cpp
     src/storage/FileReviewRepository.cpp
     src/services/ReviewService.cpp
     src/services/MatchingService.cpp
     ```
   - Bổ sung vào `HEADERS`:
     ```cmake
     include/storage/UserSerializer.h
     include/storage/BookingSerializer.h
     include/storage/ReviewSerializer.h
     include/storage/FileUserRepository.h
     include/storage/FileBookingRepository.h
     include/storage/FileReviewRepository.h
     include/services/ReviewService.h
     include/services/MatchingService.h
     ```
   - Bổ sung các tệp trên vào danh sách mã nguồn của target `TutorMatch_Tests`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Dồn toàn bộ mã mới vào các file cũ để tránh sửa CMake
- **Mô tả:** Viết mã của `UserSerializer` vào `User.cpp`; viết mã của cả 3 Concrete Repositories vào `MockDatabaseEngine.cpp`; viết mã của `ReviewService` vào `BookingService.cpp`.
- **Lý do loại bỏ:** Hoàn toàn phản tác dụng về mặt kiến trúc; tạo ra các "tệp quái vật" (God Files) với hàng nghìn dòng lệnh, vi phạm nghiêm trọng mục tiêu phân rã trách nhiệm và tính trong sáng của Kiến trúc 3 Lớp.

#### Phương án B: Chuyển đổi toàn bộ CMakeLists.txt sang dùng `file(GLOB_RECURSE)`
- **Mô tả:** Thay thế danh sách tệp tường minh bằng lệnh tự động quét thư mục `file(GLOB_RECURSE SOURCES "src/*.cpp")`.
- **Lý do loại bỏ:** Vi phạm khuyến nghị chính thức của tổ chức CMake. Lệnh GLOB không tự động nhận biết khi có tệp mới được thêm vào hoặc đổi tên mà không chạy lại `cmake`, dễ gây lỗi build ngầm (silent build failures). Liệt kê tường minh là chuẩn mực công nghiệp cao nhất trong C++.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Biên dịch sạch sẽ (Clean Compilation):** Toàn bộ các lớp mới được biên dịch đầy đủ vào cả ứng dụng chính (`TutorMatch_GUI`) và bộ kiểm thử (`TutorMatch_Tests`).
- **Minh bạch và có thể kiểm toán (Auditability):** Mọi tệp nguồn mới đều được khai báo rõ ràng trong hệ thống build, dễ dàng theo dõi qua Git diff.
- **Rủi ro bằng không (Zero Risk):** Không làm ảnh hưởng đến bất kỳ cấu hình build, link thư viện hay QML module nào hiện có.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Cần thực hiện chỉnh sửa nội dung danh sách file trong `CMakeLists.txt` tại Bước 2 và Bước 3 của lộ trình chuyển đổi Phase 2.

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Gõ sai đường dẫn tệp trong `CMakeLists.txt` làm dừng quá trình tạo Makefiles.
- **Biện pháp giảm thiểu:** Kiểm tra cú pháp CMake bằng lệnh `cmake -B build` ngay sau khi cập nhật danh sách tệp trước khi tiến hành viết mã bên trong.

---

### 6. KẾT LUẬN
Quyết định điều chỉnh tối thiểu danh sách tệp trong `CMakeLists.txt` theo **ADR-008** là sự chuẩn hoá kỹ thuật bắt buộc và đúng đắn, tháo gỡ rào cản thực thi cho Phase 2.
