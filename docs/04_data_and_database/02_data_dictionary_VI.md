# TỪ ĐIỂN DỮ LIỆU TOÀN DIỆN (DATA DICTIONARY)
## Chi tiết Thuộc tính, Kiểu Dữ liệu, Ràng buộc và Quy tắc Xác thực cho Mọi Thực thể Miền

---

## 1. THỰC THỂ NGƯỜI DÙNG CƠ SỞ (BASE ENTITY: `User`)

Lớp cơ sở trừu tượng đại diện cho tất cả các loại người dùng trong hệ thống:

| Tên trường C++ | Tên trường CSV | Kiểu dữ liệu C++ | Kiểu lưu trữ CSV | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Quy tắc xác thực |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| `id` | Cột 1 | `int` | Số nguyên | Có | `0` | **Có** | Khóa chính (PK), số nguyên dương tự tăng duy nhất (`> 0`). |
| `username` | Cột 2 | `std::string` | Chuỗi ký tự | Có | `""` | **Có** | Tên đăng nhập duy nhất (UK), không rỗng, không chứa khoảng trắng. |
| `passwordHash` | Cột 3 | `std::string` | Chuỗi ký tự | Có | `""` | **Có** | Chuỗi băm SHA-256 an toàn (độ dài 64 ký tự hex). |
| `email` | Cột 4 | `std::string` | Chuỗi ký tự | Có | `""` | **Có** | Địa chỉ email hợp lệ, kiểm tra qua regex `isValidEmail()`. |
| `fullName` | Cột 5 | `std::string` | Chuỗi ký tự | Có | `""` | **Có** | Họ tên hiển thị của người dùng, hỗ trợ tiếng Việt có dấu. |
| `role` | Cột 6 | `UserRole` (Enum) | Chuỗi ký tự | Có | `STUDENT` | **Có** | Thuộc một trong 3 giá trị: `"STUDENT"`, `"TUTOR"`, `"ADMIN"`. |
| `isActive` | Cột 7 | `bool` | Số nguyên (0/1) | Có | `true` (1) | **Có** | Trạng thái tài khoản: `1` = Hoạt động, `0` = Bị khóa bởi Quản trị viên. |

---

## 2. THỰC THỂ HỌC VIÊN (DERIVED ENTITY: `Student`)

Kế thừa từ `User`, bổ sung 2 trường chuyên biệt (Tổng cộng 9 trường trong CSV):

| Tên trường C++ | Tên trường CSV | Kiểu dữ liệu C++ | Kiểu lưu trữ CSV | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Quy tắc xác thực |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| *[7 trường cơ sở]* | Cột 1-7 | *Xem bảng User* | *Xem bảng User* | Có | - | **Có** | Vai trò cố định là `"STUDENT"`. |
| `gradeLevel` | Cột 8 | `int` | Số nguyên | Có | `10` | **Có** | Khối lớp học phổ thông, giá trị trong khoảng `[1..12]`. |
| `scheduleBitmask` | Cột 9 | `uint64_t` | Số nguyên 64-bit | Có | `0` | **Có** | Mặt nạ nhị phân 56 ca bận trong tuần. Luôn được khử bit ma bằng `ScheduleGuard::sanitizeMask()`. |

---

## 3. THỰC THỂ GIA SƯ (DERIVED ENTITY: `Tutor`)

Kế thừa từ `User`, bổ sung 7 trường chuyên biệt (Tổng cộng 14 trường trong CSV):

| Tên trường C++ | Tên trường CSV | Kiểu dữ liệu C++ | Kiểu lưu trữ CSV | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Quy tắc xác thực |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| *[7 trường cơ sở]* | Cột 1-7 | *Xem bảng User* | *Xem bảng User* | Có | - | **Có** | Vai trò cố định là `"TUTOR"`. |
| `bio` | Cột 8 | `std::string` | Chuỗi ký tự | Không | `""` | **Có** | Giới thiệu bản thân, bằng cấp và kinh nghiệm giảng dạy. |
| `subjectSpecialty` | Cột 9 | `std::string` | Chuỗi ký tự | Có | `""` | **Có** | Môn chuyên môn giảng dạy (ví dụ: `"Toán 12 - Luyện thi Đại học"`). |
| `hourlyRate` | Cột 10 | `double` | Số thực | Có | `0.0` | **Có** | Mức học phí đề xuất cho mỗi buổi học (VNĐ/buổi), `>= 0.0`. |
| `rating` | Cột 11 | `double` | Số thực | Có | `5.0` | **Có** | Điểm đánh giá trung bình tích lũy, giá trị trong đoạn `[0.0..5.0]`. |
| `totalReviews` | Cột 12 | `int` | Số nguyên | Có | `0` | **Có** | Tổng số lượt đánh giá hợp lệ đã nhận từ học viên (`>= 0`). |
| `totalEarnings` | Cột 13 | `double` | Số thực | Có | `0.0` | **Có** | Tổng giá trị học phí tích lũy từ các khóa học đã hoàn thành theo thỏa thuận (`status == "COMPLETED"`), `>= 0.0`. Không phải số tiền thực nhận (nền tảng không xử lý thanh toán/ví tiền); không tính các khóa `PENDING`, `ACCEPTED`, `REJECTED`, `CANCELLED`, `EARLY_TERMINATED`. Tự động đối soát và đồng bộ hóa từ `course_bookings.dat` khi hoàn tất khóa học hoặc khi ứng dụng khởi chạy. |
| `scheduleBitmask` | Cột 14 | `uint64_t` | Số nguyên 64-bit | Có | `0` | **Có** | Mặt nạ nhị phân 56 ca rảnh trong tuần (`VALID_SCHEDULE_MASK`). |

---

## 4. THỰC THỂ QUẢN TRỊ VIÊN (DERIVED ENTITY: `Admin`)

Kế thừa từ `User`, sử dụng trực tiếp 7 trường cơ sở, không có thuộc tính bổ sung (Đúng 7 trường trong CSV):

| Tên trường C++ | Tên trường CSV | Kiểu dữ liệu C++ | Kiểu lưu trữ CSV | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Quy tắc xác thực |
| :--- | :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| *[7 trường cơ sở]* | Cột 1-7 | *Xem bảng User* | *Xem bảng User* | Có | - | **Có** | Vai trò cố định là `"ADMIN"`. Mọi tài khoản Admin đều là Root Admin, sở hữu toàn quyền quản trị nền tảng (khóa/mở khóa tài khoản, duyệt xóa đánh giá, giám sát và hủy kết nối khóa học). |

---

## 5. THỰC THỂ KHÓA HỌC ĐỊNH KỲ (`CourseBooking`)

Lưu trữ hợp đồng đào tạo nhiều tuần giữa học viên và gia sư (Từ 12 đến 21 trường trong CSV):

| Tên trường C++ | Cột CSV | Kiểu dữ liệu | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Ý nghĩa nghiệp vụ |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| `id` | 1 | `int` | Có | `0` | **Có** | Khóa chính (PK), số nguyên tự tăng duy nhất. |
| `studentId` | 2 | `int` | Có | `0` | **Có** | Khóa ngoại (FK) trỏ tới `User.id` có vai trò `STUDENT`. |
| `tutorId` | 3 | `int` | Có | `0` | **Có** | Khóa ngoại (FK) trỏ tới `User.id` có vai trò `TUTOR`. |
| `studentName` | 4 | `std::string` | Có | `""` | **Có** | Họ tên học viên (Denormalized phục vụ hiển thị nhanh). |
| `tutorName` | 5 | `std::string` | Có | `""` | **Có** | Họ tên gia sư (Denormalized phục vụ hiển thị nhanh). |
| `subject` | 6 | `std::string` | Có | `""` | **Có** | Tên môn học của khóa đào tạo. |
| `hourlyRate` | 7 | `double` | Có | `0.0` | **Có** | Mức học phí thỏa thuận mỗi buổi học (`>= 0.0`). |
| `startDate` | 8 | `std::string` | Có | `""` | **Có** | Ngày bắt đầu khóa học theo định dạng `YYYY-MM-DD`. |
| `weeksCount` | 9 | `int` | Có | `4` | **Có** | Số tuần đào tạo, thuộc tập hợp `{4, 8, 12}`. |
| `status` | 10 | `std::string` | Có | `"PENDING"`| **Có** | Thuộc tập `{PENDING, ACCEPTED, REJECTED, COMPLETED, CANCELLED, EARLY_TERMINATED}`. |
| `createdAt` | 11 | `std::string` | Có | `""` | **Có** | Ngày tạo đơn theo định dạng `YYYY-MM-DD`. |
| `weeklySchedule` | 12 | `std::string` | Có | `""` | **Có** | Chuỗi ca học tuần định dạng `"dow:slot;dow:slot"` (ví dụ: `"0:1;4:0"`). Nguồn chân lý duy nhất cho lịch học định kỳ trong tuần. |
| `earlyTerminationRequesterId` | 13 | `int` | Không | `0` | **Có** | ID của bên đề xuất kết thúc sớm (`0` nếu chưa có yêu cầu). |
| `earlyTerminationStatus` | 14 | `std::string` | Không | `"NONE"` | **Có** | Trạng thái yêu cầu: `"NONE"`, `"PENDING"`, `"ACCEPTED"`, `"REJECTED"`. |
| `earlyTerminationReason` | 15 | `std::string` | Không | `""` | **Có** | Lý do đề xuất dừng khóa học trước thời hạn. |
| `actualEndDate` | 16 | `std::string` | Không | `""` | **Có** | Ngày kết thúc sớm thực tế (`YYYY-MM-DD`). |
| `studentAcknowledged` | 17 | `bool` (1/0) | Không | `false` | **Có** | Đánh dấu học viên đã tick đồng ý Cam kết V1. |
| `studentAckTimestamp` | 18 | `std::string` | Không | `""` | **Có** | Dấu thời gian ISO học viên xác nhận cam kết. |
| `tutorAcknowledged` | 19 | `bool` (1/0) | Không | `false` | **Có** | Đánh dấu gia sư đã tick đồng ý Cam kết V1. |
| `tutorAckTimestamp` | 20 | `std::string` | Không | `""` | **Có** | Dấu thời gian ISO gia sư xác nhận cam kết. |
| `commitmentVersion` | 21 | `std::string` | Không | `""` | **Có** | Phiên bản cam kết, giá trị chuẩn là `"COMMITMENT_V1"`. |

---

## 6. THỰC THỂ ĐÁNH GIÁ VÀ XẾP HẠNG (`Review`)

Lưu trữ nhận xét và điểm số đánh giá khóa học (Gồm đúng 8 trường trong CSV):

| Tên trường C++ | Cột CSV | Kiểu dữ liệu | Bắt buộc? | Mặc định | Lưu file? | Ràng buộc & Ý nghĩa nghiệp vụ |
| :--- | :---: | :--- | :---: | :---: | :---: | :--- |
| `id` | 1 | `int` | Có | `0` | **Có** | Khóa chính (PK), số nguyên tự tăng duy nhất. |
| `bookingId` | 2 | `int` | Có | `0` | **Có** | Khóa ngoại (FK) trỏ tới `CourseBooking.id` (Mỗi khóa học tối đa 1 review). |
| `studentId` | 3 | `int` | Có | `0` | **Có** | Khóa ngoại (FK) trỏ tới `User.id` của người gửi đánh giá. |
| `tutorId` | 4 | `int` | Có | `0` | **Có** | Khóa ngoại (FK) trỏ tới `User.id` của gia sư được đánh giá. |
| `studentName` | 5 | `std::string` | Có | `""` | **Có** | Họ tên học viên gửi đánh giá (Denormalized). |
| `ratingScore` | 6 | `int` | Có | `5` | **Có** | Điểm số đánh giá sao, bắt buộc nằm trong đoạn `[1..5]`. |
| `comment` | 7 | `std::string` | Có | `""` | **Có** | Nội dung nhận xét bằng văn bản, không được để chuỗi rỗng. |
| `createdAt` | 8 | `std::string` | Có | `""` | **Có** | Ngày gửi đánh giá theo định dạng `YYYY-MM-DD`. |
