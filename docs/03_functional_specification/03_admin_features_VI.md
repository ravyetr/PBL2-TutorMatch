# ĐẶC TẢ CHỨC NĂNG DÀNH CHO QUẢN TRỊ VIÊN (ADMIN FEATURES SPECIFICATION)
## Khảo sát và Truy vết Chi tiết các Chức năng của Vai trò Quản trị viên

---

## 1. DANH MỤC CÁC TÍNH NĂNG ĐÃ TRIỂN KHAI THỰC TẾ `[VERIFIED]`

Dưới đây là bảng tổng hợp các chức năng của Quản trị viên (Admin) được xác minh trực tiếp từ mã nguồn:

| Mã tính năng | Tên tính năng nghiệp vụ | Thành phần QML chính | Phương thức C++ Bridge | Tệp dữ liệu liên quan |
| :---: | :--- | :--- | :--- | :--- |
| **ADM-01** | Bảng Tổng quan Chỉ số Nền tảng | `AdminOverviewView.qml` | `qmlGetAdminMetrics`, `qmlGetRecentConnections` | `data/users.dat`, `data/course_bookings.dat`, `data/reviews.dat` |
| **ADM-02** | Quản lý Tài khoản Người dùng | `AdminUsersView.qml` | `qmlGetAdminUsers` | `data/users.dat` |
| **ADM-03** | Xem Chi tiết Hồ sơ Người dùng | `UserDetailModal.qml` | `qmlGetUserDetailsForAdmin` | `data/users.dat` |
| **ADM-04** | Khóa / Mở khóa Tài khoản (Lock/Unlock) | `AdminUsersView.qml`, `UserDetailModal.qml` | `qmlToggleUserLock`, `qmlToggleUserActiveStatus` | `data/users.dat` |
| **ADM-05** | Giám sát Toàn bộ Kết nối Khóa học | `AdminMatchesView.qml` | `qmlGetAllMatches` | `data/course_bookings.dat` |
| **ADM-06** | Cưỡng chế Chấm dứt Khóa học (Force Terminate)| `AdminMatchesView.qml` | `qmlAdminTerminateMatch` | `data/course_bookings.dat` |
| **ADM-07** | Kiểm duyệt & Xóa Đánh giá Vi phạm | `AdminReviewsView.qml` | `qmlGetAllReviews`, `qmlDeleteReview` | `data/reviews.dat`, `data/users.dat` |

> [!WARNING]
> **Các tính năng Admin ĐÃ BÃI BỎ HOÀN TOÀN (Tuyệt đối không còn trong hệ thống):**
> 1. **Duyệt xác minh gia sư (Tutor Verification):** Không còn nút hoặc logic phê duyệt huy hiệu xác minh cho gia sư.
> 2. **Thu phí hoa hồng (Platform Commission):** Không còn chức năng thu tiền, trích phần trăm hoa hồng hay ví điện tử.
> 3. **Phân cấp quyền quản trị (Admin Permission Level 1–9):** Đã loại bỏ hoàn toàn thuộc tính `permissionLevel`. Mọi tài khoản `ADMIN` là Root Admin với thẩm quyền quản trị tối cao, thống nhất và bình đẳng.

---

## 2. ĐẶC TẢ CHI TIẾT TỪNG TÍNH NĂNG VÀ TRUY VẾT MÃ NGUỒN

### 2.1. ADM-01: Bảng Tổng quan Chỉ số Nền tảng
- **Mục đích:** Cung cấp bức tranh toàn cảnh về sức khỏe của nền tảng kết nối.
- **Dữ liệu đầu vào:** Không (tự động tổng hợp khi nạp view).
- **Quy tắc xử lý:**
  - `AdminService::getSystemMetrics()` quét các kho lưu trữ:
    - Tổng số người dùng: `totalUsers`.
    - Số lượng học viên: `totalStudents`.
    - Số lượng gia sư: `totalTutors`.
    - Tổng số khóa học: `totalBookings`.
    - Số khóa học đã kết nối thành công (`ACCEPTED` / `COMPLETED`): `activeBookings`.
    - Tỷ lệ kết nối thành công: `matchSuccessRate = (activeBookings / totalBookings) * 100%`.
    - Tổng số đánh giá: `totalReviews`.
- **Dữ liệu đầu ra:** Hiển thị 4 thẻ số liệu KPI và danh sách 5 kết nối gần đây nhất trên `AdminOverviewView.qml`.
- **Truy vết mã nguồn:**
  - QML: [`ui/qml/AdminOverviewView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminOverviewView.qml)
  - C++ Bridge: [`GUIBridge::qmlGetAdminMetrics`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp#L480)
  - Business Service: [`AdminService::getSystemMetrics`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AdminService.cpp#L18)
  - Data Files: `data/users.dat`, `data/course_bookings.dat`, `data/reviews.dat`
  - Test Suite: Suite 3 & Suite 11 (`run_test_admin_service_filters_and_search`).

---

### 2.2. ADM-04: Khóa / Mở khóa Tài khoản (Account Lock/Unlock)
- **Mục đích:** Vô hiệu hóa tài khoản người dùng vi phạm quy chế hoặc mở lại khi đã giải quyết tranh chấp.
- **Tiền điều kiện:** Quản trị viên mở danh sách người dùng hoặc hộp thoại chi tiết.
- **Dữ liệu đầu vào:** `userId` của người dùng mục tiêu.
- **Quy tắc xử lý:**
  - Quản trị viên không thể tự khóa tài khoản của chính mình.
  - Đảo ngược trạng thái thuộc tính `isActive`: `newActiveState = !user->getIsActive()`.
  - Cập nhật đối tượng `User` vào `IUserRepository`.
  - Ghi tức thì xuống `users.dat`.
  - **Tác động:** Nếu tài khoản bị khóa (`isActive == false`), người dùng đó sẽ không thể đăng nhập vào hệ thống ở các lần sau (bị `AuthService::login` từ chối).
- **Dữ liệu đầu ra:** Cập nhật trạng thái hiển thị trên thẻ người dùng và phát tín hiệu `dataChanged()`.
- **Truy vết mã nguồn:**
  - QML: [`ui/qml/AdminUsersView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminUsersView.qml) & [`UserDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/UserDetailModal.qml)
  - C++ Bridge: [`GUIBridge::qmlToggleUserLock`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp#L525)
  - Business Service: [`AdminService::setUserActiveStatus`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AdminService.cpp#L105)
  - Repository: `FileUserRepository`
  - Data File: `data/users.dat`
  - Test Suite: Suite 10 (`run_test_admin_verification_and_lock_sync`).

---

### 2.3. ADM-06: Cưỡng chế Chấm dứt Khóa học (Force Terminate Match)
- **Mục đích:** Can thiệp giải quyết xung đột khi hai bên không thể tự thỏa thuận kết thúc sớm.
- **Tiền điều kiện:** Khóa học đang ở trạng thái `ACCEPTED` hoặc `PENDING`.
- **Dữ liệu đầu vào:** `bookingId`.
- **Quy tắc xử lý:**
  - `AdminService::terminateMatch(bookingId, outMsg)` tìm kiếm khóa học trong `ICourseBookingRepository`.
  - Chuyển trạng thái khóa học sang `CANCELLED`.
  - Toàn bộ các ca học của khóa học này được giải phóng ngay lập tức trên thời khóa biểu của cả học viên và gia sư.
- **Dữ liệu đầu ra:** Bản ghi được cập nhật trong `course_bookings.dat`.
- **Truy vết mã nguồn:**
  - QML: [`ui/qml/AdminMatchesView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminMatchesView.qml)
  - C++ Bridge: [`GUIBridge::qmlAdminTerminateMatch`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp#L560)
  - Business Service: [`AdminService::terminateMatch`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AdminService.cpp#L161)
  - Data File: `data/course_bookings.dat`
  - Test Suite: Suite 3 & Suite 24.

---

### 2.4. ADM-07: Kiểm duyệt & Xóa Đánh giá Vi phạm (Delete Violating Review)
- **Mục đích:** Loại bỏ các bài nhận xét chứa ngôn từ tiêu cực hoặc vi phạm tiêu chuẩn cộng đồng.
- **Tiền điều kiện:** Bài đánh giá tồn tại trong hệ thống.
- **Dữ liệu đầu vào:** `reviewId`.
- **Quy tắc xử lý:**
  - `AdminService::deleteViolatingReview(reviewId, outMsg)` xóa bản ghi khỏi `IReviewRepository`.
  - Tự động lấy danh sách toàn bộ các đánh giá còn lại của gia sư mục tiêu.
  - Tính toán lại điểm trung bình mới: `newAvg = allRevs.empty() ? 0.0 : (sum / allRevs.size())`.
  - Cập nhật lại thuộc tính `rating` và `totalReviews` của gia sư trong `users.dat`.
- **Dữ liệu đầu ra:** Xóa bài review khỏi `reviews.dat`, cập nhật rating trong `users.dat`.
- **Truy vết mã nguồn:**
  - QML: [`ui/qml/AdminReviewsView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/AdminReviewsView.qml)
  - C++ Bridge: [`GUIBridge::qmlDeleteReview`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp#L585)
  - Business Service: [`AdminService::deleteViolatingReview`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AdminService.cpp#L125)
  - Repositories: `FileReviewRepository`, `FileUserRepository`
  - Data Files: `data/reviews.dat`, `data/users.dat`
  - Test Suite: Suite 17 & Suite 18.
