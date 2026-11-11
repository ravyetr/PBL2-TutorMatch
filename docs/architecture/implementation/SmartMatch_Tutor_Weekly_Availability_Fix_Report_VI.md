# BÁO CÁO KỸ THUẬT: KHẮC PHỤC LỖI KHÔNG HIỂN THỊ THỜI KHÓA BIỂU RẢNH HÀNG TUẦN TRONG CHI TIẾT GIA SƯ SMARTMATCH

---

## 1. TỔNG QUAN VÀ MỤC TIÊU (OVERVIEW & OBJECTIVES)

- **Dự án:** TutorMatch (Hệ thống kết nối gia sư và học viên)
- **Công nghệ áp dụng:** C++17, Qt 6.11.2, Qt Quick / QML, MinGW 13.1.0 64-bit
- **Tập tin báo cáo:** `docs/architecture/implementation/SmartMatch_Tutor_Weekly_Availability_Fix_Report_VI.md`
- **Mục tiêu:** Khắc phục triệt để lỗi không hiển thị ma trận thời khóa biểu ca rảnh hàng tuần (**"Thời gian rảnh hàng tuần"** - Section 3) trong hộp thoại thông tin chi tiết gia sư ([`TutorDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorDetailModal.qml)) khi mở từ màn hình **Gợi ý thông minh (SmartMatch)** và **Tìm kiếm gia sư**.

---

## 2. NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE ANALYSIS)

### 2.1. Phân biệt ngữ cảnh nghiệp vụ (Domain Model Distinction)
Trong kiến trúc TutorMatch có hai khái niệm lịch trình hoàn toàn tách biệt:
1. **Lịch rảnh định kỳ hàng tuần (`User.scheduleBitmask`):**
   - Đại diện cho các khung giờ rảnh cố định trong tuần mà gia sư đã đăng ký (56 ca = 7 ngày $\times$ 8 ca).
   - Đây là **nguồn chân lý duy nhất (Single Source of Truth)** cho Section 3 *"Khung thời gian rảnh hàng tuần"* trong [`TutorDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorDetailModal.qml).
2. **Lịch đặt chỗ ca học cụ thể (Course Booking Reservations):**
   - Đại diện cho các ca học đã chốt lịch trong các tuần cụ thể của từng khóa học, nằm trong `course_bookings.dat`.

### 2.2. Điểm đứt gãy luồng dữ liệu (Data Flow Breakdown Before Fix)
Luồng dữ liệu trước khi sửa:
```
[SmartMatch View / Search View]
  │
  ├──> Click "Xem chi tiết" -> tutorDetailModal.openWithTutorId(tutorId)
  │
  └──> [TutorDetailModal.qml]
         │
         ├──> Gọi C++: guiBridge.getTutorDetail(tutorId)
         │      │
         │      └──> GUIBridge::getTutorDetail(int tutorId)
         │             │ (LẤY: id, fullName, email, bio, subjectSpecialty, hourlyRate, rating, reviews...)
         │             │
         │             └──> [BUG GỐC RỄ]: KHÔNG HỀ đưa trường "availableSlots"
         │                                hoặc "scheduleBitmask" vào QVariantMap trả về!
         │
         ├──> tutorDetail.availableSlots luôn là UNDEFINED!
         │
         ├──> rebuildSlotMap() nhận mảng rỗng [] -> availableSlotMap = {}
         │
         └──> isSlotAvailable(dayIdx, slotIdx) trả về FALSE cho tất cả 56 ô!
              Lưới 7x8 hiển thị toàn bộ dấu gạch ngang "—" (trắng trơn / không hiển thị).
```

Cụ thể, hàm giải mã ca rảnh `GUIBridge::getTutorAvailableSlots(int tutorId)` đã được viết sẵn trong [`GUIBridge.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/gui/GUIBridge.cpp), và component [`TutorDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorDetailModal.qml) đã có sẵn logic duyệt `tutorDetail.availableSlots`. Tuy nhiên, tại hàm tổng hợp dữ liệu chi tiết `GUIBridge::getTutorDetail(int tutorId)`, lập trình viên đã **bỏ quên** việc gán `result["availableSlots"]` và `result["scheduleBitmask"]`.

---

## 3. GIẢI PHÁP VÀ CÁC TẬP TIN ĐƯỢC CHỈNH SỬA (EXACT FILES MODIFIED & FIX DETAILS)

### 3.1. Sửa đổi Backend C++: [`src/gui/GUIBridge.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/gui/GUIBridge.cpp)

1. **Bổ sung trường dữ liệu vào `GUIBridge::getTutorDetail(int tutorId)`:**
   ```cpp
   // src/gui/GUIBridge.cpp:687-690
   result["isVerified"] = tutor->getIsVerified();
   result["isActive"] = tutor->getIsActive();
   result["scheduleBitmask"] = QString::number(tutor->getScheduleBitmask());
   result["availableSlots"] = getTutorAvailableSlots(tutorId);
   ```
2. **Bảo toàn hiển thị lịch rảnh declared profile trong `GUIBridge::getTutorAvailableSlots(int tutorId)`:**
   ```cpp
   // src/gui/GUIBridge.cpp:705-712
   QVariantList GUIBridge::getTutorAvailableSlots(int tutorId) {
       QVariantList result;
       if (!userService) return result;

       auto t = userService->getTutorById(tutorId);
       if (!t) return result; // Loại bỏ kiểm tra isActive không cần thiết cho việc xem lịch rảnh định kỳ
   ```

### 3.2. Sửa đổi Giao diện QML: [`ui/qml/TutorDetailModal.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/TutorDetailModal.qml)

1. **Xóa sạch trạng thái cũ và tái thiết lập vị trí cuộn khi mở gia sư mới:**
   ```qml
   function openWithTutorId(tutorId) {
       availableSlotMap = {}
       tutorDetail = {}
       if (contentFlickable) {
           contentFlickable.contentY = 0
       }
       if (typeof guiBridge !== "undefined") {
           var detail = guiBridge.getTutorDetail(tutorId)
           if (detail) {
               if (!detail.availableSlots || detail.availableSlots.length === 0) {
                   detail.availableSlots = guiBridge.getTutorAvailableSlots(tutorId)
               }
               tutorDetail = detail
           } else {
               tutorDetail = {}
           }
       }
       rebuildSlotMap()
       tutorDetailModal.visible = true
   }
   ```
2. **Cơ chế phản ứng tự động (Reactive Fallback):**
   - Bổ sung `onTutorDetailChanged: rebuildSlotMap()`.
   - Bổ sung hàm đa hình `openWithTutor(tutor)`.
   - Trong `rebuildSlotMap()`, hỗ trợ đọc trực tiếp `tutorDetail.availableSlots` hoặc gọi `guiBridge.getTutorAvailableSlots` phòng ngừa.
3. **Data Binding trực tiếp và chính xác:**
   ```qml
   // Thay vì chỉ gọi hàm JS, bind trực tiếp vào property availableSlotMap
   property bool isAvailable: Boolean(tutorDetailModal.availableSlotMap && tutorDetailModal.availableSlotMap[dayIdx + ":" + slotIdx])
   ```
4. **Xử lý trạng thái rỗng rõ ràng (B4 - Visual Semantics):**
   - Khi gia sư có bitmask rỗng (0 ca rảnh), hiển thị banner cảnh báo:
     `⚠️ Gia sư chưa cập nhật thời gian rảnh hàng tuần.`
   - Chú thích chân bảng (Footer) hiển thị chuẩn xác:
     `text: ((tutorDetail.availableSlots || []).length > 0) ? ("Tổng số: " + (tutorDetail.availableSlots || []).length + " ca rảnh/tuần") : "Gia sư chưa cập nhật thời gian rảnh hàng tuần."`

---

## 4. XÁC MINH QUY TẮC ÁNH XẠ BITMASK 7×8 (BITMASK CONVENTION VERIFICATION)

Quy ước bitmask của TutorMatch được kiểm chứng khớp $100\%$ giữa C++ [`ScheduleGuard.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/ds/ScheduleGuard.h) và QML:
- Số ngày trong tuần: $7$ ngày ($0 = \text{Thứ 2}$ đến $6 = \text{Chủ Nhật}$).
- Số ca mỗi ngày: $8$ ca ($0 = \text{07:30}$ đến $7 = \text{22:00}$).
- Vị trí bit: $\text{bitOffset} = \text{dayIndex} \times 8 + \text{slotIndex}$.
- Khóa map trong QML: `"dayIndex:slotIndex"` khớp chính xác với `item["dayIndex"]` và `item["slotIndex"]` từ backend.

---

## 5. KẾT QUẢ KIỂM THỬ TRỰC QUAN & HÌNH HỌC (PHASE C VERIFICATION)

Harness kiểm thử độc lập đã được biên dịch và chạy trên môi trường thực tế (`verify_smartmatch_tutor_detail.exe`):

### 5.1. Kịch bản 1: Gia sư nhiều ca rảnh trên nhiều ngày trong tuần (Tutor ID 2 - ThS. Lê Văn Toàn)
- **ScheduleBitmask:** `71777214294589694` (`0xFF00FF00FF00FE`).
- **Ca rảnh thực tế:** Rảnh Thứ 2, Thứ 4, Thứ 6, Chủ Nhật. Riêng Thứ 2 ca 07:30 bận (bit 0 = 0).
- **Kết quả Backend:** `availableSlots.size() == 31` (Đạt).
- **Kết quả QML Modal:** `availableSlotMap.size == 31` (Đạt).
- **Kiểm tra ca lẻ:**
  - Thứ 2 ca 07:30 (Day 0, Slot 0): **Không có lịch rảnh (—)** (Đạt chuẩn).
  - Thứ 2 ca 09:15 (Day 0, Slot 1): **Có thể đăng ký (✓)** (Đạt chuẩn).
  - Thứ 3 ca 07:30 (Day 1, Slot 0): **Không có lịch rảnh (—)** (Đạt chuẩn).

### 5.2. Kịch bản 2: Gia sư có ít ca rảnh (Tutor ID 17 - Cấu hình 3 ca rảnh)
- **Thiết lập:** Đúng 3 ca: T2 ca 09:15 (0,1), T4 ca 17:00 (2,4), T6 ca 20:30 (4,6).
- **Chuyển đổi gia sư:** Đóng modal Tutor 2, mở Tutor 17.
- **Kết quả QML Modal:** Toàn bộ 31 ca của Tutor 2 bị xóa sạch ngay lập tức. `availableSlotMap.size == 3`.
- **Kiểm tra ô:** Chỉ duy nhất 3 ô trên hiển thị "✓", 53 ô còn lại hiển thị "—".

### 5.3. Kịch bản 3: Gia sư có ma trận rảnh rỗng - 0 ca (Tutor ID 5 - Bitmask = 0)
- **Thiết lập:** `scheduleBitmask = 0ULL`.
- **Chuyển đổi gia sư:** Đóng modal Tutor 17, mở Tutor 5.
- **Kết quả QML Modal:** `availableSlots.size == 0`, `availableSlotMap.size == 0`.
- **Kiểm tra hiển thị:**
  - Banner cảnh báo hiện rõ: `⚠️ Gia sư chưa cập nhật thời gian rảnh hàng tuần.`
  - Toàn bộ 56 ô hiển thị dấu gạch ngang mờ `—`.
  - Chân bảng hiển thị: `"Gia sư chưa cập nhật thời gian rảnh hàng tuần."`

### 5.4. Kịch bản 4: Kiểm tra tính bất biến và mở lại (Idempotency & Reopening)
- **Mở lại Tutor ID 2:** Toàn bộ 31 ca rảnh được khôi phục chính xác 100%, không bị ảnh hưởng bởi các thao tác mở gia sư rỗng trước đó.

---

## 6. KẾT QUẢ BIÊN DỊCH VÀ HỒI QUY HỆ THỐNG (REGRESSION & COMPILATION RESULTS)

1. **Biên dịch GUI:**
   - Lệnh: `mingw32-make TutorMatch_GUI`
   - Kết quả: `[100%] Built target TutorMatch_GUI` — Thành công $100\%$, mã thoát $0$.
2. **Kiểm thử tự động 24 Suites:**
   - Lệnh: `TutorMatch_Tests.exe`
   - Kết quả: **24/24 Test Suites PASSED (100% PASS)**.
   - Không có bất kỳ sự thay đổi hay ảnh hưởng tiêu cực nào đến logic tính điểm MatchingEngine, ScheduleGuard, BookingService, hay dữ liệu file người dùng.

---

## 7. KẾT LUẬN

Lỗi không hiển thị thời khóa biểu ca rảnh của gia sư trong SmartMatch đã được giải quyết dứt điểm. Giải pháp tuân thủ nghiêm ngặt nguyên lý SOLID, bảo tồn tính độc lập giữa lịch rảnh định kỳ (`scheduleBitmask`) và lịch khóa học (`course_bookings`), hỗ trợ đầy đủ các trạng thái rỗng và chuyển đổi gia sư một cách mượt mà.
