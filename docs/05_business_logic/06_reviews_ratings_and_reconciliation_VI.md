# ĐÁNH GIÁ, TÍNH TOÁN XẾP HẠNG VÀ TỰ PHỤC HỒI DỮ LIỆU (REVIEWS & RATINGS RECONCILIATION)
## Phân Tích Điều Kiện Đánh Giá, Giao Dịch Bù ADR-007 và Cơ Chế Tự Phục Hồi Khi Khởi Động

---

## 1. TỔNG QUAN VÀ BỐI CẢNH THIẾT KẾ `[VERIFIED]`

Hệ thống đánh giá và xếp hạng sao (1 đến 5 sao) phản ánh trực tiếp chất lượng giảng dạy và uy tín của gia sư trên TutorMatch. Nghiệp vụ đánh giá được tách biệt khỏi `BookingService` và đóng gói độc lập trong [`ReviewService`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/ReviewService.h) theo quyết định kiến trúc **ADR-003**.

Do ứng dụng sử dụng cơ chế lưu trữ tệp tin phân tán (`users.dat`, `course_bookings.dat`, `reviews.dat`) không có bộ quản lý giao dịch cơ sở dữ liệu quan hệ (RDBMS Two-Phase Commit), việc ghi đồng thời dữ liệu đánh giá mới vào `reviews.dat` và cập nhật điểm trung bình của gia sư trong `users.dat` tiềm ẩn nguy cơ **bất nhất dữ liệu liên tệp (Cross-File Inconsistency)**. Để giải quyết triệt để vấn đề này, hệ thống áp dụng hai cơ chế bảo vệ cốt lõi:
1. **Giao dịch bù (Compensating Rollback - ADR-007):** Tự động hoàn tác xóa đánh giá nếu thao tác cập nhật gia sư thất bại.
2. **Tự phục hồi khi khởi động (Startup Self-Healing Reconciliation - ADR-007):** Đối soát và tự động cân chỉnh lại điểm trung bình của toàn bộ gia sư khi ứng dụng khởi chạy.

---

## 2. QUY TẮC ĐIỀU KIỆN ĐÁNH GIÁ (REVIEW ELIGIBILITY) `[VERIFIED]`

- **Vị trí mã nguồn:** [`src/business/services/ReviewService.cpp :: ReviewService::isEligibleForReview`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/ReviewService.cpp#L30-L76)
- **Chữ ký phương thức:**
  ```cpp
  bool ReviewService::isEligibleForReview(int studentId, int bookingId, std::string& outReason) const;
  ```

Một học viên chỉ có thể gửi đánh giá cho một khóa học khi thỏa mãn **toàn bộ 4 điều kiện**:
1. **Khóa học tồn tại:** Tìm thấy bản ghi khóa học trong `ICourseBookingRepository`.
2. **Quyền sở hữu chính chủ:** Định danh `studentId` của người gọi phải trùng với `booking->studentId`.
3. **Trạng thái kết thúc hợp lệ:** Khóa học phải ở một trong hai trạng thái:
   - `COMPLETED` (Khóa học đã hoàn thành trọn vẹn thời lượng).
   - `EARLY_TERMINATED` (Khóa học đã kết thúc sớm theo đồng thuận hai bên).
   - Các trạng thái `PENDING`, `ACCEPTED`, `REJECTED`, `CANCELLED` đều bị từ chối với thông báo lỗi cụ thể tương ứng.
4. **Quy tắc Duy nhất (One Review Per Booking):** Học viên chỉ được đánh giá tối đa **1 lần** cho mỗi khóa học (`r.bookingId == bookingId && r.studentId == studentId`). Không cho phép gửi nhiều đánh giá trùng lặp.

---

## 3. QUY TRÌNH GỬI ĐÁNH GIÁ VÀ GIAO DỊCH BÙ (ADR-007) `[VERIFIED]`

- **Vị trí mã nguồn:** [`src/business/services/ReviewService.cpp :: ReviewService::submitReview`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/ReviewService.cpp#L78-L148)

### 3.1. Sơ đồ Tuần tự Gửi Đánh giá và Cơ chế Bồi hoàn Giao dịch ADR-007 (Review Submission with Compensating Rollback Diagram)

Sơ đồ tuần tự dưới đây thể hiện chi tiết chu trình gửi đánh giá từ `ReviewModal.qml`, cơ chế kiểm tra điều kiện hợp lệ (`isEligibleForReview`), quá trình lưu trữ vào `reviews.dat`, tính toán lại điểm rating trung bình của gia sư, và đặc biệt là nhánh kích hoạt **Bồi hoàn giao dịch ADR-007 (Compensating Rollback)** để xóa bản ghi review nếu việc ghi vào `users.dat` gặp sự cố:

```mermaid
sequenceDiagram
    %% =========================================================================
    %% DIAGRAM 15: REVIEW SUBMISSION WITH ADR-007 COMPENSATING ROLLBACK
    %% =========================================================================
    autonumber
    actor Student as Học viên (QML UI)
    participant Modal as ReviewModal.qml
    participant Bridge as GUIBridge (Presentation)
    participant RevSvc as ReviewService (Business)
    participant RevRepo as IReviewRepository (DAL)
    participant UserRepo as IUserRepository (DAL)
    participant DiskReviews as reviews.dat
    participant DiskUsers as users.dat

    Student->>Modal: Chọn điểm đánh giá (1-5 sao) & nhập nhận xét (Không để trống)
    Student->>Modal: Nhấn "Gửi Đánh Giá"
    Modal->>Bridge: qmlSubmitReview(bookingId, ratingScore, comment)

    Bridge->>RevSvc: submitReview(studentId, bookingId, ratingScore, comment, outMsg)
    RevSvc->>RevSvc: lock_guard<recursive_mutex> lock(reviewMutex)

    Note over RevSvc: 1. Kiểm tra tính hợp lệ & Điều kiện (Eligibility)
    RevSvc->>RevSvc: Validate: ratingScore in [1..5] & !comment.empty()
    RevSvc->>RevSvc: isEligibleForReview(studentId, bookingId, reason)
    Note over RevSvc: Khóa học phải là COMPLETED hoặc EARLY_TERMINATED<br/>và chưa từng có review nào cho bookingId này

    Note over RevSvc,RevRepo: 2. Lưu bản ghi Review mới
    RevSvc->>RevRepo: getNextReviewId()
    RevRepo-->>RevSvc: nextRId (ví dụ: 1016)
    RevSvc->>RevSvc: Khởi tạo Review(nextRId, bookingId, studentId, tutorId, name, score, comment, date)
    RevSvc->>RevRepo: saveReview(rev)
    RevRepo->>DiskReviews: Ghi bản ghi vào reviews.dat
    DiskReviews-->>RevRepo: Ghi thành công

    Note over RevSvc,UserRepo: 3. Tính toán lại điểm Rating trung bình của Gia sư
    RevSvc->>RevRepo: getReviewsByTutorId(tutorId)
    RevRepo-->>RevSvc: Danh sách tất cả review của gia sư (bao gồm review vừa tạo)
    RevSvc->>RevSvc: double sum = sum(ratings); newAvg = sum / count;
    RevSvc->>UserRepo: findUserById(tutorId)
    UserRepo-->>RevSvc: tutorUser
    RevSvc->>RevSvc: tutor->setRating(newAvg); tutor->setTotalReviews(count);

    Note over RevSvc,UserRepo: 4. Cập nhật hồ sơ Gia sư & Cơ chế Bồi hoàn ADR-007
    RevSvc->>UserRepo: updateUser(*tutor)

    alt Trường hợp Ghi User thất bại (Lỗi đĩa / Xung đột)
        UserRepo-->>RevSvc: return false
        Note over RevSvc,RevRepo: ADR-007 COMPENSATING ROLLBACK KÍCH HOẠT!
        RevSvc->>RevRepo: deleteReview(nextRId)
        RevRepo->>DiskReviews: Xóa bản ghi review vừa lưu để tránh rác lệch pha dữ liệu
        RevSvc-->>Bridge: return false ("Lỗi khi cập nhật điểm! Thao tác đã được hoàn tác an toàn.")
        Bridge-->>Modal: Báo lỗi & dữ liệu không bị sai lệch
    else Trường hợp Ghi User thành công
        UserRepo->>DiskUsers: Cập nhật rating và totalReviews vào users.dat
        DiskUsers-->>UserRepo: Ghi thành công
        UserRepo-->>RevSvc: return true
        RevSvc-->>Bridge: return true ("Gửi đánh giá và nhận xét thành công!")
        Bridge->>Modal: Q_EMIT dataChanged()
        Bridge-->>Modal: return true
        Modal-->>Student: Đóng modal & hiển thị thông báo thành công
    end
```

#### Diễn giải Quy trình Giao dịch Bù ADR-007:

1. **Khóa Độc quyền & Thẩm định Nghiệp vụ (Bước 1–6):** `ReviewService` sử dụng khóa đệ quy `reviewMutex` để ngăn chặn các yêu cầu gửi đánh giá đồng thời cho cùng một gia sư. Hàm xác nhận điểm số trong khoảng $[1..5]$, nhận xét không rỗng và khóa học phải ở trạng thái `COMPLETED` hoặc `EARLY_TERMINATED`.
2. **Lưu Bản ghi Đánh giá Mới (Bước 7–12):** Bản ghi `Review` được cấp phát mã ID tự tăng và ghi vào `reviews.dat`.
3. **Tính toán Lại Điểm Trung bình Tuyệt đối (Bước 13–18):** Thay vì chỉ cộng dồn số học một cách tương đối, hệ thống đọc lại toàn bộ các bài đánh giá của gia sư từ `IReviewRepository` để tính trung bình cộng chính xác tuyệt đối: $\text{newAvg} = \sum \text{score} / N$.
4. **Cơ chế Bồi hoàn Bắt buộc khi Gặp Sự Cố (Compensating Rollback, Bước 19–27):** Nếu bước ghi `users.dat` thất bại (do lỗi I/O hoặc đĩa đầy), hệ thống ngay lập tức gọi `reviewRepo->deleteReview(nextRId)` để xóa bỏ bản ghi vừa lưu trong `reviews.dat`. Cơ chế này loại bỏ hoàn toàn khả năng phát sinh "đánh giá mồ côi" làm lệch pha điểm số với hồ sơ gia sư.
5. **Cập nhật Thành công (Bước 28–36):** Khi `users.dat` lưu thành công, tín hiệu `dataChanged()` được kích hoạt, modal đóng lại và giao diện phản ánh điểm số uy tín mới của gia sư.

### Công Thức Tính Điểm Đánh Giá Trung Bình:
Khi một đánh giá mới được lưu hoặc bị xóa, điểm trung bình được tính toán lại tuyệt đối trên tập hợp toàn bộ đánh giá thực tế của gia sư đó trong `reviews.dat`:
$$\text{newRating} = \begin{cases} \dfrac{\sum_{i=1}^{N} \text{ratingScore}_i}{N} & \text{nếu } N > 0 \\ 0.0 & \text{nếu } N = 0 \end{cases}$$
$$\text{totalReviews} = N$$

---

## 4. CƠ CHẾ TỰ PHỤC HỒI KHI KHỞI ĐỘNG (STARTUP SELF-HEALING RECONCILIATION) `[VERIFIED]`

Hệ thống TutorMatch triển khai hai tiến trình tự phục hồi và đối soát dữ liệu hoàn toàn tự động, độc lập và có tính bất biến (idempotent) khi khởi động ứng dụng nhằm đảm bảo tính toàn vẹn dữ liệu đa tệp:

---

### 4.1. Tự phục hồi Điểm Đánh giá & Xếp hạng (Rating Reconciliation)
- **Vị trí mã nguồn:** [`src/business/services/ReviewService.cpp :: ReviewService::reconcileRatingsOnStartup`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/ReviewService.cpp#L248-L290)
- **Điểm triệu gọi:** Hàm `main()` trong [`src/presentation/gui_main.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/gui_main.cpp#L85-L87) khi khởi chạy ứng dụng.

#### Nguyên Lý Hoạt Động:
Dữ liệu trong `reviews.dat` được xác định là **Nguồn Chân Lý Duy Nhất (Single Source of Truth - SSOT)** về lịch sử đánh giá. Khi khởi động ứng dụng:
1. `ReviewService` quét toàn bộ danh sách bản ghi trong `reviews.dat` và tích lũy thống kê theo từng `tutorId`:
   - Tổng điểm tích lũy: $\sum \text{ratingScore}$.
   - Tổng số lượt đánh giá thực tế: $\text{count}$.
2. Duyệt qua toàn bộ danh sách gia sư trong `users.dat`.
3. Đối soát điểm số lưu trữ hiện tại của gia sư (`t->getRating()`, `t->getTotalReviews()`) với số liệu kỳ vọng tính từ `reviews.dat`:
   - Điều kiện phát hiện sai lệch:
     $$\Delta_{\text{count}} = (t.\text{totalReviews} \neq \text{expectedCount})$$
     $$\Delta_{\text{rating}} = (|t.\text{rating} - \text{expectedRating}| > 0.01)$$
4. Nếu phát hiện sai lệch ($\Delta_{\text{count}}$ hoặc $\Delta_{\text{rating}}$):
   - Ghi log tự phục hồi ra console: `[ReviewService Self-Healing] Reconciling rating for Tutor ID ...`.
   - Cập nhật lại giá trị chuẩn vào thực thể `Tutor` và ghi lại vào `users.dat` qua `userRepo->updateUser(*t)`.

Cơ chế này đảm bảo dữ liệu hiển thị trên giao diện của toàn bộ gia sư luôn khớp $100\%$ với lịch sử đánh giá, ngay cả khi phiên làm việc trước bị tắt đột ngột (crash/kill process).

---

### 4.2. Tự phục hồi Tổng Học phí Tích lũy của Gia sư (Accumulated Income Reconciliation)
- **Vị trí mã nguồn:**
  - Tính thu nhập: [`BookingService::calculateTutorAccumulatedIncome(int tutorId)`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/BookingService.cpp#L646)
  - Đối soát đơn lẻ: [`BookingService::reconcileTutorEarnings(int tutorId)`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/BookingService.cpp#L662)
  - Đối soát khởi động: [`BookingService::reconcileAllTutorEarningsOnStartup()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/BookingService.cpp#L679)
- **Điểm triệu gọi:**
  - `src/presentation/gui_main.cpp`: Khởi động ứng dụng cùng với `autoCompleteExpiredCourses()`.
  - `src/presentation/bridges/GUIBridge.cpp :: initialize()`: Khi GUI Bridge khởi tạo services.
  - `BookingService::completeCourseBooking` và `BookingService::autoCompleteExpiredCourses`: Bất cứ khi nào một khóa học chuyển sang trạng thái `COMPLETED`.

#### Nguyên Lý Hoạt Động & Ranh Giới Nghiệp Vụ:
1. **Nguồn chân lý duy nhất (SSOT):** Danh sách toàn bộ các khóa học trong `course_bookings.dat`.
2. **Quy tắc công nhận thu nhập:**
   - **TutorMatch không xử lý thanh toán tài chính (Không cổng thanh toán, không ví điện tử).** Thu nhập hiển thị là **Tổng giá trị học phí tích lũy theo thỏa thuận đào tạo (Accumulated Income)**.
   - Chỉ tính các khóa học đã kết thúc trọn vẹn và đạt trạng thái `COMPLETED`:
     $$\text{cb.tutorId} == \text{targetTutorId} \quad \text{và} \quad \text{cb.status} == \text{"COMPLETED"}$$
   - Không tính các khóa `PENDING`, `ACCEPTED`, `REJECTED`, `CANCELLED` và `EARLY_TERMINATED`.
   - Công thức tính cho mỗi khóa học hợp lệ:
     $$\text{CourseCost} = \text{hourlyRate} \times \text{getTotalSessions}() = \text{hourlyRate} \times \text{weeksCount} \times \text{slotsCountPerWeek}$$
3. **Quy trình Đối soát & Tự phục hồi:**
   - Tính tổng thu nhập tích lũy kỳ vọng: $\text{expected} = \sum \text{CourseCost}$.
   - So sánh với giá trị hiện tại trong đối tượng `Tutor`: $|t.\text{getTotalEarnings}() - \text{expected}| > 0.01$.
   - Nếu có sai lệch, ghi log tự phục hồi:
     `[BookingService Self-Healing] Reconciling income for Tutor ID ... (Earnings: old -> expected VNĐ)`
   - Cập nhật `tutor->setTotalEarnings(expected)` và lưu xuống `users.dat` qua `userRepo->updateUser(*tutor)`.
4. **Tính Bất Biến (Idempotence):** Gọi hàm nhiều lần với cùng tập dữ liệu `course_bookings.dat` không gây biến thiên số liệu; giá trị luôn hội tụ chính xác tuyệt đối.
