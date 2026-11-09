# ARCHITECTURE DECISION RECORD (ADR)
## ADR-004: Tách Biệt Tuần Tự Hoá Khỏi Mô Hình Miền (Domain Model Serialization Separation)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-004
- **Tiêu đề:** Tách Biệt Tuần Tự Hoá Khỏi Mô Hình Miền (Domain Model Serialization Separation)
- **Trạng thái:** **PROPOSED** *(Đề xuất chờ phê duyệt)*
- **Ngày lập:** 2026-11-06
- **Tác giả:** Principal Software Architect & Senior C++ Engineer
- **Phạm vi tác động:** `include/models/User.h`, `include/models/Booking.h`, `include/models/Review.h`, `src/models/User.cpp`, và các Serializers mới trong `include/storage/` & `src/storage/`.

---

### 1. BỐI CẢNH (CONTEXT)
Trong mã nguồn hiện tại, interface `ISerializable` được định nghĩa bên trong `include/models/User.h` (dòng 36-41) với hai phương thức ảo thuần túy:
```cpp
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& csvLine) = 0;
};
```
Toàn bộ các thực thể cốt lõi của hệ thống (`User`, `Student`, `Tutor`, `Admin`, `Booking`, `Review`) đều kế thừa `ISerializable`. Hơn nữa, toàn bộ mã nguồn nối chuỗi CSV và bóc tách token chuỗi `stringstream` của cả 6 thực thể này lại bị gom chung vào một file duy nhất là `src/models/User.cpp` (275 dòng lệnh).

**Hạn chế kiến trúc:**
- Rò rỉ chi tiết tầng lưu trữ (định dạng tệp CSV, dấu phẩy phân cách, xử lý escape ký tự) trực tiếp vào tầng mô hình miền (Domain Model).
- Nếu thay đổi định dạng lưu trữ trong tương lai, toàn bộ các domain class đều bị ảnh hưởng và phải biên dịch lại (vi phạm nguyên lý Open/Closed Principle).
- Domain entities bị mất tính độc lập (POCO - Plain Old C++ Objects).

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Gỡ bỏ hoàn toàn `ISerializable` khỏi các Domain Entities:**
   - Xoá bỏ kế thừa `public ISerializable` khỏi `User`, `Booking`, `Review`.
   - Xoá bỏ các phương thức `serialize()` và `deserialize()` khỏi các domain classes.
   - Các thực thể miền trở thành các POCO Domain Entities thuần túy, chỉ chứa dữ liệu trạng thái, thuộc tính, và các phương thức nghiệp vụ miền (ví dụ: thao tác bitmask lịch học, kiểm tra vai trò).
2. **Xây dựng 3 bộ Serializer chuyên trách thuộc Data Access Layer:**
   - **`UserSerializer` (`include/storage/UserSerializer.h`, `src/storage/UserSerializer.cpp`):**
     - Đảm nhận chuyển đổi hai chiều giữa chuỗi CSV và các thực thể đa hình: `User`, `Student`, `Tutor`, `Admin`.
     - Phân tích cú pháp vai trò dựa trên trường số 6 (`role`) để khởi tạo đúng con trỏ đối tượng con thông qua factory method: `std::shared_ptr<User> fromCsv(const std::string& line)`.
   - **`BookingSerializer` (`include/storage/BookingSerializer.h`, `src/storage/BookingSerializer.cpp`):**
     - Đảm nhận chuyển đổi hai chiều cho thực thể `Booking`.
   - **`ReviewSerializer` (`include/storage/ReviewSerializer.h`, `src/storage/ReviewSerializer.cpp`):**
     - Đảm nhận chuyển đổi hai chiều cho thực thể `Review`.
3. **Bảo toàn 100% định dạng Byte-Level của dữ liệu vật lý:**
   - Bảo toàn tuyệt đối thứ tự các trường trong các file:
     - `users.dat`: 8 trường cơ sở + 3 trường Student / 7 trường Tutor / 0 trường Admin.
     - `bookings.dat`: 10 trường cố định.
     - `reviews.dat`: 8 trường cố định.
   - Bảo toàn xử lý phân tách dấu phẩy, escape dấu ngoặc kép theo chuẩn RFC-4180, chuyển đổi số nguyên không dấu 64-bit (`uint64_t`) cho bitmask lịch học, và bảo toàn bảng mã UTF-8 tiếng Việt có dấu.
   - Tuyệt đối không thay đổi schema của các tệp `.dat`.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Giữ nguyên ISerializable trên Domain Models
- **Mô tả:** Giữ nguyên interface `ISerializable` trong `User.h` và các phương thức `serialize()`/`deserialize()`.
- **Lý do loại bỏ:** Duy trì sự gắn kết lỏng lẻo giả tạo (false loose coupling); Domain Models tiếp tục bị ràng buộc trực tiếp vào định dạng lưu trữ CSV, vi phạm nguyên tắc cơ bản của Kiến trúc Ba Lớp.

#### Phương án B: Chuyển đổi định dạng lưu trữ sang JSON hoặc SQLite
- **Mô tả:** Sử dụng thư viện nlohmann/json hoặc SQLite để đơn giản hoá việc tuần tự hoá.
- **Lý do loại bỏ:** Vi phạm trực tiếp ràng buộc cốt lõi của đồ án: "Không sử dụng DBMS, bắt buộc sử dụng File I/O tự quản lý và cấu trúc dữ liệu tự cài đặt".

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực (Positive):
- **Phân tách rạch ròi giữa Domain Model và Data Storage:** Mô hình miền hoàn toàn độc lập với công nghệ lưu trữ.
- **Tuân thủ nguyên lý Single Responsibility (SRP):** Domain Model chỉ chịu trách nhiệm đại diện dữ liệu nghiệp vụ; Serializer chịu trách nhiệm định dạng tệp.
- **Dễ dàng kiểm thử đơn vị (Testability):** Có thể viết các bài kiểm thử Round-trip độc lập cho từng Serializer với các ca kiểm thử biên (chuỗi rỗng, dấu phẩy trong tên, ký tự đặc biệt) mà không cần khởi tạo hệ thống lưu trữ hoàn chỉnh.
- **Không làm gián đoạn dữ liệu hiện có:** Tệp `.dat` hiện tại được đọc và ghi lại với độ chính xác nhị phân từng byte.

#### Tiêu cực / Chi phí kỹ thuật (Trade-offs):
- Cần tạo thêm 3 cặp tệp header/source trong `storage/`.
- Cần cập nhật các vị trí trong `MockDatabaseEngine` và các Repositories mới để gọi Serializers thay vì gọi `entity.serialize()`.

---

### 5. RỦI RO & BIỆN PHÁP GIẢM THIỂU (RISKS & MITIGATION)
- **Rủi ro:** Một trường dữ liệu bị bỏ sót hoặc sai thứ tự cột trong quá trình tách mã từ `User.cpp` sang Serializers sẽ làm sai lệch dữ liệu toàn bộ tệp `.dat`.
- **Biện pháp giảm thiểu:**
  - Viết bộ kiểm thử `run_test_serializers_roundtrip()` đối chiếu từng dòng trong dữ liệu mẫu thực tế.
  - Kiểm tra diff từng byte giữa dữ liệu trước và sau khi round-trip trước khi thay thế hoàn toàn.

---

### 6. KẾT LUẬN
Tách biệt tuần tự hoá khỏi Domain Model là một bước tiến quan trọng về mặt công nghệ và kiến trúc phần mềm, biến TutorMatch thành một dự án chuẩn mực về thiết kế hướng đối tượng hiện đại.
