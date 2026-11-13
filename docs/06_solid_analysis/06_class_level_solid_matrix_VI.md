# MA TRẬN ĐÁNH GIÁ NGUYÊN LÝ SOLID Ở CẤP ĐỘ LỚP (CLASS-LEVEL SOLID MATRIX)
## Bảng Tổng Hợp Đánh Giá Chi Tiết 25 Lớp Của Hệ Thống TutorMatch Hiện Tại (GUI-Only)

---

## 1. TIÊU CHUẨN VÀ ĐỊNH NGHĨA KÝ HIỆU TRONG MA TRẬN `[VERIFIED]`

Hệ thống TutorMatch hiện tại (sau khi loại bỏ phân hệ CLI để chuyển sang kiến trúc GUI-only chuyên biệt) bao gồm 25 lớp và cấu trúc nghiệp vụ chính trải dài trên 3 tầng kiến trúc.

### Quy Ước Ký Hiệu Đánh Giá:
- **`GOOD`**: Lớp tuân thủ tốt nguyên lý, thiết kế mạch lạc, không có dấu hiệu vi phạm.
- **`WARNING`**: Lớp có sự chồng lấn trách nhiệm hoặc phụ thuộc nhẹ, chưa tối ưu nhưng chưa phá vỡ kiến trúc.
- **`VIOLATION`**: Lớp vi phạm rõ ràng nguyên lý, có mã nguồn chứng minh trực tiếp, tạo ra rủi ro kỹ thuật.
- **`N/A`**: Nguyên lý không áp dụng trực tiếp cho lớp này (ví dụ: các struct DTO thuần túy không áp dụng LSP/ISP, hoặc các hàm tiện ích tĩnh không áp dụng LSP).

---

## 2. MA TRẬN ĐÁNH GIÁ CẤP ĐỘ LỚP TOÀN HỆ THỐNG `[VERIFIED]`

### 2.1. Phân Hệ Trình Diễn (Presentation Layer)
| Tên Lớp / Thành Phần | Module / Đường Dẫn | SRP | OCP | LSP | ISP | DIP | Đánh Giá Chung | Nhận Định Kiến Trúc |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`gui_main.cpp`** | `src/presentation/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **GOOD** | **MẪU MỰC** | Composition Root chuẩn mực: khởi tạo DAL, tiêm vào Services, tiêm vào `GUIBridge` theo trình tự sạch. |
| **`GUIBridge`** | `include/presentation/bridges/` | **WARNING** | **GOOD** | **GOOD** | **WARNING** | **GOOD** | **TỐT (CÓ CẢNH BÁO)** | Adapter nguyên khối cho QML. Rất tốt vì không chạm vào DAL, nhưng API bề mặt quá rộng (60+ methods) cho cả 3 roles. |

---

### 2.2. Các Dịch Vụ Nghiệp Vụ (Business Services)
| Tên Lớp / Thành Phần | Module / Đường Dẫn | SRP | OCP | LSP | ISP | DIP | Đánh Giá Chung | Nhận Định Kiến Trúc |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`AuthService`** | `include/business/services/` | **GOOD** | **GOOD** | **N/A** | **GOOD** | **GOOD** | **XUẤT SẮC** | Đơn trách nhiệm: Quản lý xác thực và phiên người dùng. Phụ thuộc trừu tượng `IUserRepository`. |
| **`UserService`** | `include/business/services/` | **WARNING** | **GOOD** | **N/A** | **GOOD** | **GOOD** | **TỐT (CÓ CẢNH BÁO)** | Quản lý hồ sơ và ca rảnh. Chứa thêm logic tìm kiếm và lọc giá/sao, chồng lấn nhẹ với Search/Matching. |
| **`BookingService`** | `include/business/services/` | **VIOLATION** | **WARNING** | **N/A** | **WARNING** | **GOOD** | **CẢNH BÁO / VI PHẠM (P1)** | Quản lý tốt vòng đời khóa học, nhưng chứa thêm `submitReview` và tính rating gia sư, trùng lặp với `ReviewService`. |
| **`ReviewService`** | `include/business/services/` | **GOOD** | **GOOD** | **N/A** | **GOOD** | **GOOD** | **XUẤT SẮC** | Quản lý độc quyền vòng đời review, tính rating tích lũy, giao dịch bù ADR-007 và tự phục hồi khi khởi động. |
| **`MatchingService`** | `include/business/services/` | **GOOD** | **GOOD** | **N/A** | **GOOD** | **GOOD** | **XUẤT SẮC** | Điều phối gợi ý SmartMatch, ủy thác tính điểm cho `MatchingEngine`, phụ thuộc các repository trừu tượng. |
| **`AdminService`** | `include/business/services/` | **WARNING** | **GOOD** | **N/A** | **GOOD** | **WARNING** | **KHÁ TỐT** | Phụ thuộc concrete `BookingService`. Trong `deleteViolatingReview` tự xóa review và tính rating thay vì qua `ReviewService`. |

---

### 2.3. Thực Thể Và Giá Trị Nghiệp Vụ (Domain Entities & Value Objects)
| Tên Lớp / Thành Phần | Module / Đường Dẫn | SRP | OCP | LSP | ISP | DIP | Đánh Giá Chung | Nhận Định Kiến Trúc |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`User` (Lớp cha)** | `include/business/domain/` | **GOOD** | **WARNING** | **GOOD** | **GOOD** | **N/A** | **TỐT (ANEMIC SUBTYPE)**| Destructor ảo, giao ước rõ ràng. Tuy nhiên thiếu phương thức đa hình dẫn đến downcast ép kiểu lớp con. |
| **`Student`** | `include/business/domain/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **N/A** | **XUẤT SẮC** | Kế thừa chuẩn mực từ `User`, đóng gói `gradeLevel` và `scheduleBitmask`, sanitize bitmask an toàn. |
| **`Tutor`** | `include/business/domain/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **N/A** | **XUẤT SẮC** | Kế thừa chuẩn mực từ `User`, quản lý học phí, chuyên môn, rating tích lũy và doanh thu. |
| **`Admin`** | `include/business/domain/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **N/A** | **XUẤT SẮC** | Kế thừa chuẩn mực từ `User`, sử dụng 7 trường cơ sở, toàn quyền Root Admin thống nhất. |
| **`CourseBooking`** | `include/business/domain/` | **GOOD** | **WARNING** | **N/A** | **N/A** | **N/A** | **TỐT** | Struct nghiệp vụ 23 trường thuần túy, có các helper truy vấn (`isTerminal()`, `hasSlot()`), zero logic lưu trữ/UI. |
| **`Review`** | `include/business/domain/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Struct giá trị 8 trường, đóng gói thông tin nhận xét và số sao đánh giá. |
| **`BookingDTOs`** | `include/business/domain/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Các đối tượng truyền dữ liệu (`TimetableSessionDTO`, `StudentBookingMetrics`), tách biệt dữ liệu truyền nhận. |
| **`DateUtils`** | `include/business/domain/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Bộ hàm tĩnh tính toán thời gian ISO-8601, kiểm tra quá khứ, cộng tuần. Không phụ thuộc thư viện ngoài. |

---

### 2.4. Động Cơ Gợi Ý Và Giải Thuật (Matching & Algorithms)
| Tên Lớp / Thành Phần | Module / Đường Dẫn | SRP | OCP | LSP | ISP | DIP | Đánh Giá Chung | Nhận Định Kiến Trúc |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`MatchingEngine`** | `include/business/matching/` | **WARNING** | **GOOD** | **N/A** | **GOOD** | **GOOD** | **TỐT (CÓ CẢNH BÁO)** | Áp dụng Strategy Pattern tốt qua `IMatchingStrategy`. Tuy nhiên hàm lọc cứng `isEligible` bị đóng cứng dạng static. |
| **`IMatchingStrategy`**| `include/business/matching/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **MẪU MỰC** | Giao diện chiến lược tinh gọn (3 methods), độ gắn kết cao, không có phương thức dư thừa. |
| **`WeightedScoreStrategy`**| `include/business/matching/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **N/A** | **XUẤT SẮC** | Triển khai mô hình 100 điểm có giải trình (Explainable AI), bảo toàn trọn vẹn hợp đồng của `IMatchingStrategy`. |
| **`SubjectTaxonomy`** | `include/business/matching/` | **GOOD** | **WARNING** | **N/A** | **N/A** | **N/A** | **TỐT** | Chuẩn hóa tiếng Việt và so khớp môn học xuất sắc. Tuy nhiên `CanonicalSubject` enum là danh sách đóng. |
| **`TutorSearchMatcher`**| `include/business/matching/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Thuật toán so khớp văn bản đa từ 3 cấp độ liên quan. Thuần túy thuật toán, không phụ thuộc I/O. |
| **`ScheduleGuard`** | `include/business/algorithms/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **MẪU MỰC** | Đại số bitmask 56 ca tuần. Độ phức tạp $O(1)$ tối ưu phần cứng, bất biến `0x00FFFFFFFFFFFFFFULL` chuẩn mực. |
| **`MaxHeap<T>`** | `include/business/algorithms/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Cấu trúc dữ liệu hàng đợi ưu tiên đống cực đại, phục vụ trích xuất Top-K gia sư tối ưu. |
| **`AVLTree / HashTable / LRUCache`** | `include/business/algorithms/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Các cấu trúc dữ liệu tùy biến phục vụ đánh chỉ mục RAM và tra cứu tối ưu trong tầng Data Access. |

---

### 2.5. Tầng Truy Xuất Dữ Liệu Và Tuần Tự Hóa (Data Access Layer)
| Tên Lớp / Thành Phần | Module / Đường Dẫn | SRP | OCP | LSP | ISP | DIP | Đánh Giá Chung | Nhận Định Kiến Trúc |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **`IUserRepository`** | `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **WARNING** | **GOOD** | **TỐT (CÓ CẢNH BÁO)** | Giao diện béo (11 methods), chứa cả CRUD và truy vấn khoảng giá/sao của cây AVL. |
| **`ICourseBookingRepo`**| `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **XUẤT SẮC** | Giao ước 7 phương thức tập trung cho vòng đời hợp đồng khóa học. |
| **`IReviewRepository`** | `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **XUẤT SẮC** | Giao ước 5 phương thức chuyên biệt cho phản hồi chất lượng. |
| **`FileUserRepository`**| `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **XUẤT SẮC** | Quản lý tệp `users.dat`, chỉ mục RAM Hash, AVL Tree và LRU Cache. Ủy thác parsing cho `UserSerializer`. |
| **`FileBookingRepo`** | `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **XUẤT SẮC** | Quản lý lưu trữ tệp `course_bookings.dat`, ủy thác parsing cho `CourseBookingSerializer`. |
| **`FileReviewRepo`** | `include/data_access/repositories/` | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **GOOD** | **XUẤT SẮC** | Quản lý lưu trữ tệp `reviews.dat`, ủy thác parsing cho `ReviewSerializer`. |
| **`UserSerializer`** | `include/data_access/serializers/` | **GOOD** | **WARNING** | **N/A** | **N/A** | **N/A** | **TỐT** | Tuần tự hóa đa hình thuần túy, zero File I/O. Phải sửa khi thêm Role mới (OCP). |
| **`BookingSerializer`** | `include/data_access/serializers/` | **GOOD** | **WARNING** | **N/A** | **N/A** | **N/A** | **TỐT** | Đọc ghi 23 cột chuẩn hóa, tương thích ngược 13 cột. Phải sửa khi thêm status mới. |
| **`ReviewSerializer`** | `include/data_access/serializers/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Đọc ghi 8 cột đánh giá, kiểm tra số sao hợp lệ [1, 5]. |
| **`SerializerUtils`** | `include/data_access/serializers/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Thoát ký tự CSV an toàn (`escapeCsv`), chống tràn dấu phẩy và chuyển đổi an toàn int/double/uint64. |
| **`MockDatabaseEngine`**| `include/data_access/storage/` | **WARNING** | **GOOD** | **GOOD** | **WARNING** | **GOOD** | **TỐT (FACADE)** | Facade gom cụm 3 hợp đồng, kiêm nhiệm tự động nạp dữ liệu hạt giống khi khởi động. |
| **`ApplicationPaths`** | `include/data_access/storage/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Đơn trách nhiệm: Phân giải đường dẫn thư mục `data` và `seed` độc lập với CWD. |
| **`DataFileInspector`** | `include/data_access/storage/` | **GOOD** | **GOOD** | **N/A** | **N/A** | **N/A** | **XUẤT SẮC** | Đơn trách nhiệm: Công cụ thanh tra pháp chứng dữ liệu read-only. |
