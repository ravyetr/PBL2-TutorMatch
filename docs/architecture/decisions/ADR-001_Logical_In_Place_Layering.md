# ARCHITECTURE DECISION RECORD (ADR)
## ADR-001: Áp Dụng Phân Tách Kiến Trúc Logic Tại Chỗ (Logical In-Place Layering)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-001
- **Tiêu đề:** Áp dụng Phân Tách Kiến Trúc Logic Tại Chỗ (Logical In-Place Layering)
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Qt/QML Specialist
- **Phạm vi tác động:** Cấu trúc tổ chức thư mục dự án, hệ thống build CMake, module Qt QML và ranh giới kiến trúc.

---

### 1. BỐI CẢNH (CONTEXT)
Dự án TutorMatch là một ứng dụng máy tính C++17 / Qt 6.11 kết nối Gia sư và Học viên. Cấu trúc thư mục mã nguồn hiện tại được tổ chức theo các nhóm thành phần:
- `include/gui/`, `src/gui/`: Chứa mã nguồn giao diện đồ họa và lớp cầu nối (`GUIBridge`).
- `include/ui/`, `src/ui/`: Chứa giao diện dòng lệnh (`ConsoleApp`).
- `include/services/`, `src/services/`: Chứa các dịch vụ xử lý nghiệp vụ (`AuthService`, `UserService`, `BookingService`, `AdminService`).
- `include/storage/`, `src/storage/`: Chứa các thành phần lưu trữ (`MockDatabaseEngine`, `ApplicationPaths`, `DataFileInspector`, `IRepository.h`).
- `include/models/`, `src/models/`: Chứa các thực thể miền (`User`, `Student`, `Tutor`, `Admin`, `Booking`, `Review`, `SubjectTaxonomy`).
- `include/ds/`, `src/ds/`: Chứa các cấu trúc dữ liệu và giải thuật học thuật tự cài đặt (`HashTable`, `LRUCache`, `AVLTree`, `MaxHeap`, `DoublyLinkedList`, `ScheduleGuard`).
- `ui/qml/`: Chứa 28 tệp giao diện người dùng QML được đăng ký dưới dạng một Qt QML Module với URI `TutorMatch`.

Khi tiến hành tái cấu trúc sang Kiến trúc Ba Lớp (Three-Layer Architecture), một quyết định cơ bản cần đưa ra là: Liệu có nên di chuyển toàn bộ các tệp vật lý vào các thư mục mới mang tên `presentation/`, `business_logic/`, `data_access/` hay giữ nguyên cấu trúc thư mục hiện hành và thiết lập ranh giới phân lớp logic?

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định áp dụng phương pháp **Phân Tách Kiến Trúc Logic Tại Chỗ (Logical In-Place Layering)**:
1. **Giữ nguyên cấu trúc thư mục vật lý hiện hữu:**
   - Không di chuyển hàng loạt (mass moving) các tệp tin sang cấu trúc thư mục vật lý mới.
   - Các tệp mới phát sinh trong quá trình tái cấu trúc sẽ được phân bổ vào đúng thư mục nhóm tương ứng đã có:
     - Repositories cụ thể (`FileUserRepository`, `FileBookingRepository`, `FileReviewRepository`) và Serializers (`UserSerializer`, `BookingSerializer`, `ReviewSerializer`) được đặt trong `include/storage/` và `src/storage/`.
     - Services mới (`ReviewService`, `MatchingService`) được đặt trong `include/services/` và `src/services/`.
2. **Bảo toàn 100% cấu hình hệ thống Build và Qt Module:**
   - Bảo toàn cấu hình CMake hiện tại: `CORE_SOURCES`, `GUI_SOURCES`, `HEADERS`, `QML_FILES`.
   - Bảo toàn việc đăng ký `qt_add_qml_module(TutorMatch_GUI URI TutorMatch VERSION 1.0 QML_FILES ${QML_FILES})`.
3. **Thiết lập ranh giới phân lớp thông qua hợp đồng mã nguồn (Code Contracts & Interfaces):**
   - Ranh giới 3 lớp được thực thi nghiêm ngặt thông qua:
     - Định nghĩa trách nhiệm của từng lớp (Class Responsibilities).
     - Hợp đồng giao diện trừu tượng (`IRepository`).
     - Chiều phụ thuộc đơn hướng: Presentation $\rightarrow$ Business Logic $\rightarrow$ Data Access.
     - Cấm tuyệt đối việc include chéo tầng vi phạm quy tắc (ví dụ: `GUIBridge.h` không được include `IRepository.h` hoặc bất kỳ repository concrete nào).

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Tái cấu trúc vật lý triệt để (Physical Mass Relocation)
- **Mô tả:** Tạo 3 thư mục gốc mới `presentation/`, `business/`, `data_access/`, và di dời toàn bộ hơn 60 tệp mã nguồn C++, headers và 28 tệp QML vào các thư mục này.
- **Lý do loại bỏ:**
  - Gây xung đột nghiêm trọng với lịch sử Git (git blame/log bị phân mảnh trên toàn bộ dự án).
  - Phá vỡ cấu hình QML module của CMake, đòi hỏi phải viết lại toàn bộ đường dẫn import và alias trong 28 tệp QML, tiềm ẩn nguy cơ cao gây lỗi trắng màn hình runtime.
  - Phá vỡ toàn bộ cấu hình include path trong CMakeLists.txt và 16 bộ test suites trong `tests/TestRunner.cpp`.
  - Không mang lại thêm giá trị kỹ thuật thực chất nào về mặt phân tách phụ thuộc so với phân tách logic.

#### Phương án B: Giữ nguyên hiện trạng không áp dụng ràng buộc (Status Quo)
- **Mô tả:** Giữ nguyên cấu trúc thư mục và tiếp tục cho phép `GUIBridge` phụ thuộc trực tiếp vào tầng lưu trữ.
- **Lý do loại bỏ:** Vi phạm trực tiếp mục tiêu của nhiệm vụ tái cấu trúc 3 lớp; không giải quyết được các vi phạm kiến trúc nghiêm trọng (`VIO-01` đến `VIO-07`).

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Độ ổn định tối đa:** Không làm vỡ cấu hình CMake, không làm vỡ cơ chế tìm kiếm module của Qt Quick/QML.
- **Duy trì lịch sử Git:** Giữ nguyên lịch sử sửa đổi của các tệp nguồn cốt lõi.
- **Tập trung vào chất lượng kiến trúc:** Nguồn lực kỹ sư được tập trung 100% vào việc chuẩn hoá trách nhiệm lớp, tách biệt phụ thuộc và đảm bảo tính đúng đắn của nghiệp vụ.
- **Dễ dàng điều hướng mã nguồn:** Cấu trúc `include/` và `src/` phân theo vai trò kỹ thuật (`models`, `storage`, `services`, `ds`, `gui`) đã rất trực quan và quen thuộc với các nhà phát triển C++.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Lập trình viên mới tiếp cận dự án có thể cần đọc tài liệu kiến trúc để hiểu rõ ranh giới logic của 3 lớp thay vì chỉ nhìn vào tên thư mục vật lý.
- Cần có quy trình kiểm tra (code review hoặc linters) để ngăn ngừa việc lập trình viên vô tình thêm các `#include` vi phạm tầng.

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Lập trình viên tiếp tục include `storage/` trong `gui/` do cùng nằm trong cấu trúc thư mục quen thuộc.
- **Biện pháp giảm thiểu:** Kiểm toán danh sách `#include` trong toàn bộ các tệp thuộc `src/gui/` và `include/gui/` tại Bước 14 của lộ trình chuyển đổi; đảm bảo không có bất kỳ header nào của DAL xuất hiện trong Presentation Layer.

---

### 6. KẾT LUẬN
Quyết định áp dụng **Logical In-Place Layering** là phương án tối ưu, đáp ứng trọn vẹn cả tiêu chí an toàn hệ thống, bảo toàn môi trường build và đạt được tính trong sáng tuyệt đối của Kiến trúc Ba Lớp.
