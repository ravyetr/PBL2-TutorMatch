# ARCHITECTURE DECISION RECORD (ADR)
## ADR-007: Tính Nhất Quán Đa Tệp và Cơ Chế Tự Phục Hồi Đối Soát Khởi Động (Startup Self-Healing Reconciliation)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-007 (Supplementary to ADR-002, ADR-003)
- **Tiêu đề:** Tính Nhất Quán Đa Tệp và Cơ Chế Tự Phục Hồi Đối Soát Khởi Động (Startup Self-Healing Reconciliation)
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Data Integrity Specialist
- **Phạm vi tác động:** `FileUserRepository`, `FileReviewRepository`, `ReviewService`, `DataFileInspector`, `users.dat`, `reviews.dat`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong hệ thống TutorMatch, khi học viên gửi một đánh giá nhận xét mới (`submitReview`) hoặc khi Quản trị viên xoá một đánh giá vi phạm (`deleteReview`), thao tác nghiệp vụ tác động đồng thời lên hai tệp lưu trữ vật lý độc lập:
1. `data/reviews.dat`: Thêm hoặc xoá bản ghi đánh giá (`Review`).
2. `data/users.dat`: Cập nhật điểm đánh giá trung bình cộng (`rating`) và tổng số lượt đánh giá (`totalReviews`) của Gia sư tương ứng.

Trong môi trường lưu trữ tệp phẳng (File I/O) không sử dụng DBMS:
- Không tồn tại bộ điều phối giao dịch phân tán (Distributed Transaction Coordinator) hay giao thức 2-Phase Commit (2PC).
- Không thể đạt được tính nguyên tử tuyệt đối (ACID Transaction) đồng thời trên cả hai tệp độc lập.
- Nếu ứng dụng bị sập nguồn (Power Outage), bị buộc dừng tiến trình (Kill Process), hoặc gặp sự cố tràn đĩa giữa thời điểm ghi `reviews.dat` và ghi `users.dat`, hệ thống sẽ rơi vào trạng thái bất đồng bộ: Điểm rating hiển thị trên hồ sơ Gia sư không khớp với giá trị trung bình cộng thực tế của các bản ghi đánh giá.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định thiết lập giải pháp bảo đảm tính toàn vẹn và nhất quán dữ liệu dựa trên mô hình **Nhất Quán Cuối Cùng (Eventual Consistency) kết hợp Tự Phục Hồi Đối Soát Khởi Động (Startup Self-Healing Reconciliation)**:

1. **Xác lập Nguồn Chân Lý Bền Vững (Authoritative Source of Truth):**
   - Tệp `data/reviews.dat` là nguồn chân lý duy nhất và tối cao cho tất cả các dữ liệu đánh giá và nhận xét.
   - Các trường `rating` và `totalReviews` trong `data/users.dat` được định nghĩa rõ ràng là **Dữ liệu Phái sinh / Tối ưu hoá Đọc (Derived / Denormalized Read-Optimized Data)**, nhằm giúp các truy vấn tìm kiếm gia sư không phải quét tệp `reviews.dat` mỗi lần hiển thị.
2. **Quy trình Ghi có Rollback Bù Trừ (Compensatory Rollback Write Flow):**
   Trong phương thức `ReviewService::submitReview()`:
   - *Bước 1:* Ghi bản ghi đánh giá mới vào `FileReviewRepository` (append vào `reviews.dat`).
   - *Bước 2:* Tính toán rating mới và gọi `userRepo->updateUser(*tutor)`.
   - *Bước 3 (Xử lý lỗi):* Nếu Bước 2 thất bại (ví dụ lỗi I/O tệp `users.dat`), lập tức thực hiện hành động bù trừ: gọi `reviewRepo->deleteReview(newReviewId)` để rollback Bước 1, trả về thông báo lỗi cho người dùng và không để lại dữ liệu rác.
3. **Cơ chế Tự Phục Hồi Đối Soát Khởi Động (Startup Self-Healing Reconciliation):**
   Trong giai đoạn khởi động kho lưu trữ (tại `ReviewService::initialize()` hoặc `StorageBootstrap`):
   - Đọc toàn bộ danh sách đánh giá từ `FileReviewRepository::getAllReviews()`.
   - Gom nhóm và tính toán lại chính xác tổng số lượt đánh giá và điểm trung bình cộng số học cho từng `tutorId`:
     $$\text{rating}_{\text{actual}} = \frac{\sum \text{score}}{N}, \quad \text{totalReviews}_{\text{actual}} = N$$
   - Đối chiếu với giá trị hiện tại của từng Gia sư trong `FileUserRepository`.
   - Nếu phát hiện bất kỳ sự sai lệch nào (do crash ở phiên làm việc trước), hệ thống **tự động cập nhật lại bản ghi gia sư trong `FileUserRepository`** và ghi log chẩn đoán: `[Self-Healing] Reconciled rating for Tutor ID {id}: restored to {actual}`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Xây dựng cơ chế Write-Ahead Logging (WAL) tự tạo
- **Mô tả:** Viết một hệ thống ghi nhật ký giao dịch trước (Journal file `tm_journal.log`) với các trạng thái BEGIN, PREPARE, COMMIT để thực hiện 2-Phase Commit trên tệp tin.
- **Lý do loại bỏ:** Độ phức tạp cài đặt quá lớn, dễ sinh lỗi deadlock và hỏng hóc file journal, vượt quá quy mô và mục tiêu đào tạo của dự án học thuật C++ File I/O.

#### Phương án B: Không lưu trường rating trong users.dat (Fully Normalized)
- **Mô tả:** Xoá bỏ hoàn toàn hai trường `rating` và `totalReviews` trong `users.dat`. Mỗi khi cần hiển thị gia sư hoặc chạy thuật toán matching, đọc `reviews.dat` để tính điểm on-the-fly.
- **Lý do loại bỏ:**
  - Làm chậm nghiêm trọng hiệu năng tìm kiếm và thuật toán SmartMatch Top-K (từ $O(1)$ chuyển thành $O(N \times M)$ disk I/O).
  - Phá vỡ hai cây chỉ mục AVL (`ratingIndex`), vì AVL tree cần giá trị rating để lập chỉ mục tìm kiếm theo dải.
  - Làm thay đổi schema vật lý của `users.dat`, vi phạm ràng buộc không thay đổi cấu trúc tệp.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Khả năng tự phục hồi 100% (Crash Resilience):** Ngay cả khi xảy ra sự cố sập nguồn điện đột ngột trong lúc ghi file, hệ thống sẽ tự động lành lặn và đưa dữ liệu về trạng thái hoàn toàn nhất quán ở lần khởi động kế tiếp.
- **Không thay đổi schema tệp `.dat`:** Bảo toàn 100% định dạng và cấu trúc dữ liệu hiện hữu.
- **Hiệu năng cao:** Duy trì tốc độ đọc $O(1)$ và chỉ mục AVL $O(\log N)$ trong thời gian chạy mà không phải tính toán lại rating lặp đi lặp lại.
- **Đơn giản, trong sáng:** Giải pháp dễ hiểu, dễ kiểm thử và hoàn toàn phù hợp với triết lý thiết kế hệ thống phân tán / tệp tin.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Tăng thêm một lượng thời gian kiểm tra đối soát rất nhỏ (vài mili-giây với quy mô hàng trăm bản ghi) khi ứng dụng khởi động.

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Một bản ghi trong `reviews.dat` bị hỏng cấu trúc khiến việc tính toán đối soát khởi động bị ném ngoại lệ.
- **Biện pháp giảm thiểu:** Bọc logic đọc từng dòng trong khối `try-catch`, ghi log cảnh báo và bỏ qua dòng hỏng mà không làm sập tiến trình khởi động.

---

### 6. KẾT LUẬN
Quyết định áp dụng **Startup Self-Healing Reconciliation** là giải pháp tối ưu, giải quyết triệt để vấn đề tính nhất quán đa tệp mà không cần dùng đến các cơ chế giao dịch phức tạp của DBMS.
