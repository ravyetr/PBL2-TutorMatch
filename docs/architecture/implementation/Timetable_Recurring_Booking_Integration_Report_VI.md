# BÁO CÁO NGHIỆM THU TÍCH HỢP THỜI KHÓA BIỂU CHO ĐẶT KHÓA HỌC ĐỊNH KỲ (WEEKLY TIMETABLE INTEGRATION FOR RECURRING COURSE BOOKING)

**Dự án:** Nền tảng kết nối gia sư TutorMatch  
**Công nghệ:** C++17, Qt 6.11 (Qt Quick / QML), CMake, Flat-File Persistence  
**Trạng thái triển khai:** Hoàn thành toàn diện 100% (IMPLEMENTED & TESTED)  
**Tập kiểm thử tự động:** 21/21 Test Suites PASS (100%)  
**Tính toàn vẹn dữ liệu gốc:** Bảo toàn 100% SHA-256 đối với `users.dat`, `bookings.dat`, `reviews.dat`  
**Ngày lập báo cáo:** 06/11/2026  

---

## 1. NGUYÊN NHÂN GỐC CỦA LỖI (ROOT CAUSE OF THE MISSING BOOKING DISPLAY)

Khi người dùng thực hiện quy trình nghiệp vụ:
1. Học viên `student_nam` đặt khóa học định kỳ 4 buổi vào Thứ Sáu với gia sư `tutor_math`.
2. Khóa học được lưu thành công vào `data/course_bookings.dat`.
3. Gia sư `tutor_math` chấp nhận khóa học (trạng thái chuyển thành `ACCEPTED`).
4. **Hiện tượng lỗi:** Cả học viên lẫn gia sư đều không nhìn thấy ca học này hiển thị là `BOOKED` trên thời khóa biểu.

### Nguyên nhân cốt lõi được điều tra và xác định chính xác:
1. **Thiếu truy vấn nguồn dữ liệu khóa học trong Service/Bridge:**
   - Trong `GUIBridge::getStudentWeeklySchedule()` và `GUIBridge::getTutorWeeklySchedule()`, mã nguồn trước đây chỉ gọi `bookingService->getStudentBookings()` và `bookingService->getTutorBookings()`.
   - Các hàm này chỉ đọc dữ liệu từ tệp di sản `bookings.dat` (`std::vector<Booking>`).
   - Chúng **hoàn toàn không truy vấn** `course_bookings.dat` (`std::vector<CourseBooking>`) thông qua `courseBookingRepo`.
2. **Không có mô hình tuần và ngày tháng lịch thực tế (Calendar Week Model):**
   - Các API thời khóa biểu cũ hoàn toàn tĩnh, chỉ lặp 56 ca học (`slot` 0..7, `day` 0..6) trừu tượng mà không nhận bất kỳ tham số ngày tham chiếu nào (`referenceDate` hay `weekStartDate`).
   - `CourseBooking` sinh ra các ngày học cụ thể (`startDate + 7 * i`), nhưng hệ thống thời khóa biểu không thể biết ca học nào thuộc về tuần đang xem.
3. **Quyết định kiến trúc bảo toàn lịch rảnh (`scheduleBitmask`):**
   - Theo quyết định thiết kế đã duyệt, khi một khóa học định kỳ được duyệt, hệ thống **không xóa bit** trên `User.scheduleBitmask` của gia sư/học viên để bảo toàn khả năng sẵn sàng dài hạn.
   - Khi `getStudentWeeklySchedule()` và `getTutorWeeklySchedule()` không kiểm tra `CourseBooking`, chúng chỉ dựa vào `User.scheduleBitmask`. Do bitmask vẫn giữ nguyên, ô lịch học luôn được hiển thị là `FREE` ("RẢNH") thay vì `BOOKED`!

---

## 2. KHIẾM KHUYẾT CHÍNH XÁC TRONG LUỒNG DỮ LIỆU (EXACT DATA-FLOW DEFECT)

Sơ đồ đối chiếu điểm đứt gãy luồng dữ liệu:

```
[Trước khi sửa lỗi - Gãy tại GUIBridge & Timetable Query]
course_bookings.dat 
  -> FileCourseBookingRepository 
  -> BookingService (có getStudentCourseBookings) 
  --[ ĐỨT GÃY: GUIBridge chỉ gọi getStudentBookings() di sản ]--X
  -> GUIBridge::getStudentWeeklySchedule() (Không có tham số tuần, bỏ qua CourseBooking)
  -> QML TimetableGrid (Không có bộ lọc tuần, không hiển thị BOOKED)

[Sau khi sửa lỗi - Luồng dữ liệu hoàn chỉnh 100%]
course_bookings.dat 
  -> FileCourseBookingRepository
  -> BookingService::getAcceptedSessionsForWeek(userId, role, mondayDateStr)
      [Sinh các ngày học: startDate + 7 * i, lọc theo tuần Monday..Sunday]
  -> GUIBridge::getStudentWeeklySchedule(weekStartDateStr) / getTutorWeeklySchedule(...)
      [Tích hợp 2 nguồn: CourseBooking ACCEPTED ưu tiên + Legacy Booking + Availability Bitmask]
  -> DTO TimetableCell đầy đủ thông tin (sessionNumber, totalSessions, counterpartName, date)
  -> QML TimetableGrid & TimetableCell
      [Hiển thị thẻ BOOKED, badge Buổi X/Y, điều hướng tuần, nổi bật hôm nay]
```

---

## 3. DANH SÁCH FILE THAY ĐỔI VÀ TẠO MỚI (FILES CREATED AND MODIFIED)

| STT | Đường dẫn file | Trạng thái | Nội dung thay đổi |
| :--- | :--- | :--- | :--- |
| 1 | `include/utils/DateUtils.h` | MODIFIED | Bổ sung khai báo: `getWeekMonday`, `getWeekSunday`, `getWeekDays`, `shiftWeek`, `formatDisplayDate`. |
| 2 | `src/utils/DateUtils.cpp` | MODIFIED | Triển khai các thuật toán tính thứ hai đầu tuần, chủ nhật cuối tuần, dịch chuyển tuần $\pm N$, danh sách 7 ngày trong tuần. |
| 3 | `include/models/BookingDTOs.h` | MODIFIED | Định nghĩa cấu trúc DTO mới `TimetableSessionDTO` với đầy đủ thông tin buổi học, số thứ tự buổi, tổng số buổi và đối tác. |
| 4 | `include/services/BookingService.h` | MODIFIED | Khai báo phương thức `getAcceptedSessionsForWeek(userId, role, mondayDateStr)`. |
| 5 | `src/services/BookingService.cpp` | MODIFIED | Triển khai `getAcceptedSessionsForWeek`: an toàn đa luồng, sinh ngày học thực tế, lọc chính xác theo tuần requested, tra cứu tên đối tác. |
| 6 | `include/gui/GUIBridge.h` | MODIFIED | Cập nhật các API `getStudentWeeklySchedule`, `getTutorWeeklySchedule`, `getWeeklyScheduleMatrix` hỗ trợ tham số tuần tùy chọn; bổ sung các API điều hướng tuần `getWeekMonday`, `getWeekSunday`, `shiftWeek`, `formatWeekRangeDisplay`, `getCurrentFullDateString`, `getWeekDaysInfo`; cập nhật `initialize` nhận `cbRepo`. |
| 7 | `src/gui/GUIBridge.cpp` | MODIFIED | Triển khai tích hợp đè lớp thời khóa biểu (CourseBooking `ACCEPTED` ưu tiên cao nhất, kế tiếp là Legacy booking, cuối cùng là Availability bitmask); tích hợp `CourseBooking` vào `qmlGetTutorScheduleLessons()`. |
| 8 | `ui/qml/TimetableGrid.qml` | MODIFIED | Nâng cấp toàn diện: hiển thị ngày hiện tại (local date), thanh điều hướng tuần (`Tuần trước`, `Tuần hiện tại`, `Tuần sau`), hiển thị khoảng ngày tuần, tiêu đề cột có thứ và ngày cụ thể (`DD/MM`), nổi bật cột "Hôm nay", legend 3 trạng thái. |
| 9 | `ui/qml/TimetableCell.qml` | MODIFIED | Nâng cấp giao diện ô: hiển thị huy hiệu buổi học (`Buổi X/Y`), tên đối tác, tên môn, ToolTip chi tiết khi hover, giữ nguyên khóa khi đã đặt. |
| 10 | `ui/qml/TutorSchedule.qml` | MODIFIED | Chuyển `TimetableGrid` sang chế độ tự nạp dữ liệu theo tuần và `readOnly: true`, cho phép gia sư tự do duyệt lịch các tuần khác nhau. |
| 11 | `tests/TestRunner.cpp` | MODIFIED | Bổ sung Test Suite 21 (`run_test_timetable_recurring_course_integration`) bao phủ 100% các yêu cầu kiểm thử và hồi quy. |

---

## 4. LUỒNG DỮ LIỆU THỜI KHÓA BIỂU MỚI (NEW TIMETABLE DATA FLOW)

Mỗi khi người dùng mở màn hình hoặc chuyển tuần trên giao diện:
1. **Presentation Layer (`TimetableGrid.qml`):**
   - Xác định ngày Thứ Hai của tuần hiển thị (`currentMonday`). Nếu chưa có, gọi `guiBridge.getWeekMonday()`.
   - Lấy danh sách 7 ngày trong tuần từ `guiBridge.getWeekDaysInfo(currentMonday)`.
   - Gọi `guiBridge.getStudentWeeklySchedule(currentMonday)` (hoặc `getTutorWeeklySchedule(currentMonday)`).
2. **Adapter Layer (`GUIBridge`):**
   - Tính toán khoảng thời gian từ Thứ Hai đến Chủ Nhật (`DateUtils::addDays(monday, 6)`).
   - Gọi `BookingService::getAcceptedSessionsForWeek()`.
   - Lập ma trận 56 ô học (`sessionGrid[slot][day]`).
3. **Business Logic Layer (`BookingService`):**
   - Khóa `serviceMutex` (`std::recursive_mutex`) bảo đảm an toàn đa luồng.
   - Quét toàn bộ khóa học định kỳ của người dùng (`status == "ACCEPTED"`).
   - Với mỗi khóa học, gọi `DateUtils::generateSessionDates(startDate, weeksCount)`.
   - Kiểm tra ngày buổi học thứ $i$ ($i \in [0, \text{weeksCount}-1]$): nếu nằm trong tuần $[ \text{Monday}, \text{Sunday} ]$, đóng gói vào `TimetableSessionDTO` với `sessionNumber = i + 1`.
4. **Hợp nhất dữ liệu (Overlay Synthesis):**
   - **Ưu tiên 1:** Nếu có `CourseBooking` session $\implies$ Ô có trạng thái `BOOKED`, hiển thị tên đối tác, môn học, huy hiệu `Buổi X/Y`, `locked = true`, `editable = false`.
   - **Ưu tiên 2:** Nếu có legacy booking `ACCEPTED` $\implies$ Ô có trạng thái `BOOKED` (di sản).
   - **Ưu tiên 3:** Nếu không có lịch đặt $\implies$ Lấy từ `User.scheduleBitmask` (`FREE` hoặc `BUSY`).

---

## 5. TÁCH BIỆT AVAILABILITY VÀ RESERVATION (AVAILABILITY VS ACTUAL RESERVATION LOGIC)

- **`User.scheduleBitmask` (Lịch rảnh tổng quát):**
  - Đại diện cho các khung giờ mà gia sư hoặc học viên có thể nhận lớp định kỳ nói chung (Recurring Weekly Pattern).
  - Không bao giờ bị trừ/xóa khi chấp nhận một khóa học định kỳ, và không bị sửa khi hủy hay hoàn thành khóa học.
- **`CourseBooking` (Lịch đặt chỗ thực tế):**
  - Chỉ khi có trạng thái `ACCEPTED`, khóa học mới tạo ra các buổi học thực tế (Reservations).
  - Khóa học 4 tuần chỉ đặt chỗ trên 4 ngày Thứ Sáu cụ thể. Các ngày Thứ Sáu trước đó hoặc sau đó vẫn hiển thị `FREE` ("RẢNH") theo đúng `scheduleBitmask`.
  - Hai khóa học định kỳ vào cùng ca Thứ Sáu nhưng lệch kỳ (ví dụ: một khóa tháng 11/2026 và một khóa tháng 01/2027) hoàn toàn không xung đột và hiển thị chính xác trên thời khóa biểu của từng tuần tương ứng.

---

## 6. TÍNH TOÁN NGÀY THÁNG VÀ TUẦN LỊCH (CALENDAR WEEK & DATE CALCULATION)

Module `DateUtils` sử dụng số học lịch Gregory thuần túy (Howard Hinnant civil day algorithms) và công thức Tomohiko Sakamoto:
- **Chuẩn hóa tuần:** Tuần bắt đầu từ Thứ Hai (`dayOfWeek == 0`) đến Chủ Nhật (`dayOfWeek == 6`).
- **Xác định ngày Thứ Hai:**
  $$\text{Monday} = \text{Date} - \text{dayOfWeek}(\text{Date}) \text{ ngày}$$
- **Dịch chuyển tuần:**
  $$\text{ShiftWeek}(\text{Monday}, \Delta) = \text{Monday} + 7 \times \Delta \text{ ngày}$$
- **Ranh giới tháng và năm:** Xử lý chính xác nhờ phép đổi số ngày dân sự `toCivilDays` và `fromCivilDays`, vượt qua hoàn toàn các biên giới tháng (ví dụ: ngày 30/11 sang 01/12) và biên giới năm (ví dụ: 28/12/2026 sang 03/01/2027) cũng như năm nhuận (29/02/2024).

---

## 7. HÀNH VI HIỂN THỊ CỦA HỌC VIÊN VÀ GIA SƯ (STUDENT/TUTOR DISPLAY BEHAVIOR)

Tính đối xứng thông tin giữa học viên và gia sư được bảo đảm tuyệt đối:
- **Học viên (`student_nam`):**
  - Nhìn thấy ô Thứ Sáu ca 0 hiển thị: `BOOKED`, `displayName` = `ThS. Lê Văn Toàn`, `secondaryText` = `Toán 12 (Buổi 1/4)`, huy hiệu `Buổi 1/4`.
- **Gia sư (`tutor_math`):**
  - Nhìn thấy cùng ô Thứ Sáu ca 0 trên cùng ngày đó hiển thị: `BOOKED`, `displayName` = `Nguyễn Văn Nam`, `secondaryText` = `Toán 12 (Buổi 1/4)`, huy hiệu `Buổi 1/4`.
- Cả hai bên đều nhìn thấy cùng ngày tháng, cùng ca học, cùng môn học và cùng số thứ tự buổi học.

---

## 8. CƠ CHẾ CẬP NHẬT TỨC THÌ (STATUS REFRESH BEHAVIOR)

Khi gia sư chấp nhận khóa học (`qmlAcceptBooking`):
- `GUIBridge` phát tín hiệu `studentWeeklyScheduleChanged()`, `tutorScheduleChanged()`, và `dataChanged()`.
- Thành phần `TimetableGrid` bắt các tín hiệu này qua `Connections` và tự động thực thi `reloadSchedule()`.
- Giao diện người dùng được vẽ lại ngay lập tức mà không cần khởi động lại ứng dụng hay đổi tab.
- Khi khóa học bị hủy (`qmlCancelBooking`), ô học tự động giải phóng và chuyển về trạng thái `FREE` ("RẢNH").

---

## 9. KẾT QUẢ KIỂM THỬ TỰ ĐỘNG (AUTOMATED TEST RESULTS)

Toàn bộ 21 Test Suites đã được thực thi và vượt qua 100%:

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
[TEST] 7. Verifying Bidirectional Timetable Synchronization... -> PASS
[TEST] 8. Verifying Admin User Card Data Contract... -> PASS
[TEST] 9. Verifying User Detail Modal Contract & Error Handling... -> PASS
[TEST] 10. Verifying Verification & Lock Sync Across Views... -> PASS
[TEST] 11. Verifying AdminService Filters & Search... -> PASS
[TEST] 12. Verifying AVL Tree Multi-Value Support & Deletion... -> PASS
[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary Sanitization... -> PASS
[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence... -> PASS
[TEST] 15. Verifying DataFileInspector Read-Only Inspection... -> PASS
[TEST] 16. Verifying Phase 1 Booking Conflict Detection... -> PASS
[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)... -> PASS
[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)... -> PASS
[TEST] 19. Verifying Phase 3 End-to-End Business Workflows... -> PASS
[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)... -> PASS
[TEST] 21. Verifying Weekly Timetable Integration for Recurring Course Booking... -> PASS
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

### Các kịch bản chi tiết trong Test Suite 21:
1. `DateUtils`: Xác thực thứ trong tuần, tính ngày đầu tuần/cuối tuần, dịch chuyển tuần, ranh giới tháng (30/11 -> 01/12), ranh giới năm (28/12/2026 -> 01/01/2027), năm nhuận (29/02/2024).
2. `PENDING`: Xác nhận khóa học `PENDING` không tạo ca học `BOOKED` trên thời khóa biểu của cả hai bên.
3. `ACCEPTED`: Sau khi gia sư chấp nhận, cả học viên và gia sư đều có buổi học `BOOKED` khớp ngày, khớp ca, khớp số buổi `Buổi 1/4`.
4. `Week Filtering`: Buổi 1 chỉ xuất hiện ở Tuần 1, Buổi 2 ở Tuần 2, Buổi 3 ở Tuần 3, Buổi 4 ở Tuần 4. Tuần trước và tuần sau khóa học hiển thị `FREE` ("RẢNH").
5. `Invariant`: `scheduleBitmask` của học viên và gia sư không bị thay đổi.
6. `Cancellation`: Khi học viên hủy khóa học, ca học trên thời khóa biểu tự động biến mất và trả lại trạng thái `FREE`.

---

## 10. KẾT QUẢ BIÊN DỊCH VÀ MÃ THOÁT (BUILD RESULTS & EXIT CODES)

| Target Executable | Lệnh thực thi biên dịch | Exit Code | Trạng thái |
| :--- | :--- | :--- | :--- |
| `TutorMatch_Tests.exe` | `cmake --build . --target TutorMatch_Tests` | `0` | Biên dịch & liên kết thành công |
| `TutorMatch_GUI.exe` | `cmake --build . --target TutorMatch_GUI` | `0` | Biên dịch & liên kết thành công |
| `TutorMatch_CLI.exe` | `cmake --build . --target TutorMatch_CLI` | `0` | Biên dịch & liên kết thành công |
| `TutorMatch_Inspector.exe` | `cmake --build . --target TutorMatch_Inspector` | `0` | Biên dịch & liên kết thành công |

---

## 11. BẢO TOÀN DỮ LIỆU SẢN XUẤT (BASELINE SHA-256 COMPARISON)

| Đường dẫn tệp | SHA-256 Trước triển khai (Baseline) | SHA-256 Sau kiểm thử (Verified) | Trạng thái đối soát |
| :--- | :--- | :--- | :--- |
| `data/users.dat` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | **KHỚP 100% (Bit-for-Bit)** |
| `data/bookings.dat` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | **KHỚP 100% (Bit-for-Bit)** |
| `data/reviews.dat` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | **KHỚP 100% (Bit-for-Bit)** |

---

## 12. PHÂN ĐỊNH TRẠNG THÁI NGHIỆM THU (VERIFICATION CLASSIFICATION)

Tuân thủ nghiêm ngặt yêu cầu phân định minh bạch:
- **IMPLEMENTED (Đã triển khai):** Toàn bộ backend C++17 (`DateUtils`, `BookingDTOs`, `BookingService`, `GUIBridge`) và frontend QML (`TimetableGrid.qml`, `TimetableCell.qml`, `TutorSchedule.qml`).
- **TESTED (Đã kiểm thử tự động):** 21/21 Test Suites tự động trong `TutorMatch_Tests.exe` chạy độc lập với dữ liệu sandbox bộ nhớ, đạt tỷ lệ đạt 100% PASS. Đã kiểm thử tính năng điều hướng tuần, lọc theo tuần, overlay ưu tiên, tính toàn vẹn dữ liệu gốc bằng SHA-256 và Inspector.
- **SMOKE TESTED (Khởi chạy tiến trình):** Đã kiểm tra khởi chạy thành công `TutorMatch_GUI.exe` (PID 29256) và `TutorMatch_CLI.exe`, nạp context Qt Quick mà không có lỗi crash.
- **NOT VISUALLY VERIFIED MANUALLY (Chưa kiểm tra trực quan thủ công):** Môi trường tác vụ dòng lệnh không có màn hình hiển thị trực tiếp để chụp ảnh cửa sổ GUI thực tế; sự chính xác của hiển thị giao diện được đảm bảo qua kiểm thử thuộc tính dữ liệu và binding QML tĩnh/động.

---

## 13. HẠN CHẾ CÒN LẠI (REMAINING LIMITATIONS)

1. **Hỗ trợ múi giờ quốc tế:** Hiện tại hệ thống sử dụng ngày lịch địa phương của hệ điều hành chạy ứng dụng, chưa hỗ trợ người dùng ở hai quốc gia khác múi giờ lệch nhau qua ngày.
2. **Nghỉ lễ / ngày nghỉ:** Các buổi học trong khóa học được sinh đều đặn mỗi tuần $7 \times i$ ngày; hệ thống chưa có cấu hình danh sách ngày lễ để tự động lùi buổi học sang tuần tiếp theo.
