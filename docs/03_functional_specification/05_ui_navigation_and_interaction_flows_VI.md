# ĐIỀU HƯỚNG GIAO DIỆN VÀ LUỒNG TƯƠNG TÁC (UI NAVIGATION & INTERACTION FLOWS)
## Khảo sát Cấu trúc QML, Hệ thống Điều hướng Sidebar, Quản lý Hộp thoại Modal và Thiết kế Giao diện

---

## 1. CẤU TRÚC ĐIỀU HƯỚNG TỔNG THỂ (OVERALL UI ARCHITECTURE)

Giao diện đồ họa của TutorMatch được xây dựng trên nền tảng **Qt Quick 2.15+** với kiến trúc phân cấp rõ ràng:

```
[main.qml] (ApplicationWindow 1280x800)
    │
    ├── [LoginView.qml] (Khi guiBridge.isLoggedIn == false)
    │     ├── Biểu mẫu Đăng nhập
    │     ├── Biểu mẫu Đăng ký Học viên
    │     └── Biểu mẫu Đăng ký Gia sư
    │
    └── [MainDashboard.qml] (Khi guiBridge.isLoggedIn == true)
          ├── Top Header Bar (Thông tin User, Vai trò, Nút Đăng xuất)
          ├── Left Navigation Sidebar (Menu động theo Vai trò)
          └── Central Content StackView (Chuyển trang động không giật lag)
                ├── Student Views (Overview, Search, SmartMatch, Bookings)
                ├── Tutor Views (Overview, Bookings, Students, Statistics)
                └── Admin Views (Overview, Users, Matches, Reviews)
```

---

## 2. PHÂN NHÁNH MENU ĐIỀU HƯỚNG THEO VAI TRÒ (ROLE-BASED SIDEBAR NAVIGATION)

Khi đăng nhập thành công, thuộc tính `guiBridge.currentUserRole` sẽ quyết định danh sách các mục menu hiển thị trên thanh bên trái ([`MainDashboard.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/MainDashboard.qml)):

### 2.1. Thanh điều hướng Học viên (Role: `STUDENT`)
- **Mục 1 — Tổng quan & Lịch học (`StudentOverviewView.qml`):** Hiển thị thẻ hồ sơ học viên, thời khóa biểu các ca học trong tuần và các nút tác vụ nhanh.
- **Mục 2 — Tìm kiếm Gia sư (`StudentTutorSearchView.qml`):** Bộ lọc gia sư theo môn học, mức giá và tìm kiếm từ khóa tiếng Việt.
- **Mục 3 — Gợi ý SmartMatch (`StudentSmartMatchView.qml`):** Bảng chọn tiêu chí môn, ngân sách, lưới chọn ca rảnh và danh sách kết quả xếp hạng.
- **Mục 4 — Khóa học của tôi (`StudentBookingsView.qml`):** Theo dõi các khóa học đang chờ duyệt, khóa học đang diễn ra, lịch sử kết thúc sớm và đánh giá.

### 2.2. Thanh điều hướng Gia sư (Role: `TUTOR`)
- **Mục 1 — Bảng điều khiển (`TutorOverview.qml`):** Hiển thị thời khóa biểu tuần các lớp đang dạy và tóm tắt nhanh số lượng yêu cầu mới.
- **Mục 2 — Yêu cầu khóa học (`TutorBookings.qml`):** Danh sách các đơn đăng ký đang chờ duyệt (`PENDING`), xem cam kết và phê duyệt.
- **Mục 3 — Danh sách học viên (`TutorStudents.qml`):** Quản lý học viên đang theo học, theo dõi tiến độ tuần và gửi đề xuất kết thúc sớm.
- **Mục 4 — Thống kê & Đánh giá (`TutorStatistics.qml`):** Báo cáo tổng thu nhập tích lũy, điểm rating trung bình và danh sách tất cả nhận xét.

### 2.3. Thanh điều hướng Quản trị viên (Role: `ADMIN`)
- **Mục 1 — Tổng quan hệ thống (`AdminOverviewView.qml`):** 4 thẻ KPI nền tảng và danh sách kết nối gần đây.
- **Mục 2 — Quản lý người dùng (`AdminUsersView.qml`):** Bảng danh sách toàn bộ người dùng, tìm kiếm theo tên và nút Khóa/Mở khóa.
- **Mục 3 — Kết nối khóa học (`AdminMatchesView.qml`):** Bảng theo dõi các khóa học đang diễn ra, nút cưỡng chế dừng khóa học.
- **Mục 4 — Quản lý đánh giá (`AdminReviewsView.qml`):** Bảng kiểm duyệt đánh giá và nút xóa nhận xét vi phạm.

---

## 3. KIẾN TRÚC HỘP THOẠI VÀ LỚP PHỦ (MODAL & OVERLAY ARCHITECTURE)

Hệ thống sử dụng các hộp thoại Modal hiển thị đè lên màn hình hiện tại (Overlay) để xử lý các luồng tác vụ độc lập, giúp người dùng không bị mất ngữ cảnh làm việc:

1. **`BookingModal.qml`:**
   - Được mở từ `StudentTutorSearchView` hoặc `StudentSmartMatchView`.
   - Chứa thông tin tóm tắt gia sư, ô chọn môn học, bộ chọn thời lượng khóa học (4, 8, 12 tuần), ô chọn ngày bắt đầu (YYYY-MM-DD), lưới chọn các ca học trong tuần và hộp kiểm bắt buộc cam kết chính sách V1.
2. **`ReviewModal.qml`:**
   - Được mở từ `StudentBookingsView` khi học viên nhấn nút "Gửi đánh giá" trên thẻ khóa học `COMPLETED` hoặc `EARLY_TERMINATED`.
   - Cung cấp hàng 5 ngôi sao tương tác có hiệu ứng hover và ô văn bản nhập nhận xét phản hồi.
3. **`TutorDetailModal.qml`:**
   - Được mở khi nhấn vào thẻ gia sư.
   - Hiển thị đầy đủ tiểu sử, chuyên môn, bảng thời khóa biểu các ca rảnh và danh sách các đánh giá trước đó.
4. **`UserDetailModal.qml`:**
   - Dành riêng cho Quản trị viên khi nhấn vào một người dùng trong danh mục quản lý.
   - Xem chi tiết tất cả thuộc tính, trạng thái kích hoạt và cung cấp nút chuyển đổi trạng thái Khóa / Mở khóa tài khoản.
5. **`ProfileEditModal.qml`:**
   - Dành cho Học viên và Gia sư cập nhật thông tin cá nhân.

---

## 4. HỆ THỐNG THIẾT KẾ VÀ GIAO DIỆN CHUẨN ([`Theme.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/Theme.qml))

Toàn bộ 27 tệp QML đều sử dụng chung một tệp tài nguyên thiết kế tập trung `Theme.qml`:

- **Bảng màu chủ đạo (Color Palette):**
  - Màu thương hiệu chính (Primary): `#1E88E5` (Xanh lam công nghệ).
  - Màu phụ trợ (Secondary): `#26A69A` (Xanh mòng két).
  - Màu thành công (Success): `#43A047` (Xanh lá cây).
  - Màu cảnh báo (Warning): `#FB8C00` (Cam hổ phách).
  - Màu lỗi / Nguy hiểm (Danger/Error): `#E53935` (Đỏ son).
  - Màu nền (Background): `#F8F9FA` (Xám tro sáng).
  - Màu bề mặt thẻ (Card Surface): `#FFFFFF` (Trắng tinh khiết).
- **Hệ thống kiểu chữ (Typography):**
  - Font gia đình: `Segoe UI`, `Roboto`, `San Francisco`.
  - Tiêu đề lớn: 24px (Bold).
  - Tiêu đề mục: 18px (SemiBold).
  - Nội dung thân: 14px (Regular).
  - Chú thích nhỏ: 12px (Regular/Italic).
- **Quy chuẩn bố cục (Layout Constants):**
  - Bán kính bo góc thẻ (Card Radius): 10px.
  - Bán kính bo góc nút (Button Radius): 6px.
  - Khoảng cách lề tiêu chuẩn (Spacing): 16px, 24px.
