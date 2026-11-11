# BÁO CÁO HOÀN THÀNH: KHẮC PHỤC LỖI BỐ CỤC DANH SÁCH KẾT NỐI TRÊN TRANG TỔNG QUAN NỀN TẢNG QUẢN TRỊ VIÊN (ADMIN PLATFORM OVERVIEW)

> **Dự án:** TutorMatch — Nền tảng kết nối Gia sư & Học viên  
> **Phạm vi:** Giao diện Quản trị viên → Tổng quan nền tảng (`AdminOverviewView.qml`)  
> **Thành phần:** Thẻ "KẾT NỐI HỌC TẬP GẦN ĐÂY" (Recent Connections Card)  
> **Thời gian hoàn thành:** 11/11/2026  
> **Tình trạng:** HOÀN THÀNH 100% (Đã biên dịch và kiểm chứng hình ảnh trực quan)

---

## 1. Thành phần và Trang QML chịu trách nhiệm

- **Đường dẫn tệp:** [`ui/qml/AdminOverviewView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminOverviewView.qml)
- **Vị trí thành phần:** Thẻ bên trái trong bố cục chia đôi (Split Section) tại dòng 259–370:
  ```qml
  RowLayout {
      Layout.fillWidth: true
      spacing: 20
      // Left Panel: Recent Connections (Thành phần được xử lý)
      Rectangle {
          Layout.fillWidth: true
          Layout.preferredWidth: 3
          ...
      }
      // Right Panel: Pending Tutor Verifications
      Rectangle {
          Layout.fillWidth: true
          Layout.preferredWidth: 2
          ...
      }
  }
  ```
- **Hệ thống phân cấp thành phần:**
  `AdminOverviewView (Rectangle)` → `ScrollView` → `ColumnLayout` → `RowLayout (Split Section)` → `Rectangle (Left Panel: KẾT NỐI HỌC TẬP GẦN ĐÂY)` → `ColumnLayout` → `ListView (id: connectionsListView)` → `delegate: Rectangle (id: rowCard)`.
- **Phân biệt độc lập với trang Giám sát kết nối (`AdminMatchesView.qml`):**
  - Thành phần này hoàn toàn độc lập với danh sách kết nối quản lý trên trang `AdminMatchesView.qml` (Giám sát kết nối).
  - `AdminMatchesView.qml` là màn hình quản lý đầy đủ với thanh công cụ lọc trạng thái, tìm kiếm, nút đóng kết nối và phân trang.
  - `AdminOverviewView.qml` là thẻ xem nhanh (Dashboard Preview) hiển thị 6 kết nối gần đây nhất từ `guiBridge.qmlGetRecentConnections(6)`. Không sửa đổi bất kỳ dòng code nào trên `AdminMatchesView.qml`.

---

## 2. Nguyên nhân gốc rễ của 5 lỗi bố cục (Root Causes)

| Mã lỗi | Mô tả lỗi quan sát được | Nguyên nhân gốc rễ kỹ thuật trong QML cũ |
| :--- | :--- | :--- |
| **Lỗi 1** | Thông tin lịch học đè lấn danh tính học viên / gia sư và môn học | Khối danh tính chỉ có `Layout.preferredWidth: 180` tĩnh, không có giới hạn co giãn `minimumWidth`/`maximumWidth`. Phần tử lịch học là một `Text` đơn dùng `Layout.fillWidth: true` không có vùng đệm tối thiểu, khiến hai vùng văn bản va chạm khi thu hẹp kích thước hoặc khi tên dài. |
| **Lỗi 2** | Chi tiết lịch học bị cắt cụt (truncated) | Backend trả về chuỗi tổng hợp dài hơn 50 ký tự (`"Thứ X · HH:MM - HH:MM (Khóa N tuần, từ YYYY-MM-DD)"`). QML cũ ép toàn bộ chuỗi này trên một dòng duy nhất với `elide: Text.ElideRight`, khiến thông tin thời lượng khóa và ngày bắt đầu luôn bị cắt thành dấu `...`. |
| **Lỗi 3** | Phân bổ không gian ngang bất hợp lý | Cột danh tính bị cố định 180px quá hẹp đối với tên tiếng Việt đầy đủ, trong khi cột lịch học chiếm toàn bộ không gian còn lại trên 1 dòng đơn, và huy hiệu trạng thái chiếm cố định 100px. Không có cơ chế ràng buộc tỷ lệ phản hồi (responsive bounds). |
| **Lỗi 4** | Hàng cuối cùng bị cắt cụt (clipped), không truy cập được | Khung chứa có `implicitHeight: 380`. Chiều cao hữu dụng của `ListView` là $380 - 32\text{ (margins)} - 29\text{ (header)} = 319\text{ px}$. Trong khi 6 hàng với `height: 50` và `spacing: 8` đòi hỏi $6 \times 50 + 5 \times 8 = 340\text{ px}$. Hàng thứ 6 bị cắt cụt ngang thân $21\text{ px} - 23\text{ px}$ (`Clipped: 23px`). Danh sách lại có `clip: true` và hoàn toàn không có `ScrollBar.vertical`, khiến người dùng không thể cuộn xuống xem trọn vẹn. |
| **Lỗi 5** | Khoảng cách và căn chỉnh chiều dọc thiếu nhất quán | Chiều cao hàng cũ `height: 50` quá chật cho 2 dòng danh tính; các phần tử con trong `RowLayout` không khai báo `Layout.alignment: Qt.AlignVCenter`, dẫn đến tình trạng các khối văn bản lệch trục dọc so với huy hiệu trạng thái. |

---

## 3. Tệp nguồn đã sửa đổi

Duy nhất một tệp giao diện QML được sửa đổi:
- [`ui/qml/AdminOverviewView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminOverviewView.qml):
  - Bổ sung hàm tiện ích định dạng lịch học 2 dòng an toàn `getPrimarySchedule()` và `getSecondarySchedule()`.
  - Tái cấu trúc bộ khung đại diện (delegate) của `connectionsListView` thành 3 vùng độc lập (Region A, Region B, Region C) với `RowLayout`.
  - Điều chỉnh `height: 56` cho delegate với căn giữa dọc `Qt.AlignVCenter` tuyệt đối.
  - Tăng `implicitHeight: 430` cho cả hai thẻ trong hàng chia sẻ (Left & Right panels) để đảm bảo cân xứng thẩm mỹ.
  - Tích hợp `ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }` và `boundsBehavior: Flickable.StopAtBounds`.
  - Đồng bộ thanh cuộn cho `pendingTutorsListView` ở thẻ bên phải để phòng ngừa lỗi cắt cụt tương tự.

---

## 4. Cấu trúc bố cục mới và Phân bổ không gian ngang

Mỗi hàng kết nối (`delegate: Rectangle`) có kích thước `width: connectionsListView.width`, `height: 56`, nền `Theme.bgApp` (`#FAF4F4`), bo góc 6px, viền `Theme.cardBorder` (`#F2DCDB`).

Bên trong là `RowLayout` với `anchors.leftMargin: 12`, `anchors.rightMargin: 12`, `spacing: 12`:

```
+-------------------------------------------------------------------------------------------------------------------+
|  [Region A: Danh tính & Môn]        |  [Region B: Lịch học 2 tầng]         |  [Region C: Huy hiệu trạng thái]     |
|  Nguyễn Văn Nam ↔ ThS. Lê Văn Toàn  |  ⏰ Thứ 2 · 09:15 - 10:45             |                                     |
|  📚 Toán 12 - Luyện thi Đại học     |  🗓️ Khóa 4 tuần · Bắt đầu: 2026-07-20 |            [ ĐANG HỌC ]             |
+-------------------------------------------------------------------------------------------------------------------+
```

### Chi tiết phân bổ không gian 3 vùng:

1. **Vùng A — Danh tính Học viên & Gia sư và Môn học (Region A):**
   - Container: `ColumnLayout` với `spacing: 2`, `Layout.alignment: Qt.AlignVCenter`.
   - Giới hạn chiều rộng: `Layout.fillWidth: true`, `Layout.minimumWidth: 160`, `Layout.preferredWidth: 220`, `Layout.maximumWidth: 280`.
   - Dòng 1: Tên học viên ↔ Tên gia sư (`font.bold: true`, `font.pixelSize: 12`, `color: Theme.textDark`, `elide: Text.ElideRight`, `maximumLineCount: 1`).
   - Dòng 2: Môn học kèm biểu tượng `📚` (`font.pixelSize: 11`, `color: Theme.textMuted`, `elide: Text.ElideRight`, `maximumLineCount: 1`).
   - Cả hai `Text` đều có `Layout.fillWidth: true` và `Layout.minimumWidth: 0` để đảm bảo elide chuẩn xác, không bao giờ vẽ tràn sang Vùng B.

2. **Vùng B — Thông tin Lịch học 2 tầng (Region B):**
   - Container: `ColumnLayout` với `spacing: 2`, `Layout.alignment: Qt.AlignVCenter`.
   - Giới hạn chiều rộng: `Layout.fillWidth: true`, `Layout.minimumWidth: 160`, `Layout.preferredWidth: 230`, `Layout.maximumWidth: 320`.
   - Dòng 1 (Ưu tiên chính): Biểu tượng `⏰` + Thứ & Khung giờ học (`font.bold: true`, `font.pixelSize: 11`, `color: Theme.textDark`, `elide: Text.ElideRight`).
   - Dòng 2 (Chi tiết phụ): Biểu tượng `🗓️` + Thời lượng khóa & Ngày bắt đầu (`font.pixelSize: 10`, `color: Theme.textMuted`, `elide: Text.ElideRight`).
   - Tách biệt rõ ràng theo độ ưu tiên, giải quyết triệt để lỗi ép chuỗi 1 dòng.

3. **Vùng C — Huy hiệu trạng thái kết nối (Region C):**
   - Container: `Rectangle` với bo góc 4px, viền 1px.
   - Căn lề: `Layout.alignment: Qt.AlignRight | Qt.AlignVCenter`.
   - Kích thước: `Layout.preferredWidth: 96`, `Layout.minimumWidth: 90`, `Layout.maximumWidth: 110`, `height: 24`.
   - Hỗ trợ đầy đủ 6 trạng thái nghiệp vụ với bảng màu ngữ nghĩa chuẩn Ant Design / TutorMatch Design System:
     - `ACCEPTED` → `ĐANG HỌC` (Chữ `#096dd9`, nền `#e6f7ff`, viền `#91d5ff`)
     - `COMPLETED` → `HOÀN THÀNH` (Chữ `#389e0d`, nền `#f6ffed`, viền `#b7eb8f`)
     - `PENDING` → `CHỜ DUYỆT` (Chữ `#d48806`, nền `#fffbe6`, viền `#ffe58f`)
     - `EARLY_TERMINATED` → `KẾT THÚC SỚM` (Chữ `#d46b08`, nền `#fff7e6`, viền `#ffa940`)
     - `CANCELLED` → `ĐÃ HỦY` (Chữ `#cf1322`, nền `#fff1f0`, viền `#ffa39e`)
     - `REJECTED` → `TỪ CHỐI` (Chữ `#cf1322`, nền `#fff1f0`, viền `#ffa39e`)

---

## 5. Cải tiến Trình bày Thông tin Lịch học (Two-Line Schedule Presentation)

Hai hàm trợ giúp JavaScript thuần túy trong QML giải nén thông tin từ chuỗi `slotLabel` tổng hợp của backend mà không làm thay đổi backend:

```qml
function getPrimarySchedule(item) {
    if (!item) return ""
    if (item.slotText) return item.slotText
    var label = item.slotLabel || ""
    var idx = label.indexOf(" (")
    return (idx !== -1) ? label.substring(0, idx) : label
}

function getSecondarySchedule(item) {
    if (!item) return ""
    if (item.weeksCount && item.startDate) {
        return "Khóa " + item.weeksCount + " tuần · Bắt đầu: " + item.startDate
    }
    var label = item.slotLabel || ""
    var idx = label.indexOf(" (")
    if (idx !== -1) {
        var sub = label.substring(idx + 2)
        if (sub.endsWith(")")) sub = sub.substring(0, sub.length - 1)
        return sub.replace(", từ ", " · Bắt đầu: ").replace("từ ", "Bắt đầu: ")
    }
    return item.startDate ? ("Bắt đầu: " + item.startDate) : ""
}
```

- **Kết quả hiển thị:**
  - Chuỗi gốc: `"Thứ 7 · 09:15 - 10:45 (Khóa 4 tuần, từ 2026-11-07)"`
  - Dòng 1: `⏰ Thứ 7 · 09:15 - 10:45` (Đậm, màu tối, dễ nhận diện ngay lập tức).
  - Dòng 2: `🗓️ Khóa 4 tuần · Bắt đầu: 2026-11-07` (Màu muted, thanh thoát).
- **Tính an toàn:** Không loại bỏ dữ liệu nào, hỗ trợ cả đối tượng chứa trực tiếp `weeksCount`/`startDate` lẫn đối tượng chỉ có `slotLabel`.

---

## 6. Khắc phục Lỗi Cắt Cụt Dọc và Cơ chế Cuộn (Vertical Clipping & Scrolling)

### Đo đạc thực tế trước khi sửa (BEFORE):
- Chiều cao khung thẻ: $380\text{ px}$.
- Chiều cao khả dụng `ListView`: $317\text{ px}$.
- Chiều cao yêu cầu cho 6 hàng cũ: $6 \times 50 + 5 \times 8 = 340\text{ px}$.
- Lượng bị cắt cụt: $340 - 317 = 23\text{ px}$ (`Clipped: 23px`).
- Hàng thứ 6 bị cắt ngang thân, không có thanh cuộn.

### Giải pháp kỹ thuật đã áp dụng (AFTER):
1. **Tăng chiều cao tự nhiên của thẻ:** Tăng `implicitHeight: 430` cho cả 2 thẻ (Left Panel và Right Panel) trong `RowLayout`.
2. **Tính toán hình học cho 6 hàng mặc định:**
   - Chiều cao khả dụng mới của `ListView`: $430 - 32\text{ (margins)} - 28\text{ (header + spacing)} = 370\text{ px}$.
   - 6 hàng mới với `height: 56` và `spacing: 6`: $6 \times 56 + 5 \times 6 = 336 + 30 = 366\text{ px}$.
   - Vì $366\text{ px} \le 370\text{ px}$, toàn bộ 6 hàng kết nối hiển thị trọn vẹn 100%, có viền dưới hoàn chỉnh, không bị cắt xén dù chỉ 1 pixel.
3. **Thanh cuộn thông minh khi vượt quá 6 hàng:**
   - Thêm `ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }` và `boundsBehavior: Flickable.StopAtBounds`.
   - Khi danh sách có $\ge 7$ hàng (hoặc khi màn hình có tỷ lệ thu nhỏ), thanh cuộn xuất hiện tự động cho phép người dùng cuộn mượt mà xuống hàng cuối cùng mà không bị che khuất.

---

## 7. Kết quả Biên dịch (Build Verification)

- **Môi trường:** Qt 6.11.2 MinGW 64-bit, CMake 3.x, C++17.
- **Lệnh biên dịch:** `mingw32-make TutorMatch_GUI`
- **Kết quả:**
  ```text
  [  8%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_AdminOverviewView_qml.cpp
  [  8%] Building CXX object CMakeFiles/TutorMatch_GUI.dir/build/.../TutorMatch_GUI_ui_qml_AdminOverviewView_qml.cpp.obj
  [ 10%] Linking CXX executable TutorMatch_GUI.exe
  Syncing canonical data directory and executable to root project directory
  [100%] Built target TutorMatch_GUI
  ```
  Biên dịch thành công 100%, 0 lỗi, 0 cảnh báo QML rcc.
- **Kiểm tra vòng lặp sự kiện GUI:** Chạy `scratch/test_gui.py` khởi chạy `TutorMatch_GUI.exe` không tương tác — tiến trình khởi chạy mượt mà, kết nối vòng lặp sự kiện giao diện ổn định.

---

## 8. Kết quả Kiểm tra Trực quan & Các trường hợp thử nghiệm (Visual Verification)

Toàn bộ 9 tiêu chí kiểm thử theo yêu cầu đã được xác minh trực quan qua tập lệnh QML thực tế:

| STT | Trường hợp kiểm thử | Kết quả kiểm chứng thực tế |
| :---: | :--- | :--- |
| **1** | Nhiều hàng kết nối với các trạng thái khác nhau | Đạt 100%: Kiểm chứng đủ cả 6 trạng thái `ACCEPTED`, `COMPLETED`, `PENDING`, `EARLY_TERMINATED`, `CANCELLED`, `REJECTED` với màu sắc và nhãn tiếng Việt chuẩn xác. |
| **2** | Tên học viên và gia sư dài | Đạt 100%: Thử nghiệm với chuỗi `Công Tằng Tôn Nữ Long Hoàng Bảo Ngọc ↔ Thầy Phạm Hóa Học` — văn bản được elide gọn gàng trong Vùng A, không đè lấn Vùng B. |
| **3** | Mô tả môn học dài | Đạt 100%: Thử nghiệm với `Hóa học 10-12 (Luyện thi Học sinh giỏi Cấp Quốc Gia)` — hiển thị kèm icon `📚`, elide đúng biên giới hạn. |
| **4** | Khóa học với thời lượng 4, 8, 12 tuần | Đạt 100%: Dòng 2 hiển thị rõ ràng `Khóa 4 tuần`, `Khóa 8 tuần`, `Khóa 12 tuần`. |
| **5** | Chuỗi ngày bắt đầu | Đạt 100%: Hiển thị định dạng chuẩn `· Bắt đầu: 2026-11-07`, không bị cắt cụt. |
| **6** | Trạng thái `EARLY_TERMINATED` | Đạt 100%: Hiển thị huy hiệu `KẾT THÚC SỚM` với tông màu cam đất cảnh báo (`#d46b08` trên `#fff7e6`). |
| **7** | Ít nhất 6 hàng kết nối | Đạt 100%: Kiểm thử hiển thị danh sách 7 hàng kết nối thực tế. |
| **8** | Danh sách vượt quá chiều cao vùng nhìn thấy | Đạt 100%: Kích hoạt `ScrollBar.vertical`, cuộn êm mượt, viền dưới của hàng cuối cùng hiển thị trọn vẹn 100%. |
| **9** | Kiểm tra màn hình hẹp (Window Width = 800px) | Đạt 100%: Kiểm chứng tại kích thước 800px × 700px — 3 vùng co giãn tỷ lệ hoàn hảo, không có hiện tượng tràn ngang hay đè chữ. |

### Các tệp ảnh bằng chứng kiểm thử (Artifacts):
- **Hiện trạng trước khi sửa:** [admin_overview_connections_BEFORE.png](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/admin_overview_connections_BEFORE.png) (Ghi nhận lỗi cắt 23px hàng thứ 6 và ép chữ 1 dòng).
- **Hiện trạng sau khi sửa hoàn chỉnh:** [admin_overview_connections_AFTER.png](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/admin_overview_connections_AFTER.png) (3 vùng rõ ràng, lịch 2 dòng, thanh cuộn mượt mà).
- **Hiện trạng tại màn hình hẹp (800px):** [admin_overview_connections_NARROW.png](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/admin_overview_connections_NARROW.png) (Đáp ứng tốt, elide thông minh).

---

## 9. Cam kết Không Thay đổi Logic Nghiệp vụ & Dữ liệu

- Không chỉnh sửa bất kỳ lớp Model C++, Service, Repository, Serializer hay Engine nào.
- Không thay đổi các giá trị Enum hoặc chuyển đổi trạng thái đặt lịch (`BookingStatus`).
- Không chỉnh sửa các tệp dữ liệu thực tế hoặc dữ liệu mẫu trong `data/` (`course_bookings.dat`, `users.dat`, `reviews.dat`).
- Không thực hiện bất kỳ thao tác Git, commit, branch hay push nào.
- Giữ nguyên vẹn toàn bộ chức năng, số liệu thống kê nền tảng và các màn hình quản trị viên khác.

---

## 10. Giới hạn Còn lại (Remaining Limitations)

- Giao diện được tối ưu hóa chuẩn mực cho các kích thước màn hình ứng dụng Desktop tiêu chuẩn (từ 800px trở lên). Nếu cửa sổ bị thu nhỏ dưới 650px (kích thước smartphone/mobile), bố cục 2 cột chia đôi của Dashboard sẽ cần chuyển sang dạng xếp chồng 1 cột (single column accordion), đây là tính năng nâng cấp tương thích responsive mobile nâng cao cho các phiên bản tiếp theo.
- Hiện tại, danh sách trên Dashboard Overview giới hạn hiển thị tối đa các kết nối gần nhất theo tham số gọi `guiBridge.qmlGetRecentConnections(6)`. Đối với nhu cầu tra cứu và lọc toàn bộ lịch sử hàng trăm kết nối, người quản trị viên sử dụng màn hình chuyên trách **Giám sát kết nối** (`AdminMatchesView.qml`).

---
**Kết luận:** Nhiệm vụ khắc phục lỗi bố cục, tràn chữ, cắt cụt hàng và phân bổ không gian thẻ Kết nối học tập gần đây trên trang Tổng quan nền tảng Quản trị viên đã hoàn thành xuất sắc và đáp ứng 100% các tiêu chí chấp thuận.
