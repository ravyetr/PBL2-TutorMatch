# BÁO CÁO KỸ THUẬT: KHẮC PHỤC LỖI TRÀN NỘI DUNG THẺ KHÓA HỌC TRÊN THỜI KHÓA BIỂU (TIMETABLE COURSE CARD CONTENT OVERFLOW FIX REPORT)

---

## 1. TỔNG QUAN VÀ MỤC TIÊU (OVERVIEW & OBJECTIVES)

- **Dự án:** TutorMatch (Hệ thống kết nối gia sư và học viên)
- **Công nghệ áp dụng:** C++17, Qt 6.11.2, Qt Quick / QML, MinGW 13.1.0 64-bit
- **Tập tin báo cáo:** `docs/architecture/implementation/Timetable_Course_Card_Overflow_Fix_Report_VI.md`
- **Mục tiêu chính:** Khắc phục triệt để hiện tượng tràn viền (content overflow), đè chữ (vertical text collision) và hiển thị sai lệch kích thước thẻ khóa học đã chốt lịch (`isBooked: true`) trên lưới thời khóa biểu (`TimetableGrid` & `TimetableCell`), bảo đảm toàn vẹn tính bao đóng (clipping), phân cấp thị giác (visual hierarchy) và sự tương thích kích thước linh hoạt (responsive geometry) từ màn hình nhỏ đến màn hình độ phân giải cao.

---

## 2. PHÂN TÍCH NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE FORENSIC ANALYSIS)

Trước khi thực hiện bản vá, giao diện thẻ khóa học trên ô thời khóa biểu gặp phải 5 biểu hiện lỗi cụ thể:
1. Tên môn học / mô tả khóa học bị tràn (overflow) vượt quá đường viền dưới của thẻ.
2. Tên gia sư/học viên (`displayName`) và môn học (`subjectName`) nằm quá sát nhau theo chiều dọc, tạo cảm giác dính chữ.
3. Chữ và khối nền vẽ tự do ra ngoài ranh giới hình học của ô thay vì bị giới hạn bên trong.
4. Tên môn học dài (ví dụ: *Toán 12 - Luyện thi Đại học Quốc gia chuyên sâu*) không được ngắt hoặc rút gọn hợp lý trong các ô nhỏ.
5. Nội dung thẻ đè lấn lên đường phân cách (grid separators) và tràn sang ô của ca học liền kề.

### 2.1. Phân tích toán học về hình học container (`TimetableGrid` & `Overview`)
Trong `TutorOverview.qml` và `StudentOverviewView.qml`, component `TimetableGrid` được thiết lập:
```qml
Layout.preferredHeight: 480
Layout.minimumHeight: 440
```
Bên trong `TimetableGrid.qml`, cấu trúc cột dọc `ColumnLayout` bao gồm các thành phần cố định:
- Lề trên và dưới (`anchors.margins: 16 * 2`): **32 px**
- Tiêu đề hàng 1 (Tiêu đề + Pill ngày hôm nay): **38 px**
- Khoảng cách hàng 1 sang hàng 2: **10 px**
- Thanh điều khiển tuần + 3 Legend + Nút xóa ca bận: **30 px**
- Khoảng cách hàng điều khiển sang lưới thời khóa biểu: **10 px**
- Hàng tiêu đề các thứ trong tuần (Thứ 2 → Chủ nhật): **38 px**
- Khoảng cách giữa 9 hàng lưới (8 khe hở $\times$ 6 px): **48 px**

$$\text{Tổng chiều cao cố định} = 32 + 38 + 10 + 30 + 10 + 38 + 48 = 206\text{ px}$$

Khi container bị giới hạn ở `480 px`:

$$\text{Chiều cao khả dụng cho 8 ca học} = 480 - 206 = 274\text{ px}$$
$$\text{Chiều cao thực tế mỗi ô ca học} = \frac{274}{8} \approx \mathbf{34.25\text{ px}}$$

### 2.2. Sự xung đột kích thước trong `TimetableCell.qml` (Legacy)
Trong `TimetableCell.qml` ban đầu:
- Không khai báo `clip: true` trên `cellRoot Rectangle`.
- `textColumn` sử dụng `anchors.centerIn: parent` với `spacing: 2`.
- Khối hiển thị bên trong gồm:
  - Badge buổi học (`Buổi x/y`): `implicitHeight: 14 px`
  - Spacing: `2 px`
  - Tên gia sư/học viên (`font.pixelSize: 11`): chiều cao font thực tế $\approx 15\text{ px}$
  - Spacing: `2 px`
  - Tên môn học (`font.pixelSize: 10`): chiều cao font thực tế $\approx 14\text{ px}$

$$\text{Tổng chiều cao nội dung yêu cầu} = 14 + 2 + 15 + 2 + 14 = \mathbf{47\text{ px}}$$

Khi đặt một khối `47 px` căn giữa (`anchors.centerIn`) vào một ô chỉ cao `34.25 px`:
- Đỉnh cột chữ nằm tại: $(34.25 - 47) / 2 = \mathbf{-6.38\text{ px}}$ (nhô lên khỏi viền trên).
- Đáy cột chữ nằm tại: $34.25 + 6.38 = \mathbf{40.63\text{ px}}$ (vượt quá viền dưới $6.38\text{ px}$).
- Khoảng cách giữa hai ô liền kề chỉ là $6\text{ px}$, do đó nội dung của ô trên đè trực tiếp lên ô dưới.
- Thiếu `clip: true` khiến toàn bộ phần nhô ra được Qt Quick rasterize tự do đè lên viền ô và các ô lân cận.
- Spacing $2\text{ px}$ kết hợp với cùng một màu chữ `slotBookedText` (`#6C0820`) khiến hai dòng chữ như dính liền vào nhau.

---

## 3. GIẢI PHÁP THIẾT KẾ VÀ KIẾN TRÚC MỚI (ARCHITECTURAL & LAYOUT DESIGN)

Để xử lý tận gốc vấn đề mà không phá vỡ giao diện chung hay ảnh hưởng đến logic nghiệp vụ, giải pháp áp dụng theo 3 tầng:

```
+-----------------------------------------------------------------------+
| 1. Container Sizing (TutorOverview / StudentOverview / UserDetail)    |
|    - preferredHeight: 560px, minimumHeight: 480px                     |
|    - Đảm bảo mỗi ô ca học nhận đủ 44px - 46px trong ScrollView        |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
| 2. Grid Constraints (TimetableGrid.qml)                               |
|    - implicitHeight: 560px, implicitWidth: 800px                      |
|    - Layout.minimumHeight: 38px, Layout.preferredHeight: 46px          |
+-----------------------------------------------------------------------+
                                   |
+-----------------------------------------------------------------------+
| 3. Adaptive Card Layout & Hard Clipping (TimetableCell.qml)           |
|    - clip: true (Bảo đảm bao đóng vật lý tuyệt đối)                   |
|    - Dynamic typography & spacing theo chiều cao thực tế:             |
|      * Ultra-compact (< 40px): Badge 10px, Font 8.5/7.5, Spacing 1/1  |
|      * Compact (40px - 47px):  Badge 11px, Font 9.5/8.5, Spacing 1/2  |
|      * Standard (>= 48px):     Badge 13px, Font 10.5/9.5, Spacing 2/3 |
|    - Horizontal breathing room (anchors.leftMargin/rightMargin: 4px)  |
|    - Text.ElideRight + maximumLineCount: 1 cho cả tên và môn          |
|    - Visual hierarchy: opacity: 0.90 cho dòng môn học                 |
|    - Multi-line tooltip đầy đủ thông tin khi hover                    |
+-----------------------------------------------------------------------+
```

---

## 4. CHI TIẾT CÁC TẬP TIN ĐÃ ĐƯỢC CHỈNH SỬA (EXACT FILES MODIFIED)

### 4.1. `ui/qml/TimetableCell.qml`
- **Khai báo `clip: true`:** Ngăn chặn tuyệt đối hiện tượng tràn pixel ra ngoài đường viền ô.
- **Thêm thuộc tính tính toán thích ứng (Adaptive Properties):**
  ```qml
  readonly property bool hasBadge: sessionLabel !== ""
  readonly property bool isUltraCompact: height < 40
  readonly property bool isCompact: height < 48
  ```
- **Tái cấu trúc bố cục thành 2 tầng phân cấp:**
  1. `textColumn`: Căn giữa theo trục đứng `anchors.verticalCenter: parent.verticalCenter`, neo lề trái/phải với khoảng đệm an toàn `anchors.leftMargin: 4`, `anchors.rightMargin: 4`.
  2. Badge buổi học (`Buổi x/y`): Tự động thu nhỏ khi ô hẹp (`implicitHeight: 10px` ở chế độ ultra-compact, `11px` ở compact, `13px` ở standard). Ẩn hoàn toàn khi không có ca khóa học nhiều buổi (`hasBadge: false`), giải phóng toàn bộ không gian cho tên và môn học.
  3. `infoColumn`: Chứa tên và môn học với khoảng cách dọc linh hoạt (`spacing: 2` đến `3` px).
  4. Phân cấp thị giác: `primaryTxt` in đậm (`font.bold: true`), `secondaryTxt` thiết lập `opacity: 0.90` để tách biệt rõ ràng giữa người học/dạy và tên môn học.
  5. Cắt ngắn chữ thông minh: `elide: Text.ElideRight` và `maximumLineCount: 1` ngăn chặn việc chữ dài làm vỡ bố cục chiều dọc.
  6. Bảo toàn tooltip và hợp đồng đối tượng (`objectName: "primaryTextItem"`, `objectName: "secondaryTextItem"`).

### 4.2. `ui/qml/TimetableGrid.qml`
- Thêm `implicitWidth: 800` và `implicitHeight: 560` vào root `Rectangle`.
- Thiết lập `Layout.minimumHeight: 38` và `Layout.preferredHeight: 46` cho `slotRow`, ô hiển thị khung giờ (`Time Column Cell`) và từng `TimetableCell`.

### 4.3. `ui/qml/TutorOverview.qml` & `ui/qml/StudentOverviewView.qml`
- Điều chỉnh ràng buộc kích thước của `TimetableGrid`:
  ```qml
  Layout.preferredHeight: 560
  Layout.minimumHeight: 480
  ```
- Nhờ nằm bên trong `ScrollView`, việc tăng kích thước lên `560 px` mang lại tỉ lệ hiển thị cân đối hoàn hảo cho 8 ca học mà không làm tràn hay vỡ khung màn hình.

### 4.4. `ui/qml/UserDetailModal.qml`
- Cập nhật kích thước modal xem chi tiết người dùng dành cho Admin:
  ```qml
  width: Math.min(860, parent.width - 40)
  height: Math.min(680, parent.height - 40)
  ```
- Đảm bảo Tab 1 (Thời khóa biểu cá nhân của User) hiển thị đầy đủ, không bị nén quá mức.

---

## 5. KẾT QUẢ KIỂM THỬ TRỰC QUAN VÀ HÌNH HỌC (VISUAL & GEOMETRIC VERIFICATION)

Harness kiểm thử độc lập đã được biên dịch và thực thi (`verify_timetable_cells.exe`) trên nền Qt 6.11.2 thực tế với đầy đủ font metrics và render tree. Toàn bộ 7 kịch bản kiểm thử đều đạt chuẩn 100%:

| STT | Kịch bản kiểm thử (Test Scenario) | Kích thước ô (W $\times$ H) | Dữ liệu đầu vào | Vị trí Khối Tên (Primary Rect) | Vị trí Khối Môn (Secondary Rect) | Khoảng cách dọc (Gap) | Đáy chữ / Chiều cao ô | Kết quả |
|:---:|:---|:---:|:---|:---:|:---:|:---:|:---:|:---:|
| **1** | Tên gia sư chuẩn + Môn ngắn | $110 \times 46$ px | "ThS. Lê Văn Toàn", "Toán 12" | $[4, 7, 102, 15]$ | $[4, 25, 102, 14]$ | **$3\text{ px}$** | $39 / 46\text{ px}$ | **PASS** |
| **2** | Tên gia sư chuẩn + Môn dài | $105 \times 46$ px | "ThS. Lê Văn Toàn", "Toán 12 - Luyện thi Đại học Quốc gia chuyên sâu" | $[4, 7, 97, 15]$ | $[4, 25, 97, 14]$ (Elided) | **$3\text{ px}$** | $39 / 46\text{ px}$ | **PASS** |
| **3** | Tên gia sư dài + Môn dài | $95 \times 46$ px | "PGS. TS. Nguyễn Hoàng Nam Anh", "Vật lý 12 - Cơ học lượng tử..." | $[4, 7, 87, 15]$ (Elided) | $[4, 25, 87, 14]$ (Elided) | **$3\text{ px}$** | $39 / 46\text{ px}$ | **PASS** |
| **4** | Ô cực nhỏ hẹp (Ultra-compact) | $80 \times 36$ px | "ThS. Lê Văn Toàn", "Toán 12", "Buổi 2/4" | $[4, 11, 72, 12]$ | $[4, 24, 72, 11]$ | **$1\text{ px}$** | $35 / 36\text{ px}$ | **PASS** |
| **5** | Thẻ có badge Buổi x/y chuẩn | $110 \times 46$ px | "ThS. Lê Văn Toàn", "Toán 12 - Ôn thi THPT", "Buổi 2/4" | $[4, 15, 102, 14]$ | $[4, 31, 102, 12]$ | **$2\text{ px}$** | $43 / 46\text{ px}$ | **PASS** |
| **6** | Thẻ ca học liền kề trung bình | $100 \times 44$ px | "TS. Trần Minh Đức", "Hóa học 11 - Hóa hữu cơ...", "Buổi 4/10" | $[4, 14, 92, 14]$ | $[4, 30, 92, 12]$ | **$2\text{ px}$** | $42 / 44\text{ px}$ | **PASS** |
| **7** | Cô lập ô liền kề (Adjacent Collision) | 2 ô cao 46px, cách nhau 6px | Ô 1: Y=10..56, Ô 2: Y=62..108 | Ô 1: $[10, 10, 120, 46]$ | Ô 2: $[10, 62, 120, 46]$ | Khe hở $6.0\text{ px}$ | $0\text{ px}$ va chạm | **PASS** |

---

## 6. KẾT QUẢ BIÊN DỊCH VÀ KIỂM THỬ HỆ THỐNG (BUILD & SYSTEM TEST VERIFICATION)

### 6.1. Biên dịch ứng dụng GUI (`TutorMatch_GUI`)
Thực hiện biên dịch lại toàn bộ module QML với `mingw32-make TutorMatch_GUI`:
```
[  2%] Running qmlimportscanner for TutorMatch_GUI
[  2%] Copying TutorMatch_GUI qml sources into build dir
[  8%] Generating .rcc/qmlcache/TutorMatch_GUI_ui_qml_TimetableCell_qml.cpp
[ 10%] Building CXX object CMakeFiles/TutorMatch_GUI.dir/.../TimetableCell_qml.cpp.obj
[ 12%] Linking CXX executable TutorMatch_GUI.exe
Syncing canonical data directory and executable to root project directory
[100%] Built target TutorMatch_GUI
```
- **Trạng thái:** Thành công $100\%$, mã thoát $0$, không có cảnh báo cú pháp QML.

### 6.2. Kiểm thử hồi quy toàn diện (`TutorMatch_Tests.exe`)
Chạy bộ kiểm thử tự động toàn diện trên binary `TutorMatch_Tests.exe`:
- **Số lượng Test Suites:** 24/24 Test Suites.
- **Kết quả:** $100\%$ PASS, không có bất kỳ hồi quy nào trong logic nghiệp vụ hay cấu trúc dữ liệu.

```
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine... -> PASS
[TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy... -> PASS
[TEST] 3. Verifying DIP, ISP & SRP across decoupled Services... -> PASS
[TEST] 4. Verifying ScheduleGuard Bitmask Collision Math... -> PASS
[TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes... -> PASS
[TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence... -> PASS
[TEST] 7. Verifying Bidirectional Timetable Synchronization (Student <-> Tutor)... -> PASS
[TEST] 8. Verifying Admin User Card Data Contract & No Undefined Fields... -> PASS
[TEST] 9. Verifying User Detail Modal Contract & Non-Existent User Handling... -> PASS
[TEST] 10. Verifying Verification & Lock Sync Across Admin and Student Views... -> PASS
[TEST] 11. Verifying AdminService Filters & Subject Keyword Search... -> PASS
[TEST] 12. Verifying AVL Tree Multi-Value Support and Key/Value Deletion... -> PASS
[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary & Phantom Bit Sanitization... -> PASS
[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence & Zero-Duplication Invariants... -> PASS
[TEST] 15. Verifying DataFileInspector Read-Only Inspection & Error Handling... -> PASS
[TEST] 16. Verifying Course Booking Conflict Detection & Synchronization... -> PASS
[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)... -> PASS
[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)... -> PASS
[TEST] 19. Verifying Phase 3 End-to-End Business Workflows (E2E-01 to E2E-10)... -> PASS
[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)... -> PASS
[TEST] 21. Verifying Weekly Timetable Integration for Recurring Course Booking... -> PASS
[TEST] 22. Verifying Comprehensive Data Audit, Parsing & Migration Invariants... -> PASS
[TEST] 23. Verifying Multi-Slot Course Booking, Consensual Termination & Auto-Completion... -> PASS
[TEST SUITE 24] Comprehensive Commitment Acknowledgment & Review Eligibility Matrix... -> PASS
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

---

## 7. KẾT LUẬN VÀ GIỚI HẠN VẬN HÀNH (CONCLUSIONS & OPERATIONAL BOUNDARIES)

1. **Khắc phục dứt điểm:** Lỗi tràn chữ ở đường viền đáy, dính sát giữa tên và môn học, cũng như việc nội dung đè lên vạch ngăn cách đã được xử lý triệt để nhờ cơ chế thích ứng chiều cao kết hợp với `clip: true`.
2. **Trải nghiệm người dùng:** Các chuỗi ký tự dài đều được tự động thu gọn có dấu ba chấm (`...`) một cách thanh lịch, đồng thời khi rê chuột (hover), tooltip đa dòng sẽ cung cấp đầy đủ thông tin chi tiết về gia sư, môn học, số buổi và khung giờ.
3. **Phạm vi an toàn:** Bản vá chỉ tập trung vào tầng giao diện QML (`TimetableCell.qml`, `TimetableGrid.qml`, `TutorOverview.qml`, `StudentOverviewView.qml`, `UserDetailModal.qml`), hoàn toàn không can thiệp vào tầng dữ liệu, mô hình C++, thuật toán đặt lịch hay cơ chế bitmask 56 ca của `ScheduleGuard`.
4. **Giới hạn tối thiểu:** Kích thước hiển thị tối thiểu được khuyến nghị cho mỗi ô thời khóa biểu là $70\text{ px}$ chiều ngang và $36\text{ px}$ chiều dọc. Nếu màn hình bị thu nhỏ dưới ngưỡng này, cơ chế `clip: true` sẽ bảo vệ tuyệt đối các phần tử bên ngoài không bị vẽ đè, và người dùng vẫn có thể xem trọn vẹn nội dung thông qua tooltip tương tác.
