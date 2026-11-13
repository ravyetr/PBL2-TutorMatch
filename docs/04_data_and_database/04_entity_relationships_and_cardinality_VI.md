# QUAN HỆ THỰC THỂ VÀ BỘI SỐ LIÊN KẾT (ENTITY RELATIONSHIPS & CARDINALITY)
## Phân tích Chi tiết Khóa ngoại, Bội số Quan hệ, Quyền sở hữu và Hành vi Cập nhật/Xóa

---

## 1. BẢNG TỔNG HỢP CÁC QUAN HỆ THỰC THỂ

Trong kiến trúc lưu trữ tệp phẳng của TutorMatch, các mối quan hệ logic giữa các thực thể miền được liên kết thông qua các trường mã định danh (ID References) và được **tầng Ứng dụng C++ (Business Services & Repositories) bảo đảm tính toàn vẹn**:

| Thực thể nguồn | Bội số | Thực thể đích | Trường liên kết (Foreign Ref) | Ý nghĩa nghiệp vụ | Cơ chế cưỡng chế toàn vẹn |
| :--- | :---: | :--- | :--- | :--- | :--- |
| **Student** | $1 - N$ | **CourseBooking** | `CourseBooking.studentId`<br>$\rightarrow$ `User.id` (STUDENT) | Một học viên có thể gửi nhiều yêu cầu hoặc tham gia nhiều khóa học định kỳ khác nhau theo thời gian. | Kiểm tra tồn tại trong `BookingService::createRecurringCourseBooking`. |
| **Tutor** | $1 - N$ | **CourseBooking** | `CourseBooking.tutorId`<br>$\rightarrow$ `User.id` (TUTOR) | Một gia sư có thể tiếp nhận và giảng dạy nhiều khóa học định kỳ (miễn là không bị trùng lịch các ca học tuần). | Kiểm tra tồn tại & kiểm tra va chạm lịch bằng `ScheduleGuard`. |
| **CourseBooking** | $1 - 0..1$ | **Review** | `Review.bookingId`<br>$\rightarrow$ `CourseBooking.id` | Mỗi khóa học hoàn thành (`COMPLETED` hoặc `EARLY_TERMINATED`) sinh ra **tối đa duy nhất một bài đánh giá**. | Cưỡng chế độc quyền tại `ReviewService::isEligibleForReview`. |
| **Student** | $1 - N$ | **Review** | `Review.studentId`<br>$\rightarrow$ `User.id` (STUDENT) | Một học viên có thể viết nhiều bài đánh giá cho các khóa học khác nhau mà mình đã từng học. | Kiểm tra quyền sở hữu đơn đặt lịch của học viên đó. |
| **Tutor** | $1 - N$ | **Review** | `Review.tutorId`<br>$\rightarrow$ `User.id` (TUTOR) | Một gia sư nhận được nhiều bài nhận xét và đánh giá sao từ các học viên khác nhau; điểm số được tổng hợp vào `rating`. | Tự động tính toán lại điểm trung bình cộng tích lũy mỗi khi thêm/xóa review. |
| **User (Base)** | $1 - 1$ | **Student/Tutor/Admin** | Kế thừa hướng đối tượng (`is-a`) | Quan hệ thừa kế đa hình (Polymorphism). Mỗi dòng dữ liệu tương ứng với đúng một kiểu con cụ thể. | Phân loại tự động theo trường thứ 6 `role` trong `UserSerializer::fromCsv()`. |

### 1.1. Sơ đồ Thực thể Liên kết Mức Logic (Logical Entity Relationship Diagram)

Sơ đồ ERD logic dưới đây chi tiết hóa các trường dữ liệu, kiểu dữ liệu, các ràng buộc toàn vẹn (PK, FK, UK) và bội số liên kết giữa 3 thực thể dữ liệu chính (`Users`, `CourseBookings`, `Reviews`) trong kiến trúc TutorMatch:

```mermaid
erDiagram
    %% =========================================================================
    %% DIAGRAM 17: LOGICAL ENTITY RELATIONSHIP DIAGRAM (ERD)
    %% =========================================================================
    title LOGICAL ENTITY RELATIONSHIP DIAGRAM (ATTRIBUTES & CONSTRAINTS)

    Users {
        int id PK "Mã ID duy nhất (Tự tăng)"
        string username UK "Tên đăng nhập (Duy nhất)"
        string passwordHash "Mật khẩu băm SHA-256 (64 ký tự hex)"
        string email "Email liên hệ"
        string fullName "Họ tên hiển thị"
        string role "STUDENT | TUTOR | ADMIN"
        int isActive "1: Hoạt động, 0: Bị khóa"
        int gradeLevel "1-12 (Dành cho Student)"
        uint64 scheduleBitmask "56-slot bitmask [0..0x00FFFFFFFFFFFFFF]"
        string bio "Tiểu sử giới thiệu (Tutor)"
        string subjectSpecialty "Môn chuyên môn (Tutor)"
        double hourlyRate "Học phí VNĐ/buổi (Tutor >= 0)"
        double rating "Điểm sao trung bình [0.0..5.0] (Tutor)"
        int totalReviews "Số lượng đánh giá (Tutor >= 0)"
        double totalEarnings "Tổng thu nhập VNĐ (Tutor >= 0)"
    }

    CourseBookings {
        int id PK "Mã ID khóa học định kỳ (Tự tăng)"
        int studentId FK "Tham chiếu Users.id (Role STUDENT)"
        int tutorId FK "Tham chiếu Users.id (Role TUTOR)"
        string studentName "Tên học viên (Denormalized)"
        string tutorName "Tên gia sư (Denormalized)"
        string subject "Tên môn học"
        double hourlyRate "Học phí tại thời điểm đặt"
        string startDate "Ngày bắt đầu YYYY-MM-DD"
        int weeksCount "Thời lượng tuần (4, 8, 12)"
        string status "PENDING | ACCEPTED | REJECTED | COMPLETED | CANCELLED | EARLY_TERMINATED"
        string createdAt "Ngày tạo YYYY-MM-DD"
        string weeklySchedule "Lịch định kỳ: 'dow:slot;...' (Nguồn chân lý duy nhất)"
        int earlyTerminationRequesterId "ID người đề nghị kết thúc sớm (0 nếu không)"
        string earlyTerminationStatus "NONE | PENDING | ACCEPTED | REJECTED"
        string earlyTerminationReason "Lý do kết thúc sớm"
        string actualEndDate "Ngày kết thúc sớm thực tế"
        int studentAcknowledged "1 nếu học viên đồng ý Cam kết V1"
        string studentAckTimestamp "Dấu thời gian ISO học viên cam kết"
        int tutorAcknowledged "1 nếu gia sư đồng ý Cam kết V1"
        string tutorAckTimestamp "Dấu thời gian ISO gia sư cam kết"
        string commitmentVersion "Phiên bản cam kết: 'COMMITMENT_V1'"
    }

    Reviews {
        int id PK "Mã ID đánh giá (Tự tăng)"
        int bookingId FK "Tham chiếu CourseBookings.id (Unique per booking)"
        int studentId FK "Tham chiếu Users.id (Học viên gửi)"
        int tutorId FK "Tham chiếu Users.id (Gia sư nhận)"
        string studentName "Tên học viên (Denormalized)"
        int ratingScore "Điểm sao [1..5]"
        string comment "Nội dung nhận xét (Không rỗng)"
        string createdAt "Ngày đánh giá YYYY-MM-DD"
    }

    Users ||--o{ CourseBookings : "Student đặt / Tutor dạy"
    CourseBookings ||--o| Reviews : "1 đơn COMPLETED/EARLY sinh tối đa 1 review"
    Users ||--o{ Reviews : "Student viết / Tutor nhận"
```

#### Diễn giải ERD Logic và Ràng buộc Toàn vẹn:

1. **Khóa Chính và Tính Duy Nhất (Primary & Unique Keys):**
   - Cả ba thực thể đều sử dụng khóa chính dạng số nguyên tự tăng `id (PK)`.
   - Thuộc tính `username` trong `Users` có ràng buộc duy nhất `(UK)` nhằm ngăn chặn đăng ký trùng lặp tài khoản.
2. **Khóa Ngoại và Ràng buộc Toàn vẹn (Foreign Key Constraints):**
   - Trong `CourseBookings`: `studentId (FK)` và `tutorId (FK)` bắt buộc phải trỏ đến các ID hợp lệ trong bảng `Users` có vai trò tương ứng (`STUDENT` và `TUTOR`).
   - Trong `Reviews`: `bookingId (FK)` tham chiếu đến khóa học cụ thể; đồng thời ràng buộc $1 - 0..1$ bảo đảm mỗi đơn đặt lịch chỉ có thể sinh ra tối đa một bài đánh giá.
3. **Kỹ thuật Phi Chuẩn hóa Có Kiểm Soát (Controlled Denormalization):**
   - Thuộc tính `studentName` và `tutorName` được lưu trực tiếp trong cả `CourseBookings` và `Reviews`. Kỹ thuật này giúp hệ thống đạt tốc độ hiển thị cực đại trong môi trường lưu trữ tệp phẳng CSV mà không cần phải thực hiện các phép join bộ nhớ phức tạp.

---

## 2. QUYỀN SỞ HỮU VÀ VÒNG ĐỜI DỮ LIỆU (OWNERSHIP & LIFECYCLE)

1. **Quyền sở hữu Khóa học (`CourseBooking`):**
   - Khóa học thuộc quyền đồng sở hữu giữa Học viên tạo đơn (`studentId`) và Gia sư nhận lớp (`tutorId`).
   - Học viên có quyền hủy đơn (`CANCELLED`) khi đơn còn ở trạng thái `PENDING`.
   - Gia sư có quyền Chấp nhận (`ACCEPTED`) hoặc Từ chối (`REJECTED`) khi đơn còn ở trạng thái `PENDING`.
   - Cả hai bên đều có quyền đề xuất kết thúc sớm (`requestEarlyTermination`) khi đơn đang ở trạng thái `ACCEPTED`, nhưng quyền xác nhận kết thúc sớm (`respondEarlyTermination`) thuộc về bên còn lại.
2. **Quyền sở hữu Bản ghi Đánh giá (`Review`):**
   - Bản ghi Review thuộc về Học viên đã tạo ra nó (`studentId`) và gắn liền với Gia sư được đánh giá (`tutorId`).
   - Bản ghi Review chỉ tồn tại khi có một khóa học hợp lệ tương ứng (`bookingId`).
   - Quản trị viên (Admin) có quyền xóa bài đánh giá vi phạm tiêu chuẩn (`deleteViolatingReview`), khi đó hệ thống tự động bồi hoàn và tính toán lại điểm số cho Gia sư.

---

## 3. HÀNH VI KHI XÓA VÀ CẬP NHẬT (CASCADE & INTEGRITY BEHAVIOR)

Do không có cơ chế `ON DELETE CASCADE` tự động của RDBMS, TutorMatch triển khai các nguyên tắc bảo vệ dữ liệu ở cấp độ mã nguồn C++:

### 3.1. Hành vi khi Xóa Người Dùng (Soft Delete Policy)
- Hệ thống áp dụng triệt để chính sách **Xóa Mềm (Soft Delete)**: Thuộc tính `User::isActive` được chuyển thành `false` (0).
- Bản ghi User **KHÔNG BỊ XÓA VẬT LÝ** khỏi tệp `data/users.dat`.
- **Lý do:** Giữ nguyên vẹn tính toàn vẹn tham chiếu cho toàn bộ các khóa học cũ trong `course_bookings.dat` và các bài đánh giá lịch sử trong `reviews.dat`. Nếu xóa cứng bản ghi User, các tệp còn lại sẽ chứa mã `studentId` hoặc `tutorId` mồ côi (orphan keys).

### 3.2. Tính Bất Biến của Khóa Ngoại trong Khóa Học
- Các trường `id`, `studentId`, `tutorId` trong `CourseBooking` là **bất biến (Immutable)** sau khi đã tạo. Không có bất kỳ API nào cho phép sửa đổi học viên hoặc gia sư của một khóa học đã được lưu.

### 3.3. Cơ chế Chống Đánh Giá Trùng Lặp (Duplicate Review Prevention)
- Trong [`ReviewService::isEligibleForReview()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/ReviewService.cpp#L45): Trước khi cho phép gửi đánh giá, hệ thống duyệt toàn bộ `reviews.dat`. Nếu đã tồn tại bất kỳ bản ghi nào có `r.bookingId == targetBookingId`, hệ thống từ chối ngay lập tức với thông báo `"Bạn đã đánh giá khóa học này rồi!"`. Bội số $1 - 0..1$ được bảo đảm tuyệt đối.
