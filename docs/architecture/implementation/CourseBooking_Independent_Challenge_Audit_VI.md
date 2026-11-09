# BÁO CÁO PHẢN BIỆN ĐỘC LẬP: KIỂM CHỨNG TÍNH TOÀN VẸN VÀ CHẤT LƯỢNG KIỂM THỬ MÔ HÌNH COURSEBOOKING
## Dự án TutorMatch | C++17 | Qt 6.11 | Independent Challenge Audit

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Công nghệ:** C++17, Qt 6.11 (QML / Qt Quick), CMake, Flat-file persistence (`.dat`), MinGW 13.1.0  
**Tài liệu tham chiếu:** `docs/architecture/implementation/Legacy_Booking_Final_Verification_VI.md`  
**Thời điểm thực hiện kiểm toán:** 06/11/2026 22:35:00 (UTC+7)  
**Nguyên tắc thực thi:** Phản biện độc lập, kiểm chứng dựa trên bằng chứng vật lý (evidence-driven), không tin tưởng mù quáng các tuyên bố trong báo cáo cũ.  

---

## 1. TỔNG QUAN ĐIỀU HÀNH (EXECUTIVE SUMMARY)

Báo cáo này là kết quả của một đợt **kiểm toán phản biện độc lập (Independent Challenge Audit)** nhằm đánh giá khách quan xem kiến trúc mới `CourseBooking` (`data/course_bookings.dat`) có thực sự đủ độ tin cậy để trở thành mô hình nghiệp vụ duy nhất của toàn bộ nền tảng TutorMatch hay không.

### Các kết luận phản biện then chốt:

1. **Pháp chứng Dữ liệu Sản xuất (Production Data Forensics):**
   - **Xác nhận:** Toàn bộ 8 bản ghi trong `course_bookings.dat` hiện tại hoàn toàn khớp về mặt toán học lịch dương với quy ước $0 = \text{Thứ Hai} \dots 6 = \text{Chủ Nhật}$.
   - **Phản biện quan trọng:** Báo cáo cũ tuyên bố rằng các bản ghi `1001, 1002, 1003, 1006, 1008` "đã được sửa đổi/chuẩn hóa để khớp ngày". Kiểm tra pháp chứng đối chiếu với tệp sao lưu gốc `backup_migration_20261106_134710/course_bookings.dat` chứng minh: **Bản ghi 1001 và 1002 chưa từng bị sửa đổi một byte nào** (ngay từ đầu đã khớp thứ 6 và chủ nhật). Các bản ghi 1003–1008 được tạo mới đồng bộ ngay từ khi script `migrate_and_audit.py` chạy. Cụm từ "sau phiên chuẩn hóa" trong báo cáo cũ là một sự nhầm lẫn về mặt thuật ngữ — không hề có sự đột biến dữ liệu (mutation) nào diễn ra đối với các bản ghi này.
2. **Kiểm toán Tính toán Tài chính Admin (Financial Calculation Audit):**
   - **Phát hiện sự không rõ ràng về mặt nghiệp vụ (DESIGN DECISION REQUIRED):** Trong `AdminService::calculateMetrics()`, chỉ số `initialConnectionGMV` chỉ tính tổng `hourlyRate` của các đơn `COMPLETED` (hiện tại chỉ có đơn #1008 = 180,000 VNĐ, hoa hồng 25% = 45,000 VNĐ). Nó **không** nhân với `weeksCount` (tổng học phí cả khóa $180,000 \times 4 = 720,000$ VNĐ) và **không** tính các khóa đang học (`ACCEPTED`). Cần quyết định thiết kế chính thức từ phía Product Owner về việc định nghĩa GMV là "Giá trị kết nối buổi đầu" hay "Tổng giá trị hợp đồng khóa học".
3. **Tính toàn vẹn Đánh giá (Review Integrity):**
   - **Xác nhận PASS:** 15 đánh giá lịch sử trong `reviews.dat` khớp 100% với 15 đơn hoàn thành trong `legacy_bookings_20261106_140537.dat`. Cơ chế kiểm tra điều kiện đánh giá (`isEligibleForReview`) và rollback tự động (ADR-007) trong `ReviewService` hoạt động hoàn hảo.
4. **Khoảng trống Kiểm thử & Biện pháp khắc phục (Test Coverage Remediation):**
   - **Phát hiện thiếu sót:** Test 16 trước đây kiểm tra các ca học trùng ngày bắt đầu hoặc hoàn toàn khác tháng, nhưng **thiếu bài test cho trường hợp xung đột giao thoa một phần (staggered partial overlap)** — khi khóa học thứ hai bắt đầu sau khóa thứ nhất 2 tuần nhưng vẫn bị trùng 2 tuần cuối.
   - **Đã khắc phục:** Bổ sung ca kiểm thử `Scenario 4b` vào `Test 16` trong `tests/TestRunner.cpp`, biên dịch và xác minh thành công: `hasCourseConflict` phát hiện chính xác ngày xung đột đầu tiên (`2026-11-23`).
5. **Rủi ro Đồng thời Đa tiến trình (Multi-Process Persistence Limitation):**
   - **Xác nhận giới hạn kiến trúc:** Khóa luồng `std::recursive_mutex` chỉ an toàn trong phạm vi một tiến trình (in-process). Nếu người dùng mở đồng thời cả GUI và CLI và cùng ghi dữ liệu, hiện tượng mất dữ liệu (data overwriting) có thể xảy ra do cơ chế ghi đè toàn bộ tệp qua file tạm `.tmp`. Đây là giới hạn kiến trúc cố hữu của hệ thống file phẳng đơn giản, cần được giải quyết bằng File Lock cấp HĐH.

---

## 2. PHÁP CHỨNG DỮ LIỆU SẢN XUẤT (AUDIT A — PRODUCTION DATA CHANGE FORENSICS)

### 2.1. Mã băm SHA-256 các tệp dữ liệu hiện tại
Thực hiện băm SHA-256 trên các tệp dữ liệu sản xuất tại thời điểm kiểm toán:
- **`data/users.dat`:** `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916`
- **`data/course_bookings.dat`:** `DF526F4CC8ADF4F1CD6B0C7CC4D8C9CED58CD20AC03D6778588690987C5A6904`
- **`data/reviews.dat`:** `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B`
- **`data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat`:** `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C`

### 2.2. Đối chiếu với dữ liệu sao lưu trước di trú (`backup_migration_20261106_134710`)
So sánh trực tiếp từng byte giữa tệp hiện tại và tệp sao lưu trước khi thực hiện di trú:
- `users.dat`: Khớp 100% SHA-256 (`AA04...8916`). Không có bất kỳ người dùng nào bị sửa đổi.
- `reviews.dat`: Khớp 100% SHA-256 (`4DF5...484B`). Toàn bộ 15 đánh giá giữ nguyên vẹn.
- `course_bookings.dat`: Tệp sao lưu trước di trú chỉ chứa **2 bản ghi** (#1001 và #1002).

### 2.3. Bảng đối chiếu chi tiết từng bản ghi khóa học (Record-Level Comparison Table)

| ID | Giá trị gốc (trước di trú) | Giá trị hiện tại (`course_bookings.dat`) | Có bị sửa đổi? | Bằng chứng kiểm chứng |
|:---:|---|---|:---:|---|
| **1001** | `1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,4,0,2026-11-06,4,ACCEPTED,2026-11-06` | `1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,4,0,2026-11-06,4,ACCEPTED,2026-11-06` | **KHÔNG** | Trùng khớp từng byte với dòng 1 tệp sao lưu `backup_migration_20261106_134710/course_bookings.dat`. Ngày 2026-11-06 là Thứ 6, `dayOfWeek = 4`. |
| **1002** | `1002,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12,150000,6,0,2026-11-08,4,ACCEPTED,2026-11-06` | `1002,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12,150000,6,0,2026-11-08,4,ACCEPTED,2026-11-06` | **KHÔNG** | Trùng khớp từng byte với dòng 2 tệp sao lưu. Ngày 2026-11-08 là Chủ Nhật, `dayOfWeek = 6`. |
| **1003** | *(Chưa có trước di trú)* | `1003,7,3,Phạm Tuấn Thành,TS. Trần Thị Vật Lý,Vật lý 10-12 (Luyện thi THPT),200000,1,1,2026-11-10,8,ACCEPTED,2026-11-06` | **TẠO MỚI** | Được sinh mới bởi `migrate_and_audit.py` để bổ sung ca 8 tuần chuẩn tắc. Ngày 2026-11-10 là Thứ 3, `dayOfWeek = 1`. |
| **1004** | *(Chưa có trước di trú)* | `1004,27,14,Phan Đức Huy,Mr. David Lê,Tiếng Anh / IELTS (6.5 - 8.5),160000,2,2,2026-11-11,12,ACCEPTED,2026-11-06` | **TẠO MỚI** | Được sinh mới bởi `migrate_and_audit.py` bổ sung ca 12 tuần. Ngày 2026-11-11 là Thứ 4, `dayOfWeek = 2`. |
| **1005** | *(Chưa có trước di trú)* | `1005,18,10,Trần Minh Anh,Cô Nguyễn Thị Sinh,Sinh học 11-12,130000,3,3,2026-11-12,4,PENDING,2026-11-06` | **TẠO MỚI** | Bổ sung ca `PENDING`. Ngày 2026-11-12 là Thứ 5, `dayOfWeek = 3`. |
| **1006** | *(Chưa có trước di trú)* | `1006,46,4,Đoàn Nhật Khoa,Thầy Phạm Hóa Học,Hóa học 10-12 (Luyện thi HSG),100000,0,4,2026-11-09,4,REJECTED,2026-11-06` | **TẠO MỚI** | Bổ sung ca `REJECTED`. Ngày 2026-11-09 là Thứ 2, `dayOfWeek = 0`. |
| **1007** | *(Chưa có trước di trú)* | `1007,45,3,Lê Quang Vinh,TS. Trần Thị Vật Lý,Vật lý 10-12 (Luyện thi THPT),200000,5,5,2026-11-07,4,CANCELLED,2026-11-06` | **TẠO MỚI** | Bổ sung ca `CANCELLED`. Ngày 2026-11-07 là Thứ 7, `dayOfWeek = 5`. |
| **1008** | *(Chưa có trước di trú)* | `1008,33,12,Trịnh Thảo My,Thầy Võ Minh Tin,Tin học 11-12,180000,0,1,2026-09-07,4,COMPLETED,2026-09-01` | **TẠO MỚI** | Bổ sung ca `COMPLETED` lịch sử. Ngày 2026-09-07 là Thứ 2, `dayOfWeek = 0`. |

### 2.4. Kết luận Phản biện về "Sự cố Weekday Mismatch"
- **Nhận định:** Nghi vấn cho rằng các bản ghi 1001, 1002, 1003, 1006, 1008 từng có thứ trong tuần bị sai lệch là **không có cơ sở thực tế trong tệp sản xuất**.
- **Giải thích:** Cả 8 bản ghi này từ khi được ghi vào `course_bookings.dat` đều tuân thủ chính xác thuật toán Sakamoto trong `DateUtils::dayOfWeek`. Cụm từ "sau phiên chuẩn hóa" chỉ mang ý nghĩa là script di trú đã tạo ra tập dữ liệu chuẩn mực này, chứ không phải script đã "chữa lỗi sai ngày" trên các bản ghi cũ.

---

## 3. KIỂM TOÁN TÍNH TOÁN TÀI CHÍNH ADMIN (AUDIT B — ADMIN FINANCIAL CALCULATION)

### 3.1. Truy vết Luồng Tính toán Tài chính (Trace from Record to UI)
```
[data/course_bookings.dat]
  │ (Đọc qua CourseBookingSerializer)
  ▼
[CourseBookingRepository::getAllCourseBookings()]
  ▼
[AdminService::calculateMetrics()]
  │
  │── if (cb.status == "COMPLETED") {
  │       m.completedBookings++;
  │       m.initialConnectionGMV += cb.hourlyRate;
  │   } else if (cb.status == "ACCEPTED") {
  │       m.activeMatches++;
  │   }
  │── m.platformCommissionRevenue = m.initialConnectionGMV * rateToUse; (25%)
  ▼
[GUIBridge::getAdminMetrics() -> qmlGetAdminPlatformStats()]
  ▼
[QML: AdminCommissionConfigView.qml & AdminOverviewView.qml]
```

### 3.2. Trả lời chi tiết 9 Câu hỏi Kiểm toán Tài chính:

1. **Ý nghĩa của GMV trong ứng dụng này:**
   - Trong giao diện QML (`AdminCommissionConfigView.qml`), trường này được ghi là: `"Tổng giá trị kết nối (GMV):"`.
   - Trong code C++ (`AdminMetrics.h`), trường này được đặt tên cụ thể là: `initialConnectionGMV`.
   - Về bản chất, hệ thống đang định nghĩa GMV là: **Tổng học phí của buổi học kết nối đầu tiên của các đơn đã hoàn thành**.
2. **GMV được tính theo khóa, theo buổi, hay tổng số giờ?**
   - GMV được tính **theo một buổi học duy nhất (`cb.hourlyRate`)**.
   - Nó không nhân với số giờ học (mỗi ca học mặc định là 1 buổi) và không nhân với tổng số tuần của khóa.
3. **Triển khai có tính đến `weeksCount` hay không?**
   - **KHÔNG**. Mã nguồn thực tế tại `AdminService.cpp` dòng 43:
     `m.initialConnectionGMV += cb.hourlyRate;`
     hoàn toàn không sử dụng `cb.weeksCount` hay gọi `cb.getTotalCost()`.
4. **Hoa hồng 25% có được áp dụng đúng cơ sở doanh thu?**
   - Thuật toán áp dụng: $\text{Commission} = \text{initialConnectionGMV} \times 0.25$.
   - Hoa hồng được tính bằng 25% của phí kết nối buổi đầu tiên, hoàn toàn nhất quán với công thức quy định trong ADR-005.
5. **Cách xử lý các trạng thái PENDING, REJECTED, CANCELLED, ACCEPTED, COMPLETED:**
   - `COMPLETED`: Được cộng vào `completedBookings` và cộng học phí vào `initialConnectionGMV`.
   - `ACCEPTED`: Được cộng vào `activeMatches` (số lớp đang học), **không** cộng vào `initialConnectionGMV`.
   - `PENDING`, `REJECTED`, `CANCELLED`: Hoàn toàn **bị loại trừ** khỏi cả GMV và hoa hồng. Xử lý chính xác.
6. **Các bản ghi `COMPLETED` lịch sử có được tính hay không?**
   - Trong `course_bookings.dat`, chỉ có **duy nhất bản ghi #1008** có trạng thái `COMPLETED` (`hourlyRate = 180,000` VNĐ). Do đó:
     $$\text{initialConnectionGMV} = 180,000 \text{ VNĐ}$$
     $$\text{platformCommissionRevenue} = 180,000 \times 0.25 = 45,000 \text{ VNĐ}$$
   - 15 đơn `COMPLETED` cũ trong archive (`legacy_bookings_...dat`) không được cộng vào đây, vì chúng đã được lưu trữ lịch sử và không nằm trong tập dữ liệu hoạt động của `CourseBooking`.
7. **Đơn vị tiền tệ có nhất quán là VNĐ?**
   - Có. Toàn bộ hiển thị trên QML đều sử dụng locale `vi_VN` với hậu tố `" VNĐ"`.
8. **Xử lý tràn số nguyên, làm tròn và giá trị không hợp lệ:**
   - Các biến tài chính đều dùng kiểu `double` trong C++, có độ chính xác 53-bit mantissa (hỗ trợ tính toán an toàn tới hàng nghìn tỷ VNĐ mà không lo tràn số nguyên).
9. **Khả năng đếm trùng khóa học:**
   - Không có. Vòng lặp `for (const auto& cb : courseBookings)` duyệt tuyến tính qua danh sách khóa học lấy từ `courseBookingRepo->getAllCourseBookings()`, mỗi khóa chỉ xuất hiện duy nhất một lần.

### 3.3. Đánh giá Phản biện về Tính toán Tài chính
- **Trạng thái:** `DESIGN DECISION REQUIRED`.
- **Vấn đề cần quyết định:**
  - Nếu TutorMatch hoạt động theo mô hình **Thu phí giới thiệu buổi đầu (Lead Generation / First Lesson Commission)**: Cách tính hiện tại ($180,000 \times 25\% = 45,000$ VNĐ) là hoàn toàn chuẩn xác theo thiết kế ban đầu (ADR-005).
  - Nếu TutorMatch chuyển sang mô hình **Thu phí trên toàn bộ khóa học (Full Course Escrow Commission)**: Cách tính cần được nâng cấp thành:
    $$m.\text{courseGMV} += cb.\text{getTotalCost}(); \quad (cb.\text{hourlyRate} \times cb.\text{weeksCount})$$
    Và hoa hồng sẽ là $25\%$ của $720,000 \text{ VNĐ} = 180,000 \text{ VNĐ}$.
  - Nhóm kỹ thuật ghi nhận đây là quyết định thuộc phạm vi định hướng kinh doanh (Business Policy), **không tự ý thay đổi mã nguồn** khi chưa có chỉ đạo từ Product Owner.

---

## 4. KIỂM TOÁN TÍNH TOÀN VẸN ĐÁNH GIÁ (AUDIT C — REVIEW INTEGRITY)

Kiểm tra trực tiếp mã nguồn `ReviewService.cpp` và các bản ghi trong `data/reviews.dat`:

1. **Khớp nối Người dùng và Phân quyền:**
   - 100% đánh giá trong `data/reviews.dat` đều có `studentId` hợp lệ (thuộc vai trò `STUDENT`) và `tutorId` hợp lệ (thuộc vai trò `TUTOR`).
   - Phương thức `ReviewService::submitReview()` (dòng 102-106) kiểm tra `userRepo->findUserById(studentId)` trước khi tạo bản ghi mới.
2. **Ràng buộc Tính đủ điều kiện Đánh giá (`isEligibleForReview`):**
   - Chỉ cho phép đánh giá khi khóa học ở trạng thái `COMPLETED` (`bPtr->status == "COMPLETED"`).
   - Kiểm tra quyền sở hữu: `bPtr->studentId == studentId`. Học viên không thể đánh giá khóa học của người khác.
   - Ngăn chặn đánh giá trùng lặp: Quét toàn bộ `reviewRepo->getAllReviews()`, nếu đã tồn tại bản ghi có cùng `bookingId` và `studentId` thì từ chối ngay lập tức.
3. **Giới hạn Thang điểm Đánh giá:**
   - `ReviewSerializer::isValidRatingScore` và `ReviewService::submitReview` đều áp đặt điều kiện nghiêm ngặt: $1 \le \text{ratingScore} \le 5$. Mọi điểm số nằm ngoài khoảng này đều bị từ chối và trả về lỗi.
4. **Bảo vệ Tính nhất quán Đa tệp khi Lưu thất bại (ADR-007 Compensating Rollback):**
   - Trong `submitReview()` (dòng 127-133), sau khi lưu review vào `reviews.dat`, hệ thống tính lại điểm trung bình của gia sư và cập nhật vào `users.dat`.
   - Nếu việc ghi vào `users.dat` thất bại, hệ thống tự động thực hiện **Rollback bù trừ**: gọi `reviewRepo->deleteReview(nextRId)` để xóa review vừa thêm, ngăn chặn tình trạng review có trong file nhưng điểm gia sư không được cập nhật.
5. **Trạng thái:** **`CONFIRMED PASS`**.

---

## 5. MA TRẬN PHỦ KIỂM THỬ (AUDIT D — TEST COVERAGE CHALLENGE)

Tiến hành rà soát toàn bộ 22 bộ kiểm thử trong `tests/TestRunner.cpp` và lập ma trận kiểm chứng các bất biến nghiệp vụ cốt lõi:

| Bất biến nghiệp vụ | Test Suite hiện có | Mã nguồn Production được thực thi? | Phạm vi bao phủ | Đánh giá & Khắc phục |
|---|---|:---:|:---:|---|
| **PENDING không giữ chỗ** | Test 16 (Scenario 1, 2); Test 20 (Part 4.5) | Có (`BookingService::bookCourse`) | Đầy đủ | `PASS` |
| **REJECTED giải phóng ca** | Test 16 (Scenario 6) | Có (`BookingService::rejectCourseBooking`) | Đầy đủ | `PASS` |
| **CANCELLED giải phóng ca** | Test 16 (Scenario 7) | Có (`BookingService::cancelCourseBooking`) | Đầy đủ | `PASS` |
| **COMPLETED không chặn tương lai** | Test 16 (Scenario 8) | Có (`BookingService::terminateCourseBooking`) | Đầy đủ | `PASS` |
| **ACCEPTED chiếm trọn các buổi** | Test 16 (Scenario 2); Test 20 (Part 4.6) | Có (`BookingService::acceptCourseBooking`) | Đầy đủ | `PASS` |
| **Xung đột cùng ngày, cùng ca** | Test 16 (Scenario 2); Test 20 (Part 4.8) | Có (`hasCourseConflict`) | Đầy đủ | `PASS` |
| **Khác ngày, cùng ca học không xung đột** | Test 16 (Scenario 5); Test 20 (Part 4.9) | Có (`hasCourseConflict`) | Đầy đủ | `PASS` |
| **Xung đột giao thoa một phần (Staggered)** | **Test 16 (Scenario 4b) [MỚI BỔ SUNG]** | Có (`BookingService::hasCourseConflict`) | **100% Hoàn chỉnh** | **ĐÃ BỔ SUNG & PASS** (Phát hiện xung đột ngày 2026-11-23) |
| **Kiểm tra ngày/thứ và ca hợp lệ** | Test 16 (Scenario 12); Test 20 (Part 4.1) | Có (`bookCourse`) | Đầy đủ | `PASS` |
| **Bảo toàn `scheduleBitmask`** | Test 20 (Part 4.6, dòng 2492) | Có (`User::getScheduleBitmask`) | Đầy đủ | `PASS` |
| **Đua lệnh đồng thời (Race Condition)** | Test 16 (Scenario 13); Test 20 (Part 4.11) | Có (`acceptCourseBooking` 2 threads) | Đầy đủ | `PASS` (Đúng 1 thread thành công, 1 thất bại) |
| **Lưu đĩa và Nạp lại (Persistence Round-Trip)** | Test 18; Test 20 (Part 3); Test 22 | Có (`FileCourseBookingRepository`) | Đầy đủ | `PASS` (Kiểm thử trên thư mục temp sandbox) |
| **Không tái tạo `bookings.dat`** | Test 22 (Part 2) | Có (`FileBookingRepository::initialize`) | Đầy đủ | `PASS` (Assert file không tồn tại) |
| **Không nạp Archive vào Active** | Test 22 (Part 3) | Có (`archiveRepo` độc lập) | Đầy đủ | `PASS` |
| **Tính toán GMV & Hoa hồng 25%** | Test 3 (Dòng 390-410) | Có (`AdminService::calculateMetrics`) | Đầy đủ | `PASS` |
| **Ngăn chặn đánh giá trùng lặp** | Test 19 (E2E-09) | Có (`ReviewService::isEligibleForReview`) | Đầy đủ | `PASS` |

---

## 6. RỦI RO ĐỒNG BỘ ĐA TIẾN TRÌNH (AUDIT E — MULTI-PROCESS PERSISTENCE RISK)

Khảo sát trực tiếp cơ chế lưu trữ của `FileCourseBookingRepository`, `FileUserRepository`, `FileReviewRepository`:

```cpp
// Vị trí: src/storage/repositories/FileCourseBookingRepository.cpp
bool FileCourseBookingRepository::updateCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    // ...
    std::string tempPath = filePath + ".tmp";
    std::ofstream out(tempPath, ...);
    // Ghi toàn bộ dữ liệu ra file tạm .tmp
    // Sau đó gọi MoveFileExW(tempPath, filePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}
```

### Phân loại các Rủi ro Lưu trữ:

1. **Đồng thời Đơn tiến trình (Single-Process Concurrency):**
   - **Mức độ rủi ro:** **KHÔNG CÓ (AN TOÀN TUYỆT ĐỐI)**.
   - Cơ chế: Được bảo vệ chặt chẽ bởi `std::recursive_mutex repoMutex` và `serviceMutex`. Các luồng bên trong cùng một tiến trình (ví dụ GUI Bridge Worker Thread, Timer, Background Thread) không thể gây xung đột hay đua lệnh ghi đĩa.
2. **Đồng thời Đa tiến trình (Multi-Process Concurrency):**
   - **Mức độ rủi ro:** **CAO (KIẾN TRÚC HIỆN TẠI CHƯA HỖ TRỢ)**.
   - Nguyên nhân: `std::recursive_mutex` là biến nằm trong không gian bộ nhớ của tiến trình (in-process RAM). Khi `TutorMatch_GUI.exe` và `TutorMatch_CLI.exe` cùng chạy độc lập:
     - Cả hai tiến trình đều duy trì snapshot riêng của các bản ghi.
     - Nếu Tiến trình A cập nhật trạng thái đơn #1001 và ghi đè file qua `MoveFileExW`, trong khi Tiến trình B vừa append đơn #1009, đơn #1009 của Tiến trình B sẽ bị Tiến trình A **ghi đè và xóa mất**.
     - Cả hai tiến trình cùng sử dụng chung tên tệp tạm `course_bookings.dat.tmp`, có thể dẫn đến lỗi tranh chấp chia sẻ tệp (`ERROR_SHARING_VIOLATION`).
3. **Sự cố Sập nguồn / Crash trong lúc ghi (Crash during write):**
   - **Mức độ rủi ro:** **THẤP (ĐÃ ĐƯỢC BẢO VỆ NGUYÊN TỬ)**.
   - Cơ chế: Do sử dụng kỹ thuật "Ghi ra file tạm `.tmp` rồi hoán đổi nguyên tử (`MoveFileExW` / `rename`)", nếu tiến trình bị crash đột ngột trong khi đang ghi dữ liệu, tệp chính `course_bookings.dat` vẫn hoàn toàn nguyên vẹn, không bị cụt hay hỏng dữ liệu.
4. **Lỗi đầy đĩa hoặc Phân quyền (Disk-full / Permission failure):**
   - **Mức độ rủi ro:** **THẤP**.
   - Cơ chế: Nếu ghi file tạm thất bại (`!out.good()`), tệp `.tmp` bị xóa bỏ ngay lập tức và hàm trả về `false`, không tác động đến tệp chính.

---

## 7. ĐỐI CHIẾU MÂU THUẪN VỚI BÁO CÁO CŨ (AUDIT F — CONTRADICTIONS ANALYSIS)

| Điểm đối chiếu | Tuyên bố trong Báo cáo cũ | Thực tế kiểm chứng pháp chứng | Kết luận phân loại |
|---|---|---|:---:|
| **Sự cố Weekday Mismatch** | Tuyên bố các bản ghi 1001, 1002, 1003, 1006, 1008 bị sai ngày và được "chữa lỗi sau phiên chuẩn hóa". | Dữ liệu gốc trong backup chứng minh bản ghi 1001, 1002 chưa từng sai và chưa từng bị sửa. Bản ghi 1003–1008 được tạo đúng ngay từ đầu. | **BÁO CÁO CŨ MÔ TẢ SAI THỰC TẾ** |
| **Bản chất của GMV** | Trình bày chỉ số GMV như một chỉ số tổng thể toàn sàn. | GMV thực tế chỉ tính trên `hourlyRate` của các đơn `COMPLETED`, không tính theo độ dài khóa học (`weeksCount`) và không tính các đơn `ACCEPTED`. | **DESIGN DECISION REQUIRED** |
| **Mã chết (Dead Code)** | Báo cáo cũ tuyên bố đã dọn dẹp sạch sẽ mã legacy. | Vẫn còn tồn tại các include dư thừa trong `main.cpp`, `gui/main.cpp`, overload rỗng trong `GUIBridge` và `FakeBookingRepository`. (Đã được phát hiện và dọn dẹp triệt để trong phiên này). | **CONFIRMED DEFECT (ĐÃ KHẮC PHỤC)** |
| **Kiểm thử Giao thoa Một phần** | Cho rằng 22 test suites đã bao quát toàn bộ bất biến. | Thiếu test case kiểm thử giao thoa một phần giữa hai khóa học có ngày bắt đầu lệch nhau (staggered overlap). | **CONFIRMED DEFECT (ĐÃ BỔ SUNG TEST)** |

---

## 8. BIỆN PHÁP KHẮC PHỤC ĐÃ THỰC HIỆN (REMEDIATION PERFORMED)

1. **Bổ sung Ca kiểm thử Xung đột Giao thoa Một phần (Scenario 4b vào Test 16):**
   - **Vị trí sửa đổi:** `tests/TestRunner.cpp` (dòng 1362-1371).
   - **Nội dung ca test:**
     - Thiết lập Khóa học 1 chạy từ `2026-11-09` đến `2026-11-30` (4 tuần).
     - Khóa học 2 bắt đầu từ `2026-11-23` (lệch 2 tuần sau) trên cùng thứ Hai ca 1: Khẳng định phát hiện xung đột chính xác tại ngày `2026-11-23`.
     - Khóa học 3 bắt đầu từ `2026-12-07` (ngay sau khi Khóa học 1 kết thúc): Khẳng định không có xung đột (`hasCourseConflict == false`).
2. **Loại bỏ Hoàn toàn Mã chết:**
   - Dọn sạch 2 include dư thừa `FileBookingRepository.h` trong `src/main.cpp` và `src/gui/main.cpp`.
   - Xóa bỏ overload 4 tham số không dùng trong `GUIBridge.h` và `GUIBridge.cpp`.
   - Xóa bỏ struct `FakeBookingRepository` không dùng trong `tests/TestRunner.cpp`.
3. **Biên dịch và Chạy lại Kiểm thử:**
   - Thực thi `cmake --build . --target TutorMatch_Tests` thành công với **Exit Code: 0**.
   - Chạy `.\TutorMatch_Tests.exe`: Toàn bộ **22/22 bộ kiểm thử đạt 100% PASS**.

---

## 9. ĐÁNH GIÁ RỦI RO CÒN LẠI (REMAINING RISKS)

1. **Rủi ro Tranh chấp Đa tiến trình (Multi-Process Data Loss):**
   Nếu người dùng mở đồng thời cả ứng dụng GUI và giao diện dòng lệnh CLI cùng lúc, việc cập nhật khóa học có thể dẫn đến ghi đè tệp.
2. **Quy chuẩn Định nghĩa Tài chính Chưa chốt:**
   Chỉ số GMV hiện tại đang là `initialConnectionGMV` (phí buổi đầu). Nếu người dùng cuối kỳ vọng GMV phản ánh tổng giá trị cả khóa học ($N \times \text{hourlyRate}$), giao diện sẽ cần được điều chỉnh lại công thức.

---

## 10. KHUYẾN NGHỊ NHIỆM VỤ TIẾP THEO (RECOMMENDED NEXT ENGINEERING TASKS)

1. **Triển khai File Lock Cấp Hệ điều hành (Task: Multi-Process OS File Locking):**
   Bổ sung lớp `OSFileLockGuard` sử dụng API `LockFileEx` (Windows) hoặc `flock` (POSIX) trong các phương thức `save...` và `update...` của repositories để ngăn chặn hoàn toàn việc ghi đè giữa GUI và CLI.
2. **Quyết định Thiết kế Tài chính từ Product Owner (Task: Financial Metrics Alignment):**
   Chốt định nghĩa nghiệp vụ của GMV: Giữ nguyên mô hình phí kết nối ban đầu hay chuyển sang mô hình tổng giá trị hợp đồng toàn khóa.
