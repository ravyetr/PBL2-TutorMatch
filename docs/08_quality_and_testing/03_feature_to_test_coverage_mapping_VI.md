# MA TRẬN ĐỘ BAO PHỦ TÍNH NĂNG VÀ KIỂM THỬ (FEATURE TO TEST COVERAGE MAPPING)
## Bảng Đối Soát Toàn Diện Giữa Yêu Cầu Nghiệp Vụ, Thành Phần Mã Nguồn Và Ca Kiểm Thử Tự Động

---

## 1. NGUYÊN TẮC THIẾT KẾ MA TRẬN ĐỐI SOÁT `[VERIFIED]`

Ma trận kiểm thử dưới đây chứng minh tính truy vết hai chiều (Bidirectional Traceability) giữa:
- **Yêu cầu chức năng người dùng:** Toàn bộ 27 tính năng nghiệp vụ của Học viên (STU), Gia sư (TUT) và Quản trị viên (ADM).
- **Thành phần mã nguồn triển khai:** Tệp QML, C++ Bridge, Business Service và Kho dữ liệu.
- **Bộ kiểm thử tự động kiểm chứng:** Xác định chính xác Suite nào trong 30 Test Suites của `tests/TestRunner.cpp` đảm bảo tính đúng đắn của tính năng đó.

---

## 2. MA TRẬN BAO PHỦ TÍNH NĂNG HỌC VIÊN (STUDENT FEATURES) `[VERIFIED]`

| Mã tính năng | Tên tính năng nghiệp vụ | Lớp Service đảm nhiệm | Phương thức C++ Bridge | Bộ kiểm thử tự động kiểm chứng (Suite #) | Tỷ lệ Pass |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **STU-01** | Đăng ký & Đăng nhập Học viên | `AuthService` | `qmlLogin`, `qmlRegisterStudent` | **Suite 03, Suite 19** | 100% |
| **STU-02** | Xem Tổng quan & TKB Tuần | `BookingService` | `getWeeklyScheduleMatrix` | **Suite 04, Suite 07, Suite 21** | 100% |
| **STU-03** | Chỉnh sửa Hồ sơ Học viên | `UserService` | `updateStudentProfile` | **Suite 03, Suite 10, Suite 25** | 100% |
| **STU-04** | Đánh dấu Lịch bận Cá nhân | `UserService` | `qmlToggleStudentScheduleSlot` | **Suite 04, Suite 07, Suite 13** | 100% |
| **STU-05** | Tìm kiếm Gia sư Đa tiêu chí | `TutorSearchMatcher` | `searchAndFilterTutors` | **Suite 28** | 100% |
| **STU-06** | Gợi ý Gia sư Thông minh SmartMatch | `MatchingEngine` | `qmlSearchSmartMatches` | **Suite 01, Suite 25, Suite 27** | 100% |
| **STU-07** | Xem Chi tiết Hồ sơ Gia sư | `UserService` | `getTutorDetail`, `getTutorAvailableBookingSlots`| **Suite 06, Suite 07, Suite 28** | 100% |
| **STU-08** | Đăng ký Khóa học & Cam kết V1 | `BookingService` | `requestCourseBookingMulti` | **Suite 16, Suite 20, Suite 23, Suite 24** | 100% |
| **STU-09** | Quản lý Khóa học & Lịch sử | `BookingService` | `qmlGetStudentBookings` | **Suite 20, Suite 23** | 100% |
| **STU-10** | Hủy Đơn Đăng ký Chờ duyệt | `BookingService` | `qmlCancelBooking` | **Suite 19, Suite 20** | 100% |
| **STU-11** | Đề xuất Kết thúc sớm Khóa học | `BookingService` | `requestEarlyTermination` | **Suite 23, Suite 24** | 100% |
| **STU-12** | Gửi Đánh giá & Xếp hạng Sao | `ReviewService` | `qmlSubmitReview` | **Suite 19, Suite 24** | 100% |

---

## 3. MA TRẬN BAO PHỦ TÍNH NĂNG GIA SƯ (TUTOR FEATURES) `[VERIFIED]`

| Mã tính năng | Tên tính năng nghiệp vụ | Lớp Service đảm nhiệm | Phương thức C++ Bridge | Bộ kiểm thử tự động kiểm chứng (Suite #) | Tỷ lệ Pass |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **TUT-01** | Đăng ký & Đăng nhập Gia sư | `AuthService` | `qmlLogin`, `qmlRegisterTutor` | **Suite 03, Suite 19** | 100% |
| **TUT-02** | Xem Tổng quan & Thu nhập | `BookingService` | `qmlGetTutorTeachingMetrics` | **Suite 19, Suite 20** | 100% |
| **TUT-03** | Thiết lập Lịch dạy Cố định | `UserService` | `qmlToggleTutorScheduleSlot` | **Suite 04, Suite 07, Suite 13** | 100% |
| **TUT-04** | Cập nhật Chuyên môn & Học phí | `UserService` | `updateTutorProfile` | **Suite 06** | 100% |
| **TUT-05** | Tiếp nhận Đơn Đăng ký Học | `BookingService` | `qmlGetTutorPendingBookings` | **Suite 20, Suite 23** | 100% |
| **TUT-06** | Chấp nhận Khóa học & Cam kết V1 | `BookingService` | `qmlAcceptBooking` | **Suite 20, Suite 23, Suite 24** | 100% |
| **TUT-07** | Từ chối Đơn Đăng ký Học | `BookingService` | `qmlRejectBooking` | **Suite 20** | 100% |
| **TUT-08** | Phản hồi Kết thúc sớm | `BookingService` | `respondEarlyTermination` | **Suite 23, Suite 24** | 100% |
| **TUT-09** | Xem Lịch sử Đánh giá | `ReviewService` | `qmlGetReviewsByTutorId` | **Suite 19, Suite 24** | 100% |

---

## 4. MA TRẬN BAO PHỦ TÍNH NĂNG QUẢN TRỊ VIÊN (ADMIN FEATURES) `[VERIFIED]`

| Mã tính năng | Tên tính năng nghiệp vụ | Lớp Service đảm nhiệm | Phương thức C++ Bridge | Bộ kiểm thử tự động kiểm chứng (Suite #) | Tỷ lệ Pass |
| :---: | :--- | :--- | :--- | :--- | :---: |
| **ADM-01** | Bảng Điều hành & Chỉ số Sàn | `AdminService` | `getAdminMetrics` | **Suite 08, Suite 11** | 100% |
| **ADM-02** | Tra cứu Người dùng Đa tiêu chí | `AdminService` | `qmlGetFilteredUsers` | **Suite 11** | 100% |
| **ADM-03** | Khóa & Mở khóa Tài khoản | `AdminService` | `qmlToggleUserLock` | **Suite 10, Suite 11** | 100% |
| **ADM-04** | Giám sát & Hủy Cưỡng chế | `AdminService` | `qmlTerminateMatch` | **Suite 19, Suite 20** | 100% |
| **ADM-05** | Kiểm duyệt & Xóa Đánh giá | `AdminService` | `qmlDeleteViolatingReview` | **Suite 19, Suite 24** | 100% |
| **ADM-06** | Báo cáo Thống kê & Pháp chứng | `DataFileInspector` | `getAdminMetrics` | **Suite 15, Suite 22** | 100% |

---

## 5. MA TRẬN BAO PHỦ HẠ TẦNG KỸ THUẬT VÀ GIẢI THUẬT `[VERIFIED]`

| Hạng mục hạ tầng | Thành phần kỹ thuật | Mục tiêu kiểm định | Bộ kiểm thử tự động (Suite #) | Tỷ lệ Pass |
| :--- | :--- | :--- | :--- | :---: |
| **Giải thuật Bitmask** | `ScheduleGuard` | Giới hạn 56 ca, lọc bit ma, phát hiện xung đột $O(1)$. | **Suite 04, Suite 13** | 100% |
| **Cấu trúc Dữ liệu** | `AVLTree`, `HashTable`, `LRUCache`, `MaxHeap`, `List` | Cân bằng xoay cây, tra cứu bảng băm, trích xuất đống Top-K. | **Suite 12, Suite 30** | 100% |
| **Tuần tự hóa CSV** | Serializers (`User`, `Booking`, `Review`, `Utils`) | Mã hóa ký tự đặc biệt, tương thích ngược 13 $\to$ 23 cột. | **Suite 17, Suite 26** | 100% |
| **Kho Lưu Trữ Tệp** | `FileUserRepository`, `FileBookingRepo`, `FileReviewRepo`| Đọc ghi đa luồng, đồng bộ hóa mutex, tạo tệp hạt giống. | **Suite 14, Suite 18** | 100% |
| **Loại bỏ Xác thực Gia sư** | `Tutor`, `UserSerializer`, `AdminService` | Loại bỏ hoàn toàn cờ `isVerified`, dòng CSV gia sư 14 cột. | **Suite 29** | 100% |
