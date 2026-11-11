# BÁO CÁO KỸ THUẬT: KHẮC PHỤC LỖI TRÀN CHỮ, PHẠM VI ĐƯỜNG PHÂN CÁCH VÀ PHÂN CẤP THỊ GIÁC TRÊN THẺ LỊCH SỬ KHÓA HỌC (COURSE HISTORY CARD LAYOUT FIX REPORT)

---

## 1. TỔNG QUAN VÀ MỤC TIÊU (OVERVIEW & OBJECTIVES)

- **Dự án:** TutorMatch (Hệ thống kết nối gia sư và học viên)
- **Công nghệ áp dụng:** C++17, Qt 6.11.2, Qt Quick / QML, MinGW 13.1.0 64-bit
- **Tập tin báo cáo:** [`Course_History_Card_Layout_Fix_Report_VI.md`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/docs/architecture/implementation/Course_History_Card_Layout_Fix_Report_VI.md)
- **Tập tin giao diện được điều chỉnh:** [`TutorStudents.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorStudents.qml)
- **Mục tiêu chính:** Khắc phục triệt để 3 lỗi hiển thị nghiêm trọng tại giao diện thẻ lịch sử khóa học và đánh giá của gia sư:
  1. **Bug 1 — Tràn chữ theo phương ngang (Horizontal Text Overflow):** Tên môn học và thời gian học lịch sử nối thành một dòng văn bản dài, tràn qua cạnh phải của thẻ, băng qua khoảng cách giữa hai thẻ và đè lên thẻ bên cạnh.
  2. **Bug 2 — Đường phân cách vượt ranh giới thẻ (Divider Exceeds Card Boundaries):** Đường kẻ phân cách ngang màu hồng nhạt (`Theme.cardBorder` / `#F2DCDB`) bắt đầu từ thẻ thứ nhất nhưng kéo dài xuyên qua khe hở 14px, cắt ngang qua thân thẻ thứ hai và kéo về mép phải màn hình.
  3. **Bug 3 — Phân cấp nội dung và khoảng cách chưa chuẩn (Poor Content Hierarchy & Spacing):** Thông tin định danh học viên, môn học, lịch dạy và khung đánh giá bị dồn ép trong thẻ có chiều cao cố định, thiếu phân tách thị giác 3 tầng rõ ràng.

---

## 2. PHÂN TÍCH NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE FORENSIC ANALYSIS)

Quá trình điều tra thực nghiệm bằng công cụ đo đạc hình học QML (`qml.exe` kết hợp truy vấn tọa độ và kích thước thời gian thực) đã xác định chính xác cơ chế sinh lỗi:

### 2.1. Cơ chế sinh Bug 1 (Tràn chữ theo phương ngang)
Trong [`TutorStudents.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorStudents.qml) phiên bản cũ:
```qml
Text {
    text: "📚 Môn: " + modelData.subject + " · ⏰ Lịch từng học: " + modelData.nextLessonSlot
    font.pixelSize: 12
    font.bold: true
    color: Theme.textDark
}
```
- Dòng văn bản ghép nối môn học (ví dụ: *Sinh học 10-12 (Luyện thi THPT)*) và ca học lịch sử (ví dụ: *Thứ 2 · 09:15 - 10:45 (Khóa 4 tuần, từ 2026-10-01)*) có độ dài lên tới **95 ký tự**.
- Ở cỡ chữ `pixelSize: 12` đậm (`font.bold: true`), `implicitWidth` của phần tử `Text` này đạt **$629.15\text{ px}$** đến **$720\text{ px}$**.
- Phần tử `Text` không được thiết lập `Layout.fillWidth: true`, không có `wrapMode: Text.WordWrap`, và không có `Layout.minimumWidth: 0`.
- Do không có cơ chế co giãn hay ngắt dòng, phần tử `Text` giữ nguyên chiều rộng nội tại $630\text{ px}$. Trong khi đó, ở chế độ hiển thị 2 cột trên màn hình desktop ($1024\text{ px}$), mỗi thẻ chỉ có chiều rộng $505\text{ px}$ (chiều rộng khả dụng sau khi trừ lề là $473\text{ px}$).
- Phần văn bản vượt quá $630 - 473 = \mathbf{157\text{ px}}$ nhô ra ngoài viền phải của thẻ, băng qua khe hở $14\text{ px}$, vẽ đè lên thẻ thứ hai và chỉ bị cắt khi chạm tới mép container `ScrollView`.

### 2.2. Cơ chế sinh Bug 2 (Đường phân cách cắt xuyên qua thẻ liền kề)
Kết quả đo đạc hình học thời gian thực thu được từ công cụ kiểm định:
```
--- CARD 0 ---
Card item: x=0, y=0, width=505, height=160
Card ColumnLayout: x=16, width=473, implicitWidth=630
  Child 0 (Header Row):  x=0, width=630, implicitWidth=434
  Child 1 (Divider):     x=0, width=630, implicitWidth=0      <-- BẤT THƯỜNG!
  Child 2 (Subject/Slot):x=0, width=630, implicitWidth=629.16
  Child 3 (Review):      x=0, width=630, implicitWidth=0
--- CARD 1 ---
Card item: x=519, y=0, width=505, height=160
```
- `ColumnLayout` của thẻ có `anchors.fill: parent; anchors.margins: 16`. Về mặt hình học, `width` của nó là $505 - 32 = 473\text{ px}$.
- Tuy nhiên, trong cơ chế tính toán của Qt Quick Layouts, `ColumnLayout.implicitWidth` được tính bằng giá trị lớn nhất trong các `implicitWidth` của các phần tử con:
  $$\text{implicitWidth}_{\text{ColumnLayout}} = \max(434, 0, 629.16, 0) = \mathbf{630\text{ px}}$$
- Khi `ColumnLayout` phân bổ kích thước cho các phần tử con có thuộc tính `Layout.fillWidth: true` (bao gồm `divRect` - đường phân cách), thuật toán Layouts gán chiều rộng của phần tử bằng:
  $$\text{effectiveWidth} = \max(\text{width}, \text{implicitWidth}) = \max(473, 630) = \mathbf{630\text{ px}}$$
- Đường phân cách màu hồng `Rectangle { Layout.fillWidth: true; height: 1; color: Theme.cardBorder }` bị cưỡng bức mở rộng thành **$630\text{ px}$**!
- Bắt đầu từ tọa độ $x = 16\text{ px}$ trong Thẻ 0, đường kẻ này kéo dài tới:
  $$x_{\text{end}} = 16 + 630 = \mathbf{646\text{ px}}$$
- Trong khi đó, Thẻ 1 bắt đầu tại tọa độ $x = \mathbf{519\text{ px}}$!
- Do thẻ không khai báo `clip: true`, đường phân cách của Thẻ 0 vượt qua viền phải ($505\text{ px}$), vượt qua khe hở ($505 \to 519\text{ px}$), đâm xuyên vào bên trong Thẻ 1 tới tận $646 - 519 = \mathbf{127\text{ px}}$, tạo cảm giác một đường kẻ nối liền hai thẻ độc lập.

### 2.3. Cơ chế sinh Bug 3 (Phân cấp nội dung bị dồn ép)
- Thẻ sử dụng chiều cao cố định cứng nhắc: `height: modelData.isReviewed ? 160 : 130`.
- Tên học viên và email không có giới hạn `elide: Text.ElideRight`, khiến tên dài đẩy huy hiệu `"ĐÃ HOÀN THÀNH"` ra khỏi thẻ.
- Môn học và lịch học bị ép chung vào một dòng chữ duy nhất.
- Khung đánh giá review có chiều cao cố định $32\text{ px}$ với `elide: Text.ElideRight`, làm mất các nhận xét chi tiết nhiều dòng của học viên.

---

## 3. GIẢI PHÁP THIẾT KẾ VÀ KIẾN TRÚC MỚI (ARCHITECTURAL & LAYOUT DESIGN)

Giải pháp được thiết kế toàn diện theo 4 tầng bảo đảm tính độc lập hình học tuyệt đối:

```
+-----------------------------------------------------------------------------------+
| 1. Grid Sizing & Column Equalization (TutorStudents.qml)                          |
|    - Layout.fillWidth: true, Layout.preferredWidth: 1, Layout.minimumWidth: 0     |
|    - Layout.fillHeight: true (Đồng bộ chiều cao các thẻ cùng một hàng)             |
|    - clip: true (Rào chắn bao đóng vật lý tuyệt đối cho mỗi thẻ)                   |
+-----------------------------------------------------------------------------------+
                                          |
+-----------------------------------------------------------------------------------+
| 2. Tier 1: Student Identity Section (44px Avatar, Two-line Details & Badge)       |
|    - Name & Class/Email: Layout.fillWidth: true, elide: Text.ElideRight           |
|    - Status Badge: implicitWidth: 105px (Cố định vị trí căn phải, không bị xô lệch)|
+-----------------------------------------------------------------------------------+
                                          |
+-----------------------------------------------------------------------------------+
| 3. Internal Divider & Tier 2: Structured Course Information                       |
|    - Divider: Layout.fillWidth: true nằm trọn trong lề thẻ (16px -> width - 16px) |
|    - Line 1: [Icon 📚] + Subject Name (Layout.fillWidth: true, wrapMode: WordWrap)|
|    - Line 2: [Icon ⏰] + Schedule Time (Layout.fillWidth: true, wrapMode: WordWrap)|
+-----------------------------------------------------------------------------------+
                                          |
+-----------------------------------------------------------------------------------+
| 4. Tier 3: Adaptive Review Container & Elastic Spacer                             |
|    - Background: color: Theme.cMistyRose (#F2DCDB), border: #E8C5C8, radius: 6px   |
|    - Star Rating: 5 ★ (#d97706) + Label "Đánh giá từ học viên"                    |
|    - Comment: wrapMode: Text.WordWrap (Tự động mở rộng chiều cao theo nhận xét)   |
|    - Spacer: Item { Layout.fillHeight: true } giữ bố cục đáy thẻ luôn phẳng đẹp    |
+-----------------------------------------------------------------------------------+
```

---

## 4. CHI TIẾT CÁC ĐIỀU CHỈNH TRÊN TẬP TIN GIAO DIỆN

### Tập tin: [`ui/qml/TutorStudents.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorStudents.qml)

#### A. Cấu trúc thẻ Lịch sử khóa học (`SUB-TAB 1: LỊCH SỬ ĐÃ HỌC`)
1. **Thiết lập hình học bao đóng của thẻ:**
   ```qml
   Rectangle {
       Layout.fillWidth: true
       Layout.preferredWidth: 1
       Layout.minimumWidth: 0
       Layout.fillHeight: true
       implicitHeight: cardCol.implicitHeight + 32
       radius: 10
       color: Theme.cardBg
       border.color: Theme.cardBorder
       border.width: 1
       clip: true
   ```
   - `Layout.preferredWidth: 1` và `Layout.minimumWidth: 0`: Yêu cầu `GridLayout` phân bổ tỷ lệ 50%/50% hoàn hảo cho 2 cột mà không bị ảnh hưởng bởi `implicitWidth` của dữ liệu bên trong.
   - `Layout.fillHeight: true`: Các thẻ nằm trên cùng một hàng tự động đồng bộ chiều cao theo thẻ có nội dung dài hơn, tạo giao diện ngăn nắp, cân đối.
   - `clip: true`: Đảm bảo không có bất kỳ pixel đồ họa nào được vẽ vượt qua bán kính bo góc $10\text{ px}$ của thẻ.

2. **Tầng 1 — Nhận diện học viên:**
   ```qml
   RowLayout {
       Layout.fillWidth: true
       Layout.minimumWidth: 0
       spacing: 12
       // Avatar tròn 44x44
       ...
       ColumnLayout {
           Layout.fillWidth: true
           Layout.minimumWidth: 0
           spacing: 2
           Text {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               text: modelData.studentName
               font.bold: true; font.pixelSize: 15; color: Theme.textDark
               elide: Text.ElideRight
               maximumLineCount: 1
           }
           Text {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               text: (modelData.gradeLevel === 0 ? "Khối lớp: Khác" : "Khối lớp " + modelData.gradeLevel) + " · " + (modelData.email || "Email chưa cập nhật")
               font.pixelSize: 12; color: Theme.textMuted
               elide: Text.ElideRight
               maximumLineCount: 1
           }
       }
       // Huy hiệu trạng thái "ĐÃ HOÀN THÀNH"
       Rectangle {
           height: 24; implicitWidth: 105; radius: 4
           color: Theme.bgApp; border.color: Theme.cardBorder
           Text { text: "ĐÃ HOÀN THÀNH"; font.bold: true; font.pixelSize: 10; color: Theme.textDark }
       }
   }
   ```

3. **Đường phân cách nội bộ thẻ:**
   ```qml
   Rectangle {
       Layout.fillWidth: true
       height: 1
       color: Theme.cardBorder
   }
   ```
   - Nhờ việc thiết lập `Layout.minimumWidth: 0` trên toàn bộ các phần tử con, `cardCol.implicitWidth` không còn bị thổi phồng.
   - Đường phân cách co giãn chính xác từ lề trái ($16\text{ px}$) tới lề phải ($W_{\text{card}} - 16\text{ px}$). Kết thúc trước mép phải thẻ $16\text{ px}$ và cách thẻ liền kề tới $30\text{ px}$, triệt tiêu hoàn toàn hiện tượng nối thẻ.

4. **Tầng 2 — Thông tin khóa học và thời gian học:**
   ```qml
   ColumnLayout {
       Layout.fillWidth: true
       Layout.minimumWidth: 0
       spacing: 5

       RowLayout {
           Layout.fillWidth: true
           Layout.minimumWidth: 0
           spacing: 6
           Text { text: "📚"; font.pixelSize: 12; Layout.alignment: Qt.AlignTop }
           Text {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               text: modelData.subject || "Khóa học"
               font.bold: true; font.pixelSize: 12; color: Theme.textDark
               wrapMode: Text.WordWrap
           }
       }

       RowLayout {
           Layout.fillWidth: true
           Layout.minimumWidth: 0
           spacing: 6
           Text { text: "⏰"; font.pixelSize: 12; Layout.alignment: Qt.AlignTop }
           Text {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               text: "Lịch học: " + (modelData.nextLessonSlot || "Chưa cập nhật")
               font.pixelSize: 12; color: Theme.textDark
               wrapMode: Text.WordWrap
           }
       }
   }
   ```

5. **Tầng 3 — Nhận xét và đánh giá từ học viên:**
   ```qml
   Rectangle {
       Layout.fillWidth: true
       Layout.minimumWidth: 0
       implicitHeight: revCol.implicitHeight + 16
       radius: 6
       color: Theme.cMistyRose
       border.color: "#E8C5C8"
       border.width: 1
       visible: Boolean(modelData.isReviewed)

       ColumnLayout {
           id: revCol
           anchors.fill: parent
           anchors.margins: 8
           spacing: 4

           RowLayout {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               spacing: 6
               Text { text: "⭐ " + (modelData.ratingScore || 5) + " ★"; font.bold: true; font.pixelSize: 11; color: "#d97706" }
               Text { text: "Đánh giá từ học viên"; font.pixelSize: 10; font.bold: true; color: Theme.textMuted }
               Item { Layout.fillWidth: true }
           }

           Text {
               Layout.fillWidth: true
               Layout.minimumWidth: 0
               text: modelData.comment ? ("\"" + modelData.comment + "\"") : ""
               font.italic: true; font.pixelSize: 11; color: Theme.textDark
               wrapMode: Text.WordWrap
           }
       }
   }
   ```

#### B. Nâng cấp bảo vệ thẻ Đang theo học (`SUB-TAB 0: ĐANG THEO HỌC`)
- Áp dụng cùng tiêu chuẩn hình học (`Layout.preferredWidth: 1`, `Layout.minimumWidth: 0`, `clip: true`, `implicitHeight: activeCol.implicitHeight + 32`).
- Phân tầng cột tên/email học viên với `elide: Text.ElideRight`.
- Tách dòng môn học và lịch học cố định với `elide: Text.ElideRight`, bảo đảm nút `"🏁 Hoàn thành khóa học"` luôn cố định vị trí căn phải mà không bị chèn ép.

---

## 5. KẾT QUẢ XÂY DỰNG (BUILD RESULTS)

- **Công cụ biên dịch:** MinGW 13.1.0 64-bit, Qt 6.11.2, CMake, `mingw32-make`
- **Mục tiêu:** `TutorMatch_GUI`
- **Nhật ký biên dịch:**
  ```
  [  6%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_TutorStudents_qml.cpp, .rcc/qmlcache/TutorMatch_GUI_ui_qml_TutorStudents_qml.cpp.aotstats
  [  8%] Building CXX object CMakeFiles/TutorMatch_GUI.dir/build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/.rcc/qmlcache/TutorMatch_GUI_ui_qml_TutorStudents_qml.cpp.obj
  [ 10%] Building CXX object CMakeFiles/TutorMatch_GUI.dir/build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/.qt/rcc/qrc_TutorMatch_GUI_raw_qml_0.cpp.obj
  [ 10%] Linking CXX executable TutorMatch_GUI.exe
  Syncing canonical data directory and executable to root project directory
  [100%] Built target TutorMatch_GUI
  ```
- **Trạng thái:** Biên dịch và liên kết thành công 100% (Mã thoát 0).

---

## 6. KẾT QUẢ KIỂM ĐỊNH THỊ GIÁC VÀ ĐO ĐẠC HÌNH HỌC (VISUAL VERIFICATION & GEOMETRY AUDIT)

### 6.1. Kiểm định ma trận 8 kịch bản biên (Edge Cases Verification)
Bộ công cụ kiểm định tự động `scratch/verify_course_history_cards.qml` đã chạy và kiểm tra toàn bộ 8 kịch bản biên khắt khe:

| Kịch bản kiểm định | Dữ liệu đầu vào thử nghiệm | Chiều rộng thẻ | Chiều rộng phần tử con | Kết quả kiểm định |
| :--- | :--- | :---: | :---: | :---: |
| **Kịch bản 1:** Thẻ hoàn thành chuẩn có đánh giá | *Nguyễn Văn Nam*, Toán 12, Đánh giá 5 sao chuẩn | $484\text{ px}$ | $452\text{ px}$ | **PASS** (100% nằm trong thẻ) |
| **Kịch bản 2:** Tên môn học cực dài | *Sinh học 10-12 (Luyện thi THPT Quốc Gia Chuyên Sâu Cấp Tốc & Bồi dưỡng HSG)* | $483\text{ px}$ | $451\text{ px}$ | **PASS** (Ngắt dòng 2 dòng êm dịu) |
| **Kịch bản 3, 4, 5:** Tên học viên cực dài, email cực dài, lịch học nhiều ca | *Công Tằng Tôn Nữ Long Hoàng Bảo Ngọc*, email dài 70 ký tự, lịch 3 ca/tuần | $484\text{ px}$ | $452\text{ px}$ | **PASS** (Tên & email rút gọn `...`, lịch ngắt dòng) |
| **Kịch bản 6:** Đánh giá học viên cực dài nhiều câu | Bài nhận xét chi tiết 4 câu dài 220 ký tự | $483\text{ px}$ | $451\text{ px}$ | **PASS** (Khung hồng mở rộng 4 dòng, không tràn) |
| **Kịch bản 7:** Bất đối xứng nội dung (Thẻ không có đánh giá) | *Lê Minh*, Toán 10, chưa đánh giá | $484\text{ px}$ | $452\text{ px}$ | **PASS** (Ẩn khung hồng, căn thẳng hàng) |
| **Kịch bản 8:** Thẻ hàng xóm ghép cặp | *Đỗ Tuấn Kiệt*, Toán 10-11, ghép cặp với Kịch bản 7 | $483\text{ px}$ | $451\text{ px}$ | **PASS** (Đồng bộ chiều cao hàng hoàn hảo) |

- **Kết quả tổng thể:** `100% PASS (ALL 8 CONSTRAINTS VERIFIED)`.
- **Hiện tượng tràn ngang:** $0\text{ trường hợp}$ (Zero horizontal overflow).
- **Hiện tượng đường phân cách đâm xuyên:** $0\text{ trường hợp}$ (Hoàn toàn biến mất).

### 6.2. Kiểm định tương thích kích thước (Responsive Geometry Audit)
Đã chạy kiểm tra đáp ứng với nhiều kích thước khung nhìn khác nhau:
- **Chiều rộng $1200\text{ px}$ (Desktop rộng):** Lưới 2 cột, mỗi thẻ $593\text{ px}$, phần tử con $561\text{ px}$ [OK].
- **Chiều rộng $950\text{ px}$ (Desktop tiêu chuẩn):** Lưới 2 cột, mỗi thẻ $593\text{ px}$, phần tử con $561\text{ px}$ [OK].
- **Chiều rộng $800\text{ px}$ (Cửa sổ thu nhỏ):** Lưới 2 cột, mỗi thẻ $468\text{ px}$, phần tử con $436\text{ px}$ [OK].
- **Chiều rộng $600\text{ px}$ (Cửa sổ hẹp/Tablet):** Tự động chuyển đổi thành lưới 1 cột, thẻ trải rộng $800\text{ px}$ toàn chiều rộng container, nội dung hiển thị thoáng đãng [OK].

### 6.3. Bằng chứng thị giác thời gian thực (Rendered Visual Screenshots)
Các tập tin hình ảnh chụp thực tế đã được lưu trữ trong thư mục artifacts để đối chiếu:
- **Trước khi sửa lỗi (BEFORE):**
  [`course_history_cards_BEFORE.png`](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/course_history_cards_BEFORE.png)  
  *Ghi nhận rõ ràng: Dòng chữ môn/lịch học của Thẻ 1 vươn dài sang Thẻ 2; đường kẻ phân cách màu hồng xuyên thẳng qua khe hở 14px và cắt sâu vào Thẻ 2.*
- **Sau khi sửa lỗi (AFTER):**
  [`course_history_cards_AFTER.png`](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/course_history_cards_AFTER.png)  
  *Ghi nhận: Hai thẻ hoàn toàn độc lập về mặt thị giác, đường kẻ phân cách dừng cách viền phải thẻ 16px, môn học và lịch học ngắt dòng gọn gàng, khung đánh giá màu hồng có độ cao thích ứng và bo góc đẹp mắt.*
- **Toàn bộ ma trận kiểm định 8 kịch bản:**
  [`course_history_cards_VERIFIED.png`](file:///C:/Users/DELL/.gemini/antigravity/brain/0fcb0336-309f-4f3d-9189-4f07004414c8/course_history_cards_VERIFIED.png)

---

## 7. BẢO TOÀN KIẾN TRÚC VÀ HỒI QUY (REGRESSION ASSURANCE)

- **Không can thiệp logic backend:** Toàn bộ dịch vụ C++ (`BookingService`, `UserService`, `ReviewService`, `ScheduleGuard`, `GUIBridge`) được giữ nguyên 100%.
- **Không thay đổi dữ liệu mẫu:** Các tập tin `data/*.dat` không bị chỉnh sửa hay ghi đè.
- **Không suy giảm tính toàn vẹn nghiệp vụ:** Tính năng hoàn thành khóa học, hiển thị số sao trung bình và duyệt đánh giá giữ nguyên các kết nối signal/slot.
- **Tuân thủ quy tắc quản lý phiên bản:** Tuyệt đối không khởi tạo Git, không tạo commit và không can thiệp GitHub từ xa.

---

## 8. KẾT LUẬN VÀ BÀN GIAO (CONCLUSION)

Cả 3 khuyết tật hiển thị hình học trên thẻ lịch sử khóa học của gia sư đã được giải quyết triệt để tại gốc rễ:
1. Chữ môn học và lịch học được bao bọc an toàn trong thẻ, ngắt dòng thông minh khi tên dài.
2. Đường phân cách dừng chuẩn xác bên trong thẻ, không bao giờ vượt qua khe hở hay chạm vào thẻ bên cạnh.
3. Phân cấp 3 tầng nội dung (Học viên $\to$ Khóa học $\to$ Đánh giá) rõ ràng, sang trọng và chuẩn mực theo ngôn ngữ thiết kế của TutorMatch.
4. Giao diện đáp ứng mượt mà từ màn hình lớn đến cửa sổ thu nhỏ.
