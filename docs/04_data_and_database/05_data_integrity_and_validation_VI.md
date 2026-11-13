# TÍNH TOÀN VẸN DỮ LIỆU VÀ QUY TẮC KIỂM TRA (DATA INTEGRITY & VALIDATION)
## Báo cáo Kiểm toán Tính Toàn vẹn, Khả năng Chống Hỏng Dữ liệu và Ma trận Thẩm định

---

## 1. KẾT QUẢ KIỂM TOÁN TÍNH TOÀN VẸN DỮ LIỆU HIỆN TẠI `[VERIFIED]`

Dựa trên kết quả thực thi công cụ chẩn đoán chuyên sâu [`TutorMatch_Inspector`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/tools/inspect_main.cpp) trực tiếp trên kho dữ liệu thực tế tại `data/*.dat`:

| Tệp Dữ Liệu Thực Tế | Tổng số dòng đọc được | Số bản ghi Hợp Lệ (Valid) | Số bản ghi Lỗi (Invalid) | Tỷ lệ Toàn vẹn | Trạng thái bảo vệ tệp |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **`data/users.dat`** | 52 | 52 | 0 | **100%** | Read-Only An Toàn |
| **`data/course_bookings.dat`** | 23 | 23 | 0 | **100%** | Read-Only An Toàn |
| **`data/reviews.dat`** | 15 | 15 | 0 | **100%** | Read-Only An Toàn |
| **`data/bookings.dat`** (Archive) | 16 | 16 | 0 | **100%** | Read-Only An Toàn |
| **TỔNG TOÀN BỘ HỆ THỐNG** | **90 bản ghi** | **90 bản ghi** | **0 lỗi** | **100% HOÀN HẢO** | **Tuyệt đối chính xác** |

---

## 2. CÁC NGUY CƠ DỮ LIỆU VÀ GIẢI PHÁP PHÒNG VỆ KIẾN TRÚC

| Nguy cơ toàn vẹn dữ liệu | Khả năng xảy ra trên Flat-File | Giải pháp kiến trúc đã triển khai trong TutorMatch |
| :--- | :---: | :--- |
| **Trùng lặp Khóa chính (Duplicate Primary Keys)** | Trung bình | • Mỗi Repository duy trì một hàm `getNextId()` quét toàn bộ các bản ghi hiện có, tìm `max(id)` và cộng thêm 1.<br>• Khởi tạo bộ đệm in-memory tự động deduplicate các dòng trùng ID (theo cơ chế ghi đè bản ghi mới nhất). |
| **Tham chiếu Mồ côi (Orphan References)** | Thấp | • Áp dụng chính sách Xóa Mềm (`isActive = false`) thay vì xóa cứng User.<br>• Các khóa học cũ vẫn luôn giữ được liên kết toàn vẹn tới gia sư và học viên tương ứng. |
| **Sai lệch điểm Rating tích lũy (Rating Desynchronization)** | Trung bình | • **Tự phục hồi lúc khởi động (ADR-007 Self-Healing):** `ReviewService::reconcileRatingsOnStartup()` quét lại toàn bộ file `reviews.dat` để đồng bộ lại điểm rating thực tế vào `users.dat`.<br>• **Bồi hoàn giao dịch (Compensating Rollback):** Tự động rollback xóa bài review nếu cập nhật User thất bại. |
| **Hỏng cấu trúc CSV do ký tự đặc biệt (CSV Injection/Break)** | Cao | • Toàn bộ các thao tác ghi đều đi qua [`SerializerUtils::escapeCsv()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/SerializerUtils.cpp), tự động bọc ngoặc kép `"` và nhân đôi ngoặc kép `""` nếu có dấu phẩy hoặc xuống dòng.<br>• `SerializerUtils::splitCsvLine()` hỗ trợ phân tách chuỗi nằm trong ngoặc kép chuẩn RFC 4180. |
| **Cháy / Rỗng file khi mất điện đột ngột (File Corruption / Zero-byte)** | Cao trên Flat-File | • **Ghi tệp tạm thời nguyên tử (Atomic File Replacement):** Ghi toàn bộ dữ liệu ra tệp `.tmp` trước, sau đó gọi `MoveFileExW` / `std::filesystem::rename` để hoán đổi nguyên tử sang tệp `.dat`. |
| **Lỗi tràn số và bit ma trên lịch học (Bitmask Phantom Bits)** | Thấp | • [`ScheduleGuard::sanitizeMask()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/business/algorithms/ScheduleGuard.h) luôn thực hiện phép `AND` với `0x00FFFFFFFFFFFFFFULL` để triệt tiêu mọi bit từ bit 56 đến bit 63 trước khi lưu và sau khi đọc. |

---

## 3. CƠ CHẾ BẢO VỆ TƯƠNG THÍCH NGƯỢC CỦA BỘ TUẦN TỰ HÓA (BACKWARD COMPATIBILITY)

Hệ thống lưu trữ của TutorMatch đã trải qua nhiều giai đoạn nâng cấp nghiệp vụ (ví dụ: bổ sung khóa học nhiều ca, bổ sung cam kết V1, decommission cờ xác minh gia sư). Để không làm hỏng dữ liệu cũ:

1. **Tuần tự hóa User:**
   - [`UserSerializer::fromCsv()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/UserSerializer.cpp) kiểm tra số lượng trường:
     - Nếu dòng Tutor có 14 trường (chuẩn mới): Nạp trực tiếp.
     - Nếu dòng Tutor có 15 trường (chuẩn cũ còn cờ `isVerified`): Tự động bỏ qua cờ xác minh mà không báo lỗi, sau đó khi ghi lại sẽ tự động làm sạch thành 14 trường chuẩn.
2. **Tuần tự hóa CourseBooking:**
   - [`CourseBookingSerializer::fromCsv()`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/data_access/serializers/CourseBookingSerializer.cpp) linh hoạt:
     - Dòng có 13 trường cơ sở: Tự động điền các giá trị mặc định cho 10 trường cam kết mở rộng (`earlyTerminationStatus = "NONE"`, `weeklySchedule = day:slot`).
     - Dòng có 23 trường: Đọc đầy đủ toàn bộ thông tin cam kết V1 và tiến trình kết thúc sớm.
