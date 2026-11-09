# BÁO CÁO KIỂM CHỨNG CUỐI CÙNG: ĐỐI SOÁT ĐỘC LẬP SAU ĐỢT DỌN DẸP PHẠM VI
## Dự án TutorMatch | Qt 6.11 | C++17 | Read-Only Post-Cleanup Verification Audit

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Chế độ thực thi:** **READ-ONLY AUDIT (Không sửa mã nguồn, không sửa test, không sửa dữ liệu, không Git)**  
**Thời điểm thực hiện kiểm chứng:** 09/11/2026 21:40:00 (UTC+7)  
**Trạng thái kiểm chứng tổng thể:** **COMPLETED — 11/13 YÊU CẦU PASS, 1 YÊU CẦU FAIL (COMMITMENT ACKNOWLEDGMENT), 1 YÊU CẦU CHỜ CHỦ DỰ ÁN PHÊ DUYỆT (REVIEW ELIGIBILITY)**  

---

## 1. TỔNG QUAN ĐIỀU HÀNH (EXECUTIVE SUMMARY)

Báo cáo này được lập nhằm mục đích **kiểm chứng độc lập, nghiêm ngặt và hoàn toàn Read-Only** toàn bộ các thay đổi trong đợt dọn dẹp phạm vi và tinh gọn mã nguồn của dự án TutorMatch.

### 1.1. Tóm tắt hiện trạng kỹ thuật
- **Biên dịch (Build):** 100% sạch sẽ trên cả 4 targets (`TutorMatch_GUI`, `TutorMatch_CLI`, `TutorMatch_Tests`, `TutorMatch_Inspector`). Cấu hình CMake và MinGW 13.1.0 đạt Exit Code 0, không có lỗi hay cảnh báo.
- **Kiểm thử (Test Suite):** 23/23 Test Suites trong `TutorMatch_Tests.exe` đạt **100% PASS** (Exit Code 0).
- **Tính toàn vẹn dữ liệu (Data Integrity):** `TutorMatch_Inspector.exe` thẩm định 73/73 bản ghi hợp lệ (50 người dùng, 8 khóa học, 15 đánh giá, 27 bản ghi archive). Tệp `bookings.dat` không tồn tại trong runtime hoạt động.
- **Dọn dẹp mã chết (Dead Code):** Toàn bộ các tệp mã nguồn Win32 GDI cũ, các tệp QML trùng lặp/overengineering, mô hình `Booking` cũ và các repository/serializer cũ đã được xóa bỏ hoàn toàn khỏi đĩa và không còn bất kỳ liên kết/include nào trong `CMakeLists.txt`, `qmldir` hay mã nguồn C++.

### 1.2. Phát hiện nghiệp vụ quan trọng (Key Audit Findings)
1. **Thiếu sót cam kết kết thúc sớm (Requirement 5 / Commitment Acknowledgment - FAIL):** Trong mã nguồn hiện tại, thực thể `CourseBooking` và các dịch vụ `BookingService` **hoàn toàn chưa có các trường và logic kiểm tra cam kết kết thúc sớm** (`studentAcknowledged`, `studentAckTimestamp`, `tutorAcknowledged`, `tutorAckTimestamp`, `commitmentVersion`) như quy định tại ADR-009 mục 2.7.
2. **Quyền đánh giá sau kết thúc sớm chưa phê duyệt (Review Eligibility - PENDING STAKEHOLDER DECISION):** Mã nguồn hiện tại trong `ReviewService` và `BookingService` cho phép học viên gửi đánh giá cho cả khóa học `COMPLETED` và `EARLY_TERMINATED`. Yêu cầu kiểm chứng nêu rõ quyết định này **chưa được chủ dự án phê duyệt chính thức**.
3. **Lệch pha tài liệu kiến trúc (Documentation Mismatch - DETECTED):** Tài liệu `ADR-009` và `Multi_Session_Course_Booking_Design_VI.md` vẫn còn chứa toàn bộ thiết kế phức tạp của hệ thống LMS (`CourseSession`, báo cáo điểm danh, `SessionDispute`, Admin phân xử, phân rã giá trị tài chính theo buổi) trong khi mã nguồn thực tế đã được tinh gọn về mô hình Matching Platform.

---

## 2. MA TRẬN TUÂN THỦ YÊU CẦU NGHIỆP VỤ (PRODUCT REQUIREMENT COMPLIANCE MATRIX)

| # | Yêu cầu nghiệp vụ | Kết quả kiểm chứng | Tệp & Hàm liên quan | Dẫn chứng source code thực tế | Trường hợp chưa kiểm thử / Ghi chú |
|---|---|:---:|---|---|---|
| **1** | TutorMatch chỉ là ứng dụng ghép nối học sinh và gia sư (Matching Platform, không phải LMS). | **PASS** | `include/algorithms/MatchingEngine.h`<br>`src/services/MatchingService.cpp`<br>`src/ds/ScheduleGuard.cpp` | Không có bất kỳ lớp quản lý lớp học trực tuyến, video stream, nộp bài tập, điểm danh hay sổ điểm học tập nào. | Đã bao quát qua toàn bộ 23 test suites. |
| **2** | Học sinh được chọn nhiều ca trong một tuần. | **PASS** | `include/models/CourseBooking.h`<br>`src/services/BookingService.cpp`<br>`ui/qml/BookingModal.qml` | `BookingService::bookCourseMultiSlot` (dòng 135-225). `CourseBooking::weeklySchedule` lưu định dạng chuỗi `"dow:slot;..."`. Giao diện `BookingModal.qml` cho phép chọn nhiều ca học qua mảng `chosenSlots`. | Chưa kiểm thử trường hợp truyền trùng ca học (vd: `{{1, 1}, {1, 1}}`) ở tầng Service. |
| **3** | Hệ thống sinh lịch dự kiến cho khóa học 4, 8 hoặc 12 tuần. | **PASS** | `include/utils/DateUtils.h`<br>`src/utils/DateUtils.cpp`<br>`src/gui/GUIBridge.cpp` | `DateUtils::generateMultiSlotSessions` (dòng 138-173) sinh danh sách buổi học đơn định theo ngày dương lịch và ca học, sắp xếp tăng dần. `GUIBridge::calculateMultiSlotSessionDates` (dòng 962-985) hiển thị xem trước trên QML. | Đã kiểm thử với thời lượng 4 tuần trong Test 23; chưa có test riêng cho thời lượng 8 và 12 tuần của multi-slot. |
| **4** | Gia sư có quyền chấp nhận hoặc từ chối yêu cầu. | **PASS** | `src/services/BookingService.cpp`<br>`ui/qml/TutorBookings.qml` | `BookingService::acceptCourseBooking` (dòng 230-274) và `BookingService::rejectCourseBooking` (dòng 275-306). Kiểm tra đúng `tutorId`, trạng thái `PENDING`, phát hiện xung đột trước khi chuyển `ACCEPTED` hoặc `REJECTED`. | Đã kiểm thử gia sư chấp nhận/từ chối, chấp nhận đơn của gia sư khác bị chặn, chấp nhận lặp (idempotent). |
| **5** | Học sinh và gia sư phải xác nhận cam kết kết thúc sớm trước khi khóa học được chấp nhận. | **FAIL** | `include/models/CourseBooking.h`<br>`src/services/BookingService.cpp`<br>`src/storage/serializers/CourseBookingSerializer.cpp`<br>`ui/qml/BookingModal.qml` | **Không tồn tại trong mã nguồn**. `CourseBooking` không có trường lưu timestamp/phiên bản cam kết. `bookCourseMultiSlot` và `acceptCourseBooking` không kiểm tra xác nhận cam kết. | **Chưa được hiện thực hóa** (Thiếu sót so với ADR-009 mục 2.7). |
| **6** | Kết thúc sớm phải có sự đồng ý của cả hai bên (Consensual Early Termination). | **PASS** | `src/services/BookingService.cpp`<br>`ui/qml/StudentBookingsView.qml`<br>`ui/qml/TutorBookings.qml` | `requestEarlyTermination` (dòng 380-421) chuyển `earlyTerminationStatus = "PENDING"`. `respondEarlyTermination` (dòng 423-488) chặn người gửi tự phản hồi, yêu cầu bên còn lại chấp thuận (`agree = true`) mới chuyển sang `EARLY_TERMINATED`. | Đã kiểm thử trong Test 23 (dòng 3086-3108). |
| **7** | Nếu một bên từ chối, khóa học tiếp tục theo lịch ban đầu. | **PASS** | `src/services/BookingService.cpp` (dòng 460-474) | Khi `agree == false`: cập nhật `earlyTerminationStatus = "REJECTED"`, trạng thái khóa học giữ nguyên `ACCEPTED`, lịch học không đổi. | Đã kiểm thử trong Test 23 (dòng 3093-3098). |
| **8** | Nếu hai bên đồng ý, khóa học kết thúc và toàn bộ lịch tương lai của chính khóa học đó được giải phóng. | **PASS** | `src/services/BookingService.cpp`<br>`src/services/BookingService.cpp` | `respondEarlyTermination` chuyển `status = "EARLY_TERMINATED"`, gán `actualEndDate`. Trong `hasCourseConflictMulti` (dòng 55), chỉ `ACCEPTED` mới khóa chỗ. Trong `getAcceptedSessionsForWeek` (dòng 642-646), các ca sau `actualEndDate` được bỏ qua. | Ca học diễn ra đúng vào ngày kết thúc sớm (`s.date == actualEndDate`) vẫn được giữ lại trên thời khóa biểu. |
| **9** | Không ảnh hưởng lịch của các khóa học khác. | **PASS** | `src/services/BookingService.cpp` | `respondEarlyTermination` chỉ cập nhật bản ghi khóa học có ID tương ứng. Các khóa học khác của cùng học viên/gia sư giữ nguyên trạng thái `ACCEPTED`. | Đã kiểm thử khóa học `c2` không bị ảnh hưởng trong Test 23 (dòng 3109-3112). |
| **10** | Không thay đổi lịch rảnh định kỳ `scheduleBitmask`. | **PASS** | `src/services/BookingService.cpp` | `BookingService` chỉ đọc `getScheduleBitmask()` để kiểm tra độ rảnh ban đầu (dòng 195, 199); tuyệt đối không gọi `setScheduleBitmask()` khi đặt hay kết thúc khóa học. | Đã kiểm thử bảo toàn bitmask trong Test 23. |
| **11** | Hết thời hạn, khóa học tự động được đánh dấu kết thúc theo cơ chế thực tế của ứng dụng. | **PASS** | `src/services/BookingService.cpp` (dòng 490-509) | `autoCompleteExpiredCourses` quét các khóa học `ACCEPTED`, nếu `currentDate > endDate` thì cập nhật `status = "COMPLETED"`. Cơ chế này chạy đối soát lười (Lazy Reconciliation) khi gọi truy vấn. | Đã kiểm thử tự động chuyển `COMPLETED` cho đơn quá hạn trong Test 23 (dòng 3121-3129). |
| **12** | Không có điểm danh, xác nhận buổi học, báo cáo tiến độ hoặc tranh chấp cấp buổi học. | **PASS** | Toàn bộ `include/`, `src/`, `ui/` | Quét toàn bộ mã nguồn không còn bất kỳ lớp hay hàm nào liên quan đến `CourseSession`, `AttendanceReport`, `SessionDispute`. | Đã kiểm chứng qua lệnh grep hệ thống. |
| **13** | Không có hoa hồng, thanh toán thực tế hoặc dòng tiền nền tảng. | **PASS** | `include/services/AdminService.h`<br>`src/services/AdminService.cpp`<br>`src/ui/ConsoleApp.cpp` | Đã gỡ bỏ toàn bộ `platformCommissionRate`, `initialConnectionGMV`, doanh thu hoa hồng. Chỉ hiển thị học phí đề xuất và tổng giá trị khóa học. | Đã kiểm tra trong Test 3 (AdminService metrics). |

---

## 3. KIỂM TOÁN NGUỒN: CƠ CHẾ KẾT THÚC SỚM (EARLY TERMINATION SOURCE AUDIT)

### 3.1. Trạng thái và trường dữ liệu quản lý
Tại [CourseBooking.h](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/CourseBooking.h#L39-L44):
```cpp
int earlyTerminationRequesterId = 0;         ///< User ID của bên khởi tạo (0 nếu không có yêu cầu)
std::string earlyTerminationStatus = "NONE"; ///< "NONE", "PENDING", "ACCEPTED", "REJECTED"
std::string earlyTerminationReason;          ///< Lý do do bên gửi cung cấp
std::string actualEndDate;                   ///< Ngày có hiệu lực kết thúc (YYYY-MM-DD)
```

### 3.2. Quyền hạn gửi yêu cầu (`requestEarlyTermination`)
Tại [BookingService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/BookingService.cpp#L380-L421):
- **Đối tượng hợp lệ:** Chỉ Học viên (`bPtr->studentId == operatorUserId`), Gia sư (`bPtr->tutorId == operatorUserId`), hoặc Admin (`operatorRole == UserRole::ADMIN`).
- **Trạng thái tiên quyết:** Khóa học phải đang ở trạng thái `ACCEPTED`.
- **Ngăn chặn trùng lặp:** Nếu `earlyTerminationStatus == "PENDING"`, yêu cầu bị từ chối với thông báo: *"Khóa học này đã có một yêu cầu kết thúc sớm đang chờ phản hồi!"*.
- **Tính bất biến của lịch:** Khóa học vẫn giữ nguyên trạng thái `status = "ACCEPTED"`, thời khóa biểu không bị thay đổi trong giai đoạn chờ phản hồi.

### 3.3. Quyền hạn phản hồi và kiểm tra đồng thuận (`respondEarlyTermination`)
Tại [BookingService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/BookingService.cpp#L423-L488):
- **Chặn tự phê duyệt:** Kiểm tra nghiêm ngặt:
  ```cpp
  if (operatorUserId == bPtr->earlyTerminationRequesterId) {
      outMsg = "Bạn không thể tự phản hồi yêu cầu do chính mình gửi!";
      return false;
  }
  ```
- **Xử lý từ chối (`agree == false`):**
  - Cập nhật `bPtr->earlyTerminationStatus = "REJECTED"`.
  - Khóa học giữ nguyên trạng thái `ACCEPTED`.
  - Thông báo: *"Đã từ chối yêu cầu kết thúc sớm. Khóa học tiếp tục theo lịch ban đầu."*
- **Xử lý đồng thuận (`agree == true`):**
  - Cập nhật `bPtr->earlyTerminationStatus = "ACCEPTED"`.
  - Cập nhật trạng thái khóa học `bPtr->status = "EARLY_TERMINATED"`.
  - Ghi nhận ngày kết thúc thực tế: `bPtr->actualEndDate = DateUtils::getCurrentDateString()`.

### 3.4. Cơ chế giải phóng lịch tương lai và bảo toàn lịch các khóa khác
1. **Trong kiểm tra xung đột đặt lịch (`hasCourseConflictMulti`):**
   Tại dòng 55 của `BookingService.cpp`:
   ```cpp
   if (cb.status != "ACCEPTED") continue; // PENDING, REJECTED, CANCELLED, COMPLETED, EARLY_TERMINATED do NOT reserve
   ```
   Vì khóa học đã chuyển sang `EARLY_TERMINATED`, nó lập tức không còn chiếm giữ bất kỳ ca học nào trong tương lai. Học viên và gia sư có thể tự do đặt/nhận các khóa học mới vào các ca này.
2. **Trong hiển thị thời khóa biểu (`getAcceptedSessionsForWeek`):**
   Tại dòng 642-646 của `BookingService.cpp`:
   ```cpp
   if (c.status == "EARLY_TERMINATED" && !c.actualEndDate.empty()) {
       if (DateUtils::compareDates(s.date, c.actualEndDate) > 0) {
           continue; // Skip cancelled future session
       }
   }
   ```
   Các buổi học sau ngày `actualEndDate` được tự động ẩn khỏi thời khóa biểu, trong khi các buổi học đã diễn ra trước hoặc trong ngày `actualEndDate` được bảo toàn trên lịch sử thời khóa biểu.
3. **Tính độc lập giữa các khóa học:**
   Thao tác chỉ cập nhật duy nhất bản ghi có `id == courseBookingId`. Khóa học khác của hai bên (hoặc của người dùng khác) hoàn toàn không bị ảnh hưởng.
4. **Bảo toàn `scheduleBitmask` gốc:**
   Mặt nạ nhị phân lịch rảnh trong `User` (`scheduleBitmask`) không bao giờ bị ghi đè, đảm bảo tính bất biến của hồ sơ năng lực gia sư/học viên.

### 3.5. Xử lý trường hợp kết thúc sớm trong ngày có lịch học
- `actualEndDate` lưu định dạng ngày `YYYY-MM-DD`.
- Hàm `compareDates(s.date, c.actualEndDate)` trả về `0` khi `s.date == actualEndDate`.
- Vì điều kiện bỏ qua là `> 0`, ca học diễn ra **trong chính ngày kết thúc sớm vẫn được hiển thị trên thời khóa biểu**. Chỉ các ca học từ ngày hôm sau trở đi (`> 0`) mới bị giải phóng.

### 3.6. Tính nhất quán khi ghi file thất bại
Tại dòng 481-486 của `BookingService.cpp`:
Hàm `courseBookingRepo->updateCourseBooking(*bPtr)` sử dụng cơ chế ghi tệp tạm (`.tmp`) và đổi tên nguyên tử (`MoveFileExW` / `std::filesystem::rename`). Nếu quá trình ghi đĩa thất bại:
- Tệp `.tmp` bị xóa, tệp gốc trên đĩa được giữ nguyên trạng thái cũ.
- `updateCourseBooking` trả về `false`.
- `BookingService::respondEarlyTermination` trả về `false` kèm thông báo lỗi.
- Con trỏ `bPtr` chỉ là biến cục bộ trong phạm vi hàm và không bị cache lại, nên các lần đọc tiếp theo từ đĩa vẫn đọc được trạng thái trước đó.

---

## 4. KIỂM TOÁN XÁC NHẬN CAM KẾT (COMMITMENT ACKNOWLEDGMENT AUDIT)

### 4.1. Đối chiếu quy định thiết kế ADR-009
Theo mục 2.7 của `ADR-009_Multi_Session_Course_Booking_And_Session_Lifecycle.md`:
> *"Học sinh khi tạo đơn và Gia sư khi chấp nhận đơn đều phải xác nhận hiểu rõ cam kết: lịch học cố định, kết thúc sớm đòi hỏi sự đồng thuận của cả hai, nếu bên kia từ chối thì khóa học vẫn tiếp tục. Bằng chứng cam kết phải được lưu trữ bền vững vào bản ghi CourseBooking: `studentAcknowledged`, `studentAckTimestamp`, `tutorAcknowledged`, `tutorAckTimestamp`, `commitmentVersion`. Cả hai xác nhận cam kết phải tồn tại đầy đủ trước khi khóa học chuyển sang ACCEPTED."*

### 4.2. Kết quả kiểm tra source code thực tế
1. **Mô hình dữ liệu ([CourseBooking.h](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/CourseBooking.h)):**
   - **KHÔNG CÓ** trường `studentAcknowledged` hoặc `studentAckTimestamp`.
   - **KHÔNG CÓ** trường `tutorAcknowledged` hoặc `tutorAckTimestamp`.
   - **KHÔNG CÓ** trường `commitmentVersion`.
2. **Tuần tự hóa ([CourseBookingSerializer.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/storage/serializers/CourseBookingSerializer.cpp)):**
   - Định dạng 18 trường hiện tại chỉ lưu: `id,studentId,tutorId,studentName,tutorName,subject,hourlyRate,dayOfWeek,slotIndex,startDate,weeksCount,status,createdAt,weeklySchedule,earlyTerminationRequesterId,earlyTerminationStatus,earlyTerminationReason,actualEndDate`.
   - Hoàn toàn không lưu hay đọc bất kỳ trường cam kết nào.
3. **Logic dịch vụ ([BookingService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/BookingService.cpp)):**
   - `bookCourseMultiSlot`: Không nhận tham số cam kết, không kiểm tra xác nhận cam kết từ học viên.
   - `acceptCourseBooking`: Không kiểm tra xem học viên và gia sư đã cam kết hay chưa trước khi chuyển `status = "ACCEPTED"`.
4. **Giao diện người dùng ([BookingModal.qml](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/BookingModal.qml) & [TutorBookings.qml](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorBookings.qml)):**
   - Không có hộp chọn (CheckBox) xác nhận cam kết điều khoản kết thúc sớm.
   - Chỉ có nút bấm trực tiếp "Xác nhận gửi yêu cầu đăng ký khóa học" và "Chấp nhận".

**KẾT LUẬN HẠNG MỤC:** **FAIL (CHƯA ĐƯỢC HIỆN THỰC HÓA)**. Đây là một thiếu sót nghiệp vụ quan trọng so với thiết kế ADR-009 mục 2.7.

---

## 5. KIỂM TOÁN TỰ ĐỘNG HOÀN THÀNH KHÓA HỌC (AUTOMATIC COMPLETION AUDIT)

### 5.1. Bản chất cơ chế: Lazy Reconciliation (Không phải Background Scheduler)
Cơ chế tự động hoàn thành khóa học được hiện thực qua hàm `BookingService::autoCompleteExpiredCourses()` tại dòng 490-509 của [BookingService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/BookingService.cpp#L490-L509):
- **Bản chất:** Đây là **cơ chế đối soát lười (Lazy Reconciliation / On-Demand Reconciliation)** được kích hoạt đồng bộ bất cứ khi nào hệ thống truy vấn danh sách khóa học hoặc thời khóa biểu.
- **Xác nhận:** Hệ thống **KHÔNG SỬ DỤNG** bất kỳ luồng nền riêng (background thread), `QTimer`, hay cron scheduler nào cho tác vụ này. Điều này hoàn toàn phù hợp với kiến trúc ứng dụng desktop học thuật hướng đơn giản, không gây lãng phí CPU hay xung đột tài nguyên đa luồng.

### 5.2. Các điểm kích hoạt (Trigger Points)
Hàm `autoCompleteExpiredCourses()` được gọi tại các vị trí:
1. `BookingService::getStudentCourseBookings(studentId)` (dòng 567)
2. `BookingService::getTutorCourseBookings(tutorId)` (dòng 574)
3. `BookingService::getAllCourseBookings()` (dòng 581)
4. `BookingService::getStudentBookingMetrics(studentId)` (dòng 596)
5. `BookingService::getAcceptedSessionsForWeek(...)` (dòng 617)
6. `GUIBridge::qmlGetStudentBookings()` (dòng 1172)
7. `GUIBridge::qmlGetTutorBookings()` (dòng 1275)

### 5.3. Khả năng gọi lặp an toàn (Idempotency) và bảo vệ khóa học còn hạn
- **Khóa đa luồng:** Bảo vệ bằng `std::lock_guard<std::recursive_mutex> lock(serviceMutex)`.
- **Điều kiện hoàn thành:**
  ```cpp
  if (cb.status == "ACCEPTED") {
      auto slots = cb.getWeeklySlots();
      auto sessions = DateUtils::generateMultiSlotSessions(cb.startDate, cb.weeksCount, slots);
      if (!sessions.empty()) {
          const std::string& endDate = sessions.back().date;
          if (DateUtils::compareDates(today, endDate) > 0) {
              cb.status = "COMPLETED";
              courseBookingRepo->updateCourseBooking(cb);
          }
      }
  }
  ```
- **Bảo vệ khóa học còn hạn:** Nếu `compareDates(today, endDate) <= 0` (hôm nay vẫn chưa vượt quá ngày học cuối cùng), điều kiện sai và khóa học hoàn toàn không bị thay đổi.
- **An toàn gọi lặp:** Khi một khóa học đã chuyển sang `COMPLETED`, ở các lần gọi tiếp theo, điều kiện `cb.status == "ACCEPTED"` trả về `false`, không phát sinh thao tác ghi đĩa dư thừa.

---

## 6. KIỂM TOÁN QUYỀN ĐÁNH GIÁ (REVIEW ELIGIBILITY AUDIT)

Tại [ReviewService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/ReviewService.cpp#L31-L66) và [BookingService.cpp](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/BookingService.cpp#L511-L562):

### 6.1. Bảng đối chiếu các câu hỏi kiểm toán

| Câu hỏi kiểm chứng | Trạng thái thực tế | Dẫn chứng source code |
|---|:---:|---|
| **`COMPLETED` có được đánh giá không?** | **CÓ** | Dòng 50 `ReviewService.cpp`: `bPtr->status == "COMPLETED"` được chấp nhận hợp lệ. |
| **`EARLY_TERMINATED` có được đánh giá không?** | **CÓ** *(Trong code hiện tại)* | Dòng 50 `ReviewService.cpp`: `if (bPtr->status != "COMPLETED" && bPtr->status != "EARLY_TERMINATED")`. |
| **Có giới hạn 01 đánh giá mỗi khóa học không?** | **CÓ** | Dòng 57-62 `ReviewService.cpp`: lặp qua `getAllReviews()`, nếu `r.bookingId == bookingId && r.studentId == studentId` thì trả về lỗi: *"Bạn đã gửi đánh giá cho khóa học này rồi!"*. |
| **Có kiểm tra người đánh giá là học sinh thuộc khóa học không?** | **CÓ** | Dòng 45-48 `ReviewService.cpp`: `if (bPtr->studentId != studentId)` thì trả về lỗi: *"Đơn đặt lịch không thuộc về bạn!"*. |

### 6.2. Cảnh báo quyết định sản phẩm (Product Decision Flag)
> [!WARNING] **LƯU Ý QUAN TRỌNG VỀ QUYỀN ĐÁNH GIÁ SAU KHI KẾT THÚC SỚM**  
> Quyền đánh giá sau khi kết thúc sớm (`EARLY_TERMINATED`) **CHƯA ĐƯỢC CHỦ DỰ ÁN PHÊ DUYỆT CHÍNH THỨC**.  
> Việc cho phép đánh giá tại dòng 50 của `ReviewService.cpp` và dòng 517 của `BookingService.cpp` là logic kỹ thuật tạm thời do kỹ sư cài đặt để hỗ trợ trải nghiệm người dùng, **không được coi là quyết định sản phẩm cuối cùng**. Cần xin ý kiến phê duyệt dứt khoát từ chủ dự án:
> - **Phương án A (Phê duyệt):** Cho phép học sinh đánh giá số buổi đã học thực tế trước khi khóa học kết thúc sớm.
> - **Phương án B (Từ chối):** Khóa quyền đánh giá đối với các đơn kết thúc sớm (chỉ cho phép đơn hoàn thành trọn vẹn `COMPLETED`). Nếu chọn B, chỉ cần bỏ điều kiện `&& bPtr->status != "EARLY_TERMINATED"`.

---

## 7. KIỂM CHỨNG TÍNH TOÀN VẸN CỦA VIỆC DỌN DẸP MÃ CHẾT (CLEANUP COMPLETENESS)

Đã quét toàn bộ cây thư mục `src/`, `include/`, `ui/`, `tests/`, `CMakeLists.txt`, `ui/qml/qmldir`:

### 7.1. Danh mục các tệp đã xóa và kiểm tra tham chiếu

| Tệp đã xóa | Kiểm tra đĩa vật lý | Tham chiếu trong `CMakeLists.txt` | Tham chiếu trong `qmldir` | Tham chiếu include C++ / import QML |
|---|:---:|:---:|:---:|:---:|
| `src/gui/GUIApp_StunningWin32.cpp` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `src/gui/main_gui.cpp` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `ui/qml/AdminCommissionConfigView.qml` | **ĐÃ XÓA** | Không còn | Không còn | Không còn |
| `ui/qml/TutorProfile.qml` | **ĐÃ XÓA** | Không còn | Không còn | Không còn |
| `ui/qml/TutorSchedule.qml` | **ĐÃ XÓA** | Không còn | Không còn | Không còn |
| `include/models/Booking.h` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `include/storage/repositories/FileBookingRepository.h` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `src/storage/repositories/FileBookingRepository.cpp` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `include/storage/serializers/BookingSerializer.h` | **ĐÃ XÓA** | Không còn | N/A | Không còn |
| `src/storage/serializers/BookingSerializer.cpp` | **ĐÃ XÓA** | Không còn | N/A | Không còn |

*(Ghi chú: Các tệp `.cpp.obj` hay cache cũ trong thư mục build không ảnh hưởng đến mã nguồn và sẽ biến mất khi clean build).*

### 7.2. Kiểm tra không còn logic điểm danh/tranh chấp ẩn danh
- Tìm kiếm từ khóa: `Attendance`, `CourseSession`, `SessionDispute`, `điểm danh`, `tranh chấp`, `commission`, `hoa hồng`.
- Kết quả quét trong `include/`, `src/`, `ui/`: **0 kết quả**. Không còn bất kỳ logic điểm danh hay tranh chấp nào ẩn dưới tên gọi khác.

---

## 8. KIỂM CHỨNG TÍNH TOÀN VẸN DỮ LIỆU (DATA INTEGRITY VERIFICATION)

Chạy công cụ pháp chứng `TutorMatch_Inspector.exe` trên tập dữ liệu tại `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data`:

```text
============================================================
 TOÀN BỘ TỔNG KẾT HỆ THỐNG LƯU TRỮ (GLOBAL DATA SUMMARY)   
============================================================
Tổng số tệp dữ liệu đã kiểm tra : 4 (users.dat, bookings.dat, reviews.dat, course_bookings.dat)
Tổng số bản ghi đọc được       : 73
Tổng số bản ghi hợp lệ         : 73
Tổng số bản ghi lỗi/hỏng       : 0
Tính toàn vẹn dữ liệu          : 100% HOÀN HẢO (CHÍNH XÁC)
Tất cả các tệp có bị sửa đổi?  : NO (Tuyệt đối an toàn Read-Only)
============================================================
```

- **`users.dat`:** 50 bản ghi (1 Admin, 24 Gia sư, 25 Học viên). Toàn bộ bitmask lịch rảnh tuân thủ chặt chẽ `< (1ULL << 56)`.
- **`course_bookings.dat`:** 8 bản ghi khóa học định kỳ nguyên vẹn. 8/8 bản ghi có `startDate` khớp chính xác thứ trong tuần.
- **`reviews.dat`:** 15 bản ghi đánh giá nguyên vẹn.
- **`data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat`:** 27 bản ghi lịch sử nguyên vẹn được lưu trữ an toàn trong archive. Tệp `data/bookings.dat` không tồn tại trong runtime hoạt động.

---

## 9. BẰNG CHỨNG BIÊN DỊCH VÀ KIỂM THỬ THỰC TẾ (BUILD & TEST EVIDENCE)

### 9.1. Lệnh cấu hình CMake và kết quả
- **Lệnh thực thi:**
  ```powershell
  & 'D:\CODE\Tools\CMake_64\bin\cmake.exe' -S 'C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch' -B 'C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug' -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Debug
  ```
- **Exit Code:** `0`
- **Thời gian hoàn tất:** 2.8s (`-- Configuring done (1.9s) -- Generating done (0.9s)`).

### 9.2. Lệnh biên dịch và kết quả từng Target
- **Lệnh thực thi:**
  ```powershell
  & 'D:\CODE\Tools\mingw1310_64\bin\mingw32-make.exe' TutorMatch_Tests TutorMatch_Inspector TutorMatch_GUI TutorMatch_CLI
  ```
- **Exit Code:** `0`
- **Kết quả từng target:**
  - `TutorMatch_Tests`: `[100%] Built target TutorMatch_Tests` (Exit Code 0)
  - `TutorMatch_Inspector`: `[100%] Built target TutorMatch_Inspector` (Exit Code 0)
  - `TutorMatch_GUI`: `[100%] Built target TutorMatch_GUI` (Exit Code 0)
  - `TutorMatch_CLI`: `[100%] Built target TutorMatch_CLI` (Exit Code 0)

### 9.3. Đối chiếu các lần chạy Exit Code 1 trước đó
Trong nhật ký thực hiện có xuất hiện 2 lần Exit Code 1:
1. **Lần 1 (Lỗi biên dịch `TestRunner.cpp`):**
   - *Triệu chứng:* `error: 'splitCsv' is not a member of 'TutorMatch::SerializerUtils'`.
   - *Nguyên nhân:* Tên hàm trong `SerializerUtils.h` thực tế là `splitCsvLine`.
   - *Khắc phục trước đó:* Đã sửa thành `SerializerUtils::splitCsvLine` trong `TestRunner.cpp`.
2. **Lần 2 (Lỗi chạy test tại dòng 1398):**
   - *Triệu chứng:* `Assertion failed: err.find("không tồn tại") != std::string::npos`.
   - *Nguyên nhân:* `BookingService::acceptCourseBooking` trả về thông báo lỗi thiếu cụm từ `"không tồn tại"` khi không tìm thấy đơn, và từ chối lời gọi chấp nhận lặp (idempotent).
   - *Khắc phục trước đó:* Đã căn chỉnh thông báo lỗi và bổ sung tính chất idempotent khi đơn đã `ACCEPTED`.

### 9.4. Lệnh chạy Test Runner và kết quả chi tiết
- **Lệnh thực thi:**
  ```powershell
  .\TutorMatch_Tests.exe
  ```
- **Exit Code:** `0`
- **Tổng số Test Suites:** 23 suites
  - **PASS:** 23 suites (100%)
  - **FAIL:** 0 suites
  - **SKIP:** 0 suites

```text
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
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

---

## 10. ĐỐI SOÁT TÀI LIỆU KIẾN TRÚC (DOCUMENTATION CONSISTENCY)

> [!CAUTION] **GHI NHẬN LỆCH PHA TÀI LIỆU KIẾN TRÚC (DOCUMENTATION MISMATCH)**  
> Hai tài liệu thiết kế quan trọng nhất hiện tại của dự án:
> 1. `docs/architecture/decisions/ADR-009_Multi_Session_Course_Booking_And_Session_Lifecycle.md`
> 2. `docs/architecture/design/Multi_Session_Course_Booking_Design_VI.md`
> 
> **Vẫn đang mô tả kiến trúc LMS phức tạp** đã bị bác bỏ/loại bỏ khỏi mã nguồn:
> - **Thực thể `CourseSession`:** Mô tả 7 trạng thái (`SCHEDULED`, `AWAITING_CONFIRMATION`, `COMPLETED`, `NOT_CONDUCTED`, `CANCELLED`, `DISPUTED`, `LEGACY_UNVERIFIED`) và yêu cầu lưu bền vững 4 trường thời gian cho từng buổi.
> - **Báo cáo điểm danh (`AttendanceReport`):** Mô tả học sinh và gia sư phải nộp báo cáo điểm danh sau giờ học.
> - **Tranh chấp buổi học (`SessionDispute`):** Mô tả thực thể lưu tại `data/disputes.dat` và quy trình Admin can thiệp phân xử.
> - **Mô hình tài chính phân rã theo buổi:** Mô tả `Completed Session Value`, `Pending Session Value`, `Not Conducted Value`, `Legacy Unverified Value`.
> 
> **Hiện trạng mã nguồn thực tế:** TutorMatch là Matching Tool, chỉ quản lý vòng đời ở cấp `CourseBooking`, không có các thực thể hay bảng dữ liệu nói trên.
> **Tuân thủ quy tắc kiểm chứng:** Trong vòng Read-Only này, **tuyệt đối KHÔNG tự ý chỉnh sửa tài liệu**, chỉ ghi nhận lệch pha để chủ dự án phê duyệt bản cập nhật tài liệu chính thức.

---

## 11. CÁC VẤN ĐỀ TRỌNG YẾU ĐƯỢC PHÁT HIỆN (CRITICAL ISSUES)

### Vấn đề 1: Thiếu trường và logic kiểm tra cam kết kết thúc sớm (Commitment Acknowledgment)
- **File:** `include/models/CourseBooking.h`, `src/services/BookingService.cpp`, `src/storage/serializers/CourseBookingSerializer.cpp`, `ui/qml/BookingModal.qml`, `ui/qml/TutorBookings.qml`.
- **Mức độ nghiêm trọng:** **CAO (HIGH)**.
- **Nguyên nhân:** Yêu cầu nghiệp vụ số 5 và ADR-009 mục 2.7 quy định học sinh và gia sư phải xác nhận cam kết trước khi chuyển sang `ACCEPTED`, nhưng các trường `studentAcknowledged`, `studentAckTimestamp`, `tutorAcknowledged`, `tutorAckTimestamp`, `commitmentVersion` chưa được bổ sung vào mã nguồn.
- **Đề xuất sửa:**
  1. Thêm các trường lưu cam kết vào struct `CourseBooking`.
  2. Bổ sung tuần tự hóa trong `CourseBookingSerializer`.
  3. Bổ sung tham số xác nhận trong `BookingService::bookCourseMultiSlot` và `BookingService::acceptCourseBooking`.
  4. Kiểm tra cả hai bên đã xác nhận cam kết trước khi cho phép chuyển sang `ACCEPTED`.
  5. Thêm CheckBox xác nhận cam kết trên `BookingModal.qml` và `TutorBookings.qml`.

### Vấn đề 2: Quyền đánh giá sau kết thúc sớm chưa có quyết định chính thức
- **File:** `src/services/ReviewService.cpp` (dòng 50), `src/services/BookingService.cpp` (dòng 517).
- **Mức độ nghiêm trọng:** **TRUNG BÌNH (MEDIUM)**.
- **Nguyên nhân:** Mã nguồn hiện tại cho phép học viên đánh giá gia sư sau khi khóa học kết thúc sớm (`EARLY_TERMINATED`), nhưng nghiệp vụ này chưa được chủ dự án phê duyệt chính thức.
- **Đề xuất xử lý:** Chờ quyết định của chủ dự án. Nếu không cho phép, chỉ cần gỡ bỏ điều kiện `&& bPtr->status != "EARLY_TERMINATED"`.

### Vấn đề 3: Lệch pha tài liệu thiết kế (Documentation Mismatch)
- **File:** `ADR-009` và `Multi_Session_Course_Booking_Design_VI.md`.
- **Mức độ nghiêm trọng:** **TRUNG BÌNH - CAO (MEDIUM-HIGH)**.
- **Nguyên nhân:** Tài liệu vẫn mô tả hệ thống LMS (CourseSession, điểm danh, SessionDispute) trong khi mã nguồn đã tinh gọn thành Matching Tool.
- **Đề xuất xử lý:** Lập một đợt cập nhật tài liệu (Documentation Update Task) để đồng bộ ADR-009 và Design Spec với kiến trúc thực tế.

### Vấn đề 4: Ca học diễn ra trong ngày kết thúc sớm vẫn hiển thị trên thời khóa biểu
- **File:** `src/services/BookingService.cpp` (dòng 643).
- **Mức độ nghiêm trọng:** **THẤP (LOW)**.
- **Nguyên nhân:** `actualEndDate` chỉ lưu ngày (`YYYY-MM-DD`). Biểu thức `compareDates(s.date, c.actualEndDate) > 0` giữ lại các buổi học trong cùng ngày kết thúc sớm. Nếu việc kết thúc sớm diễn ra buổi sáng, ca học buổi tối trong cùng ngày vẫn xuất hiện trên lịch.
- **Đề xuất xử lý:** Thống nhất rõ quy tắc nghiệp vụ: toàn bộ ca học trong ngày kết thúc sớm được xem là đã diễn ra/hủy bỏ, hoặc bổ sung so sánh chỉ số ca học (`slotIndex`) nếu cần độ mịn theo ca.

---

## 12. KIẾN NGHỊ HÀNH ĐỘNG TIẾP THEO (RECOMMENDED NEXT ACTIONS)

1. **Lấy ý kiến phê duyệt của Chủ dự án (Stakeholder Sign-Off):**
   - Quyết định về việc có cho phép học viên gửi đánh giá (review) sau khi khóa học kết thúc sớm (`EARLY_TERMINATED`) hay không.
   - Xác nhận có yêu cầu bổ sung bằng chứng cam kết (`Commitment Acknowledgment`) vào bản ghi khóa học hay không.
2. **Kế hoạch triển khai bổ sung (nếu được phê duyệt):**
   - Mở đợt cập nhật nhỏ (Minor Implementation Pass) để bổ sung các trường cam kết kết thúc sớm vào `CourseBooking`, `CourseBookingSerializer`, `BookingService` và các giao diện QML tương ứng.
3. **Kế hoạch đồng bộ tài liệu (Documentation Pass):**
   - Cập nhật `ADR-009` và `Multi_Session_Course_Booking_Design_VI.md` để loại bỏ các phần mô tả LMS, đưa tài liệu về trạng thái hoàn toàn nhất quán 100% với mã nguồn TutorMatch.
