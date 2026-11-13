# THUẬT NGỮ VÀ TỪ ĐIỂN KHÁI NIỆM HỆ THỐNG (TERMINOLOGY & GLOSSARY)
## Bảng Tra cứu Khái niệm Nghiệp vụ, Kỹ thuật và Bất biến Hệ thống

---

## 1. THUẬT NGỮ NGHIỆP VỤ (DOMAIN TERMS)

| Thuật ngữ | Tên tiếng Anh | Định nghĩa & Ý nghĩa kỹ thuật |
| :--- | :--- | :--- |
| **Học viên** | Student | Người dùng đóng vai trò tìm kiếm gia sư, có các thuộc tính: khối lớp (`gradeLevel`), ngân sách tối đa và mặt nạ lịch bận cá nhân (`scheduleBitmask`). |
| **Gia sư** | Tutor | Người dùng cung cấp dịch vụ giảng dạy, có các thuộc tính: chuyên môn (`subjectSpecialty`), học phí (`hourlyRate`), điểm đánh giá (`rating`), số lượt đánh giá (`totalReviews`), tổng thu nhập (`totalEarnings`) và lịch rảnh (`scheduleBitmask`). |
| **Quản trị viên** | Admin | Người dùng điều hành nền tảng, sở hữu vai trò Root Admin với toàn quyền quản trị hệ thống, quyền khóa tài khoản và xóa đánh giá vi phạm. |
| **Khóa học định kỳ** | CourseBooking / Recurring Course | Hợp đồng đào tạo giữa một học viên và một gia sư, diễn ra trong 4, 8 hoặc 12 tuần liên tiếp, gồm một hoặc nhiều ca học cố định trong tuần (`weeklySchedule`). |
| **Đánh giá & Nhận xét** | Review | Bản ghi phản hồi chất lượng từ học viên gửi cho gia sư sau khi khóa học đã chuyển sang trạng thái `COMPLETED` hoặc `EARLY_TERMINATED`. Bao gồm số sao (1-5) và văn bản nhận xét. |
| **Cam kết hai phía V1** | Commitment V1 (`COMMITMENT_V1`) | Điều khoản pháp lý - nghiệp vụ quy định: Một khóa học chỉ được chính thức xác lập (`ACCEPTED`) khi cả học viên và gia sư đều tick xác nhận đồng ý với chính sách. Khóa học chỉ được kết thúc sớm khi có sự đồng thuận từ cả hai phía. |
| **Kết thúc sớm có đồng thuận** | Consensual Early Termination | Quy trình cho phép một bên đề xuất dừng khóa học trước thời hạn. Khóa học chỉ chuyển sang trạng thái `EARLY_TERMINATED` khi bên còn lại phản hồi đồng ý (`agree == true`). |

---

## 2. THUẬT NGỮ THUẬT TOÁN VÀ KỸ THUẬT (TECHNICAL & ALGORITHMIC TERMS)

| Thuật ngữ | Ký hiệu / Định dạng | Giải thích bản chất kỹ thuật |
| :--- | :---: | :--- |
| **Mặt nạ nhị phân thời khóa biểu** | `scheduleBitmask`<br>(`uint64_t`) | Biểu diễn toàn bộ 56 ca học của một tuần (7 ngày x 8 ca/ngày) trong một số nguyên 64-bit không dấu duy nhất. Mỗi bit tương ứng với một ca học cụ thể: bit 1 là rảnh/bận, bit 0 là ngược lại. |
| **Mặt nạ hợp lệ 56 ca** | `VALID_SCHEDULE_MASK`<br>(`0x00FFFFFFFFFFFFFFULL`) | Hằng số nhị phân dùng để khử các bit thừa ngoài 56 ca hợp lệ (từ bit 56 đến bit 63), ngăn ngừa hiện tượng bit ma (phantom bits). |
| **Độ dời bit ca học** | `bitOffset` | Công thức xác định vị trí bit: `bitOffset = (dayIndex * 8) + slotIndex`, trong đó `dayIndex` thuộc `[0..6]` (Thứ 2 đến Chủ nhật) và `slotIndex` thuộc `[0..7]`. |
| **Kiểm tra va chạm lịch** | `hasConflict(m1, m2)` | Phép toán logic kiểm tra trùng ca: `(m1 & m2) != 0`. Nếu khác 0 nghĩa là tồn tại ít nhất một ca học bị trùng lặp. |
| **Gợi ý thông minh SmartMatch** | SmartMatch | Thuật toán xếp hạng gia sư theo thang điểm 100 minh bạch: Lịch rảnh (35đ), Giá cả (25đ), Đánh giá sao (15đ), Thâm niên giảng dạy (15đ), Độ tin cậy (10đ). Sử dụng hàng đợi ưu tiên Max-Heap để lấy Top-K. |
| **Lọc môn học bắt buộc** | Mandatory Subject Filter | Quy tắc bất biến trong SmartMatch: Nếu gia sư không giảng dạy môn học mà học viên yêu cầu, gia sư sẽ bị loại bỏ ngay từ vòng lọc ứng viên hợp lệ (`isEligible`), không được đưa vào tính điểm. |
| **Bồi hoàn giao dịch ADR-007** | Compensating Rollback | Cơ chế bảo toàn tính toàn vẹn dữ liệu đa tệp: Khi lưu một bản ghi Review mới, nếu thao tác cập nhật điểm rating cho User vào file `users.dat` thất bại, hệ thống tự động xóa bản ghi Review vừa tạo khỏi file `reviews.dat`. |
| **Tự phục hồi khi khởi động** | Startup Self-Healing Reconciliation | Quy trình chạy tại hàm `main()` khi ứng dụng khởi động: Quét lại toàn bộ các bản ghi trong `reviews.dat`, tính lại chính xác điểm rating trung bình cho từng gia sư và đồng bộ lại vào `users.dat`. |

---

## 3. CÁC TRẠNG THÁI KHÓA HỌC (COURSEBOOKING STATUSES)

| Trạng thái | Mã định danh | Ý nghĩa nghiệp vụ |
| :--- | :---: | :--- |
| **Chờ duyệt** | `PENDING` | Học viên đã gửi yêu cầu và đồng ý cam kết; đang chờ gia sư xem xét. |
| **Đã chấp nhận** | `ACCEPTED` | Gia sư đã đồng ý cam kết và chấp nhận yêu cầu; khóa học chính thức diễn ra và các ca học được khóa trên thời khóa biểu. |
| **Đã từ chối** | `REJECTED` | Gia sư từ chối yêu cầu; không tạo lịch học. |
| **Đã hoàn thành** | `COMPLETED` | Khóa học đã kết thúc đủ số tuần theo thời hạn; học viên được mở quyền đánh giá. |
| **Đã hủy** | `CANCELLED` | Khóa học bị hủy bởi học viên (khi còn PENDING) hoặc bị cưỡng chế hủy bởi Quản trị viên. |
| **Kết thúc sớm** | `EARLY_TERMINATED` | Khóa học được dừng trước thời hạn dựa trên sự đồng thuận của cả hai bên; toàn bộ các ca học tương lai được giải phóng; học viên vẫn có quyền gửi đánh giá. |
