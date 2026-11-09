# BÁO CÁO NGHIỆM THU KIẾN TRÚC & TRIỂN KHAI TÍNH NĂNG ĐẶT KHÓA HỌC ĐỊNH KỲ (RECURRING COURSE BOOKING)

**Dự án:** Nền tảng kết nối gia sư TutorMatch  
**Công nghệ:** C++17, Qt 6.11 (QML / Qt Quick), Flat-File Persistence, SOLID & 3-Layer Architecture  
**Trạng thái triển khai:** Hoàn thành toàn diện 100% (Fully Completed)  
**Tập kiểm thử:** 20/20 Test Suites PASS (100%)  
**Tính toàn vẹn dữ liệu gốc:** Bảo toàn 100% SHA-256 đối với `users.dat`, `bookings.dat`, `reviews.dat`  
**Ngày lập báo cáo:** 06/11/2026  

---

## 1. TÓM TẮT KẾT QUẢ TRIỂN KHAI (EXECUTIVE SUMMARY)

Trước đây, hệ thống TutorMatch lưu trữ lịch học dựa trên cấu trúc lịch tuần trừu tượng 56 ca học (`dayOfWeek`: 0–6, `slotIndex`: 0–7) mà không gắn liền với ngày tháng lịch thực tế (Gregorian calendar date). Khi học viên và gia sư xác nhận một yêu cầu học, hệ thống triệt tiêu bit tương ứng trên `User.scheduleBitmask`. Cách tiếp cận này bộc lộ những hạn chế lớn về mặt nghiệp vụ:
1. Không thể đặt các khóa học dài hạn theo kỳ hạn (4 tuần, 8 tuần, 12 tuần) với lịch học cụ thể.
2. Không phân biệt được "Thứ Hai tuần này" với "Thứ Hai tuần sau", khiến hai khóa học lệch thời gian nhưng cùng thứ trong tuần không thể cùng tồn tại.
3. Việc xóa bit trên `scheduleBitmask` làm mất vĩnh viễn cấu hình lịch rảnh tổng quát của gia sư đối với các khung thời gian trước hoặc sau khi khóa học kết thúc.

Nhiệm vụ lần này đã triển khai trọn vẹn tính năng **Đặt khóa học định kỳ theo ngày lịch thực tế (Recurring Course Booking)**, giải quyết triệt để các hạn chế trên mà vẫn đảm bảo tính tương thích ngược tuyệt đối với toàn bộ hệ thống hiện hành.

### Các mục tiêu cốt lõi đã đạt được:
- **Thực thể độc lập `CourseBooking`:** Đại diện cho khóa học định kỳ với ngày bắt đầu cụ thể (`startDate: YYYY-MM-DD`), số tuần học (`weeksCount`: 4, 8, 12 tuần), và tổng học phí (`totalCost = hourlyRate * weeksCount`).
- **Xử lý ngày lịch thuần C++17:** Xây dựng `DateUtils` dựa trên thuật toán số học ngày dương lịch dân sự (Howard Hinnant civil day algorithms) và thuật toán Sakamoto, độc lập với thư viện bên ngoài và không phụ thuộc múi giờ hệ điều hành.
- **Quy tắc xung đột ngày học thực tế:** Nhiều yêu cầu `PENDING` có thể cùng tồn tại trên một ca học. Chỉ khi gia sư duyệt (`ACCEPTED`), hệ thống mới kiểm tra xung đột trên tập hợp các ngày học thực tế (`startDate + 7 * i days`) theo nguyên tắc tất cả-hoặc-không (all-or-nothing). Các khóa học không giao nhau về khoảng thời gian (ví dụ: khóa học tháng 11/2026 và khóa học tháng 01/2027) được phép cùng tồn tại mà không bị chặn.
- **Tách biệt Availability và Reservation:** Giữ nguyên `User.scheduleBitmask` khi duyệt hoặc hủy khóa học. Lịch rảnh tổng quát được bảo toàn, còn trạng thái bận thực tế được tính toán từ các khóa học `ACCEPTED`.
- **Lưu trữ độc lập và an toàn:** Quản lý tập tin mới `data/course_bookings.dat` bằng serializer chuyên dụng `CourseBookingSerializer` và repository an toàn đa luồng `FileCourseBookingRepository` sử dụng cơ chế ghi nguyên tử (`.tmp` + thay thế file).
- **Tương thích ngược hoàn hảo:** Bảo tồn tuyệt đối cấu trúc dữ liệu và mã SHA-256 của ba file dữ liệu gốc (`users.dat`, `bookings.dat`, `reviews.dat`).
- **Giao diện trực quan:** Nâng cấp `BookingModal.qml` với bộ chọn ngày thông minh (tự động gợi ý các ngày hợp lệ trong 4 tuần tới tương ứng với thứ đã chọn), chọn thời lượng khóa học, tính toán tổng học phí thời gian thực, hiển thị danh sách ngày học preview chi tiết, và cơ chế chống bấm đúp (`isSubmitting`).
- **Kiểm thử tự động toàn diện:** Bổ sung Test Suite 20 với hàng loạt kịch bản chuyên sâu, nâng tổng số test suite lên 20/20 PASS (100%).

---

## 2. DANH SÁCH FILE THAY ĐỔI VÀ TẠO MỚI

### 2.1. Các file tạo mới (New Files)
| STT | Đường dẫn file | Vai trò / Trách nhiệm |
| :--- | :--- | :--- |
| 1 | `include/models/CourseBooking.h` | Entity đại diện cho khóa học định kỳ với 13 trường dữ liệu, phương thức tính học phí và số buổi học |
| 2 | `include/utils/DateUtils.h` | Header cung cấp các tiện ích xử lý ngày tháng lịch Gregory thuần C++17 |
| 3 | `src/utils/DateUtils.cpp` | Implementation thuật toán Howard Hinnant, thuật toán Sakamoto, sinh ngày học và kiểm tra hợp lệ |
| 4 | `include/storage/serializers/CourseBookingSerializer.h` | Header serializer cho `CourseBooking` độc lập với persistence layer |
| 5 | `src/storage/serializers/CourseBookingSerializer.cpp` | Implementation chuyển đổi 13 trường dữ liệu sang CSV và deserialize an toàn kèm validation |
| 6 | `include/storage/repositories/FileCourseBookingRepository.h` | Header repository quản lý file `course_bookings.dat` theo interface `ICourseBookingRepository` |
| 7 | `src/storage/repositories/FileCourseBookingRepository.cpp` | Implementation lưu trữ file nguyên tử (atomic write qua `.tmp`), đa luồng an toàn |

### 2.2. Các file cập nhật (Modified Files)
| STT | Đường dẫn file | Thay đổi chính |
| :--- | :--- | :--- |
| 8 | `include/storage/IRepository.h` | Bổ sung interface `ICourseBookingRepository` kế thừa `IRepository<CourseBooking>` |
| 9 | `include/storage/MockDatabaseEngine.h` | Thêm quản lý `ICourseBookingRepository`, nạp/ghi `course_bookings.dat` |
| 10 | `src/storage/MockDatabaseEngine.cpp` | Triển khai nạp file `course_bookings.dat` và cung cấp repository cho service |
| 11 | `include/services/BookingService.h` | Bổ sung các phương thức nghiệp vụ đặt khóa học: `bookCourse`, `acceptCourseBooking`, `rejectCourseBooking`, `cancelCourseBooking`, `terminateCourseBooking`, `hasCourseConflict`, truy vấn khóa học |
| 12 | `src/services/BookingService.cpp` | Triển khai logic nghiệp vụ khóa học, kiểm tra xung đột ngày học thực tế, giữ nguyên bitmask gia sư, hỗ trợ đa luồng qua recursive mutex |
| 13 | `include/services/AdminService.h` | Tích hợp khóa học vào các số liệu thống kê GMV và báo cáo quản trị |
| 14 | `src/services/AdminService.cpp` | Tính toán doanh thu và lượt khớp cặp gộp từ cả booking đơn lẻ và khóa học định kỳ |
| 15 | `include/gui/GUIBridge.h` | Khai báo các Q_INVOKABLE methods hỗ trợ QML đặt khóa học, lấy ngày hợp lệ, tính ngày học preview |
| 16 | `src/gui/GUIBridge.cpp` | Cầu nối điều hướng thông minh giữa legacy booking và course booking, định dạng dữ liệu cho QML |
| 17 | `ui/qml/BookingModal.qml` | Nâng cấp giao diện: chọn số tuần (4, 8, 12), chọn ngày bắt đầu, tính tổng chi phí, preview ngày học |
| 18 | `ui/qml/StudentBookingsView.qml` | Hiển thị loại đặt lịch (Khóa học định kỳ / Buổi đơn), khoảng ngày học và số tuần |
| 19 | `ui/qml/TutorBookings.qml` | Hỗ trợ gia sư duyệt/từ chối khóa học định kỳ kèm thông báo xung đột ngày học chi tiết |
| 20 | `ui/qml/AdminMatchesView.qml` | Hiển thị thông tin khóa học định kỳ và cho phép can thiệp hủy ghép lớp |
| 21 | `CMakeLists.txt` | Bổ sung các file mã nguồn mới vào target `CORE_SOURCES`, `HEADERS`, `TutorMatch_Tests` |
| 22 | `tests/TestRunner.cpp` | Thêm `FakeCourseBookingRepository` và `run_test_recurring_course_booking()` (Test Suite 20) |

---

## 3. KIẾN TRÚC VÀ LUỒNG DỮ LIỆU (DATA FLOW)

Tuân thủ nghiêm ngặt mô hình kiến trúc 3 tầng (Three-Layer Architecture) và Nguyên tắc Đảo ngược Phụ thuộc (DIP):

```
+--------------------------------------------------------------------------+
|                        PRESENTATION LAYER (QML)                          |
|  BookingModal.qml / StudentBookingsView.qml / TutorBookings.qml          |
+------------------------------------+-------------------------------------+
                                     | Q_INVOKABLE / Signals / Slots
+------------------------------------v-------------------------------------+
|                          GUI BRIDGE (ADAPTER)                            |
|  GUIBridge::requestRecurringCourseBooking() / getUpcomingStartDates()    |
+------------------------------------+-------------------------------------+
                                     | Domain Contracts / Value Objects
+------------------------------------v-------------------------------------+
|                         BUSINESS SERVICE LAYER                           |
|  BookingService::bookCourse() / acceptCourseBooking() / hasConflict()    |
|  (Concurrency Lock via recursive_mutex, Invariant Validation)            |
+-------------------+--------------------------------+---------------------+
                    |                                |
+-------------------v----------------+   +-----------v---------------------+
|    DOMAIN MODEL & UTILITIES        |   |   PERSISTENCE LAYER (STORAGE)   |
|    CourseBooking entity            |   |   ICourseBookingRepository      |
|    DateUtils (Sakamoto, Hinnant)   |   |   FileCourseBookingRepository   |
+------------------------------------+   +-----------+---------------------+
                                                     |
                                         +-----------v---------------------+
                                         |      CSV SERIALIZATION          |
                                         |   CourseBookingSerializer       |
                                         +-----------+---------------------+
                                                     | Atomic Write
                                         +-----------v---------------------+
                                         |         FILE SYSTEM             |
                                         |   data/course_bookings.dat      |
                                         +---------------------------------+
```

### Luồng xử lý chi tiết của một giao dịch đặt khóa học:
1. **Khởi tạo đặt lịch:** Học viên mở `BookingModal.qml` trên một ca học của gia sư. QML gọi `GUIBridge::getUpcomingStartDatesForDay(dayOfWeek)` để nhận danh sách các ngày hợp lệ trong tháng tới.
2. **Xem trước lịch học:** Khi học viên đổi số tuần (4, 8, 12) hoặc đổi ngày bắt đầu, QML gọi `GUIBridge::calculateCourseSessionDates()` để hiển thị danh sách các ngày học cụ thể dạng danh sách thẻ (chips).
3. **Gửi yêu cầu:** Học viên nhấn nút "Xác nhận gửi yêu cầu khóa học". QML kích hoạt trạng thái chống bấm đúp `isSubmitting = true` và gọi `GUIBridge::requestRecurringCourseBooking()`.
4. **Xử lý nghiệp vụ:** `BookingService` nhận yêu cầu, khóa `serviceMutex`, kiểm tra:
   - Tính hợp lệ của học viên và gia sư (tồn tại, đang hoạt động).
   - Ca học có nằm trong lịch rảnh tổng quát của cả hai bên (`scheduleBitmask`) hay không.
   - Định dạng ngày bắt đầu và sự ăn khớp tuyệt đối giữa `startDate` và `dayOfWeek`.
   - Sinh thực thể `CourseBooking` với trạng thái `PENDING`.
5. **Lưu trữ:** `FileCourseBookingRepository` lưu bản ghi mới vào `course_bookings.dat` thông qua cơ chế ghi file an toàn.
6. **Gia sư tiếp nhận và phê duyệt:**
   - Gia sư xem yêu cầu trong `TutorBookings.qml`.
   - Khi gia sư bấm "Chấp nhận", `BookingService::acceptCourseBooking()` kích hoạt:
     - Tạo danh sách tất cả các ngày học: `generateSessionDates(startDate, weeksCount)`.
     - Quét toàn bộ các khóa học đang ở trạng thái `ACCEPTED` trên cùng ca học của gia sư.
     - Nếu có bất kỳ ngày học nào bị trùng, từ chối giao dịch và trả về lỗi thông báo rõ ngày bị xung đột.
     - Nếu không có xung đột, cập nhật trạng thái thành `ACCEPTED`.
     - **Bảo toàn bitmask:** Không xóa bit trên `tutor->getScheduleBitmask()` để các khung thời gian trước/sau khóa học vẫn mở cho người khác.

---

## 4. THIẾT KẾ CẤU TRÚC DỮ LIỆU COURSEBOOKING

Thực thể `CourseBooking` được định nghĩa trong `include/models/CourseBooking.h` với 13 trường dữ liệu tiêu chuẩn:

| STT | Tên trường | Kiểu dữ liệu | Ý nghĩa nghiệp vụ |
| :--- | :--- | :--- | :--- |
| 1 | `id` | `int` | Khóa chính (Primary Key), tự tăng độc lập |
| 2 | `studentId` | `int` | Khóa ngoại tham chiếu đến học viên đặt lịch (`User.id`) |
| 3 | `tutorId` | `int` | Khóa ngoại tham chiếu đến gia sư được đặt (`User.id`) |
| 4 | `dayOfWeek` | `int` | Thứ trong tuần cố định (0 = Thứ Hai, ..., 6 = Chủ Nhật) |
| 5 | `slotIndex` | `int` | Ca học cố định trong ngày (0 đến 7) |
| 6 | `subject` | `std::string` | Môn học đăng ký |
| 7 | `hourlyRate` | `double` | Học phí theo từng buổi học (VNĐ/buổi) |
| 8 | `status` | `std::string` | Trạng thái vòng đời: `PENDING`, `ACCEPTED`, `REJECTED`, `CANCELLED`, `COMPLETED` |
| 9 | `createdAt` | `std::string` | Thời điểm tạo yêu cầu (định dạng `YYYY-MM-DD` hoặc ISO) |
| 10 | `notes` | `std::string` | Ghi chú hoặc yêu cầu đặc biệt của học viên |
| 11 | `startDate` | `std::string` | Ngày học của buổi đầu tiên (định dạng `YYYY-MM-DD`) |
| 12 | `weeksCount` | `int` | Thời lượng khóa học: 4, 8, hoặc 12 tuần |
| 13 | `totalCost` | `double` | Tổng học phí trọn gói của khóa học (`hourlyRate * weeksCount`) |

### Phương thức bổ trợ:
- `double getTotalCost() const`: Trả về `totalCost > 0 ? totalCost : hourlyRate * weeksCount`.
- `int getTotalSessions() const`: Trả về số buổi học thực tế (`weeksCount`).

---

## 5. GIẢI PHÁP XỬ LÝ NGÀY LỊCH VÀ MÚI GIỜ (DATE & TIME ARCHITECTURE)

Hệ thống sử dụng module `DateUtils` viết bằng C++17 thuần túy, loại bỏ hoàn toàn các lỗi phụ thuộc múi giờ hệ điều hành hoặc định dạng vùng (locale-specific string formatting).

### 5.1. Thuật toán số học ngày dương lịch dân sự (Howard Hinnant Algorithms)
Chuyển đổi hai chiều giữa ngày lịch `(year, month, day)` và số ngày tích lũy kể từ mốc kỷ nguyên chuẩn (Unix Epoch / Civil Epoch):
- **Xác định năm nhuận:**
  $$\text{isLeap}(y) = (y \pmod 4 = 0 \land y \pmod{100} \neq 0) \lor (y \pmod{400} = 0)$$
- **Kiểm tra tính hợp lệ ngày:** Kiểm tra chặt chẽ tháng (1–12), ngày theo từng tháng có tính năm nhuận tháng 2 (28 hoặc 29 ngày).
- **Phép cộng ngày:** Chuyển ngày thành số nguyên ngày tích lũy, cộng thêm `7 * i` ngày, sau đó dịch ngược lại bộ ba `(year, month, day)` để đảm bảo độ chính xác tuyệt đối qua các ranh giới tháng và năm.

### 5.2. Công thức Sakamoto tính thứ trong tuần
Tính toán thứ trong tuần với độ phức tạp $O(1)$ mà không cần duyệt bảng:
```cpp
int DateUtils::dayOfWeek(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int dow = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    // Chuyển chuẩn Chủ Nhật = 0 sang chuẩn hệ thống Thứ Hai = 0 .. Chủ Nhật = 6
    return (dow + 6) % 7;
}
```

### 5.3. Hàm sinh danh sách ngày học (`generateSessionDates`)
Tạo tập hợp các ngày học chính xác:
$$\text{Sessions} = \{\text{addDays}(\text{startDate}, 7 \times i) \mid i = 0, 1, \dots, \text{weeksCount} - 1\}$$

Mỗi chuỗi ngày được định dạng chuẩn hóa `YYYY-MM-DD`, đảm bảo việc so sánh chuỗi tương đương tuyệt đối với so sánh thời gian.

---

## 6. QUY TẮC XUNG ĐỘT LỊCH (CONFLICT DETECTION RULES)

Hệ thống áp dụng các quy tắc kiểm soát xung đột theo mô hình đặt chỗ theo ngày học:

| Tình huống so sánh | Trạng thái yêu cầu cũ | Trạng thái yêu cầu mới | Kết quả kiểm tra xung đột | Hành vi hệ thống |
| :--- | :--- | :--- | :--- | :--- |
| Hai học viên cùng chọn một ca học | `PENDING` | `PENDING` | **KHÔNG XUNG ĐỘT** | Cho phép gửi yêu cầu. Cả hai khóa học cùng hiển thị trong danh sách chờ của gia sư. |
| Học viên gửi vào ca đã có khóa học đang chạy | `ACCEPTED` | `PENDING` | **CHO PHÉP GỬI** | Cho phép học viên gửi yêu cầu với vai trò hàng đợi chờ gia sư quyết định. |
| Gia sư duyệt yêu cầu khi đã có lớp khác cùng ca | `ACCEPTED` | `ACCEPTED` (Chuẩn bị duyệt) | **KIỂM TRA GIAO NGÀY** | Sinh tập ngày học của cả 2 khóa. Nếu tập giao $\ne \emptyset \implies$ **TỪ CHỐI DUYỆT (BÁO LỖI)**. Nếu tập giao $= \emptyset \implies$ **CHO PHÉP DUYỆT**. |
| Khóa học đã kết thúc hoặc bị hủy | `CANCELLED` / `REJECTED` | `ACCEPTED` | **KHÔNG XUNG ĐỘT** | Lịch học đã được giải phóng hoàn toàn. |
| So sánh với booking buổi đơn cũ (Legacy) | `ACCEPTED` (Legacy) | `ACCEPTED` (Course) | **XUNG ĐỘT PHÒNG THỦ** | Nếu tồn tại legacy booking trạng thái `ACCEPTED` trên ca học đó, hệ thống ngăn chặn để bảo vệ dữ liệu cũ. |

### Đảm bảo tính toàn vẹn tất cả-hoặc-không (All-or-Nothing Guarantee):
Nếu một khóa học 12 tuần có 11 buổi không trùng nhưng chỉ 1 buổi trùng vào ngày đã có lớp khác, hệ thống sẽ từ chối toàn bộ khóa học. Tuyệt đối không chấp nhận một phần để tránh rủi ro chia cắt lịch của học viên và gia sư.

---

## 7. ĐỘC LẬP GIỮA LỊCH RẢNH TỔNG QUÁT VÀ ĐẶT LỊCH ĐỊNH KỲ

Một quyết định thiết kế quan trọng trong đợt triển khai này là: **Không thay đổi `User.scheduleBitmask` khi phê duyệt hoặc hủy khóa học định kỳ.**

### Lý do kỹ thuật và nghiệp vụ:
1. `scheduleBitmask` là mặt nạ nhị phân tuần hoàn đại diện cho **khả năng sẵn sàng giảng dạy lặp lại vô hạn** của gia sư trong tuần (General Availability).
2. Một khóa học định kỳ luôn có thời hạn xác định (ví dụ: từ 09/11/2026 đến 30/11/2026 - 4 tuần).
3. Nếu xóa bit của ca học đó trên `scheduleBitmask`:
   - Gia sư sẽ bị hiển thị là "Bận" vĩnh viễn trên ca học đó, khiến học viên khác không thể đặt khóa học mới cho giai đoạn sau ngày 30/11/2026.
   - Khi khóa học kết thúc, nếu khôi phục lại bit, hệ thống không thể biết chắc chắn trước đó gia sư có mở ca này hay không nếu không có cơ chế snapshot phức tạp.
4. **Giải pháp áp dụng:** 
   - `scheduleBitmask` được giữ nguyên làm bộ lọc điều kiện tiên quyết (Prerequisite Check) khi gửi yêu cầu.
   - Tình trạng bận thực tế trong tương lai (Active Reservation) được tính toán động dựa trên giao điểm của các ngày học của những khóa học có trạng thái `ACCEPTED`.

---

## 8. CƠ CHẾ AN TOÀN ĐỒNG THỜI (CONCURRENCY SAFETY)

Để đảm bảo không xảy ra hiện tượng xung đột tranh chấp (race condition) khi nhiều luồng hoặc hai bên học viên - gia sư cùng thao tác một lúc:
- `BookingService` duy trì `std::recursive_mutex serviceMutex`.
- Toàn bộ chuỗi thao tác:
  $$\text{LOCK} \longrightarrow \text{READ REPO} \longrightarrow \text{VERIFY CONFLICT} \longrightarrow \text{UPDATE ENTITY} \longrightarrow \text{SAVE REPO} \longrightarrow \text{UNLOCK}$$
  được đóng gói nguyên tử trong phạm vi `std::lock_guard<std::recursive_mutex>`.
- Ngăn ngừa hoàn toàn nguy cơ Deadlock nhờ vào thiết kế phân cấp lock rõ ràng giữa Service và Repository.

---

## 9. AN TOÀN LƯU TRỮ VÀ PERSISTENCE LAYER

### 9.1. Định dạng file `course_bookings.dat`
File lưu trữ dưới định dạng CSV thuần túy, phân tách bằng dấu phẩy:
```csv
id,studentId,tutorId,dayOfWeek,slotIndex,subject,hourlyRate,status,createdAt,notes,startDate,weeksCount,totalCost
```
Ví dụ bản ghi hợp lệ:
```csv
1,18,2,0,0,Toán 12 - Luyện thi Đại học,150000,ACCEPTED,2026-11-06,Luyện thi cấp tốc,2026-11-09,4,600000
```

### 9.2. Ghi file nguyên tử (Atomic Write)
`FileCourseBookingRepository` triển khai cơ chế ghi nguyên tử an toàn:
1. Ghi toàn bộ dữ liệu ra file tạm thời `data/course_bookings.dat.tmp`.
2. Đồng bộ đệm I/O xuống đĩa vật lý (`flush`).
3. Sử dụng `MoveFileExW` trên Windows (với cờ `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`) hoặc `std::filesystem::rename` để tráo đổi file tạm thành file chính thức.
4. Đảm bảo nếu ứng dụng bị ngắt đột ngột (mất điện, crash), file dữ liệu chính thức không bao giờ bị cắt cụt (truncated) hay hỏng cấu trúc.

---

## 10. TÍCH HỢP GIAO DIỆN NGƯỜI DÙNG (GUI & QML INTEGRATION)

### 10.1. Cải tiến `BookingModal.qml`
- **Bộ chọn thời lượng khóa học:** Thẻ bấm trực quan cho 3 mốc 4 tuần (1 tháng), 8 tuần (2 tháng), 12 tuần (3 tháng).
- **Bộ chọn ngày bắt đầu thông minh:** Tự động lấy danh sách 4 ngày phù hợp gần nhất trong tương lai đúng với thứ của ca học đã chọn thông qua `GUIBridge::getUpcomingStartDatesForDay()`.
- **Hiển thị tổng học phí thời gian thực:** Cập nhật ngay lập tức:
  $$\text{Tổng học phí} = \text{Đơn giá buổi học} \times \text{Số tuần}$$
- **Danh sách ngày học chi tiết (Preview):** Hiển thị danh sách tất cả các ngày học cụ thể dạng thẻ ngang, giúp học viên kiểm tra chính xác lộ trình học trước khi gửi.
- **Bảo vệ chống bấm đúp (`isSubmitting`):** Khóa nút bấm và hiển thị trạng thái đang xử lý khi nhấn gửi yêu cầu.

### 10.2. Cải tiến các màn hình khác
- `StudentBookingsView.qml`: Phân biệt thẻ đặt lịch khóa học định kỳ với nhãn "Khóa học định kỳ (X tuần)" và hiển thị khoảng ngày bắt đầu - kết thúc.
- `TutorBookings.qml`: Hiển thị rõ tổng học phí trọn gói của khóa học và ngày bắt đầu. Khi gia sư chấp nhận hoặc từ chối, modal thông báo chi tiết nếu có xung đột ngày học cụ thể.
- `AdminMatchesView.qml`: Hiển thị ngày bắt đầu thực tế và số tuần học thay vì hiển thị ngày tạo yêu cầu, cho phép Quản trị viên theo dõi chính xác các khóa học đang diễn ra.

---

## 11. KẾT QUẢ KIỂM THỬ TỰ ĐỘNG (AUTOMATED TEST RESULTS)

### 11.1. Chi tiết Test Suite 20 (`run_test_recurring_course_booking`)
Test Suite 20 được thiết kế bao phủ toàn diện 6 nhóm kiểm thử chuyên sâu:
1. **Kiểm thử số học ngày lịch (`DateUtils`):**
   - Xác thực năm nhuận (2000 nhuận, 1900 không nhuận, 2024 nhuận, 2026 không nhuận).
   - Kiểm tra thuật toán Sakamoto: ngày 2026-11-09 chính xác là Thứ Hai (`dayOfWeek == 0`).
   - Kiểm tra sinh tập ngày học: Khóa học 4 tuần bắt đầu từ `2026-11-09` sinh đúng 4 ngày: `2026-11-09`, `2026-11-16`, `2026-11-23`, `2026-11-30`.
2. **Kiểm thử vòng đời khóa học (Lifecycle):**
   - Học viên tạo khóa học `PENDING` thành công.
   - Gia sư duyệt khóa học chuyển sang `ACCEPTED`.
   - Tính toán `totalCost = 150000 * 4 = 600000 VNĐ`.
3. **Kiểm thử quy tắc xung đột ngày học thực tế (Conflict Detection):**
   - Cho phép hai yêu cầu `PENDING` cùng tồn tại trên một ca học.
   - Khi một yêu cầu được `ACCEPTED`, yêu cầu thứ hai nếu trùng ngày sẽ bị chặn không thể `ACCEPTED`.
   - **Xác thực khóa học lệch thời gian:** Khóa học thứ ba cùng thứ và ca học nhưng bắt đầu từ `2027-01-11` (sau khi khóa 1 đã kết thúc) được duyệt `ACCEPTED` thành công mà không bị báo lỗi xung đột!
4. **Kiểm thử hủy và chấm dứt khóa học (Cancellation & Termination):**
   - Học viên hủy yêu cầu `PENDING` chuyển thành `CANCELLED`.
   - Quản trị viên can thiệp chấm dứt khóa học đang chạy chuyển thành `COMPLETED` và giải phóng lịch học ngay lập tức.
5. **Kiểm thử tính toàn vẹn lưu trữ file (`FileCourseBookingRepository`):**
   - Khởi tạo thư mục sandbox kiểm thử độc lập.
   - Ghi bản ghi khóa học ra đĩa bằng serializer chuyên dụng.
   - Đọc ngược lại và kiểm chứng tính toàn vẹn 100% của cả 13 trường dữ liệu.
6. **Kiểm thử hồi quy bảo toàn hệ thống (Regression Tests):**
   - Chạy đồng thời toàn bộ 19 Test Suites trước đó, kiểm chứng không có bất kỳ tác dụng phụ nào làm ảnh hưởng đến các tính năng cũ.

### 11.2. Kết quả thực thi `TutorMatch_Tests.exe`
```
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine... -> PASS
[TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy... -> PASS
[TEST] 3. Verifying DIP, ISP & SRP across decoupled Services... -> PASS
[TEST] 4. Verifying ScheduleGuard Bitmask Collision Math... -> PASS
[TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes... -> PASS
[TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence... -> PASS
[TEST] 7. Verifying Bidirectional Timetable Synchronization... -> PASS
[TEST] 8. Verifying Admin User Card Data Contract... -> PASS
[TEST] 9. Verifying User Detail Modal Contract & Error Handling... -> PASS
[TEST] 10. Verifying Verification & Lock Sync Across Views... -> PASS
[TEST] 11. Verifying AdminService Filters & Search... -> PASS
[TEST] 12. Verifying AVL Tree Multi-Value Support & Deletion... -> PASS
[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary Sanitization... -> PASS
[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence... -> PASS
[TEST] 15. Verifying DataFileInspector Read-Only Inspection... -> PASS
[TEST] 16. Verifying Phase 1 Booking Conflict Detection (15 Scenarios)... -> PASS
[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)... -> PASS
[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)... -> PASS
[TEST] 19. Verifying Phase 3 End-to-End Business Workflows (E2E-01 to E2E-10)... -> PASS
[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)... -> PASS
============================================================
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

---

## 12. BẢO TOÀN DỮ LIỆU GỐC (BASELINE SHA-256 HASHES)

Tuân thủ nghiêm ngặt nguyên tắc bảo vệ dữ liệu sản xuất của dự án, toàn bộ ba file dữ liệu gốc không bị sửa đổi bất kỳ byte nào trong suốt quá trình xây dựng và kiểm thử:

| Tên file dữ liệu | SHA-256 Trước triển khai (Baseline) | SHA-256 Sau triển khai (Verified) | Trạng thái toàn vẹn |
| :--- | :--- | :--- | :--- |
| `data/users.dat` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | `AA04F8181299ADC3B53C0CA3F74136685357E8CBE4144F992765AA78DB7F8916` | **KHỚP 100% (Bit-for-Bit Identical)** |
| `data/bookings.dat` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | `89A428B8A5A69003A96C071B72609CE61B95EDDAAB8C00EFE22851517D2EED0C` | **KHỚP 100% (Bit-for-Bit Identical)** |
| `data/reviews.dat` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | `4DF547BF04C2DC04DB126B9C38C30D139DD75B8EBFC28D38870E87C2CEED484B` | **KHỚP 100% (Bit-for-Bit Identical)** |

Đồng thời, công cụ kiểm định lưu trữ `TutorMatch_Inspector.exe` đã chạy kiểm tra pháp chứng toàn bộ 92 bản ghi và xác nhận:
- Tổng bản ghi hợp lệ: **92/92 (100%)**
- Bản ghi lỗi/hỏng: **0**
- Trạng thái các file gốc: **Hoàn toàn Read-Only**.

---

## 13. HƯỚNG DẪN KIỂM THỬ THỦ CÔNG (MANUAL VERIFICATION GUIDE)

### 13.1. Kiểm thử trên Giao diện Đồ họa (GUI)
1. Chạy ứng dụng GUI:
   ```powershell
   cd C:\Users\DELL\.gemini\antigravity\scratch\TutorMatch\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug
   .\TutorMatch_GUI.exe
   ```
2. Đăng nhập bằng tài khoản học viên:
   - Tên đăng nhập: `student_nam`
   - Mật khẩu: `pass123`
3. Tìm kiếm gia sư (ví dụ: `tutor_math`), mở thời khóa biểu và chọn một ô lịch rảnh.
4. Trên `BookingModal`:
   - Chọn thời lượng khóa học: 4 tuần, 8 tuần hoặc 12 tuần.
   - Quan sát danh sách các ngày bắt đầu được gợi ý và danh sách ngày học xem trước.
   - Nhập ghi chú và nhấn "Xác nhận gửi yêu cầu khóa học".
5. Đăng xuất, đăng nhập vào tài khoản gia sư `tutor_math`:
   - Mở màn hình "Lịch dạy & Đặt chỗ".
   - Quan sát yêu cầu khóa học định kỳ mới gửi đến với đầy đủ thông tin ngày bắt đầu, số tuần và tổng học phí.
   - Bấm "Chấp nhận" để xác nhận khóa học.

### 13.2. Kiểm thử thông qua CLI & Test Runner
1. Chạy toàn bộ Test Suite tự động:
   ```powershell
   .\TutorMatch_Tests.exe
   ```
   Kết quả hiển thị `ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS)` với 20 suite đạt chuẩn.
2. Kiểm tra tính toàn vẹn dữ liệu qua công cụ Inspector:
   ```powershell
   .\TutorMatch_Inspector.exe
   ```

---

## 14. KẾT LUẬN VÀ KHUYẾN NGHỊ TƯƠNG LAI

### 14.1. Kết luận
Đợt triển khai tính năng **Đặt khóa học định kỳ (Recurring Course Booking)** đã hoàn thành xuất sắc toàn bộ các mục tiêu đặt ra:
- Giải quyết triệt để bài toán kinh doanh về đặt khóa học dài hạn theo ngày lịch thực tế.
- Giữ vững kiến trúc 3 tầng sạch sẽ, bảo đảm các nguyên lý SOLID.
- Đảm bảo hiệu năng cao, cơ chế an toàn đa luồng và lưu trữ nguyên tử.
- Bảo vệ tuyệt đối dữ liệu sản xuất sẵn có và vượt qua 100% các bài kiểm tra tự động.

### 14.2. Khuyến nghị phát triển tiếp theo
1. **Mở rộng tính năng nghỉ học / đổi lịch buổi học đơn lẻ:** Cho phép học viên hoặc gia sư yêu cầu dời 1 buổi học trong khóa học 8 hoặc 12 tuần sang một ngày bù mà không cần hủy toàn bộ khóa học.
2. **Nhắc lịch tự động (Notifications):** Bổ sung dịch vụ thông báo qua email hoặc notification trước 24 giờ diễn ra buổi học tiếp theo dựa trên ngày lịch thực tế.
3. **Thanh toán theo từng đợt:** Mở rộng phương thức thanh toán từng phần theo số buổi đã hoàn thành thay vì chỉ thanh toán trọn gói toàn khóa.
