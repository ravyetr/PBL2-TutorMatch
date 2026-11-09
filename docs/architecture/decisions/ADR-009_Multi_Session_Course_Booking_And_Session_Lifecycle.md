# ARCHITECTURE DECISION RECORD (ADR)
## ADR-009: Kiến Trúc Khóa Học Nhiều Buổi/Tuần, Cam Kết Kết Thúc Sớm và Quyền Đánh Giá Trên Sàn Kết Nối
### (Multi-Session Recurring Course Booking, Commitment Acknowledgment & Review Eligibility on Matching Platform)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-009
- **Tiêu đề:** Kiến Trúc Khóa Học Nhiều Buổi/Tuần, Cam Kết Kết Thúc Sớm và Quyền Đánh Giá Trên Sàn Kết Nối
- **Trạng thái:** **ACCEPTED** *(Đã phê duyệt và triển khai hoàn tất 100%)*
- **Ngày lập:** 2026-11-09 (Cập nhật chuẩn hóa theo phạm vi Sàn kết nối gia sư — Loại bỏ triệt để LMS)
- **Tác giả:** Principal Software Architect, Senior Domain Modeler & Data Integrity Reviewer
- **Tài liệu thiết kế chi tiết tham chiếu:** `docs/architecture/design/Multi_Session_Course_Booking_Design_VI.md`
- **Phạm vi tác động:** `include/models/CourseBooking.h`, `include/services/BookingService.h`, `include/services/ReviewService.h`, `src/storage/serializers/CourseBookingSerializer.cpp`, `src/gui/GUIBridge.cpp`, `ui/qml/BookingModal.qml`, `ui/qml/TutorBookings.qml`, `tests/TestRunner.cpp`.

---

### 1. BỐI CẢNH (CONTEXT)

1. **Hạn chế của mô hình đơn ca ban đầu:**
   - Phiên bản ban đầu của `CourseBooking` gắn cứng với một cặp `dayOfWeek` (0..6) và `slotIndex` (0..7), chỉ cho phép đặt đúng 1 buổi mỗi tuần trong suốt 4, 8 hoặc 12 tuần.
   - Nhu cầu thực tế của học viên và gia sư đòi hỏi học nhiều buổi mỗi tuần (ví dụ: Thứ Hai và Thứ Sáu, hoặc nhiều ca trong tuần) với thời lượng khóa học linh hoạt.
2. **Loại bỏ tư duy LMS overengineering:**
   - Trong các phiên bản thảo luận trước, đã có đề xuất thêm thực thể buổi học riêng (`CourseSession`), báo cáo điểm danh từng buổi (`Attendance Confirmation`), tranh chấp điểm danh (`SessionDispute`), phân xử của Quản trị viên (`Admin Arbitration`), và phân bổ tài chính per-session (`Completed Session Value`, `Pending Session Value`, `Legacy Unverified Value`).
   - Sau khi rà soát phạm vi sản phẩm thực tế của TutorMatch (dự án C++17, Qt 6.11, QML, lưu trữ file phẳng), chủ dự án đã chốt dứt khoát: **TutorMatch là Sàn kết nối tìm kiếm & thỏa thuận lịch học giữa gia sư và học sinh (Matching & Scheduling Platform), KHÔNG PHẢI là hệ thống quản trị học tập (LMS).**
   - Nền tảng không thu hoa hồng, không giữ tiền học phí, không tích hợp cổng thanh toán trực tuyến, và không có nghiệp vụ giám sát điểm danh từng buổi học ngoài đời thực.
3. **Yêu cầu bảo vệ cam kết và đồng thuận kết thúc sớm:**
   - Để tránh việc một bên tự ý hủy ngang gây thiệt hại lịch học của bên còn lại, khóa học cần có cam kết trách nhiệm rõ ràng trước khi có hiệu lực (`ACCEPTED`).
   - Kết thúc sớm phải dựa trên cơ chế **đồng thuận hai bên (mutual consent)**.
   - Quyền đánh giá gia sư phải mở cho học viên khi khóa học `COMPLETED` (hoàn thành) hoặc `EARLY_TERMINATED` (kết thúc sớm có thỏa thuận).

---

### 2. QUYẾT ĐỊNH KIẾN TRÚC (ARCHITECTURAL DECISIONS)

Chúng tôi quyết định chuẩn hóa các nguyên tắc kiến trúc sau:

#### 2.1. Cấu Trúc Khóa Học Nhiều Buổi/Tuần & Sinh Lịch Tất Định
1. **Lịch tuần linh hoạt (`weeklySchedule`):**
   - Khóa học cho phép chọn từ 1 đến 56 ca học trong tuần (7 ngày $\times$ 8 ca), chuỗi tuần `weeksCount` $\in \{4, 8, 12\}$.
   - Lưu trữ dạng chuỗi chuẩn hóa: `"dow:slot;dow:slot"` (ví dụ: `"0:0;2:0;4:0"` là Thứ Hai, Thứ Tư, Thứ Sáu ca 0).
   - Duy trì `dayOfWeek` và `slotIndex` đại diện cho ca học chính đầu tiên để phục vụ tương thích ngược.
2. **Sinh lịch tất định (Deterministic Schedule Generation):**
   - Tổng số buổi học của khóa học: $N = k \times \text{weeksCount}$ (với $k$ là số ca học trong tuần).
   - Với mỗi ca $(w_j, s_j)$, ngày học đầu tiên $d_0(w_j)$ là ngày đầu tiên $\ge \text{startDate}$ có thứ trong tuần bằng $w_j$:
     $$\Delta = (w_j - \text{dayOfWeek}(\text{startDate}) + 7) \pmod 7$$
     $$d_0(w_j) = \text{addDays}(\text{startDate}, \Delta)$$
   - Các buổi tiếp theo cách nhau 7 ngày: $d_i = \text{addDays}(d_0(w_j), i \times 7)$ ($i = 0 \dots \text{weeksCount}-1$).
   - Toàn bộ $N$ buổi được sắp xếp theo thời gian tăng dần (`scheduledDate`, `slotIndex`).
   - `endDate` được xác định chính xác là ngày của buổi học cuối cùng $d_{N-1}$.
3. **Chuẩn hóa 8 Khung Giờ Ca Học (90 phút/ca):**
   - Ca 0: 07:30 - 09:00 | Ca 1: 09:15 - 10:45 | Ca 2: 13:30 - 15:00 | Ca 3: 15:15 - 16:45
   - Ca 4: 17:00 - 18:30 | Ca 5: 18:45 - 20:15 | Ca 6: 20:30 - 22:00 | Ca 7: 22:00 - 23:30

#### 2.2. Cơ Chế Giữ Chỗ & Bất Biến Mặt Nạ Nhị Phân (Schedule Bitmask)
1. **Bảo toàn Mặt nạ rảnh/bận (`scheduleBitmask`):**
   - `scheduleBitmask` của Gia sư và Học viên phản ánh khung giờ rảnh cá nhân cố định trong tuần.
   - Khi tạo hoặc chấp nhận khóa học, `scheduleBitmask` **KHÔNG BỊ ghi đè hay trừ bit**.
2. **Khóa lịch động theo trạng thái CourseBooking:**
   - Trạng thái `PENDING`: **KHÔNG GIỮ LỊCH**. Nhiều học viên có thể cùng gửi yêu cầu vào một ca học.
   - Trạng thái `ACCEPTED`: **KHÓA LỊCH TOÀN BỘ** các ngày học thực tế của khóa học. Khi gia sư chấp nhận đơn A, hệ thống ngăn chặn mọi đơn khác trùng bất kỳ buổi học nào trong khoảng thời gian có hiệu lực.
   - Trạng thái `CANCELLED`, `REJECTED`, `EARLY_TERMINATED`, `COMPLETED`: **GIẢI PHÓNG LỊCH HOÀN TOÀN**. Khung giờ lập tức mở lại cho các yêu cầu mới.

#### 2.3. Bằng Chứng Cam Kết Bền Vững (Commitment Acknowledgment - COMMITMENT_V1)
1. **Nguyên tắc bắt buộc hai bên cùng xác nhận:**
   - Để khóa học chuyển sang trạng thái `ACCEPTED`, cả học viên và gia sư đều phải xác nhận đã hiểu và đồng ý với điều khoản cam kết (`CURRENT_COMMITMENT_VERSION = "COMMITMENT_V1"`).
   - Học viên xác nhận khi tạo đơn đăng ký khóa học trên `BookingModal.qml`.
   - Gia sư xác nhận trước khi chấp nhận đơn trên `TutorBookings.qml`.
   - Gia sư có thể từ chối (`REJECT`) đơn mà **KHÔNG CẦN** xác nhận cam kết.
2. **Ràng buộc an toàn tại Backend:**
   - Backend từ chối tạo booking nếu học viên chưa xác nhận (`studentAcknowledged == false`).
   - Backend từ chối chấp nhận booking nếu gia sư chưa xác nhận hoặc học viên chưa xác nhận.
   - Client không thể tự gửi timestamp giả mạo; timestamp được sinh độc quyền bởi backend theo chuẩn ISO (`DateUtils::getCurrentTimestampString()`).
   - Không thể bổ sung xác nhận cam kết nếu đơn không ở trạng thái `PENDING`.
3. **Các trường lưu trữ trên `CourseBooking`:**
   - `studentAcknowledged` (bool) & `studentAckTimestamp` (string: `YYYY-MM-DD HH:MM:SS`)
   - `tutorAcknowledged` (bool) & `tutorAckTimestamp` (string: `YYYY-MM-DD HH:MM:SS`)
   - `commitmentVersion` (string: `"COMMITMENT_V1"`)

#### 2.4. Kết Thúc Sớm Theo Cơ Chế Đồng Thuận (Consensual Early Termination)
1. **Quy trình đồng thuận:**
   - Một bên (Học viên hoặc Gia sư) có thể gửi yêu cầu kết thúc sớm kèm lý do.
   - Khóa học ghi nhận `earlyTerminationStatus = "PENDING"`, `earlyTerminationRequesterId`, `earlyTerminationReason`. Khóa học vẫn giữ `status = "ACCEPTED"` và tiếp tục diễn ra.
   - Nếu bên còn lại **ĐỒNG Ý**: khóa học chuyển `status = "EARLY_TERMINATED"`, `earlyTerminationStatus = "ACCEPTED"`, ghi nhận `actualEndDate`, và giải phóng toàn bộ các ca học tương lai.
   - Nếu bên còn lại **TỪ CHỐI**: `earlyTerminationStatus = "REJECTED"`, khóa học tiếp tục diễn ra bình thường theo lịch `ACCEPTED`.
2. **Quyền can thiệp Quản trị viên:**
   - Quản trị viên (`ADMIN`) có quyền can thiệp chấm dứt khóa học trong trường hợp bất khả kháng (`qmlAdminTerminateMatch` / `terminateCourseBooking`).

#### 2.5. Quyền Đánh Giá Khóa Học Chuẩn Hóa
1. **Điều kiện đủ để đánh giá (`ReviewService::isEligibleForReview`):**
   - Khóa học ở trạng thái `COMPLETED`: **HỢP LỆ (ELIGIBLE)**.
   - Khóa học ở trạng thái `EARLY_TERMINATED`: **HỢP LỆ (ELIGIBLE)**.
   - Khóa học ở trạng thái `PENDING`, `ACCEPTED`, `REJECTED`, `CANCELLED`: **KHÔNG HỢP LỆ (INELIGIBLE)**.
2. **Quy tắc toàn vẹn:**
   - Mỗi học viên chỉ được đánh giá tối đa 01 lần cho mỗi khóa học (`bookingId`).
   - Người gửi đánh giá bắt buộc phải là chính học viên của khóa học (`studentId == bPtr->studentId`).
   - Không đòi hỏi bất kỳ điểm danh hay thực thể `CourseSession` nào.

#### 2.6. Định Dạng Lưu Trữ & Tính Tương Thích Ngược Tuyệt Đối
1. **Lưu trữ CSV 23 trường trong `data/course_bookings.dat`:**
   `id, studentId, tutorId, studentName, tutorName, subject, hourlyRate, dayOfWeek, slotIndex, startDate, weeksCount, status, createdAt, weeklySchedule, earlyTerminationRequesterId, earlyTerminationStatus, earlyTerminationReason, actualEndDate, studentAcknowledged, studentAckTimestamp, tutorAcknowledged, tutorAckTimestamp, commitmentVersion`
2. **Tương thích ngược tự nhiên:**
   - Bản ghi cũ 13 trường: được tự động phân giải thành khóa học 1 ca, `studentAcknowledged = false`, `tutorAcknowledged = false`.
   - Bản ghi 18 trường (multi-slot chưa cam kết): giữ nguyên tính toàn vẹn.
   - Bộ giải mã `CourseBookingSerializer::fromCsv` bảo đảm không phát sinh ngoại lệ, không làm mất dữ liệu sản xuất.

---

### 3. HẬU QUẢ VÀ ĐÁNH GIÁ (CONSEQUENCES)

#### Ưu điểm:
- **Đúng bản chất Sàn kết nối:** Loại bỏ hoàn toàn phức tạp dư thừa của LMS (không session, không điểm danh, không tranh chấp, không chia nhỏ tài chính theo buổi).
- **Trải nghiệm rõ ràng, minh bạch:** Cả học viên và gia sư đều nắm rõ cam kết ngay từ lúc gửi và duyệt yêu cầu.
- **Dữ liệu vững chắc:** Định dạng dữ liệu nhất quán, có kiểm chứng 100% bằng 24 Test Suites và công cụ Data Inspector.
- **Không overengineering:** Dễ bảo trì, dễ kiểm thử, lý tưởng cho đồ án tốt nghiệp và môi trường bảo vệ luận văn.

#### Hạn chế & Giám sát:
- Không giám sát được việc gia sư và học viên có thực sự gặp nhau ngoài đời thực hay không (đúng tôn chỉ của sàn kết nối trung gian).
- Khóa học cũ dạng 13 trường muốn chuyển sang `ACCEPTED` cần được học viên và gia sư xác nhận cam kết bổ sung qua API quy định.

---

### 4. TRẠNG THÁI KIỂM CHỨNG (VERIFICATION STATUS)
- Toàn bộ 24 Test Suites trong `TutorMatch_Tests` đều **PASS 100%**.
- Tệp dữ liệu `data/course_bookings.dat` gồm 8 bản ghi sản xuất được bảo toàn nguyên vẹn, zero corruption.
- Cả 4 targets (`TutorMatch_GUI`, `TutorMatch_CLI`, `TutorMatch_Tests`, `TutorMatch_Inspector`) biên dịch thành công không cảnh báo.
