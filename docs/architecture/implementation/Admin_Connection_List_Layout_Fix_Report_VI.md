# BÁO CÁO KỸ THUẬT: KHẮC PHỤC LỖI BỐ CỤC, TRÀN CHỮ VÀ PHÂN TRANG DANH SÁCH KẾT NỐI QUẢN TRỊ VIÊN (ADMIN CONNECTION LIST LAYOUT & OVERFLOW FIX REPORT)

---

## 1. TỔNG QUAN VÀ MỤC TIÊU (OVERVIEW & OBJECTIVES)

- **Dự án:** TutorMatch (Hệ thống kết nối gia sư và học viên)
- **Công nghệ áp dụng:** C++17, Qt 6.11.2, Qt Quick / QML, MinGW 13.1.0 64-bit
- **Tập tin báo cáo:** [`Admin_Connection_List_Layout_Fix_Report_VI.md`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/docs/architecture/implementation/Admin_Connection_List_Layout_Fix_Report_VI.md)
- **Tập tin giao diện chính được điều chỉnh:** [`ui/qml/AdminMatchesView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminMatchesView.qml)
- **Tập tin phụ trợ đồng bộ hóa trạng thái:** [`ui/qml/AdminOverviewView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminOverviewView.qml), [`ui/qml/UserDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/UserDetailModal.qml)
- **Mục tiêu chính:** Khắc phục triệt để 5 khuyết tật bố cục trên màn hình Quản trị viên (Tab *"Giám sát Kết nối"*):
  1. Thông tin lịch học (thời gian ca học) đè lấn hoặc va chạm với vùng định danh học viên/gia sư và tên môn học.
  2. Chi tiết lịch trình và thời lượng khóa học bị cắt xén (truncate/elide) sai vị trí.
  3. Không gian hàng ngang phân bổ bất hợp lý giữa 3 phần: Thông tin học viên/gia sư, Lịch học, và Huy hiệu trạng thái / Thao tác.
  4. Đáy danh sách kết nối bị cắt xén (clipping) khi số lượng dòng vượt quá chiều cao vùng nhìn thấy.
  5. Khoảng cách (spacing) và gióng hàng dọc (vertical alignment) không đồng nhất giữa các dòng liền kề.

---

## 2. PHÂN TÍCH NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE FORENSIC ANALYSIS)

Thông qua việc đo đạc hình học thời gian thực bằng công cụ kiểm định tọa độ pixel QML, chúng tôi đã xác định chính xác các nguyên nhân cốt lõi:

### 2.1. Vấn đề 1: Trôi lệch tọa độ ngang và xung đột giữa Vùng A và Vùng B
Trong [`AdminMatchesView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminMatchesView.qml) cũ:
```qml
// Participant Info (Student <-> Tutor & Subject)
ColumnLayout {
    Layout.preferredWidth: 240
    spacing: 2
    Text { text: modelData.studentName + " ↔ " + modelData.tutorName; elide: Text.ElideRight }
    Text { text: "Môn học: " + modelData.subject + " · " + ... }
    Text { text: "Ngày bắt đầu: " + modelData.startDate + " (" + modelData.weeksCount + " tuần) · Tạo ngày: " + modelData.createdAt }
}

// Schedule Time Slot
ColumnLayout {
    Layout.fillWidth: true
    spacing: 2
    Text { text: "Lịch học cố định:" }
    Text { text: modelData.slotText || modelData.slotLabel || ""; elide: Text.ElideRight }
}
```
- **Xung đột phân cấp thông tin:** Dòng 3 của Vùng A cố gắng nhồi nhét cả ngày bắt đầu, số tuần khóa học và ngày tạo đơn vào một độ rộng chật hẹp (`preferredWidth: 240`). Trong khi đó, Vùng B lại chỉ hiển thị nhãn *"Lịch học cố định:"* và chuỗi ca học ngắn.
- **Trôi dạt tọa độ (Column Wandering):** Kết quả đo đạc pixel cho thấy Vùng A và Vùng B có tọa độ $x$ và chiều rộng $w$ nhảy múa liên tục giữa các dòng liền kề:
  - Dòng 0: Vùng A bắt đầu tại $x = 74\text{ px}$, rộng $277\text{ px}$; Vùng B tại $x = 507\text{ px}$.
  - Dòng 1: Vùng A bắt đầu tại $x = 65\text{ px}$, rộng $393\text{ px}$; Vùng B tại $x = 577\text{ px}$.
  - Dòng 5: Vùng A bắt đầu tại $x = 87\text{ px}$, rộng $295\text{ px}$; Vùng B tại $x = 637\text{ px}$.
  Cột Lịch học (Vùng B) bị xê dịch tới **$130\text{ px}$** giữa các dòng, hoàn toàn phá vỡ cấu trúc gióng thẳng hàng của bảng quản trị.
- **Đè chữ ở cửa sổ thu nhỏ:** Khi chiều rộng cửa sổ giảm xuống $800\text{ px}$, Vùng A có tên dài (rộng $346\text{ px}$) cộng với nút đóng kết nối ($100\text{ px}$) đã ép Vùng B co cụm còn dưới $100\text{ px}$, dẫn tới việc chữ lịch học bị cắt cụt thành `Thứ 4...` hoặc tràn sang vùng bên cạnh.

### 2.2. Vấn đề 2: Huy hiệu trạng thái và nút thao tác xô lệch vị trí
- Ở trạng thái `ACCEPTED` ("ĐANG HỌC"), nút `"Đóng kết nối"` hiển thị (`visible: true`), chiếm thêm $100\text{ px} + 14\text{ px}$ khoảng cách.
- Ở các trạng thái khác (`COMPLETED`, `CANCELLED`, `PENDING`, `EARLY_TERMINATED`), nút bị ẩn (`visible: false`).
- Do Vùng C không có khung chứa cố định kích thước (`preferredWidth`) mà nằm tự do trong `RowLayout` cùng với Vùng B (`Layout.fillWidth: true`), việc nút ẩn/hiện làm toàn bộ huy hiệu trạng thái bị giật cục:
  - Dòng 0 (`ACCEPTED`): Huy hiệu nằm tại $x = 697\text{ px}$.
  - Dòng 2 (`EARLY_TERMINATED`): Huy hiệu nằm tại $x = 787\text{ px}$.
  - Dòng 5 (`REJECTED`): Huy hiệu nhảy tít sang $x = 862\text{ px}$.
  Huy hiệu trạng thái trôi dạt một khoảng cách lên tới **$165\text{ px}$**, làm mất tính nhất quán thị giác.

### 2.3. Vấn đề 3: Cắt cụt đáy danh sách do lồng ghép container cuộn sai lệch
- Tại `AdminMatchesView.qml`:
  - `ScrollView` bao bọc bên ngoài.
  - Bên trong `ScrollView`, component `ListView` được gán:
    `implicitHeight: Math.max(300, count * 86)` kết hợp với `clip: true`.
- **Sai số toán học:** Mỗi thẻ dòng có chiều cao $80\text{ px}$ và khoảng cách `spacing: 10\text{ px}`, tức mỗi phần tử chiếm đúng $90\text{ px}$ theo trục dọc.
  Công thức cũ sử dụng hệ số $86\text{ px}$ dẫn tới việc `implicitHeight` bị hụt $4\text{ px}$ cho mỗi dòng.
  - Với 7 dòng: Chiều cao thực là $7 \times 90 - 10 = 620\text{ px}$, nhưng `implicitHeight` báo về $7 \times 86 = 602\text{ px}$ (hụt $18\text{ px}$).
  - Với 15 dòng: Chiều cao thực là $1340\text{ px}$, nhưng báo về $1290\text{ px}$ (hụt **$50\text{ px}$**).
- **Xung đột cuộn lồng nhau (Nested Scrolling Collision):** `ListView` là một container cuộn độc lập lồng trong `ScrollView`. Khi chuột rê vào vùng danh sách, `ListView` cướp sự kiện lăn chuột nhưng lại bị giới hạn chiều cao ảo, làm dòng cuối cùng bị cắt xén một nửa và không thể cuộn xem trọn vẹn.

---

## 3. THIẾT KẾ BỐ CỤC KIẾN TRÚC MỚI (NEW ARCHITECTURAL LAYOUT)

Bố cục mỗi dòng kết nối được tái cấu trúc thành 3 phân vùng độc lập tuyệt đối nằm trong một `RowLayout` chuẩn mực:

```
+---------------------------------------------------------------------------------------------------------+
| [40px Icon] | [Region A: 320px]             | [Region B: 260px]          | [Region C: 220px (Right)]    |
|             |                               |                            |                              |
|   🔗 / ✓    | Student ↔ Tutor Name (Bold)   | ⏰ Thứ 7 · 09:15 - 10:45   | [KẾT THÚC SỚM] [Đóng kết nối]|
|    / ⛔     | 📚 Môn học · Học phí          | 🗓️ Khóa 4 tuần · Bắt đầu   |           (Huy hiệu căn phải)|
|             | 📅 Ngày tạo đơn               |   2026-07-20               |                              |
+---------------------------------------------------------------------------------------------------------+
```

### 3.1. Phân bổ không gian hàng ngang (Horizontal Space Allocation)

| Phân vùng | Vai trò nội dung | Ràng buộc kích thước Qt Quick Layouts | Cơ chế chống tràn |
| :--- | :--- | :--- | :--- |
| **Icon** | Chỉ thị biểu tượng trạng thái trực quan | `Layout.preferredWidth: 40`, `Layout.preferredHeight: 40`, `radius: 20` | Cố định, căn giữa theo trục dọc |
| **Vùng A** | Định danh Học viên ↔ Gia sư & Môn học | `Layout.fillWidth: true`, `Layout.minimumWidth: 230`, `Layout.preferredWidth: 320`, `Layout.maximumWidth: 420` | Giới hạn tối đa $420\text{ px}$, tất cả các dòng chữ đều có `elide: Text.ElideRight` |
| **Vùng B** | Thông tin Lịch học & Khóa học đa tầng | `Layout.fillWidth: true`, `Layout.minimumWidth: 200`, `Layout.preferredWidth: 260` | Dòng 1 (Thứ & giờ ca học), Dòng 2 (Số tuần & ngày bắt đầu), tách biệt hoàn toàn |
| **Vùng C** | Huy hiệu trạng thái & Nút tác vụ | `Layout.preferredWidth: 220`, `Layout.minimumWidth: 120`, `Layout.alignment: Qt.AlignRight` | Chứa `Item { Layout.fillWidth: true }` đàn hồi, giữ mọi huy hiệu thẳng tắp ở mép phải |

### 3.2. Cấu trúc trình bày lịch học phân tầng (Two-Line Schedule Presentation)
- **Dòng 1 (Thông tin chính - Primary):**  
  `⏰` + Ca học lịch sử: `Thứ 2 · 09:15 - 10:45` (Chữ đậm `font.bold: true`, màu `Theme.textDark`, cỡ 12px).
- **Dòng 2 (Thông tin thứ cấp - Secondary):**  
  `🗓️` + Thời lượng & Ngày khai giảng: `Khóa 4 tuần · Bắt đầu: 2026-07-20` (Màu `Theme.textMuted`, cỡ 11px).
- Cả hai dòng đều có biểu tượng nhận diện riêng, tạo nhịp điệu thị giác rõ ràng và giải phóng Vùng A khỏi gánh nặng hiển thị ngày tháng.

### 3.3. Chuẩn hóa huy hiệu trạng thái (Status Badge Standardization)
Hỗ trợ đầy đủ và đồng bộ toàn bộ 6 trạng thái nghiệp vụ của TutorMatch:
- `ACCEPTED`: "ĐANG HỌC" (Nền xanh nhạt `#e6f7ff`, viền `#91d5ff`, chữ xanh dương `#096dd9`).
- `COMPLETED`: "HOÀN THÀNH" (Nền xanh lá nhạt `#f6ffed`, viền `#b7eb8f`, chữ xanh lá `#389e0d`).
- `PENDING`: "CHỜ DUYỆT" (Nền vàng nhạt `#fffbe6`, viền `#ffe58f`, chữ cam đậm `#d48806`).
- `EARLY_TERMINATED`: "KẾT THÚC SỚM" (Nền cam cảnh báo `#fff7e6`, viền `#ffa940`, chữ cam đất `#d46b08`). Độ rộng huy hiệu tự co giãn `Math.max(92, statusLabel.implicitWidth + 18)` đảm bảo không bị cụt chữ.
- `CANCELLED`: "ĐÃ HỦY" (Nền đỏ nhạt `#fff1f0`, viền `#ffa39e`, chữ đỏ `#cf1322`).
- `REJECTED`: "TỪ CHỐI" (Nền đỏ nhạt `#fff1f0`, viền `#ffa39e`, chữ đỏ `#cf1322`).

### 3.4. Loại bỏ hoàn toàn lỗi cắt xén đáy danh sách (Zero-Clipping Scrolling)
- Thay thế hoàn toàn cơ chế `ListView` lồng ghép thủ công bằng mô hình chuẩn của Qt Quick:  
  `ColumnLayout` + `Repeater` đặt trực tiếp dưới `ScrollView`.
- **Lợi ích kiến trúc:**
  - `ColumnLayout` tự động tính toán tổng chiều cao chính xác từng pixel:
    $$\text{implicitHeight}_{\text{total}} = \sum (\text{implicitHeight}_{\text{row}}) + (\text{count} - 1) \times \text{spacing}$$
  - `ScrollView` quản lý sự kiện cuộn mượt mà duy nhất từ đầu trang tới cuối trang.
  - Không còn hiện tượng xung đột chuột lăn và loại bỏ hoàn toàn việc dòng cuối cùng bị che lấp.

---

## 4. CHI TIẾT CÁC TẬP TIN ĐÃ ĐƯỢC CHỈNH SỬA (EXACT FILES MODIFIED)

### 1. [`ui/qml/AdminMatchesView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminMatchesView.qml)
- **Bộ lọc nghiệp vụ (`filteredMatches`):** Cập nhật tab *"Đã hủy"* (`activeFilterIndex === 2`) để hiển thị toàn diện các trạng thái kết thúc khóa học gồm `CANCELLED`, `REJECTED` và `EARLY_TERMINATED`.
- **Cấu trúc danh sách:** Chuyển đổi từ `ListView` sang `ColumnLayout { id: matchesRowsCol; spacing: 10 }` + `Repeater`.
- **Bố cục dòng kết nối (`rowCard`):**
  - Chiều cao dòng chuẩn hóa: `implicitHeight: 84`, `clip: true`.
  - Thiết lập `RowLayout` với `anchors.margins: 16` và `spacing: 16`.
  - Triển khai Vùng A, Vùng B và Vùng C theo đúng tỷ lệ phân bổ kích thước đã thiết kế.
  - Trình bày lịch học 2 tầng riêng biệt với icon `⏰` và `🗓️`.
  - Cố định Vùng C với huy hiệu trạng thái tự co giãn và nút `"Đóng kết nối"` nằm gọn gàng bên phải.
  - Nâng cấp khung hiển thị trạng thái rỗng (Empty State) với thông báo thân thiện.

### 2. [`ui/qml/AdminOverviewView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminOverviewView.qml)
- Bổ sung định dạng màu sắc và nhãn hiển thị `"KẾT THÚC SỚM"` cho trạng thái `EARLY_TERMINATED` tại widget *"KẾT NỐI HỌC TẬP GẦN ĐÂY"*.

### 3. [`ui/qml/UserDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/UserDetailModal.qml)
- Đồng bộ hóa huy hiệu `"KẾT THÚC SỚM"` cho trạng thái `EARLY_TERMINATED` tại Tab *"Lịch sử kết nối"*.

---

## 5. KẾT QUẢ XÂY DỰNG (BUILD RESULTS)

- **Môi trường:** MinGW 13.1.0 64-bit, Qt 6.11.2, CMake, `mingw32-make`
- **Mục tiêu xây dựng:** `TutorMatch_GUI`
- **Nhật ký biên dịch:**
  ```
  [  8%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_AdminMatchesView_qml.cpp, .aotstats
  [ 10%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_AdminOverviewView_qml.cpp, .aotstats
  [ 12%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_UserDetailModal_qml.cpp, .aotstats
  [ 14%] Building CXX object .../TutorMatch_GUI_ui_qml_AdminMatchesView_qml.cpp.obj
  [ 14%] Building CXX object .../TutorMatch_GUI_ui_qml_AdminOverviewView_qml.cpp.obj
  [ 14%] Building CXX object .../TutorMatch_GUI_ui_qml_UserDetailModal_qml.cpp.obj
  [ 16%] Linking CXX executable TutorMatch_GUI.exe
  Syncing canonical data directory and executable to root project directory
  [100%] Built target TutorMatch_GUI
  ```
- **Trạng thái:** Thành công 100% (Mã thoát 0).

---

## 6. KẾT QUẢ KIỂM ĐỊNH THỊ GIÁC VÀ ĐO ĐẠC HÌNH HỌC (VISUAL VERIFICATION & GEOMETRY AUDIT)

### 6.1. Đo đạc tọa độ pixel thực tế trên ma trận 10 kịch bản biên
Bộ công cụ kiểm định tự động `scratch/verify_admin_matches.qml` đã chạy và đo đạc tọa độ của 10 dòng kết nối (vượt quá chiều cao khung nhìn):

| Dòng | Kịch bản kiểm định dữ liệu | Tọa độ Vùng A | Tọa độ Vùng B | Tọa độ Vùng C | Đánh giá gióng hàng & Chống tràn |
| :---: | :--- | :---: | :---: | :---: | :---: |
| **1** | Khóa học `ACCEPTED` chuẩn (*Nguyễn Văn Nam ↔ ThS. Lê Văn Toàn*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Zero overlap, thẳng hàng tuyệt đối) |
| **2** | Tên học viên & gia sư cực dài (*Công Tằng Tôn Nữ... ↔ GS. TS. Nguyễn Đình...*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Tên rút gọn `...`, không lấn Vùng B) |
| **3** | Trạng thái `EARLY_TERMINATED`, thời lượng 12 tuần (*Lương Thị Quỳnh*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Huy hiệu cam `"KẾT THÚC SỚM"` vừa vặn) |
| **4** | Khóa học `COMPLETED` chuẩn 4 tuần (*Phan Đức Huy ↔ Mr. David Lê*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Gióng hàng thẳng tắp) |
| **5** | Đơn đặt lịch `PENDING` chờ duyệt (*Trần Minh Anh ↔ Cô Nguyễn Thị Sinh*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Gióng hàng thẳng tắp) |
| **6** | Đơn đặt lịch `REJECTED` bị từ chối (*Đoàn Nhật Khoa*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Gióng hàng thẳng tắp) |
| **7** | Đơn đặt lịch `CANCELLED` đã hủy (*Lê Quang Vinh*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Gióng hàng thẳng tắp) |
| **8** | Khóa học 8 tuần, chuỗi ngày bắt đầu dài (*Trần Khánh Vy*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Lịch 2 dòng hiển thị rõ ràng) |
| **9** | Mô tả lịch học nhiều ca trong tuần (*Thái Bình An*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Hiển thị trọn vẹn, không va chạm) |
| **10**| Dòng vượt quá chiều cao màn hình (*Đỗ Tuấn Kiệt*) | $x=56, w=345$ | $x=417, w=281$ | $x=714, w=237$ | **PASS** (Nằm trọn trong luồng cuộn) |

- **Hiện tượng trôi lệch cột:** Triệt tiêu hoàn toàn. Cả 10 dòng đều có Vùng A bắt đầu tại $x=56\text{ px}$, Vùng B tại $x=417\text{ px}$ và Vùng C tại $x=714\text{ px}$.
- **Hiện tượng đè lấn:** $0\text{ trường hợp}$ (Zero horizontal overlap).

### 6.2. Kiểm định khả năng cuộn và chống cắt xén đáy (Scrolling & Accessibility Audit)
- Chiều cao container hàng (`rowsCol.implicitHeight`): **$930\text{ px}$**
- Chiều cao nội dung khả dụng (`ScrollView.contentHeight`): **$968\text{ px}$** (bao gồm khoảng đệm lề trên và lề dưới).
- Tọa độ đáy của dòng thứ 10: $y_{\text{bottom}} = 846 + 84 = \mathbf{930\text{ px}}$.
- Do $968\text{ px} \ge 930\text{ px}$, người dùng có thể cuộn chuột xem trọn vẹn $100\%$ dòng cuối cùng với đầy đủ lề đệm $20\text{ px}$ dưới đáy. Hoàn toàn không còn hiện tượng mất dòng hay cắt xén pixel.

### 6.3. Kiểm định đáp ứng ở cửa sổ thu nhỏ (Responsive Viewport Audit at 800px)
Tại kích thước cửa sổ hẹp $800\text{ px}$ ($W_{\text{row}} = 743\text{ px}$):
- Vùng A: $x=56\text{ px}$, rộng $256\text{ px}$.
- Vùng B: $x=328\text{ px}$, rộng $220\text{ px}$.
- Vùng C: $x=564\text{ px}$, rộng $147\text{ px}$.
- Cả 3 vùng duy trì khoảng cách an toàn, không có bất kỳ ký tự nào bị vẽ tràn ra ngoài ranh giới thẻ, và huy hiệu trạng thái vẫn được giữ cố định ở mép phải.

### 6.4. Bằng chứng thị giác thời gian thực (Rendered Visual Screenshots)
Các ảnh chụp màn hình render thực tế đã được lưu trữ tại thư mục artifacts:
- **Trước khi sửa (BEFORE):**  
  [`admin_connection_list_BEFORE.png`](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/admin_connection_list_BEFORE.png)  
  *Ghi nhận: Cột lịch học xô lệch giữa các dòng, chữ ngày tháng nhồi nhét ở Vùng A bị cắt xén, huy hiệu trạng thái nhảy loạn xạ và đáy danh sách bị hụt pixel.*
- **Sau khi sửa (AFTER / VERIFIED):**  
  [`admin_connection_list_AFTER.png`](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/admin_connection_list_AFTER.png)  
  *Ghi nhận: 3 phân vùng độc lập gióng thẳng hàng tuyệt đối, thông tin lịch học 2 tầng thanh thoát, huy hiệu trạng thái căn phải đều đặn, và toàn bộ 10 dòng có thể cuộn xem trọn vẹn.*

---

## 7. BẢO TOÀN KIẾN TRÚC VÀ HỒI QUY (REGRESSION ASSURANCE)

- **Không can thiệp logic backend:** Giữ nguyên toàn bộ các phương thức C++ (`qmlGetAllMatches`, `qmlAdminTerminateMatch`, `BookingService`).
- **Không thay đổi cấu trúc dữ liệu:** Các thuộc tính model (`modelData.id`, `studentName`, `tutorName`, `slotText`, `weeksCount`, `status`) được kết nối chính xác và đầy đủ.
- **Không chỉnh sửa dữ liệu mẫu:** Các tập tin `data/*.dat` giữ nguyên trạng.
- **Tuân thủ quy tắc quản lý mã nguồn:** Không khởi tạo Git, không tạo commit và không can thiệp GitHub từ xa.

---

## 8. KẾT LUẬN (CONCLUSION)

Toàn bộ 5 khuyết tật trên danh sách kết nối Quản trị viên đã được giải quyết dứt điểm:
1. Thông tin lịch học và định danh học viên/gia sư tách biệt rành mạch, không còn bất kỳ sự va chạm nào.
2. Lịch học được cấu trúc phân tầng 2 dòng với đầy đủ thứ, giờ, thời lượng và ngày bắt đầu.
3. Không gian hàng ngang phân bổ khoa học, các cột gióng thẳng hàng hoàn hảo từ dòng đầu đến dòng cuối.
4. Danh sách cuộn mượt mà không giới hạn, đáy danh sách không bị cắt xén.
5. Mã nguồn giao diện sạch sẽ, tuân thủ chặt chẽ tiêu chuẩn Qt Quick Layouts.
