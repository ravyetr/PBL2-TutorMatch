# ĐÁNH GIÁ NGUYÊN LÝ ĐÓNG/MỞ (OPEN/CLOSED PRINCIPLE - OCP)
## Phân Tích Độ Sẵn Sàng Mở Rộng Hệ Thống, Rào Cản Trừu Tượng Hóa và Các Điểm Sửa Đổi Cốt Lõi

---

## 1. NGUYÊN TẮC CỐT LÕI CỦA OCP TRONG TUTORMATCH `[VERIFIED]`

Nguyên lý Đóng/Mở (Open/Closed Principle - OCP) khẳng định: **Một thực thể phần mềm (lớp, module, hàm) phải mở cho việc mở rộng (Open for Extension) nhưng đóng cho việc sửa đổi (Closed for Modification)**.

Mục tiêu thiết kế hướng tới:
- Cho phép tích hợp giải thuật mới, cơ chế lưu trữ mới, hoặc định dạng dữ liệu mới mà không phải chỉnh sửa mã nguồn cốt lõi đã được kiểm thử ổn định.
- Cơ chế đạt được OCP chủ yếu dựa trên **Tính Đa Hình (Polymorphism)**, **Giao Diện Trừu Tượng (Interfaces)** và **Mô Hình Chiến Lược (Strategy Pattern)**.

---

## 2. PHÂN TÍCH 5 KỊCH BẢN MỞ RỘNG NGHIỆP VỤ THỰC TẾ `[VERIFIED]`

### 2.1. Kịch Bản 1: Bổ Sung Chiến Lược Gợi Ý Mới (Strategy Pattern) — ĐẠT XUẤT SẮC (`GOOD`)
- **Nhu cầu nghiệp vụ:** Bổ sung thuật toán đề xuất dựa trên mô hình học máy (ví dụ `MachineLearningMatchingStrategy`) hoặc lọc cộng tác.
- **Hiện trạng thiết kế:**
  - `MatchingEngine` sở hữu con trỏ trừu tượng [`std::unique_ptr<IMatchingStrategy> activeStrategy`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/matching/MatchingEngine.h#L46).
  - Giao diện `IMatchingStrategy` định nghĩa các phương thức thuần ảo:
    ```cpp
    virtual TutorMatchResult calculateScoreWithCriteria(const SmartMatchCriteria& criteria,
                                                        const Tutor& tutor,
                                                        const std::vector<CourseBooking>& activeBookings = {}) = 0;
    ```
  - Phương thức `MatchingEngine::setStrategy(...)` cho phép hoán đổi chiến lược linh hoạt ngay tại runtime.
- **Tác động mã nguồn:** Lập trình viên chỉ cần tạo lớp mới kế thừa `IMatchingStrategy`. **Hoàn toàn không phải sửa đổi mã nguồn** của `MatchingEngine.h`, `MatchingEngine.cpp` hay các dịch vụ gọi đến nó.

---

### 2.2. Kịch Bản 2: Thay Thế Cơ Chế Lưu Trữ (Persistence Engine Swap) — ĐẠT XUẤT SẮC (`GOOD`)
- **Nhu cầu nghiệp vụ:** Chuyển đổi từ tệp phẳng phân tách dấu phẩy (`.dat`) sang cơ sở dữ liệu quan hệ SQLite (`sqlite3`) hoặc cơ sở dữ liệu đám mây PostgreSQL.
- **Hiện trạng thiết kế:**
  - Tầng Nghiệp vụ hoàn toàn phụ thuộc vào 3 giao diện trừu tượng trong [`include/data_access/repositories/IRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/repositories/IRepository.h):
    `IUserRepository`, `ICourseBookingRepository`, `IReviewRepository`.
  - Các service nghiệp vụ (`AuthService`, `BookingService`, `ReviewService`, `MatchingService`) nhận con trỏ thông qua Dependency Injection.
- **Tác động mã nguồn:** Chỉ cần triển khai 3 lớp mới (`SqliteUserRepository`, `SqliteBookingRepository`, `SqliteReviewRepository`) trong `data_access/` và cắm vào điểm ráp nối `gui_main.cpp`. **Không phải sửa đổi bất kỳ dòng mã nào** trong tầng nghiệp vụ hay giao diện người dùng.

---

### 2.3. Kịch Bản 3: Bổ Sung Tiêu Chí Lọc Cứng Ứng Viên — CẢNH BÁO (`WARNING`)
- **Nhu cầu nghiệp vụ:** Thêm điều kiện cứng mới khi gợi ý gia sư (ví dụ: khoảng cách địa lý tối đa, chứng chỉ sư phạm bắt buộc).
- **Hiện trạng thiết kế:**
  - Logic lọc cứng nằm trong hàm tĩnh [`MatchingEngine::isEligible`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/matching/MatchingEngine.cpp#L250-L320).
  - Hàm này kiểm tra tuần tự và đóng cứng các điều kiện: khớp môn học, tài khoản kích hoạt, khối lớp, ngân sách và lịch rảnh.
- **Tác động mã nguồn:** Bắt buộc phải chỉnh sửa trực tiếp mã nguồn của `MatchingEngine.cpp`. Logic lọc cứng chưa được trừu tượng hóa thành chuỗi các bộ lọc (`Chain of Responsibility` hoặc `Filter Pipeline`).

---

### 2.4. Kịch Bản 4: Bổ Sung Trạng Thái Khóa Học Mới — CẢNH BÁO (`WARNING`)
- **Nhu cầu nghiệp vụ:** Bổ sung trạng thái `PAUSED` (Tạm dừng) hoặc `DISPUTED` (Tranh chấp).
- **Hiện trạng thiết kế:**
  - Trạng thái khóa học sử dụng chuỗi văn bản thuần `std::string status`, được kiểm định trong `CourseBookingSerializer::isValidStatus`.
  - Các câu lệnh kiểm tra trạng thái (`if (cb.status == "ACCEPTED")`) nằm rải rác tại nhiều vị trí trong `BookingService.cpp`, `AdminService.cpp`, `ReviewService.cpp`, và `GUIBridge.cpp`.
- **Tác động mã nguồn:** Cần sửa đổi hàm thẩm định trong serializer và rà soát thủ công hàng chục câu lệnh so sánh chuỗi literal trong các dịch vụ. Hệ thống chưa áp dụng mẫu thiết kế State Pattern để mở rộng trạng thái tự động.

---

### 2.5. Kịch Bản 5: Bổ Sung Vai Trò Người Dùng Mới (New User Role) — VI PHẠM (`VIOLATION`)
- **Nhu cầu nghiệp vụ:** Bổ sung vai trò **Phụ huynh (PARENT)** hoặc **Tổ chức giáo dục (INSTITUTION)**.
- **Hiện trạng thiết kế:**
  - Vai trò được đóng cứng trong `enum class UserRole { STUDENT, TUTOR, ADMIN }`.
  - Mô hình thực thể `User` bị coi là "Anemic Subtype Model": các hành vi đặc thù không được đa hình hóa mà dựa vào việc kiểm tra vai trò bằng `if (user->getRole() == UserRole::...)` kết hợp ép kiểu xuống `std::static_pointer_cast<Student>` hoặc `std::static_pointer_cast<Tutor>`.
- **Tác động mã nguồn:** Khi thêm vai trò `PARENT`, lập trình viên phải sửa:
  1. `enum class UserRole` và các hàm chuyển đổi chuỗi trong `User.h`.
  2. Tạo lớp `Parent : public User`.
  3. Sửa `UserSerializer::toCsv` (thêm nhánh chuyển đổi cho Parent).
  4. Sửa `UserSerializer::fromCsv` (thêm nhánh giải mã cho Parent).
  5. Sửa đổi hơn 60 vị trí kiểm tra `UserRole` và downcast trên toàn bộ các Service và `GUIBridge`.
- **Kết luận:** Cây kế thừa `User` phụ thuộc vào `enum` và ép kiểu downcast vi phạm nghiêm trọng OCP đối với việc mở rộng loại người dùng mới.
