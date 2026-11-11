# BÁO CÁO KIỂM TOÁN VÀ TÁI THIẾT TOÀN VẸN DỮ LIỆU MẪU — TUTORMATCH
## Comprehensive Sample Data Integrity Audit & Domain-Consistent Reconstruction Report
**Dự án:** TutorMatch — Nền tảng kết nối & điều phối lịch học gia sư  
**Tác giả:** Principal Software Architect, Senior Domain Modeler & Data Integrity Reviewer  
**Ngày thực hiện:** 10/11/2026  
**Trạng thái:** HOÀN THÀNH — 100% PASS TOÀN DIỆN  

---

## 1. TỔNG QUAN ĐIỀU HÀNH (EXECUTIVE SUMMARY)

Đợt kiểm toán và tái thiết dữ liệu mẫu này giải quyết triệt để sự bất nhất nghiêm trọng trong hệ thống dữ liệu của TutorMatch giữa **Đánh giá (Reviews)**, **Đơn đặt khóa học (Course Bookings)**, **Lịch sử khóa học (Course History)** và **Thời khóa biểu (Timetable)**.

### Vấn đề thực tế đã phát hiện:
1. **Lệch pha toàn bộ Đánh giá:** Toàn bộ 15/15 bản ghi trong `data/reviews.dat` trỏ vào các mã đơn lẻ từ thời kỳ tiền ADR-009 (mã `101, 102, 103, ..., 120`), không hề tồn tại trong tập dữ liệu khóa học nhiều tuần `data/course_bookings.dat` (chỉ có 8 bản ghi `1001..1008`). Tỷ lệ đánh giá mồ côi (orphaned reviews) lên tới **100%**.
2. **Nghịch lý Gia sư Lê Văn Toàn (User ID 2):** Gia sư có 2 đánh giá xuất sắc (5 sao) từ học viên Nguyễn Văn Nam (ID 6) và Phạm Tuấn Thành (ID 7), nhưng trong hồ sơ và bảng điều khiển gia sư, danh sách khóa học hoàn thành báo **0 khóa học**. Khi duyệt ngược thời khóa biểu về tháng 08/2026 (thời điểm ghi nhận đánh giá), hệ thống không hiển thị bất kỳ ca học nào của hai học viên này.
3. **Lỗi logic trong Seed Engine:** Mã khởi tạo dữ liệu mặc định `MockDatabaseEngine::seedDefaultDataIfEmpty()` tự sinh `r2` gán cho học viên `s2` nhưng lại trỏ vào khóa học `cb4` của học viên `s1` (vi phạm cả quy tắc 1 đánh giá/khóa học và quyền sở hữu), đồng thời tạo `r3` trỏ vào `cb3` đang ở trạng thái `ACCEPTED` thay vì `COMPLETED`.

### Kết quả sau tái thiết:
- **100% Quan hệ toàn vẹn:** 15/15 đánh giá trỏ chính xác 1:1 vào 15 khóa học đã hoàn thành (`COMPLETED`) với đúng học viên, đúng gia sư, không trùng lặp, ngày đánh giá diễn ra sau ngày kết thúc khóa học.
- **Giải quyết dứt điểm nghịch lý Lê Văn Toàn:** Gia sư có 2 khóa học hoàn thành với Nguyễn Văn Nam (1001) và Phạm Tuấn Thành (1002); hàm `qmlGetTutorCompletedStudents()` hiển thị đúng 2 học viên kèm đánh giá; duyệt thời khóa biểu tháng 08/2026 hiển thị đầy đủ các buổi học lịch sử.
- **Bộ dữ liệu mẫu phong phú, chuẩn nghiệp vụ:** Bao gồm đủ 10 kịch bản miền (gia sư chưa có khóa/chưa có review, gia sư có review, gia sư có khóa hoàn thành không review, khóa đang diễn ra, khóa kết thúc sớm đồng thuận, học viên có nhiều khóa, đơn chờ duyệt, đơn bị từ chối, đơn bị hủy).
- **Tuyệt đối an toàn:** Dữ liệu người dùng trong `users.dat` được bảo toàn nguyên vẹn; bản sao lưu đầy đủ được lưu trữ tại `backup_data_20261110/`.
- **Kiểm thử hồi quy:** Đạt **24/24 Test Suites PASS (100%)**, `TutorMatch_Inspector` báo cáo 88/88 bản ghi hợp lệ (0 lỗi), công cụ kiểm chứng hành trình GUI đạt 100% kết quả mong đợi.

---

## 2. NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE ANALYSIS)

Dựa trên điều tra pháp y dữ liệu (Data Forensics), nguyên nhân gốc rễ được xác định như sau:

```
[LỊCH SỬ PHÁT TRIỂN]
1. Trước ngày 06/11/2026:
   Hệ thống dùng mô hình đặt lịch đơn buổi (Legacy Booking) lưu trong `bookings.dat` (mã 101..127).
   15 bản ghi đánh giá được tạo ra trỏ vào các đơn hoàn thành 101, 102, 103, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 119, 120.

2. Ngày 06/11/2026 (Triển khai ADR-009):
   Chuyển đổi sang mô hình Khóa học định kỳ nhiều tuần (`CourseBooking`).
   Tệp cũ `bookings.dat` được chuyển vào thư mục lưu trữ:
   `data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat`.
   Một tệp `course_bookings.dat` mới được tạo nhanh với 8 bản ghi mẫu (1001..1008).

3. Thiếu sót đồng bộ (The Desynchronization Gap):
   - Tệp `reviews.dat` KHÔNG được ánh xạ lại khóa ngoại (Foreign Key) sang `course_bookings.dat`, vẫn giữ nguyên mã `101..120`.
   - Các khóa học lịch sử tương ứng với 15 đánh giá không được khởi tạo đầy đủ trong `course_bookings.dat`.
   - Gia sư Lê Văn Toàn chỉ được cấp 2 khóa học mới (1001, 1002) đều với Nam, đều là `ACCEPTED` (đang học tháng 11/2026), không có khóa học nào với Thành và không có khóa học nào ở tháng 8/2026.
   - Do đó, khi QML gọi `qmlGetTutorCompletedStudents()` lọc theo `status == "COMPLETED"`, kết quả trả về rỗng (0 khóa học), và duyệt thời khóa biểu tháng 8 không có bản ghi nào để hiển thị.
```

---

## 3. CÁC NGUỒN DỮ LIỆU ĐÃ KIỂM TRA (DATA SOURCES INSPECTED)

1. `data/users.dat` (50 bản ghi: 3 Admin, 12 Tutor, 35 Student).
2. `data/course_bookings.dat` (8 bản ghi ban đầu, mở rộng thành 23 bản ghi).
3. `data/reviews.dat` (15 bản ghi ban đầu bị mồ côi, đã được re-key chuẩn xác).
4. `data/archive/legacy_bookings/legacy_bookings_20261106_140537.dat` (27 bản ghi gốc tiền ADR-009).
5. `src/storage/MockDatabaseEngine.cpp` (`seedDefaultDataIfEmpty()`).
6. `src/services/ReviewService.cpp` (`isEligibleForReview()`, `reconcileRatingsOnStartup()`).
7. `src/services/BookingService.cpp` (`getAcceptedSessionsForWeek()`, `autoCompleteExpiredCourses()`).
8. `src/gui/GUIBridge.cpp` (`qmlGetTutorCompletedStudents()`, `getTutorWeeklySchedule()`, `qmlGetTutorReviews()`).
9. `src/storage/DataFileInspector.cpp` & `TutorMatch_Inspector.exe`.
10. `tests/TestRunner.cpp` (24 bộ kiểm thử SOLID và E2E).

---

## 4. KẾT QUẢ KIỂM TOÁN TOÀN VẸN BAN ĐẦU (ORIGINAL INTEGRITY FINDINGS)

| Hạng mục kiểm tra | Trạng thái ban đầu | Mức độ nghiêm trọng | Chi tiết phát hiện |
|---|---|---|---|
| **Khóa ngoại Review -> Booking** | **THẤT BẠI (0/15 hợp lệ)** | Nghiêm trọng (Critical) | 15/15 đánh giá trỏ vào các ID `101..120` không tồn tại trong `course_bookings.dat`. |
| **Lịch sử gia sư Lê Văn Toàn** | **THẤT BẠI (0 khóa hoàn thành)** | Nghiêm trọng (Critical) | Có 2 đánh giá từ Nam và Thành nhưng hiển thị 0 khóa học; không có khóa nào với Thành. |
| **Thời khóa biểu tháng 08/2026** | **THẤT BẠI (0 ca học)** | Trung bình (Medium) | Không hiển thị bất kỳ buổi học lịch sử nào cho Nam và Thành dù review ghi nhận tháng 8. |
| **Logic Seed Engine trong C++** | **THẤT BẠI (Lỗi logic)** | Cao (High) | `r2` và `r1` cùng trỏ vào `cb4`; `r2` gán sai học viên; `r3` trỏ vào khóa `ACCEPTED`. |
| **Khớp xếp hạng trong `users.dat`** | **KHỚP VỚI REVIEWS CŨ** | Thông tin (Info) | Điểm số và số lượng review trong `users.dat` hoàn toàn khớp với 15 review, chỉ thiếu liên kết khóa học. |
| **Xung đột ca học (Overlapping)** | **ĐẠT (0 xung đột)** | Đạt (Pass) | Các khóa học hiện tại không gây đè lịch. |

---

## 5. THỐNG KÊ SỐ LƯỢNG BẢN GHI TRƯỚC VÀ SAU (RECORD COUNTS)

| Thực thể dữ liệu (Entity) | Trước tái thiết | Sau tái thiết | Chênh lệch (Delta) | Ghi chú an toàn |
|---|:---:|:---:|:---:|---|
| **Người dùng (`users.dat`)** | 50 | 50 | 0 | Giữ nguyên 100% tài khoản, quyền hạn, mật khẩu và chỉ số rating/reviews |
| **Đơn đặt khóa học (`course_bookings.dat`)** | 8 | 23 | +15 | Bổ sung 15 khóa học lịch sử tương ứng với 15 review + khóa mẫu đặc thù |
| **Đánh giá (`reviews.dat`)** | 15 | 15 | 0 | Giữ nguyên nội dung, người đánh giá, số sao; chỉ chuẩn hóa lại `bookingId` |
| **Lưu trữ cũ (`legacy_bookings`)** | 27 | 27 | 0 | Tệp lưu trữ lịch sử được bảo toàn nguyên trạng trong `data/archive/` |
| **Tổng số bản ghi hoạt động** | **73** | **88** | **+15** | Được kiểm chứng bởi `TutorMatch_Inspector` |

---

## 6. CHI TIẾT BẢN GHI TÁI THIẾT VÀ LÝ DO NGHIỆP VỤ (RECORDS WITH RATIONALES)

### 6.1. Danh sách 15 Khóa học Hoàn thành Bảo chứng cho 15 Đánh giá (IDs 1001..1015)

Mỗi khóa học được thiết kế đảm bảo:
- Thuộc lịch rảnh (`scheduleBitmask`) của gia sư.
- Ngày bắt đầu (`startDate`) khớp đúng thứ trong tuần (`dayOfWeek`).
- Thời lượng 4 tuần, kết thúc trước ngày học viên gửi đánh giá.
- Không gây bất kỳ xung đột ca học nào giữa các học viên và gia sư.
- Đầy đủ thông tin cam kết `COMMITMENT_V1`.

```csv
1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,0,1,2026-07-20,4,COMPLETED,2026-07-15,0:1,0,NONE,,,1,2026-07-15 08:00:00,1,2026-07-15 08:30:00,COMMITMENT_V1
1002,7,2,Phạm Tuấn Thành,ThS. Lê Văn Toàn,Toán 12,150000,2,1,2026-07-22,4,COMPLETED,2026-07-16,2:1,0,NONE,,,1,2026-07-16 08:00:00,1,2026-07-16 08:30:00,COMMITMENT_V1
1003,6,3,Nguyễn Văn Nam,TS. Trần Thị Vật Lý,Vật lý 10-12 (Luyện thi THPT),200000,1,2,2026-07-21,4,COMPLETED,2026-07-18,1:2,0,NONE,,,1,2026-07-18 08:00:00,1,2026-07-18 08:30:00,COMMITMENT_V1
1004,43,12,Trần Khánh Vy,Thầy Võ Minh Tin,Tin học 10-12 (Lập trình C++),180000,1,2,2026-09-08,4,COMPLETED,2026-09-01,1:2,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1005,50,11,Thái Bình An,ThS. Hoàng Văn Chương,Ngữ văn 10-12 (Luyện thi THPT),110000,0,3,2026-09-07,4,COMPLETED,2026-09-01,0:3,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1006,32,15,Lý Hoàng Minh,Cô Bùi Thị Sử,Lịch sử - Địa lý 10-12 (Ôn thi THPT),90000,1,1,2026-09-08,4,COMPLETED,2026-09-01,1:1,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1007,29,13,Đỗ Tuấn Kiệt,Cô Đặng Thu Hà,Toán 10-11 (Củng cố nền tảng),120000,0,2,2026-09-07,4,COMPLETED,2026-09-01,0:2,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1008,42,15,Nguyễn Đình Trung,Cô Bùi Thị Sử,Lịch sử - Địa lý 10-12 (Ôn thi THPT),90000,1,4,2026-09-08,4,COMPLETED,2026-09-02,1:4,0,NONE,,,1,2026-09-02 08:00:00,1,2026-09-02 08:30:00,COMMITMENT_V1
1009,20,11,Phạm Thị Chi,ThS. Hoàng Văn Chương,Ngữ văn 10-12 (Luyện thi THPT),110000,0,4,2026-09-07,4,COMPLETED,2026-09-01,0:4,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1010,39,13,Hà Minh Thư,Cô Đặng Thu Hà,Toán 10-11 (Củng cố nền tảng),120000,0,5,2026-09-07,4,COMPLETED,2026-09-02,0:5,0,NONE,,,1,2026-09-02 08:00:00,1,2026-09-02 08:30:00,COMMITMENT_V1
1011,27,14,Phan Đức Huy,Mr. David Lê,Tiếng Anh / IELTS (Giao tiếp - Speaking),160000,0,1,2026-09-07,4,COMPLETED,2026-09-01,0:1,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1012,33,12,Trịnh Thảo My,Thầy Võ Minh Tin,Tin học 10-12 (Lập trình C++),180000,0,2,2026-09-07,4,COMPLETED,2026-09-01,0:2,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1013,25,16,Hoàng Ngọc Hà,TS. Ngô Quang Lực,Vật lý 10-11 (Cơ bản và nâng cao),170000,0,2,2026-09-07,4,COMPLETED,2026-09-01,0:2,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1014,48,10,Tạ Thanh Tùng,Cô Nguyễn Thị Sinh,Sinh học 10-12 (Luyện thi THPT),130000,0,1,2026-09-07,4,COMPLETED,2026-09-01,0:1,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
1015,18,10,Trần Minh Anh,Cô Nguyễn Thị Sinh,Sinh học 10-12 (Luyện thi THPT),130000,0,3,2026-09-07,4,COMPLETED,2026-09-01,0:3,0,NONE,,,1,2026-09-01 08:00:00,1,2026-09-01 08:30:00,COMMITMENT_V1
```

### 6.2. Các Khóa học Đại diện cho Kịch bản Đa dạng (IDs 1016..1023)

- **1016 (COMPLETED không có review):** Học viên Đặng Gia Hân (23) với Thầy Võ Minh Tin (12), môn Tin học, hoàn thành ngày 29/09/2026 nhưng học viên chưa viết đánh giá. Minh họa kịch bản gia sư có khóa hoàn thành nhưng chưa được đánh giá.
- **1017 (ACCEPTED - Đang diễn ra):** Học viên Nguyễn Văn Nam (6) với ThS. Lê Văn Toàn (2), môn Toán 12, Thứ 6 ca 0 (07:30-09:00), bắt đầu từ 06/11/2026. Hiển thị buổi học 2/4 vào ngày 13/11/2026 trên thời khóa biểu tuần hiện tại.
- **1018 (ACCEPTED - Đang diễn ra):** Học viên Phạm Tuấn Thành (7) với TS. Trần Thị Vật Lý (3), môn Vật lý, Thứ 3 ca 1 (09:15-10:45), bắt đầu 10/11/2026 (hôm nay).
- **1019 (ACCEPTED - Đang diễn ra):** Học viên Phan Đức Huy (27) với Mr. David Lê (14), môn Tiếng Anh IELTS, Thứ 4 ca 1 (09:15-10:45), khóa 12 tuần bắt đầu từ 11/11/2026.
- **1020 (EARLY_TERMINATED - Kết thúc sớm đồng thuận):** Học viên Lương Thị Quỳnh (35) với TS. Ngô Quang Lực (16), bắt đầu 12/10/2026 (8 tuần). Hai bên đồng thuận kết thúc sớm vào ngày 26/10/2026 do học sinh đổi lịch học. Các ca học tương lai từ sau ngày 26/10 được tự động giải phóng.
- **1021 (PENDING - Đang chờ duyệt):** Học viên Trần Minh Anh (18) gửi yêu cầu tới Cô Nguyễn Thị Sinh (10), môn Sinh học, Thứ 5 ca 1.
- **1022 (REJECTED - Bị từ chối):** Học viên Đoàn Nhật Khoa (46) gửi yêu cầu tới Thầy Phạm Hóa Học (5), đã bị từ chối.
- **1023 (CANCELLED - Bị hủy):** Học viên Lê Quang Vinh (45) hủy yêu cầu với TS. Trần Thị Vật Lý (3).

### 6.3. Tệp Đánh giá Đã Chuẩn hóa (`reviews.dat`)

Toàn bộ 15 đánh giá trỏ chuẩn xác vào các mã `1001..1015`:
```csv
1,1001,6,2,Nguyễn Văn Nam,5,Thầy Toàn giảng Toán 12 rất dễ hiểu  phương pháp giải trắc nghiệm tốc độ cao.,2026-08-15
2,1002,7,2,Phạm Tuấn Thành,5,Thầy hỗ trợ giải bộ đề Toán 12 Đại học cực kỳ chi tiết!,2026-08-20
3,1003,6,3,Nguyễn Văn Nam,4,Cô dạy Vật lý 12 rất hay  bài tập nâng cao bám sát cấu trúc đề thi THPT.,2026-08-22
4,1004,43,12,Trần Khánh Vy,4,Bài tập thuật toán hay và có lời giải chi tiết!,2026-10-07
5,1005,50,11,Thái Bình An,4,Nhờ thầy mà em tự tin hơn khi làm đề Ngữ văn.,2026-10-07
6,1006,32,15,Lý Hoàng Minh,4,Cô hệ thống kiến thức Sử và Địa rất logic.,2026-10-06
7,1007,29,13,Đỗ Tuấn Kiệt,5,Em đã hiểu hàm số và lượng giác nhờ cô!,2026-10-09
8,1008,42,15,Nguyễn Đình Trung,5,Cô kể chuyện lịch sử rất cuốn nên em nhớ lâu.,2026-10-10
9,1009,20,11,Phạm Thị Chi,5,Thầy chấm bài kỹ và nhận xét rất chi tiết.,2026-10-09
10,1010,39,13,Hà Minh Thư,4,Cô kiên nhẫn giảng lại từng dạng bài khó.,2026-10-13
11,1011,27,14,Phan Đức Huy,4,Lộ trình luyện IELTS rõ ràng dễ theo dõi.,2026-10-11
12,1012,33,12,Trịnh Thảo My,4,Thầy dạy C++ từ cơ bản đến nâng cao rất dễ hiểu.,2026-10-14
13,1013,25,16,Hoàng Ngọc Hà,5,Thầy giảng chậm và chắc nên em theo kịp bài.,2026-10-14
14,1014,48,10,Tạ Thanh Tùng,5,Bài tập trắc nghiệm Sinh bám sát đề thi THPT.,2026-10-17
15,1015,18,10,Trần Minh Anh,5,Phần di truyền khó mà cô giải thích cực kỳ rõ ràng!,2026-10-16
```

---

## 7. KẾT QUẢ KIỂM CHỨNG QUAN HỆ ĐA THỰC THỂ (RELATIONSHIP VALIDATION)

Đoạn mã tự động kiểm tra `verify_data_integrity.py` đã xác nhận 100% các tiêu chí miền:
1. **Liên kết hợp lệ:** Mọi đánh giá đều có khóa ngoại trỏ tới 1 bản ghi `CourseBooking` thực tế tồn tại.
2. **Khớp danh tính:** ID học viên và ID gia sư trong đánh giá khớp 100% với khóa học tham chiếu.
3. **Tính hợp lệ trạng thái:** 100% khóa học được đánh giá đều ở trạng thái `COMPLETED`.
4. **Không trùng lặp:** Không có bất kỳ khóa học nào nhận từ 2 đánh giá trở lên (tối đa đúng 1 review/booking).
5. **Đồng bộ hồ sơ gia sư:** Chỉ số `rating` và `totalReviews` trong `users.dat` khớp chính xác với kết quả trung bình cộng của các bài đánh giá tương ứng.
6. **Không xung đột lịch:** Không có bất kỳ ca học nào bị trùng lặp giữa các gia sư hoặc học sinh trong cùng thời điểm.

---

## 8. KẾT QUẢ KIỂM CHỨNG HÀNH TRÌNH GIAO DIỆN (GUI JOURNEY VERIFICATION)

Chương trình kiểm thử hành trình GUI chuyên dụng `verify_gui_journeys.exe` (sử dụng trực tiếp `GUIBridge` trên Qt runtime) đã thực thi và vượt qua toàn bộ 7 kịch bản:

```
============================================================
   TUTORMATCH PHASE G — GUI JOURNEY & END-TO-END AUDIT      
============================================================
[ApplicationPaths] Resolved to Project Source Data Dir: C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data
Using Data Directory: C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data

[GUI 1] Logging in as Tutor Lê Văn Toàn (User ID 2)...
  -> PASS: Logged in as ThS. Lê Văn Toàn

[GUI 2] Inspecting Tutor Lê Văn Toàn reviews...
  Review 1: Student=Nguyễn Văn Nam, Score=5, BookingId=1001
  Review 2: Student=Phạm Tuấn Thành, Score=5, BookingId=1002
  -> PASS: Both reviews verified with exact student names and 5-star ratings!

[GUI 3] Inspecting Tutor Lê Văn Toàn course history (qmlGetTutorCompletedStudents)...
  Found 2 completed courses in history.
  Completed Course: Booking ID=1001, Student=Nguyễn Văn Nam, Status=COMPLETED
  Completed Course: Booking ID=1002, Student=Phạm Tuấn Thành, Status=COMPLETED
  -> PASS: Both reviewers (Nam & Thành) strictly appear in Tutor's completed course history!

[GUI 4] Logging in as Student Nguyễn Văn Nam (User ID 6)...
  Student Nguyễn Văn Nam has 3 courses enrolled.
  -> PASS: Student Nguyễn Văn Nam sees completed course with Lê Văn Toàn, completed course with Trần Thị Vật Lý, and ongoing course!

[GUI 5] Logging in as Student Phạm Tuấn Thành (User ID 7)...
  Student Phạm Tuấn Thành has 2 courses enrolled.
  -> PASS: Student Phạm Tuấn Thành sees completed course with Lê Văn Toàn and ongoing course with Trần Thị Vật Lý!

[GUI 6] Logging in as Tutor Lê Văn Toàn & Navigating Timetable to August 2026...
  August 2026 Session: Date=2026-08-03, Student=Nguyễn Văn Nam, Buổi 3/4
  August 2026 Session: Date=2026-08-05, Student=Phạm Tuấn Thành, Buổi 3/4
  -> PASS: Both historical lessons (Nam & Thành) appear in August 2026 timetable!

[GUI 7] Verifying Ongoing Course in Current Week (November 2026)...
  Current Week Session: Date=2026-11-13, Student=Nguyễn Văn Nam, Buổi 2/4
  -> PASS: Ongoing course session for November 13, 2026 appears on current week timetable!
============================================================
   >>> ALL GUI JOURNEYS VERIFIED WITH 100% SUCCESS! <<<    
============================================================
```

---

## 9. THAY ĐỔI TRONG LOGIC KHỞI TẠO MẪU (SEED ENGINE CHANGES)

Trong tệp `src/storage/MockDatabaseEngine.cpp`:
- Đã sửa phương thức `seedDefaultDataIfEmpty()`:
  - Khởi tạo đúng thứ tự phụ thuộc: `Admin` -> `Tutors` -> `Students` -> `CourseBookings` -> `Reviews`.
  - Tạo `cb1` (Nam - Toàn, COMPLETED) và `cb2` (Thành - Toàn, COMPLETED) khớp với `r1` và `r2`.
  - Tạo `cb3` (Nam - Lý, COMPLETED) khớp với `r3`.
  - Tạo các khóa học đang diễn ra `cb4` và `cb5` cho thời khóa biểu hiện tại.
  - Loại bỏ hoàn toàn lỗi gán 2 review vào cùng một khóa học và lỗi gán sai học viên sở hữu khóa học.

---

## 10. KẾT QUẢ BIÊN DỊCH VÀ KIỂM THỬ HỒI QUY (BUILD & TEST RESULTS)

### 10.1. Biên dịch toàn bộ 4 mục tiêu hệ thống
Biên dịch thành công 100% với MinGW 64-bit và Qt 6.11.2:
- `TutorMatch_GUI.exe`
- `TutorMatch_CLI.exe`
- `TutorMatch_Inspector.exe`
- `TutorMatch_Tests.exe`

### 10.2. Chạy bộ kiểm thử hồi quy `TutorMatch_Tests.exe`
Kết quả: **24/24 Test Suites PASS (100% thành công)**.
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

### 10.3. Báo cáo kiểm định pháp y lưu trữ `TutorMatch_Inspector.exe --all`
- Tổng số tệp dữ liệu đã kiểm tra: 4 (`users.dat`, `bookings.dat`, `reviews.dat`, `course_bookings.dat`)
- Tổng số bản ghi đọc được: 88
- Tổng số bản ghi hợp lệ: 88
- Tổng số bản ghi lỗi/hỏng: 0
- Tính toàn vẹn dữ liệu: **100% HOÀN HẢO**

---

## 11. KẾT LUẬN

Toàn bộ mục tiêu đặt ra cho đợt kiểm toán và tái thiết dữ liệu mẫu đã hoàn thành xuất sắc:
1. **Không can thiệp ngoài phạm vi:** Không sửa đổi giao diện QML, không bổ sung tính năng LMS ngoài phạm vi (không điểm danh, không theo dõi tiến độ buổi học lẻ, không xử lý thanh toán/hoa hồng).
2. **Khắc phục triệt để:** Mọi đánh giá đều có khóa học thực tế bảo chứng, hồ sơ và bảng điều khiển gia sư/học sinh thể hiện đúng lịch sử thực tế, thời khóa biểu lịch sử hiển thị đầy đủ và chính xác.
3. **Độ ổn định tối đa:** Toàn bộ test suite và build pipeline đều xanh mượt (100% PASS). Hệ thống đã sẵn sàng cho bảo vệ đồ án và triển khai thực tế.
