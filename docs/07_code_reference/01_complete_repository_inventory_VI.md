# DANH MỤC TÀI SẢN MÃ NGUỒN TOÀN BỘ REPOSITORY (COMPLETE REPOSITORY INVENTORY)
## Khảo Sát Chi Tiết Toàn Bộ Tệp Tin Header, Source C++, Giao Diện QML, Tệp Dữ Liệu và Cấu Hình Build

---

## 1. TỔNG QUAN TÀI NGUYÊN TOÀN DỰ ÁN `[VERIFIED]`

Sau đợt chuyển đổi sang kiến trúc GUI-only và tối ưu hóa phân tầng, kho mã nguồn TutorMatch bao gồm chính xác:
- **33 Tệp tiêu đề C++ (`include/`)**: Định nghĩa cấu trúc lớp, giao diện trừu tượng, DTO và giải thuật.
- **25 Tệp triển khai C++ (`src/`)**: Hiện thực hóa logic nghiệp vụ, kho lưu trữ, tuần tự hóa và giao diện đồ họa.
- **27 Tệp giao diện QML (`ui/qml/`)**: 15 Khung nhìn (Views), 5 Hộp thoại (Modals), 6 Thành phần dùng chung (Components), và 1 bộ chỉ dẫn module (`qmldir`).
- **1 Tệp kiểm thử tự động toàn diện (`tests/TestRunner.cpp`)**: 4.370 dòng mã, 30 Test Suites độc lập.
- **3 Tệp dữ liệu hoạt động (`data/*.dat`)** và **3 tệp hạt giống nguyên bản (`data/seed/*.dat`)**.
- **1 Tệp cấu hình CMake (`CMakeLists.txt`)** định nghĩa 3 targets thực thi: `TutorMatch_GUI`, `TutorMatch_Tests`, `TutorMatch_Inspector`.

---

## 2. DANH MỤC TỆP TIÊU ĐỀ C++ (33 HEADERS) `[VERIFIED]`

### 2.1. Phân Hệ Trình Diễn (Presentation Layer — 1 Header)
| Đường dẫn tương đối | Lớp / Khối khai báo | Trách nhiệm chính | Phụ thuộc chính | Người tiêu thụ chính |
| :--- | :--- | :--- | :--- | :--- |
| [`include/presentation/bridges/GUIBridge.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/presentation/bridges/GUIBridge.h) | `class GUIBridge : public QObject` | Bộ điều hợp Presentation Adapter, phơi bày hơn 60 phương thức `Q_INVOKABLE`, signals và properties cho QML. | `QObject`, 6 Business Services, Domain DTOs | `gui_main.cpp`, toàn bộ mã nguồn QML |

---

### 2.2. Phân Hệ Dịch Vụ Nghiệp Vụ (Business Services — 6 Headers)
| Đường dẫn tương đối | Lớp / Khối khai báo | Trách nhiệm chính | Phụ thuộc chính | Người tiêu thụ chính |
| :--- | :--- | :--- | :--- | :--- |
| [`include/business/services/AuthService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/AuthService.h) | `class AuthService` | Xác thực đăng nhập, đăng ký tài khoản Học viên/Gia sư, quản lý phiên làm việc. | `IUserRepository`, `User` | `GUIBridge`, `gui_main.cpp`, `TestRunner` |
| [`include/business/services/UserService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/UserService.h) | `class UserService` | Cập nhật hồ sơ, quản lý thời khóa biểu rảnh/bận, tra cứu người dùng theo ID/vai trò. | `IUserRepository`, `User` | `GUIBridge`, `gui_main.cpp`, `TestRunner` |
| [`include/business/services/BookingService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/BookingService.h) | `class BookingService` | Quản lý vòng đời khóa học nhiều tuần, cam kết hai bên, đồng thuận kết thúc sớm. | `ICourseBookingRepo`, `IUserRepo`, `IReviewRepo` | `GUIBridge`, `AdminService`, `TestRunner` |
| [`include/business/services/ReviewService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/ReviewService.h) | `class ReviewService` | Quản lý vòng đời đánh giá, kiểm tra điều kiện, giao dịch bù ADR-007, tự phục hồi khi khởi động. | `IReviewRepo`, `IUserRepo`, `ICourseBookingRepo` | `GUIBridge`, `gui_main.cpp`, `TestRunner` |
| [`include/business/services/MatchingService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/MatchingService.h) | `class MatchingService` | Điều phối tìm kiếm SmartMatch, phối hợp dữ liệu gia sư và các khóa học đang hoạt động. | `IUserRepo`, `ICourseBookingRepo`, `MatchingEngine` | `GUIBridge`, `gui_main.cpp`, `TestRunner` |
| [`include/business/services/AdminService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/services/AdminService.h) | `class AdminService` | Thống kê số liệu toàn sàn, khóa/mở khóa người dùng, kiểm duyệt đánh giá, chấm dứt kết nối. | `IUserRepo`, `IReviewRepo`, `BookingService` | `GUIBridge`, `gui_main.cpp`, `TestRunner` |

---

### 2.3. Phân Hệ Thực Thể Nghiệp Vụ (Domain Entities — 5 Headers)
| Đường dẫn tương đối | Lớp / Khối khai báo | Trách nhiệm chính | Phụ thuộc chính | Người tiêu thụ chính |
| :--- | :--- | :--- | :--- | :--- |
| [`include/business/domain/User.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/User.h) | `User`, `Student`, `Tutor`, `Admin`, `enum class UserRole` | Cây phân cấp thực thể người dùng, đóng gói danh tính, phân quyền, khối lớp, lịch rảnh, học phí. | Không phụ thuộc thư viện ngoài | Toàn bộ hệ thống |
| [`include/business/domain/CourseBooking.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/CourseBooking.h) | `struct CourseBooking` | Struct thực thể khóa học định kỳ 23 trường, cam kết `COMMITMENT_V1`, đồng thuận kết thúc sớm. | `<string>`, `<vector>`, `<cstdint>` | Services, Repositories, Serializers |
| [`include/business/domain/Review.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/Review.h) | `struct Review` | Struct thực thể phản hồi đánh giá 8 trường (rating 1-5 sao, bình luận, ngày tạo). | `<string>` | Services, Repositories, Serializers |
| [`include/business/domain/BookingDTOs.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/BookingDTOs.h) | `TimetableSessionDTO`, `StudentBookingMetrics`, `TutorTeachingMetrics` | Các đối tượng truyền dữ liệu (DTO) phục vụ hiển thị ma trận lịch tuần và thẻ thống kê số liệu. | `<string>` | `BookingService`, `GUIBridge` |
| [`include/business/domain/DateUtils.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/domain/DateUtils.h) | `class DateUtils` | Bộ tiện ích tĩnh xử lý ngày tháng ISO-8601, so sánh khoảng thời gian, tính thứ trong tuần, băm SHA-256. | `<string>`, `<vector>` | Toàn bộ hệ thống |

---

### 2.4. Phân Hệ Động Cơ Ghép Đôi & Thuật Toán (Matching & Algorithms — 10 Headers)
| Đường dẫn tương đối | Lớp / Khối khai báo | Trách nhiệm chính | Phụ thuộc chính | Người tiêu thụ chính |
| :--- | :--- | :--- | :--- | :--- |
| [`include/business/matching/IMatchingStrategy.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/matching/IMatchingStrategy.h) | `IMatchingStrategy`, `SmartMatchCriteria`, `TutorMatchResult` | Giao diện chiến lược tính điểm đa hình và các cấu trúc tiêu chí/kết quả ghép đôi. | `User`, `CourseBooking` | `MatchingEngine`, `MatchingService` |
| [`include/business/matching/MatchingEngine.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/matching/MatchingEngine.h) | `MatchingEngine`, `WeightedScoreMatchingStrategy` | Chiến lược 100 điểm giải trình (35/25/15/15/10), lọc hợp lệ `isEligible`, trích xuất Top-K. | `IMatchingStrategy`, `MaxHeap` | `MatchingService`, `TestRunner` |
| [`include/business/matching/SubjectTaxonomy.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/matching/SubjectTaxonomy.h) | `SubjectTaxonomy`, `enum class CanonicalSubject`, `ParsedSubject` | Từ điển chuẩn hóa môn học tiếng Việt, loại bỏ dấu thanh điệu, phân tích khối lớp mục tiêu. | `<string>`, `<vector>` | `MatchingEngine`, `TutorSearchMatcher` |
| [`include/business/matching/TutorSearchMatcher.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/matching/TutorSearchMatcher.h) | `TutorSearchMatcher`, `SearchRelevanceTier`, `SearchQueryIntent` | Động cơ tìm kiếm gia sư từ khóa tự do, phân tầng độ liên quan 4 cấp độ, lọc khoảng giá/sao. | `User`, `SubjectTaxonomy` | `UserService`, `GUIBridge` |
| [`include/business/algorithms/ScheduleGuard.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/ScheduleGuard.h) | `class ScheduleGuard` | Đại số bitmask 56 ca học tuần trên số nguyên 64-bit `uint64_t`, kiểm tra xung đột $O(1)$. | `<cstdint>`, `<string>` | Toàn bộ hệ thống |
| [`include/business/algorithms/MaxHeap.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/MaxHeap.h) | `template <typename T> class MaxHeap` | Cấu trúc dữ liệu hàng đợi ưu tiên đống cực đại, trích xuất Top-K phần tử lớn nhất. | `<vector>` | `MatchingEngine`, `TestRunner` |
| [`include/business/algorithms/AVLTree.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/AVLTree.h) | `template <typename K, typename V> class AVLTree` | Cây tìm kiếm nhị phân tự cân bằng AVL, phục vụ truy vấn khoảng học phí và điểm rating $O(\log N + K)$. | Template C++ thuần túy | `FileUserRepository`, `TestRunner` |
| [`include/business/algorithms/HashTable.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/HashTable.h) | `template <typename K, typename V> class HashTable` | Bảng băm đóng gói chỉ mục RAM tra cứu người dùng theo ID và Username với độ phức tạp $O(1)$. | Template C++ thuần túy | `FileUserRepository`, `TestRunner` |
| [`include/business/algorithms/LRUCache.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/LRUCache.h) | `template <typename K, typename V> class LRUCache` | Bộ đệm phần tử gần nhất (Least-Recently-Used), kết hợp bảng băm và danh sách liên kết kép. | `DoublyLinkedList` | `FileUserRepository`, `TestRunner` |
| [`include/business/algorithms/DoublyLinkedList.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/business/algorithms/DoublyLinkedList.h) | `template <typename T> class DoublyLinkedList` | Cấu trúc danh sách liên kết đôi hỗ trợ thao tác thêm/xóa hai đầu $O(1)$. | Template C++ thuần túy | `LRUCache`, `TestRunner` |

---

### 2.5. Phân Hệ Truy Xuất Dữ Liệu (Data Access Layer — 11 Headers)
| Đường dẫn tương đối | Lớp / Khối khai báo | Trách nhiệm chính | Phụ thuộc chính | Người tiêu thụ chính |
| :--- | :--- | :--- | :--- | :--- |
| [`include/data_access/repositories/IRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/repositories/IRepository.h) | `IUserRepository`, `ICourseBookingRepository`, `IReviewRepository` | 3 Giao diện kho dữ liệu trừu tượng cho 3 thực thể cốt lõi theo nguyên lý ISP và DIP. | `User`, `CourseBooking`, `Review` | Services, Concrete Repositories |
| [`include/data_access/repositories/FileUserRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/repositories/FileUserRepository.h) | `class FileUserRepository : public IUserRepository` | Lưu trữ tệp `data/users.dat`, quản lý chỉ mục RAM HashTable, AVLTree và LRUCache. | `IUserRepository`, Serializers | `gui_main.cpp`, `MockDatabaseEngine` |
| [`include/data_access/repositories/FileCourseBookingRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/repositories/FileCourseBookingRepository.h) | `class FileCourseBookingRepository : public ICourseBookingRepository` | Lưu trữ tệp `data/course_bookings.dat`, bảo vệ ghi đa luồng qua mutex. | `ICourseBookingRepo`, Serializers | `gui_main.cpp`, `MockDatabaseEngine` |
| [`include/data_access/repositories/FileReviewRepository.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/repositories/FileReviewRepository.h) | `class FileReviewRepository : public IReviewRepository` | Lưu trữ tệp `data/reviews.dat`, cung cấp truy vấn đánh giá theo gia sư/học viên. | `IReviewRepository`, Serializers | `gui_main.cpp`, `MockDatabaseEngine` |
| [`include/data_access/serializers/UserSerializer.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/serializers/UserSerializer.h) | `class UserSerializer` | Tuần tự hóa đa hình đối tượng `User` (`Student`, `Tutor`, `Admin`) thành dòng CSV 14 cột. | `User`, `SerializerUtils` | `FileUserRepository`, `TestRunner` |
| [`include/data_access/serializers/CourseBookingSerializer.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/serializers/CourseBookingSerializer.h) | `class CourseBookingSerializer` | Đọc ghi thực thể `CourseBooking` thành chuỗi CSV 23 cột, hỗ trợ tương thích ngược định dạng cũ 13 cột. | `CourseBooking`, `SerializerUtils` | `FileCourseBookingRepo`, `TestRunner` |
| [`include/data_access/serializers/ReviewSerializer.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/serializers/ReviewSerializer.h) | `class ReviewSerializer` | Đọc ghi thực thể `Review` thành chuỗi CSV 8 cột, kiểm định thang điểm số sao [1, 5]. | `Review`, `SerializerUtils` | `FileReviewRepo`, `TestRunner` |
| [`include/data_access/serializers/SerializerUtils.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/serializers/SerializerUtils.h) | `class SerializerUtils` | Xử lý ký tự đặc biệt CSV (`escapeCsv`, `unescapeCsv`), tách dòng an toàn chống lỗi tràn dấu phẩy. | `<string>`, `<vector>` | Toàn bộ Serializers |
| [`include/data_access/storage/MockDatabaseEngine.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/storage/MockDatabaseEngine.h) | `class MockDatabaseEngine` | Facade gom cụm 3 repository, kiêm nhiệm tự động sao chép nạp dữ liệu hạt giống khi khởi động. | 3 Interface Repositories | `gui_main.cpp`, `TestRunner` |
| [`include/data_access/storage/ApplicationPaths.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/storage/ApplicationPaths.h) | `class ApplicationPaths` | Xác định đường dẫn tuyệt đối đến thư mục `data/` và `data/seed/` độc lập với CWD của hệ điều hành. | `<string>` | Toàn bộ Repositories |
| [`include/data_access/storage/DataFileInspector.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/data_access/storage/DataFileInspector.h) | `class DataFileInspector` | Công cụ thanh tra pháp chứng đọc-chỉ cấu trúc 3 tệp `.dat`, phát hiện lỗi định dạng và đếm bản ghi. | `<string>`, `<vector>` | Target `TutorMatch_Inspector` |

---

## 3. DANH MỤC TỆP TRIỂN KHAI C++ (25 SOURCES) `[VERIFIED]`

### 3.1. Presentation Layer (2 Files)
- [`src/presentation/gui_main.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/gui_main.cpp): Điểm khởi nhập (Entry Point) của ứng dụng GUI, Composition Root của hệ thống.
- [`src/presentation/bridges/GUIBridge.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp): Hiện thực hóa toàn bộ các phương thức adapter QML.

### 3.2. Business Layer (12 Files)
- [`src/business/services/AuthService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AuthService.cpp): Triển khai xác thực, đăng nhập và đăng ký.
- [`src/business/services/UserService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/UserService.cpp): Triển khai cập nhật hồ sơ và quản lý lịch rảnh cá nhân.
- [`src/business/services/BookingService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/BookingService.cpp): Triển khai đặt khóa học định kỳ, cam kết V1, hủy sớm.
- [`src/business/services/ReviewService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/ReviewService.cpp): Triển khai gửi đánh giá, giao dịch bù ADR-007, tự phục hồi khi khởi động.
- [`src/business/services/MatchingService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/MatchingService.cpp): Triển khai điều phối gợi ý thông minh SmartMatch.
- [`src/business/services/AdminService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/services/AdminService.cpp): Triển khai thống kê chỉ số, khóa tài khoản, xóa đánh giá vi phạm.
- [`src/business/matching/MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/matching/MatchingEngine.cpp): Triển khai thuật toán 100 điểm đa tiêu chí và Max-Heap.
- [`src/business/matching/SubjectTaxonomy.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/matching/SubjectTaxonomy.cpp): Triển khai chuẩn hóa tiếng Việt và so khớp môn học.
- [`src/business/matching/TutorSearchMatcher.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/matching/TutorSearchMatcher.cpp): Triển khai tìm kiếm gia sư đa từ khóa 4 cấp độ.
- [`src/business/algorithms/ScheduleGuard.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/algorithms/ScheduleGuard.cpp): Triển khai chuyển đổi định dạng chuỗi ca học và thời gian.
- [`src/business/domain/User.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/domain/User.cpp): Triển khai các phương thức hiển thị hồ sơ của `Student`, `Tutor`, `Admin`.
- [`src/business/domain/DateUtils.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/domain/DateUtils.cpp): Triển khai các thuật toán xử lý ngày tháng ISO và mã băm SHA-256.

### 3.3. Data Access Layer (10 Files)
- [`src/data_access/repositories/FileUserRepository.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/repositories/FileUserRepository.cpp): Quản lý tệp `users.dat` và chỉ mục RAM.
- [`src/data_access/repositories/FileCourseBookingRepository.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/repositories/FileCourseBookingRepository.cpp): Quản lý tệp `course_bookings.dat`.
- [`src/data_access/repositories/FileReviewRepository.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/repositories/FileReviewRepository.cpp): Quản lý tệp `reviews.dat`.
- [`src/data_access/serializers/UserSerializer.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/UserSerializer.cpp): Đọc ghi dòng CSV người dùng 14 trường.
- [`src/data_access/serializers/CourseBookingSerializer.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/CourseBookingSerializer.cpp): Đọc ghi dòng CSV khóa học 23 trường.
- [`src/data_access/serializers/ReviewSerializer.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/ReviewSerializer.cpp): Đọc ghi dòng CSV đánh giá 8 trường.
- [`src/data_access/serializers/SerializerUtils.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/SerializerUtils.cpp): Xử lý escape và tokenize chuỗi CSV.
- [`src/data_access/storage/ApplicationPaths.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/storage/ApplicationPaths.cpp): Phân giải đường dẫn tệp dữ liệu an toàn.
- [`src/data_access/storage/MockDatabaseEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/storage/MockDatabaseEngine.cpp): Facade kết nối và tự động gieo hạt giống dữ liệu.
- [`src/data_access/storage/DataFileInspector.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/storage/DataFileInspector.cpp): Hiện thực hóa bộ đọc pháp chứng dữ liệu.

### 3.4. Tools Target (1 File)
- [`src/tools/inspect_main.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/tools/inspect_main.cpp): Điểm khởi nhập của công cụ dòng lệnh `TutorMatch_Inspector`.

---

## 4. DANH MỤC THÀNH PHẦN GIAO DIỆN QML (27 FILES) `[VERIFIED]`

### 4.1. Khung Nhìn Chính & Vai Trò (Views — 15 Files)
1. `ui/qml/views/LoginView.qml`: Màn hình Đăng nhập và Đăng ký người dùng.
2. `ui/qml/views/MainDashboard.qml`: Bộ khung điều hướng chính (Sidebar và Header).
3. `ui/qml/views/StudentOverviewView.qml`: Trang tổng quan của Học viên (thẻ chỉ số, thời khóa biểu tuần).
4. `ui/qml/views/StudentTutorSearchView.qml`: Trang tìm kiếm gia sư tự do kèm bộ lọc giá/sao/môn học.
5. `ui/qml/views/StudentSmartMatchView.qml`: Trang gợi ý gia sư thông minh SmartMatch kèm giải trình.
6. `ui/qml/views/StudentBookingsView.qml`: Trang quản lý khóa học của học viên (hủy đơn, kết thúc sớm, đánh giá).
7. `ui/qml/views/TutorOverviewView.qml`: Trang tổng quan của Gia sư (thống kê thu nhập, số khóa dạy).
8. `ui/qml/views/TutorRequestsView.qml`: Trang quản lý yêu cầu đăng ký dạy (duyệt/từ chối kèm cam kết).
9. `ui/qml/views/TutorScheduleView.qml`: Trang thiết lập lịch rảnh cố định của gia sư.
10. `ui/qml/views/TutorProfileView.qml`: Trang quản lý hồ sơ chuyên môn và học phí của gia sư.
11. `ui/qml/views/AdminOverviewView.qml`: Trang điều hành tổng thể toàn sàn dành cho Quản trị viên.
12. `ui/qml/views/AdminUserManagementView.qml`: Trang quản lý người dùng (tìm kiếm, khóa/mở khóa tài khoản).
13. `ui/qml/views/AdminConnectionManagementView.qml`: Trang quản lý kết nối và can thiệp cưỡng chế hủy khóa học.
14. `ui/qml/views/AdminReviewsView.qml`: Trang kiểm duyệt và xóa bỏ các đánh giá vi phạm tiêu chuẩn.
15. `ui/qml/views/AdminReportsView.qml`: Trang báo cáo chi tiết và thống kê doanh thu toàn sàn.

### 4.2. Hộp Thoại Nổi (Modals — 5 Files)
1. `ui/qml/modals/BookingModal.qml`: Hộp thoại đăng ký khóa học định kỳ nhiều tuần và cam kết `COMMITMENT_V1`.
2. `ui/qml/modals/ReviewModal.qml`: Hộp thoại gửi đánh giá sao (1-5) và nhận xét cho khóa học đã hoàn tất.
3. `ui/qml/modals/TutorDetailModal.qml`: Hộp thoại xem chi tiết hồ sơ gia sư, tiểu sử và ca rảnh.
4. `ui/qml/modals/ProfileEditModal.qml`: Hộp thoại chỉnh sửa thông tin cá nhân và mật khẩu.
5. `ui/qml/modals/EarlyTerminationModal.qml`: Hộp thoại đề xuất hoặc phản hồi đồng thuận kết thúc sớm khóa học.

### 4.3. Thành Phần Tái Sử Dụng (Components — 6 Files + 1 qmldir)
1. `ui/qml/components/TimetableGrid.qml`: Lưới ma trận thời khóa biểu tuần 8 dòng $\times$ 7 cột.
2. `ui/qml/components/TimetableCell.qml`: Ô hiển thị ca học đơn lẻ trong thời khóa biểu (Booked, Free, Busy).
3. `ui/qml/components/TutorCard.qml`: Thẻ hiển thị tóm tắt thông tin gia sư kèm huy hiệu khớp SmartMatch.
4. `ui/qml/components/StudentProfileCard.qml`: Thẻ hiển thị tóm tắt thông tin học viên.
5. `ui/qml/components/CourseBookingCard.qml`: Thẻ hiển thị hợp đồng khóa học kèm nút tác vụ theo ngữ cảnh.
6. `ui/qml/components/ReviewItemCard.qml`: Thẻ hiển thị chi tiết một nhận xét đánh giá.
7. `ui/qml/qmldir`: Tệp cấu hình nạp module nội bộ của Qt Quick QML.
