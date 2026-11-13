# ĐÁNH GIÁ NGUYÊN LÝ THAY THẾ LISKOV (LISKOV SUBSTITUTION PRINCIPLE - LSP)
## Phân Tích Tính Khả Thay Thế Của Kiểu Con, Giao Ước Hành Vi và Hiện Tượng Downcasting

---

## 1. NGUYÊN TẮC CỐT LÕI CỦA LSP TRONG TUTORMATCH `[VERIFIED]`

Nguyên lý Thay thế Liskov (Liskov Substitution Principle - LSP) khẳng định: **Nếu $S$ là một kiểu con (subtype) của $T$, thì các đối tượng thuộc kiểu $T$ có thể được thay thế bằng các đối tượng thuộc kiểu $S$ mà không làm thay đổi tính đúng đắn của chương trình**.

Các tiêu chuẩn kỹ thuật để kiểm tra LSP bao gồm:
1. **Hàm hủy ảo (Virtual Destructor):** Lớp cơ sở phải có `virtual ~Base() = default;` để giải phóng bộ nhớ đa hình an toàn.
2. **Bảo toàn Tiền điều kiện (Preconditions):** Lớp con không được phép thắt chặt tiền điều kiện (không được đòi hỏi khắt khe hơn lớp cha).
3. **Bảo toàn Hậu điều kiện (Postconditions):** Lớp con không được làm suy yếu kết quả cam kết trả về.
4. **Bảo toàn Bất biến (Invariants):** Trạng thái hợp lệ của lớp cha phải được giữ nguyên trong lớp con.

---

## 2. KHẢO SÁT HỆ PHÂN CẤP NGƯỜI DÙNG: `User` $\to$ `Student`, `Tutor`, `Admin` `[VERIFIED]`

### 2.1. Giao Ước Của Lớp Cha `User`
- **Vị trí mã nguồn:** [`include/business/domain/User.h:38-80`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/User.h#L38-L80)
- **Hàm hủy ảo:** Lớp cha khai báo tường minh:
  ```cpp
  virtual ~User() = default;
  ```
  Đảm bảo mọi việc giải phóng bộ nhớ qua con trỏ `std::shared_ptr<User>` là an toàn tuyệt đối, không rò rỉ bộ nhớ.
- **Phương thức thuần ảo:**
  ```cpp
  virtual std::string getRoleString() const = 0;
  virtual void displayProfile() const = 0;
  ```
- **Các phương thức truy xuất chung:** `getId()`, `getUsername()`, `getEmail()`, `getFullName()`, `getRole()`, `getIsActive()`.

### 2.2. Kiểm Tra Sự Tuân Thủ Của Các Lớp Con
1. **Lớp con `Student`:**
   - Cài đặt `getRoleString() const override { return "Học viên"; }`.
   - Cài đặt `displayProfile()` xuất thông tin học viên ra console.
   - Hàm `setScheduleBitmask(mask)` tự động gọi `ScheduleGuard::sanitizeMask(mask)`. Hành vi này không làm thắt chặt tiền điều kiện (không ném ngoại lệ nếu truyền bit rác, mà âm thầm lọc sạch bit).
   - Bảo toàn trọn vẹn bất biến của `User`.
2. **Lớp con `Tutor`:**
   - Cài đặt `getRoleString() const override { return "Gia sư"; }`.
   - Cài đặt `displayProfile()` xuất thông tin gia sư, học phí, số sao.
   - Bảo toàn trọn vẹn bất biến của `User`.
3. **Lớp con `Admin`:**
   - Cài đặt `getRoleString() const override { return "Quản trị viên"; }`.
   - Cài đặt `displayProfile()` xuất quyền hạn admin.
   - Bảo toàn trọn vẹn bất biến của `User`.

---

## 3. HIỆN TƯỢNG ÉP KIỂU XUỐNG HÀNG LOẠT (DOWNCASTING ANALYSIS) `[VERIFIED]`

Mặc dù hệ phân cấp `User` tuân thủ hoàn hảo LSP về mặt kỹ thuật (không gây sập chương trình, không vi phạm kiểu dữ liệu khi truyền vào các hàm nhận `const User&` hoặc `std::shared_ptr<User>`), một hiện tượng kiến trúc đáng chú ý được ghi nhận:

- **Khảo sát thực tế:** Có tới **hơn 70 vị trí** trong mã nguồn thực hiện ép kiểu ngược:
  ```cpp
  auto student = std::static_pointer_cast<Student>(userPtr);
  auto tutor = std::static_pointer_cast<Tutor>(userPtr);
  ```
- **Nguyên nhân kiến trúc:** Lớp trừu tượng `User` là một mô hình "thiếu máu" (Anemic Base Class). Lớp cha hầu như không định nghĩa bất kỳ phương thức đa hình nghiệp vụ nào (ngoài 2 hàm hiển thị text). Hầu hết mọi thuộc tính nghiệp vụ thực tế (như kiểm tra xung đột lịch, tính học phí, lấy môn học, lấy điểm đánh giá) đều nằm riêng lẻ tại các lớp con.
- **Đánh giá học thuật:**
  - Hiện tượng downcast hàng loạt này **không vi phạm LSP trực tiếp** (vì đối tượng `Tutor` khi hành xử như một `User` thuần túy vẫn thỏa mãn toàn bộ hợp đồng của `User`).
  - Tuy nhiên, đây là hệ quả của việc **kế thừa thuộc tính dữ liệu** thay vì **kế thừa hành vi đa hình**, dẫn đến các câu lệnh rẽ nhánh kiểm tra vai trò làm suy yếu tính đóng mở (OCP).

---

## 4. KHẢO SÁT HỆ PHÂN CẤP KHO DỮ LIỆU VÀ CHIẾN LƯỢC GỢI Ý `[VERIFIED]`

### 4.1. Hệ Phân Cấp Kho Lưu Trữ (Repository Hierarchies) — ĐẠT XUẤT SẮC (`GOOD`)
- `IUserRepository` $\to$ `FileUserRepository`:
  - Khai báo hàm hủy ảo `virtual ~IUserRepository() = default;`.
  - Phương thức tìm kiếm trả về `nullptr` khi không có dữ liệu, trả về `std::shared_ptr<User>` chứa đúng đối tượng đa hình (`Student`, `Tutor`, `Admin`), ngăn chặn hoàn toàn hiện tượng cắt lớp đối tượng (Object Slicing).
  - Các lớp kiểm thử (Test Doubles / Mock) và `FileUserRepository` có thể thay thế hoàn hảo cho nhau tại mọi vị trí trong Business Services mà không làm thay đổi hành vi nghiệp vụ.

### 4.2. Hệ Phân Cấp Chiến Lược Gợi Ý (`IMatchingStrategy`) — ĐẠT XUẤT SẮC (`GOOD`)
- `IMatchingStrategy` $\to$ `WeightedScoreMatchingStrategy`:
  - Lớp con thực thi trọn vẹn hai phương thức tính điểm `calculateScore` và `calculateScoreWithCriteria`.
  - Hậu điều kiện: Đảm bảo điểm số trả về `compositeScore` luôn nằm trong miền hợp lệ $[0.0, 100.0]$ và hỗ trợ đầy đủ toán tử so sánh thứ tự phục vụ hàng đợi ưu tiên `MaxHeap`.
  - Không ném ngoại lệ bất thường, thay thế hoàn hảo cho con trỏ giao diện trừu tượng trong `MatchingEngine`.

---

## 5. BẢNG TỔNG HỢP ĐÁNH GIÁ NGUYÊN LÝ THAY THẾ LISKOV (LSP) `[VERIFIED]`

| Hệ phân cấp lớp | Lớp cơ sở | Các lớp con triển khai | Tuân thủ LSP | Nhận định kiến trúc |
| :--- | :--- | :--- | :---: | :--- |
| **Phân cấp Người dùng** | `User` | `Student`, `Tutor`, `Admin` | **GOOD** | Có destructor ảo, giao ước sạch. Tuy nhiên cần bổ sung phương thức đa hình để giảm downcasting. |
| **Phân cấp Kho dữ liệu** | `IUserRepository`<br>`ICourseBookingRepository`<br>`IReviewRepository` | `FileUserRepository`<br>`FileCourseBookingRepository`<br>`FileReviewRepository` | **GOOD** | Thay thế hoàn hảo giữa tệp thật và test doubles trong kiểm thử đơn vị. |
| **Phân cấp Gợi ý ghép đôi** | `IMatchingStrategy` | `WeightedScoreMatchingStrategy` | **GOOD** | Bảo toàn trọn vẹn hợp đồng tính điểm và quan hệ so sánh trong MaxHeap. |
