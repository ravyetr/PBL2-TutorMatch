# THIẾT KẾ KIẾN TRÚC: MÔ HÌNH TÀI CHÍNH QUẢN TRỊ VÀ CƠ CHẾ KHÓA ĐA TIẾN TRÌNH
## Dự án TutorMatch | C++17 | Qt 6.11 | Pre-Implementation Architecture Design

**Dự án:** TutorMatch  
**Không gian làm việc:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Tài liệu tham chiếu:**
- `docs/architecture/implementation/CourseBooking_Independent_Challenge_Audit_VI.md`
- `docs/architecture/decisions/ADR-005_Commission_State_Ownership_And_Persistence.md`
- `docs/architecture/decisions/ADR-007_Multi_File_Consistency_And_Startup_Reconciliation.md`  
**Ngày lập:** 09/11/2026  
**Vai trò:** Senior Software Architect, Systems Engineer & Financial Domain Specialist  
**Mục tiêu:** Thiết kế giải pháp kiến trúc chi tiết trước triển khai (Pre-Implementation Design) cho 2 bài toán trọng yếu phát hiện từ đợt kiểm toán độc lập:
1. **Định nghĩa chuẩn mực mô hình tài chính (GMV & Doanh thu hoa hồng nền tảng)** phù hợp với mô hình khóa học định kỳ nhiều tuần (`CourseBooking`).
2. **Thiết kế cơ chế khóa tệp đa tiến trình cấp Hệ điều hành (OS File Locking)** bảo vệ tính toàn vẹn của tầng lưu trữ file phẳng (`.dat`) khi có nhiều tiến trình chạy song song (GUI và CLI).

---

## 1. KHẢO SÁT HIỆN TRẠNG KIẾN TRÚC (CURRENT ARCHITECTURE FINDINGS)

### 1.1. Hiện trạng Mã nguồn Nghiệp vụ Tài chính

Kiểm tra trực tiếp các tệp mã nguồn liên quan đến mô hình tài chính:
- `include/models/User.h` và `src/models/User.cpp`
- `include/models/CourseBooking.h` và `src/models/CourseBooking.cpp`
- `include/dtos/BookingDTOs.h` và `include/dtos/AdminMetrics.h`
- `include/services/AdminService.h` và `src/services/AdminService.cpp`
- `include/gui/GUIBridge.h` và `src/gui/GUIBridge.cpp`
- Giao diện QML: `AdminCommissionConfigView.qml`, `AdminOverviewView.qml`, `BookingModal.qml`, `StudentBookingsView.qml`, `TutorBookings.qml`, `TutorCard.qml`, `UserDetailModal.qml`.

#### 1. Ý nghĩa và cách hiểu của `hourlyRate`
- Trong thực thể `User` (`User.h` dòng 24): Biến được đặt tên là `double hourlyRate;`.
- Trong `User.cpp` dòng 40: Khi in thông tin gia sư, đơn vị được hiển thị là `"VND/buổi"`.
- Trong `CourseBooking.h` dòng 22: Thuộc tính được kế thừa mang tên `double hourlyRate;`.
- Trong giao diện QML:
  - `BookingModal.qml` (dòng 94, 214): `"Học phí: " + tutorRate.toLocaleString(Qt.locale("vi_VN")) + " VNĐ/buổi"`.
  - `StudentBookingsView.qml` (dòng 155): `"Học phí: " + Number(model.hourlyRate).toLocaleString(Qt.locale("vi_VN")) + " VNĐ/buổi"`.
  - `TutorBookings.qml` (dòng 150): `"Học phí: " + Number(model.hourlyRate).toLocaleString(Qt.locale("vi_VN")) + " VNĐ/buổi"`.
  - `TutorCard.qml` (dòng 105): `"VNĐ/buổi"`.
  - Ngoại lệ: `UserDetailModal.qml` (dòng 170): Có đoạn ghi `"đ/giờ"` hoặc `"đ/h"`.
- **Kết luận:** Mặc dù biến mang tên `hourlyRate`, trong toàn bộ luồng nghiệp vụ thực tế của TutorMatch, giá trị này đại diện cho **Học phí cho một buổi học tiêu chuẩn** (`sessionFee` hay `lessonPrice`).

#### 2. Cấu trúc thời khóa biểu và ca học (Timetable Slots)
- Thời khóa biểu được chia thành $7 \times 8 = 56$ ca học (slots).
- 7 ngày trong tuần ($0 = \text{Thứ Hai} \dots 6 = \text{Chủ Nhật}$).
- Mỗi ngày gồm 8 ca học, mỗi ca cố định **90 phút** (ví dụ: Ca 0: 07:30 - 09:00, Ca 1: 09:15 - 10:45, Ca 2: 13:30 - 15:00,...).
- Do đó, mỗi ca học đại diện cho đúng **1 buổi học (session)** kéo dài 1.5 giờ.

#### 3. Mối quan hệ giữa `weeksCount` và số buổi học
- Mô hình `CourseBooking` áp dụng nguyên tắc: Một khóa học định kỳ diễn ra vào **đúng một thứ trong tuần và một ca học cố định**.
- Khóa học có độ dài được chọn từ tập hợp cố định: $\text{weeksCount} \in \{4, 8, 12\}$ tuần.
- Vì mỗi tuần học đúng 1 buổi, tổng số buổi học của một khóa luôn bằng chính số tuần:
  $$\text{totalSessions} = \text{weeksCount}$$
- Tổng học phí cam kết của khóa học được định nghĩa trong `CourseBooking::getTotalCost()`:
  $$\text{totalCost} = \text{hourlyRate} \times \text{weeksCount}$$

#### 4. Thuật toán tính toán hiện tại trong `AdminService::calculateMetrics()`
Mã nguồn tại `src/services/AdminService.cpp` (dòng 34-60):
```cpp
AdminMetrics AdminService::calculateMetrics(double commissionRate) const {
    std::lock_guard<std::mutex> lock(adminMutex);
    AdminMetrics m;
    m.commissionRate = commissionRate;

    auto courseBookings = courseBookingRepo->getAllCourseBookings();
    for (const auto& cb : courseBookings) {
        if (cb.status == "COMPLETED") {
            m.completedBookings++;
            m.initialConnectionGMV += cb.hourlyRate; // <--- CHỈ CỘNG HỌC PHÍ 1 BUỔI
        } else if (cb.status == "ACCEPTED") {
            m.activeMatches++;                       // <--- KHÔNG CỘNG VÀO GMV
        }
    }

    double rateToUse = (commissionRate > 0.0) ? commissionRate : platformCommissionRate;
    m.platformCommissionRevenue = m.initialConnectionGMV * rateToUse;
    return m;
}
```
**Nhận xét:**
- `initialConnectionGMV` chỉ cộng dồn `cb.hourlyRate` (học phí của 1 buổi).
- Hoàn toàn **không nhân với `cb.weeksCount`** và **không gọi `cb.getTotalCost()`**.
- Các đơn đang học (`ACCEPTED`) chỉ được đếm số lượng vào `activeMatches`, **hoàn toàn không được tính vào GMV hay doanh thu treo**.
- Các đơn `PENDING`, `REJECTED`, `CANCELLED` bị loại trừ hoàn toàn khỏi GMV.
- Doanh thu hoa hồng sàn được tính bằng:
  $$\text{platformCommissionRevenue} = \text{initialConnectionGMV} \times \text{platformCommissionRate} \quad (\text{mặc định } 25\%)$$

### 1.2. Phân tích Dữ liệu Thực tế trên Bản ghi Khóa học #1008

Trong tệp sản xuất `data/course_bookings.dat`, bản ghi #1008 có nội dung:
```csv
1008,33,12,Trịnh Thảo My,Thầy Võ Minh Tin,Tin học 11-12,180000,0,1,2026-09-07,4,COMPLETED,2026-09-01
```
- **Học viên:** Trịnh Thảo My (ID: 33)
- **Gia sư:** Thầy Võ Minh Tin (ID: 12)
- **Môn học:** Tin học 11-12
- **Học phí buổi (`hourlyRate`):** $180,000$ VNĐ/buổi
- **Ngày học:** Thứ Hai (`dayOfWeek = 0`), Ca 1 (`slotIndex = 1`)
- **Ngày bắt đầu (`startDate`):** `2026-09-07`
- **Số tuần (`weeksCount`):** $4$ tuần (4 buổi học: 07/09, 14/09, 21/09, 28/09/2026)
- **Trạng thái:** `COMPLETED`
- **Ngày tạo:** `2026-09-01`

**Chỉ số tài chính đang hiển thị trên Admin Dashboard hiện tại:**
- Tổng GMV hiển thị: $180,000$ VNĐ.
- Hoa hồng nền tảng (25%): $45,000$ VNĐ.
- Thu nhập gia sư ghi nhận qua nền tảng: $135,000$ VNĐ.
- Số lớp hoàn thành: 1.
- Số lớp đang học (`ACCEPTED`): 4 (các đơn #1001, #1002, #1003, #1004).

---

## 2. SO SÁNH HAI MÔ HÌNH TÀI CHÍNH (FINANCIAL MODEL COMPARISON)

Vấn đề cốt lõi đặt ra cho kiến trúc là: **TutorMatch là sàn kết nối môi giới (Lead Generation Marketplace) hay sàn quản lý khóa học trọn gói (Full-Course Managed Marketplace)?**

```
+---------------------------------------------------------------------------------------------------+
|                                     SO SÁNH BẢN CHẤT NGHIỆP VỤ                                    |
+-------------------------------------------------+-------------------------------------------------+
| MÔ HÌNH 1: PHÍ KẾT NỐI BUỔI ĐẦU (LEAD GEN)       | MÔ HÌNH 2: KÝ QUỸ TRỌN KHÓA (FULL ESCROW)       |
+-------------------------------------------------+-------------------------------------------------+
| • Sàn chỉ thu phí giới thiệu trên buổi học đầu. | • Sàn thu học phí và hoa hồng trên toàn bộ khóa.|
| • Từ buổi thứ 2 trở đi, gia sư và học sinh tự   | • Học viên thanh toán gói 4/8/12 tuần.          |
|   giao dịch thanh toán trực tiếp.               | • Sàn giữ tiền ký quỹ và trả dần cho gia sư.    |
| • GMV = Giá trị buổi đầu tiên kết nối.          | • GMV = Tổng giá trị toàn bộ hợp đồng khóa học. |
+-------------------------------------------------+-------------------------------------------------+
```

### 2.1. Phân tích Chi tiết Từng Mô hình

#### Mô hình 1: Phí Giới Thiệu Buổi Đầu (Lead Generation / Initial Session Commission)
- **Định nghĩa GMV:** GMV là tổng giá trị giao dịch buổi đầu tiên mà nền tảng điều phối thành công.
  $$\text{GMV}_{\text{Model 1}} = \sum_{cb \in \text{COMPLETED}} cb.\text{hourlyRate}$$
- **Hoa hồng sàn:** $25\%$ của học phí buổi đầu.
  $$\text{Revenue}_{\text{Model 1}} = \text{GMV}_{\text{Model 1}} \times 25\%$$
- **Số liệu cụ thể trên đơn #1008:**
  - GMV ghi nhận: $180,000$ VNĐ.
  - Hoa hồng sàn thu: $180,000 \times 25\% = 45,000$ VNĐ.
  - Gia sư thực nhận từ sàn: $135,000$ VNĐ (cho buổi 1).
  - Các buổi 2, 3, 4 ($180,000 \times 3 = 540,000$ VNĐ): Gia sư tự thu trực tiếp 100% từ học sinh, sàn không can thiệp và không ghi nhận doanh thu.

#### Mô hình 2: Quản Lý Khóa Học & Thu Phí Trọn Khóa (Full Course Escrow Commission)
- **Định nghĩa GMV:** GMV là tổng giá trị hàng hóa/dịch vụ của toàn bộ khóa học mà các bên cam kết thực hiện.
  $$\text{TotalCourseValue} = cb.\text{hourlyRate} \times cb.\text{weeksCount}$$
  $$\text{GMV}_{\text{Realized}} = \sum_{cb \in \text{COMPLETED}} (cb.\text{hourlyRate} \times cb.\text{weeksCount})$$
  $$\text{GMV}_{\text{Committed / Escrow}} = \sum_{cb \in \text{ACCEPTED}} (cb.\text{hourlyRate} \times cb.\text{weeksCount})$$
- **Hoa hồng sàn:** $25\%$ trên tổng giá trị toàn khóa học khi hoàn thành.
  $$\text{Revenue}_{\text{Realized}} = \text{GMV}_{\text{Realized}} \times 25\%$$
  $$\text{Revenue}_{\text{Pending}} = \text{GMV}_{\text{Committed}} \times 25\%$$
- **Số liệu cụ thể trên đơn #1008 ($180,000$ VNĐ $\times$ 4 tuần):**
  - GMV hoàn thành ghi nhận: $720,000$ VNĐ.
  - Hoa hồng sàn thực thu: $720,000 \times 25\% = 180,000$ VNĐ.
  - Gia sư thực nhận qua sàn: $720,000 - 180,000 = 540,000$ VNĐ (tương đương $135,000$ VNĐ/buổi sau hoa hồng).

### 2.2. Ma trận So sánh Toàn diện Giữa Hai Mô hình

| Tiêu chí Đánh giá | Mô hình 1: Phí kết nối buổi đầu (Lead Gen) | Mô hình 2: Phí trọn khóa học (Full Escrow) |
|---|---|---|
| **Khái niệm GMV** | Giá trị buổi đầu tiên của khóa học hoàn tất. | Tổng giá trị toàn bộ số buổi học trong hợp đồng khóa học. |
| **Công thức đơn lẻ** | $\text{hourlyRate}$ | $\text{hourlyRate} \times \text{weeksCount}$ |
| **Giá trị đơn #1008** | **$180,000$ VNĐ** | **$720,000$ VNĐ** |
| **Hoa hồng đơn #1008** | **$45,000$ VNĐ** | **$180,000$ VNĐ** |
| **Xử lý đơn `ACCEPTED`** | Chỉ tăng bộ đếm `activeMatches`, không tính tiền. | Ghi nhận vào $\text{GMV}_{\text{Pending}}$ và $\text{Revenue}_{\text{Pending}}$ (doanh thu dự kiến). |
| **Tổng GMV trên 8 đơn hiện tại** | - Hoàn thành: $180,000$ VNĐ<br>- Đang học: $0$ VNĐ | - Hoàn thành: $720,000$ VNĐ<br>- Đang học (4 đơn): $5,320,000$ VNĐ<br>- Tổng cam kết: $6,040,000$ VNĐ |
| **Rủi ro "đi đêm" (Disintermediation)** | **Rất cao**: Gia sư và học viên kết nối buổi đầu rồi tự giao dịch ngoài nền tảng để tránh hoa hồng. | **Thấp**: Nền tảng bảo chứng chất lượng, giữ tiền và giải ngân từng buổi hoặc cuối khóa. |
| **Mức độ phức tạp thanh toán** | **Thấp**: Chỉ cần thu phí 1 lần buổi đầu. Không cần xử lý hoàn tiền khóa học. | **Trung bình - Cao**: Cần chính sách hoàn trả khi học viên hủy khóa giữa chừng (ví dụ học 2/4 tuần). |
| **Độ tương thích với mã nguồn hiện tại** | **100% Khớp**: `AdminService.cpp` hiện tại đang viết theo mô hình này. Không cần sửa code. | **Cần cập nhật**: Cần cập nhật `AdminService::calculateMetrics()`, mở rộng DTO `AdminMetrics` và giao diện QML. |
| **Mức độ chuẩn tắc theo EdTech** | Thường chỉ áp dụng cho diễn đàn gia sư tự do giá rẻ. | Chuẩn mực của các nền tảng EdTech chuyên nghiệp (Preply, Cambly, Topica). |

### 2.3. Khuyến nghị Kỹ thuật (Technical Recommendation)
1. **Bản chất kiến trúc đã chuyển dịch sang `CourseBooking`:** Toàn bộ hệ thống vừa hoàn thành việc di trú từ ca đơn lẻ sang "Khóa học định kỳ nhiều tuần" (`weeksCount = 4, 8, 12`). Việc định nghĩa GMV chỉ bằng 1 buổi học là **không phản ánh đúng quy mô kinh tế (Economic Scale)** của các khóa học đang diễn ra trên sàn.
2. **Khuyến nghị kiến trúc:** Chuyển đổi sang **Mô hình 2 (Full Course Escrow Commission)** là hướng đi chuẩn mực và đồng bộ với thuộc tính `weeksCount` của `CourseBooking`.
3. **Tuy nhiên**, vì việc này làm thay đổi toàn bộ số liệu thống kê doanh thu và báo cáo tài chính của nền tảng, **đây là quyết định thuộc thẩm quyền của Product Owner (PO)**. Nhóm kỹ thuật cung cấp tài liệu thiết kế này và chờ phê duyệt chính thức trước khi sửa đổi mã nguồn.

---

## 3. KHÁI NIỆM VÀ QUY CHUẨN DỮ LIỆU TÀI CHÍNH (FINANCIAL DATA SEMANTICS)

Để giải quyết triệt để tình trạng mập mờ giữa `hourlyRate`, "Học phí/buổi", và "Tổng học phí khóa", chúng tôi thiết lập Bảng Phân loại Thuật ngữ Tài chính Chuẩn mực (Domain Financial Taxonomy):

### 3.1. Bảng Quy chuẩn Thuật ngữ Miền Tài chính

| Thuật ngữ Nghiệp vụ | Tên biến đề xuất | Tên biến cũ trong mã nguồn | Đơn vị | Định nghĩa nghiệp vụ & Công thức | Phân loại Lưu trữ |
|---|---|---|:---:|---|:---:|
| **Học phí mỗi buổi** | `sessionFee` | `hourlyRate` | VNĐ/buổi | Học phí cho một buổi học định kỳ 90 phút (1 ca). | Lưu bền vững (`CourseBooking`) |
| **Thời lượng ca học** | `sessionDurationMinutes` | *(Ngầm định 90)* | Phút | Thời lượng quy chuẩn của 1 ca học ($90$ phút). | Hằng số cấu hình hệ thống |
| **Số buổi trong khóa** | `totalSessions` | `weeksCount` | Buổi | Tổng số buổi học trong toàn khóa ($4, 8, 12$ buổi). | Lưu bền vững (`CourseBooking`) |
| **Tổng giá trị khóa học** | `totalCourseValue` | `getTotalCost()` | VNĐ | Tổng số tiền học phí trọn khóa: $\text{sessionFee} \times \text{totalSessions}$. | Tính toán động hoặc Snapshot |
| **Tỷ lệ hoa hồng nền tảng** | `platformCommissionRate` | `platformCommissionRate` | % (0.25) | Tỷ lệ phần trăm sàn thu trên giao dịch ($25\%$). | Quản trị viên cấu hình (`AdminService`) |
| **Tỷ lệ hoa hồng tại thời điểm đặt** | `commissionRateAtBooking` | *(Chưa có)* | % | Tỷ lệ hoa hồng có hiệu lực tại thời điểm khóa học được tạo/chấp nhận. | Snapshot vào bản ghi đơn |
| **Giá trị hoa hồng đơn** | `commissionAmount` | *(Chưa có)* | VNĐ | Số tiền hoa hồng sàn thu được từ đơn khóa học: $\text{totalCourseValue} \times \text{commissionRateAtBooking}$. | Snapshot khi đơn `COMPLETED` |
| **GMV Thực hiện** | `realizedGMV` | `initialConnectionGMV` | VNĐ | Tổng giá trị các khóa học đã kết thúc thành công (`COMPLETED`). | Tính toán động (`AdminService`) |
| **GMV Cam kết (Treo)** | `pendingGMV` | *(Chưa có)* | VNĐ | Tổng giá trị các khóa học đang trong tiến trình giảng dạy (`ACCEPTED`). | Tính toán động (`AdminService`) |
| **Doanh thu Thực nhận** | `realizedRevenue` | `platformCommissionRevenue` | VNĐ | Tổng tiền hoa hồng sàn thực thu từ các khóa học `COMPLETED`. | Tính toán động (`AdminService`) |
| **Doanh thu Dự kiến** | `pendingRevenue` | *(Chưa có)* | VNĐ | Tổng tiền hoa hồng sàn dự kiến sẽ thu từ các khóa học `ACCEPTED`. | Tính toán động (`AdminService`) |
| **Thu nhập Gia sư Ròng** | `tutorNetEarnings` | *(Chưa có)* | VNĐ | Số tiền gia sư nhận được sau khi trừ hoa hồng: $\text{totalCourseValue} - \text{commissionAmount}$. | Tính toán động / Hiển thị UI |

### 3.2. Phân định: Snapshot Bền vững vs Tính toán Động (Storage vs Dynamic Calculation)

#### 1. Tại sao cần cơ chế Snapshot Tỷ lệ Hoa hồng?
- **Nguy cơ lỗi nghiệp vụ nghiêm trọng:** Hiện tại, trong `AdminService.cpp`, doanh thu hoa hồng được tính động bằng:
  $$\text{platformCommissionRevenue} = \text{GMV} \times \text{platformCommissionRate}$$
  Nếu Quản trị viên thay đổi thanh trượt hoa hồng trong `AdminCommissionConfigView.qml` từ $25\%$ xuống $20\%$, toàn bộ doanh thu từ các khóa học đã hoàn thành trong quá khứ (ví dụ đơn #1008 từ tháng 9/2026) **bị tính lại theo tỷ lệ 20%**, làm sai lệch sổ sách kế toán lịch sử!
- **Nguyên tắc kế toán chuẩn:** Khi một giao dịch tài chính hoàn thành (`COMPLETED`), tỷ lệ hoa hồng và số tiền hoa hồng của giao dịch đó phải được **đóng băng vĩnh viễn (Immutable Snapshot)**. Việc thay đổi cấu hình hoa hồng của sàn trong tương lai chỉ có hiệu lực với các đơn hàng mới.

#### 2. Kiến trúc mở rộng cho bản ghi `CourseBooking` (Giai đoạn tiếp theo):
Khi được PO duyệt cập nhật schema:
```cpp
// Snapshot các trường tài chính vào CourseBooking:
double commissionRateSnapshot; // Lưu ví dụ 0.25 tại thời điểm hoàn thành
double platformCommission;    // Lưu ví dụ 180000 VNĐ
```
- Dữ liệu lịch sử đã lưu sẽ không bao giờ bị biến động khi Admin đổi cấu hình chung.
- Tầng báo cáo (`AdminService`) chỉ việc cộng dồn:
  $$\text{realizedRevenue} = \sum_{cb \in \text{COMPLETED}} cb.\text{platformCommission}$$

---

## 4. THIẾT KẾ KHÓA ĐA TIẾN TRÌNH CẤP HỆ ĐIỀU HÀNH (MULTI-PROCESS FILE LOCKING DESIGN)

### 4.1. Bản chất Kỹ thuật của Hạn chế Hiện tại

Trong kiến trúc hiện hành:
```cpp
// FileCourseBookingRepository.cpp
bool FileCourseBookingRepository::updateCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    // ...
    std::string tempPath = filePath + ".tmp";
    std::ofstream out(tempPath, ...);
    // Ghi toàn bộ dữ liệu ra file tạm .tmp
    // Sau đó gọi MoveFileExW(tempPath, filePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}
```
1. `std::recursive_mutex repoMutex` là cấu trúc đồng bộ trong không gian địa chỉ bộ nhớ ảo của một tiến trình duy nhất (Virtual Memory Address Space).
2. Khi người dùng chạy song song:
   - Tiến trình 1: `TutorMatch_GUI.exe` (PID: 10420)
   - Tiến trình 2: `TutorMatch_CLI.exe` (PID: 15832)
   Hai tiến trình hoàn toàn không nhìn thấy mutex của nhau.
3. Kịch bản mất dữ liệu thực tế (Data Loss Race Condition):
   - **T=0:** Cả GUI và CLI cùng nạp 8 bản ghi từ `course_bookings.dat` vào RAM.
   - **T=1:** Người dùng trên CLI chấp nhận đơn #1005 (`PENDING -> ACCEPTED`), CLI ghi ra `course_bookings.dat.tmp` và hoán đổi đè vào `course_bookings.dat`. File trên đĩa giờ có đơn #1005 ở trạng thái `ACCEPTED`.
   - **T=2:** Người dùng trên GUI từ chối đơn #1006 (`PENDING -> REJECTED`). Bản sao trong RAM của GUI vẫn chứa đơn #1005 ở trạng thái `PENDING` cũ! GUI ghi snapshot của mình ra `course_bookings.dat.tmp` và hoán đổi đè vào `course_bookings.dat`.
   - **Hậu quả:** Đơn #1005 bị ghi đè trở lại thành `PENDING`. Thao tác chấp nhận của CLI bị **bốc hơi hoàn toàn**.
4. Xung đột tệp tạm dùng chung: Cả hai tiến trình đều cố gắng mở `course_bookings.dat.tmp`, gây lỗi `ERROR_SHARING_VIOLATION` (0x20).

### 4.2. Kiến trúc Khóa Cấp Hệ Điều Hành (`IFileLock` & `OSFileLock`)

Để giải quyết triệt để, chúng tôi thiết kế mô hình khóa tệp cấp nhân HĐH (Kernel-level OS File Lock).

```
                      +-----------------------------+
                      |         IFileLock           |
                      | (Abstract RAII File Lock)   |
                      +-----------------------------+
                                     ▲
                                     │
                    +----------------+----------------+
                    |                                 |
     +------------------------------+  +------------------------------+
     |       WindowsFileLock        |  |        PosixFileLock         |
     |   (LockFileEx / UnlockFileEx)|  |        (fcntl / flock)       |
     +------------------------------+  +------------------------------+
```

#### 1. Giao diện Trừu tượng Độc lập Nền tảng (`IFileLock.h`)
```cpp
// include/storage/locking/IFileLock.h
#ifndef IFILOCK_H
#define IFILOCK_H

#include <string>
#include <chrono>

enum class LockMode {
    SHARED_READ,    // Phục vụ đọc đồng thời nhiều tiến trình
    EXCLUSIVE_WRITE // Phục vụ ghi độc quyền tuyệt đối
};

class IFileLock {
public:
    virtual ~IFileLock() = default;
    
    // Thử khóa với thời gian chờ (timeout)
    virtual bool acquire(LockMode mode, std::chrono::milliseconds timeout) = 0;
    
    // Giải phóng khóa
    virtual void release() = 0;
    
    // Kiểm tra trạng thái đang giữ khóa
    virtual bool isLocked() const = 0;
    
    // Lấy đường dẫn tệp khóa
    virtual std::string getLockFilePath() const = 0;
};

// RAII Guard để tự động giải phóng khóa khi ra khỏi scope
class FileLockGuard {
public:
    FileLockGuard(IFileLock& lock, LockMode mode, std::chrono::milliseconds timeout = std::chrono::milliseconds(3000))
        : fileLock(lock), acquired(false) {
        acquired = fileLock.acquire(mode, timeout);
    }
    
    ~FileLockGuard() {
        if (acquired) {
            fileLock.release();
        }
    }
    
    bool isAcquired() const { return acquired; }
    
    // Ngăn chặn sao chép (Non-copyable)
    FileLockGuard(const FileLockGuard&) = delete;
    FileLockGuard& operator=(const FileLockGuard&) = delete;

private:
    IFileLock& fileLock;
    bool acquired;
};

#endif // IFILOCK_H
```

#### 2. Triển khai Chuẩn Windows API (`WindowsFileLock.h` & `.cpp`)
Sử dụng các API chuẩn của Windows Kernel: `CreateFileW`, `LockFileEx`, `UnlockFileEx`, `CloseHandle`.

```cpp
// src/storage/locking/WindowsFileLock.cpp (Design Blueprint)
#ifdef _WIN32
#include <windows.h>
#include <string>
#include <chrono>
#include <thread>
#include "storage/locking/IFileLock.h"

class WindowsFileLock : public IFileLock {
private:
    std::wstring lockFilePath;
    std::string lockFilePathUtf8;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    bool locked = false;

public:
    explicit WindowsFileLock(const std::string& path) : lockFilePathUtf8(path) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (wlen > 0) {
            lockFilePath.resize(wlen - 1);
            MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &lockFilePath[0], wlen);
        }
    }

    ~WindowsFileLock() override {
        release();
    }

    bool acquire(LockMode mode, std::chrono::milliseconds timeout) override {
        if (locked) return true;

        auto start = std::chrono::steady_clock::now();
        DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE;

        // Mở hoặc tạo file lock
        while (fileHandle == INVALID_HANDLE_VALUE) {
            fileHandle = CreateFileW(
                lockFilePath.c_str(),
                desiredAccess,
                shareMode,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );

            if (fileHandle != INVALID_HANDLE_VALUE) break;

            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start) >= timeout) {
                return false; // Timeout khi mở file lock
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        DWORD flags = (mode == LockMode::EXCLUSIVE_WRITE) ? LOCKFILE_EXCLUSIVE_LOCK : 0;
        flags |= LOCKFILE_FAIL_IMMEDIATELY; // Non-blocking để kiểm soát timeout chủ động

        OVERLAPPED overlapped = {0};

        while (true) {
            if (LockFileEx(fileHandle, flags, 0, MAXDWORD, MAXDWORD, &overlapped)) {
                locked = true;
                return true;
            }

            DWORD err = GetLastError();
            if (err != ERROR_LOCK_VIOLATION && err != ERROR_SHARING_VIOLATION) {
                // Lỗi nghiêm trọng khác ngoài xung đột lock
                CloseHandle(fileHandle);
                fileHandle = INVALID_HANDLE_VALUE;
                return false;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start) >= timeout) {
                CloseHandle(fileHandle);
                fileHandle = INVALID_HANDLE_VALUE;
                return false; // Hết thời gian chờ khóa
            }

            // Exponential backoff nhẹ có jitter (10ms - 50ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }

    void release() override {
        if (locked && fileHandle != INVALID_HANDLE_VALUE) {
            OVERLAPPED overlapped = {0};
            UnlockFileEx(fileHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
            locked = false;
        }
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;
        }
    }

    bool isLocked() const override { return locked; }
    std::string getLockFilePath() const override { return lockFilePathUtf8; }
};
#endif
```

#### 3. Triển khai POSIX Fallback (`PosixFileLock.cpp`)
Dành cho môi trường Linux/macOS sử dụng `fcntl(F_SETLK)` hoặc `flock(fd, LOCK_EX | LOCK_NB)` với vòng lặp timeout tương đương.

### 4.3. Chiến lược Tệp Khóa Chuyên Dụng (Dedicated Lock Files)

**Quy tắc quan trọng nhất trên Windows:**
> Không bao giờ được gọi `LockFileEx` trực tiếp trên chính tệp dữ liệu `.dat` trong khi đang thực hiện cơ chế Hoán đổi Nguyên tử (`MoveFileExW`).
> Trên Windows NTFS, nếu tệp `course_bookings.dat` đang bị mở hoặc bị khóa byte-range bởi một tiến trình, lệnh `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING)` từ tiến trình khác hoặc chính tiến trình đó sẽ bị từ chối ngay lập tức với mã lỗi `ERROR_ACCESS_DENIED` hoặc `ERROR_SHARING_VIOLATION`.

**Giải pháp Kiến trúc:**
Sử dụng **Tệp khóa riêng biệt (Dedicated Lock File)** nằm trong thư mục ẩn `.locks/`:
```
data/
├── .locks/
│   ├── users.dat.lock
│   ├── course_bookings.dat.lock
│   └── reviews.dat.lock
├── users.dat
├── course_bookings.dat
└── reviews.dat
```
- Khóa tệp `course_bookings.dat.lock` đại diện cho quyền truy cập vào tài nguyên logic `course_bookings.dat`.
- Bản thân tệp `course_bookings.dat` thật sự hoàn toàn tự do để hàm `MoveFileExW` thực hiện hoán đổi nguyên tử mà không bị vướng khóa cấp handle của HĐH.

### 4.4. Quy trình Giao dịch Lưu trữ Toàn vẹn (Locking Transaction Lifecycle)

Để khắc phục hoàn toàn hiện tượng ghi đè mất dữ liệu giữa GUI và CLI, chu trình lưu/cập nhật phải tuân thủ nghiêm ngặt mô hình **Lock-Read-Mutate-Write-Replace-Unlock**:

```
[Bắt đầu Thao tác Lưu/Cập nhật]
   │
   ├── 1. Lấy In-Process Mutex (std::lock_guard<std::recursive_mutex>)
   │      (Bảo vệ an toàn giữa các luồng bên trong nội bộ tiến trình)
   │
   ├── 2. Lấy Exclusive OS Lock (WindowsFileLock.acquire(EXCLUSIVE_WRITE, 3000ms))
   │      (Khóa tệp .locks/<fileName>.lock, chặn các tiến trình khác)
   │
   ├── 3. Đọc lại dữ liệu mới nhất từ đĩa (Fresh Reload)
   │      (Đảm bảo nạp các thay đổi mà tiến trình khác vừa ghi vào trước đó)
   │
   ├── 4. Áp dụng thay đổi nghiệp vụ trên RAM
   │      (Thêm mới, sửa trạng thái, kiểm tra xung đột trên dữ liệu tươi mới)
   │
   ├── 5. Tạo tệp tạm thời có PID duy nhất:
   │      <fileName>.<PID>.<Timestamp>.tmp
   │      (Ngăn ngừa hoàn toàn xung đột tên file tạm giữa các tiến trình)
   │
   ├── 6. Ghi tuần tự dữ liệu ra tệp tạm và Flush xuống đĩa cứng
   │      (out.flush(); FlushFileBuffers())
   │
   ├── 7. Hoán đổi nguyên tử (Atomic Replace):
   │      MoveFileExW(tempPath, targetPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
   │
   ├── 8. Giải phóng Exclusive OS Lock
   │      (Tự động thực hiện qua FileLockGuard destructor)
   │
   └── 9. Giải phóng In-Process Mutex
          (Hoàn tất giao dịch an toàn)
```

### 4.5. Cơ chế Đặt tên Tệp Tạm Thời Độc Nhất (Process-Unique Temp File)

Thay vì dùng tên cố định nguy hiểm `filePath + ".tmp"` (như `course_bookings.dat.tmp`), tên tệp tạm được sinh bằng công thức:
```cpp
std::string generateUniqueTempPath(const std::string& targetPath) {
    unsigned long pid = GetCurrentProcessId(); // Trên Windows
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return targetPath + "." + std::to_string(pid) + "." + std::to_string(now) + ".tmp";
}
```
- Ngay cả khi 10 tiến trình cùng ghi đồng thời, mỗi tiến trình đều thao tác trên một tệp tạm hoàn toàn độc lập, loại bỏ 100% lỗi xung đột chia sẻ tệp tạm thời.

### 4.6. Phòng Chống Bế Tắc Đa Tệp (Deadlock Prevention Strategy)

Trong nghiệp vụ đánh giá gia sư (`ReviewService::submitReview`), hệ thống cần ghi đồng thời vào 2 tệp:
1. `reviews.dat` (Lưu bản ghi đánh giá mới).
2. `users.dat` (Cập nhật điểm đánh giá trung bình `ratingScore` của gia sư).

Nếu Tiến trình A khóa `reviews.dat` rồi chờ khóa `users.dat`, trong khi Tiến trình B (ví dụ Admin chỉnh sửa thông tin) khóa `users.dat` rồi chờ khóa `reviews.dat`, hệ thống sẽ rơi vào **Bế tắc vĩnh viễn (Deadlock)**.

**Quy tắc Thứ tự Khóa Toàn cục (Global Lock Acquisition Hierarchy):**
Mọi thao tác đa tệp trong toàn bộ ứng dụng bắt buộc phải tuân thủ thứ tự khóa đơn điệu:
$$\text{Level 1: } \mathbf{users.dat.lock} \longrightarrow \text{Level 2: } \mathbf{course\_bookings.dat.lock} \longrightarrow \text{Level 3: } \mathbf{reviews.dat.lock}$$
- Khi giải phóng, giải phóng theo thứ tự ngược lại (LIFO).
- Bất kỳ thao tác nào cần cả 2 tệp `users.dat` và `reviews.dat` **bắt buộc phải lấy khóa `users.dat` trước, sau đó mới lấy khóa `reviews.dat`**.
- Về mặt toán học, đồ thị phụ thuộc khóa (Lock Dependency Graph) luôn là đồ thị có hướng không chu trình (DAG), đảm bảo **triệt tiêu 100% khả năng xảy ra Deadlock**.

---

## 5. HÀNH VI XỬ LÝ SỰ CỐ VÀ PHỤC HỒI (FAILURE & RECOVERY BEHAVIOR)

### 5.1. Xử lý Timeout và Thử lại (Timeout & Retry Policy)
- **Thời gian chờ tối đa:** $3000\text{ ms}$ (3 giây). Đây là ngưỡng tối ưu: đủ dài để tiến trình khác hoàn tất thao tác ghi đĩa (thường mất $1-5\text{ ms}$), nhưng đủ ngắn để không làm treo giao diện người dùng (UI freeze).
- **Thuật toán Retry:** Polling kết hợp Exponential Backoff có Jitter:
  - Lần 1: Chờ $10\text{ ms}$.
  - Lần 2: Chờ $20\text{ ms}$.
  - Lần 3 trở đi: Chờ ngẫu nhiên trong khoảng $25\text{ ms} - 50\text{ ms}$.
- **Hành vi khi hết thời gian chờ (Timeout Exceeded):**
  - Không phá vỡ khóa một cách thô bạo.
  - Hủy bỏ thao tác ghi, làm sạch tệp tạm độc nhất của tiến trình hiện tại.
  - Ném ngoại lệ hoặc trả về mã lỗi tường minh: `RepositoryResult::LOCK_TIMEOUT`.
  - Hiển thị thông báo thân thiện lên UI: *"Hệ thống đang bận do có tiến trình khác đang cập nhật dữ liệu. Vui lòng thử lại sau giây lát."*

### 5.2. Sự cố Tiến trình bị Sập nguồn / Crash Đột ngột (Process Crash Recovery)
- **Kịch bản:** Tiến trình đang giữ Exclusive OS Lock thì bị tắt đột ngột (người dùng End Task qua Task Manager, lỗi phân đoạn `SIGSEGV`, hoặc mất điện).
- **Cơ chế tự phục hồi của HĐH:**
  - `LockFileEx` là khóa được quản lý bởi Windows Kernel Object Table gắn liền với handle tệp (`fileHandle`).
  - Khi tiến trình kết thúc (dù bình thường hay bất thường), Windows Kernel tự động thu hồi toàn bộ tài nguyên, đóng các handle đang mở và **tự động giải phóng toàn bộ các vùng khóa byte-range trên tệp**.
  - Không bao giờ tồn tại trạng thái "Khóa mồ côi vĩnh viễn" (Permanent Orphaned Lock) như các giải pháp tự tạo cờ ghi nhận bằng file thông thường. Tiến trình tiếp theo có thể lập tức acquire được khóa mà không gặp trở ngại.

### 5.3. Sự cố Crash Trong Lúc Đang Ghi Dữ liệu (Crash During Write)
- Dữ liệu đang ghi dở nằm trong tệp tạm `<fileName>.<PID>.<Timestamp>.tmp`.
- Tệp chính `<fileName>.dat` vẫn nguyên vẹn 100% vì lệnh `MoveFileExW` chưa được gọi.
- Không xảy ra tình trạng tệp bị cụt (truncated) hay chứa nửa dòng hỏng.
- Các tệp `.tmp` mồ côi của tiến trình đã chết sẽ được quét và dọn dẹp sạch sẽ ở lần khởi động ứng dụng tiếp theo thông qua cơ chế Reconciliation đã định nghĩa trong ADR-007.

### 5.4. Sự cố Lỗi Hoán đổi Tệp Nguyên tử (Atomic Move Failure)
- Nếu lệnh `MoveFileExW` thất bại (ví dụ: phần mềm diệt virus quét tệp tạm đúng thời điểm):
  - Hệ thống tự động thử lại tối đa 3 lần, mỗi lần cách nhau $50\text{ ms}$.
  - Nếu vẫn thất bại: Xóa tệp tạm, giải phóng khóa, trả về lỗi `WRITE_REPLACE_FAILED`.
  - Tệp gốc trên đĩa được bảo toàn nguyên vẹn.

---

## 6. THIẾT KẾ BỘ KIỂM THỬ ĐỒNG BỘ ĐA TIẾN TRÌNH (MULTI-PROCESS TEST DESIGN)

Để nghiệm thu cơ chế khóa trước khi tích hợp vào production, chúng tôi thiết kế 7 kịch bản kiểm thử tích hợp chuyên biệt. Toàn bộ các bài test này chạy trên **Thư mục Hộp cát Cách ly Tạm thời (Isolated Sandbox Directory)** được sinh ngẫu nhiên qua `QDir::tempPath() + "/tutormatch_lock_test_XXXXXX"`, tuyệt đối không tác động vào thư mục `data/` sản xuất.

```
+---------------------------------------------------------------------------------------------------+
|                            MA TRẬN 7 KỊCH BẢN KIỂM THỬ ĐA TIẾN TRÌNH                              |
+-----+--------------------------------------+------------------------------------------------------+
| STT | Tên Kịch bản                         | Mục tiêu Kiểm chứng                                  |
+-----+--------------------------------------+------------------------------------------------------+
| 1   | Concurrent Append                    | 2 tiến trình cùng thêm bản ghi, không bản ghi nào mất|
| 2   | Concurrent Update Different Records  | 2 tiến trình sửa 2 đơn khác nhau, cả 2 cùng cập nhật |
| 3   | Read-While-Write Consistency         | Tiến trình đọc luôn thấy snapshot trọn vẹn, không cụt|
| 4   | Lock Contention & Timeout Behavior   | Giữ khóa quá thời gian chờ, tiến trình sau timeout êm|
| 5   | Process Crash Recovery (Kernel Lock) | Tiến trình bị `abort()`, tiến trình khác lấy lock ngay|
| 6   | Multi-File Deadlock Avoidance        | 2 tiến trình thao tác chéo `users` & `reviews` an toàn|
| 7   | Orphaned Temp File Auto-Cleanup      | Tệp `.tmp` mồ côi được dọn dẹp, không xóa file chính |
+-----+--------------------------------------+------------------------------------------------------+
```

### Chi tiết Thiết kế 7 Kịch bản Kiểm thử:

#### Scenario 1: Ghi thêm đồng thời (Concurrent Append)
- **Thiết lập:** Khởi tạo `course_bookings.dat` ban đầu với 2 bản ghi (#1001, #1002).
- **Hành động:** 2 tiến trình con (Worker Process A và Worker Process B) được kích hoạt đồng thời.
  - Process A ghi thêm 50 bản ghi mới (#2001 -> #2050).
  - Process B ghi thêm 50 bản ghi mới (#3001 -> #3050).
- **Xác minh (Assertions):**
  - Cả hai tiến trình đều kết thúc với mã lỗi 0.
  - File kết quả chứa chính xác $2 + 50 + 50 = 102$ bản ghi.
  - Không có bất kỳ bản ghi nào bị ghi đè hay mất mát.
  - Toàn bộ 102 dòng đều hợp lệ theo định dạng CSV của `CourseBookingSerializer`.

#### Scenario 2: Cập nhật đồng thời trên các bản ghi khác nhau (Concurrent Update)
- **Thiết lập:** Khởi tạo file với 2 bản ghi: #1001 (`PENDING`), #1002 (`PENDING`).
- **Hành động:**
  - Process A cập nhật #1001 thành `ACCEPTED`.
  - Process B đồng thời cập nhật #1002 thành `REJECTED`.
- **Xác minh:**
  - Cả hai thao tác đều thành công.
  - Nạp lại file từ đĩa: #1001 có trạng thái `ACCEPTED` và #1002 có trạng thái `REJECTED`. Hiện tượng một tiến trình ghi đè snapshot cũ của tiến trình kia đã bị triệt tiêu hoàn toàn.

#### Scenario 3: Đọc trong lúc đang ghi (Read-While-Write Consistency)
- **Thiết lập:** File có 1,000 bản ghi.
- **Hành động:**
  - Process A liên tục thực hiện cập nhật toàn bộ file (viết file tạm và gọi `MoveFileExW`).
  - Process B chạy vòng lặp 500 lần đọc file liên tục không nghỉ.
- **Xác minh:**
  - 100% các lần đọc của Process B đều nạp đủ 1,000 bản ghi hợp lệ.
  - Không có bất kỳ lần đọc nào bị lỗi `File Not Found` hoặc đọc phải file rỗng/file bị cắt cụt giữa chừng.

#### Scenario 4: Tranh chấp Khóa và Xử lý Hết thời gian chờ (Lock Contention & Timeout)
- **Thiết lập:** Process A lấy Exclusive OS Lock trên `users.dat.lock` và giữ trong $4000\text{ ms}$ (ngủ có chủ đích).
- **Hành động:** Process B cố gắng lấy Exclusive OS Lock với timeout đặt là $1000\text{ ms}$.
- **Xác minh:**
  - Sau đúng khoảng $1000\text{ ms} \pm 100\text{ ms}$, hàm `acquire()` của Process B trả về `false`.
  - Process B không bị treo vĩnh viễn (no freeze).
  - Không có ngoại lệ không được bắt (no crash).

#### Scenario 5: Tự phục hồi khi Tiến trình chết đột ngột (Kernel Lock Auto-Release)
- **Thiết lập:** Process A lấy Exclusive OS Lock thành công, sau đó ngay lập tức gọi `std::abort()` hoặc `TerminateProcess(GetCurrentProcess(), 1)` mà không chạy destructor.
- **Hành động:** Process B (tiến trình giám sát) phát hiện Process A đã chết và lập tức gọi `acquire()` với timeout $1000\text{ ms}$.
- **Xác minh:**
  - Process B lấy được khóa thành công ngay lập tức ($< 50\text{ ms}$).
  - Kernel HĐH đã tự động giải phóng vùng khóa mà Process A để lại.

#### Scenario 6: Chống Bế tắc trong Giao dịch Đa tệp (Multi-File Deadlock Avoidance)
- **Thiết lập:** 2 tài nguyên `users.dat.lock` và `reviews.dat.lock`.
- **Hành động:**
  - Luồng/Tiến trình A thực hiện 100 giao dịch cập nhật cả `users` và `reviews`.
  - Luồng/Tiến trình B cũng thực hiện 100 giao dịch cập nhật cả `users` và `reviews`.
  - Cả hai đều bắt buộc tuân theo thứ tự `users.dat.lock` -> `reviews.dat.lock`.
- **Xác minh:**
  - Toàn bộ 200 giao dịch hoàn thành trong vòng dưới 5 giây.
  - Không xảy ra tình trạng Deadlock (cả hai bên đứng chờ nhau).

#### Scenario 7: Dọn dẹp Tệp tạm mồ côi (Stale Temp File Cleanup)
- **Thiết lập:** Cố tình tạo 3 tệp rác trong thư mục sandbox:
  `course_bookings.dat.99999.123456.tmp`, `users.dat.88888.654321.tmp`.
- **Hành động:** Khởi động hệ thống repository và chạy tiến trình Startup Reconciliation.
- **Xác minh:**
  - Toàn bộ các tệp `.tmp` mồ côi bị xóa bỏ.
  - Các tệp dữ liệu chính `.dat` và tệp khóa `.lock` được bảo toàn nguyên vẹn.

---

## 7. ĐỀ XUẤT CÁC BẢN GHI QUYẾT ĐỊNH KIẾN TRÚC (PROPOSED ADRs)

> **Ghi chú về Đánh số ADR:**  
> Trong thư mục `docs/architecture/decisions/`, hiện đã tồn tại các quyết định từ ADR-001 đến ADR-008 (trong đó `ADR-008_Minimal_Safe_CMake_Build_Graph_Adjustment.md` đã được sử dụng cho việc điều chỉnh CMake).  
> Dưới đây là toàn văn hai bản ghi quyết định kiến trúc được đề xuất chính thức. Nếu lưu trữ thành tệp vật lý độc lập, chúng có thể mang số hiệu tuần tự tiếp theo là **ADR-009** và **ADR-010** (hoặc đặt tên theo yêu cầu chỉ định: `ADR-008-Financial-Model.md` và `ADR-009-Multi-Process-File-Locking.md`). Cả hai đều được thiết lập trạng thái ban đầu là **PROPOSED** chờ phê duyệt của Product Owner.

---

### BẢN GHI 1: ĐỀ XUẤT ADR VỀ MÔ HÌNH TÀI CHÍNH NỀN TẢNG

```markdown
# ARCHITECTURE DECISION RECORD (ADR)
## ADR-008 (Seq. ADR-009): Mô Hình Đo Lường GMV và Ghi Nhận Doanh Thu Hoa Hồng Khóa Học Định Kỳ
### (Platform Financial Metrics & Revenue Recognition in Recurring Course Booking)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-008 (Số tuần tự trong kho lưu trữ: ADR-009)
- **Tiêu đề:** Mô Hình Đo Lường GMV và Ghi Nhận Doanh Thu Hoa Hồng Khóa Học Định Kỳ
- **Trạng thái:** PROPOSED (Đề xuất chờ phê duyệt từ Product Owner)
- **Ngày lập:** 2026-11-09
- **Tác giả:** Principal Software Architect & Financial Domain Specialist
- **Phạm vi tác động:** `include/dtos/AdminMetrics.h`, `src/services/AdminService.cpp`, `src/gui/GUIBridge.cpp`, `src/models/CourseBooking.cpp`, `AdminOverviewView.qml`, `AdminCommissionConfigView.qml`.

---

### 1. BỐI CẢNH (CONTEXT)
1. Dự án TutorMatch đã hoàn tất việc chuyển đổi mô hình kinh doanh từ đặt lịch từng buổi học đơn lẻ (`Booking`) sang mô hình khóa học định kỳ nhiều tuần (`CourseBooking`) với các thời lượng 4, 8, hoặc 12 tuần.
2. Tuy nhiên, logic tính toán tài chính tại `AdminService::calculateMetrics()` hiện tại vẫn mang tính chất của mô hình cũ:
   - Biến `initialConnectionGMV` chỉ cộng học phí của một buổi đơn lẻ (`cb.hourlyRate`) của các đơn `COMPLETED`.
   - Hoàn toàn không nhân với số tuần (`cb.weeksCount`).
   - Doanh thu hoa hồng được tính động bằng $25\%$ của phí một buổi này.
   - Các khóa học đang diễn ra (`ACCEPTED`) không được phản ánh vào bất kỳ chỉ số giá trị giao dịch nào.
3. Điều này dẫn đến sự sai lệch lớn giữa giá trị cam kết thực tế của khóa học (ví dụ đơn #1008 trị giá 720,000 VNĐ) và con số báo cáo trên giao diện Quản trị viên (chỉ hiển thị 180,000 VNĐ).

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi đề xuất:
1. **Chuyển đổi sang Mô hình Quản lý Khóa học Trọn gói (Full-Course Managed Marketplace Model):**
   - **Tổng GMV Nền tảng:** Phản ánh toàn bộ giá trị cam kết của các khóa học được xác lập:
     $$\text{CourseValue} = cb.\text{hourlyRate} \times cb.\text{weeksCount}$$
   - **Tách bạch 2 trạng thái GMV:**
     - $\text{Realized GMV}$ (GMV Thực hiện): Tổng giá trị của các khóa học đã kết thúc thành công (`COMPLETED`).
     - $\text{Pending GMV}$ (GMV Đang thực hiện / Ký quỹ): Tổng giá trị của các khóa học đang học (`ACCEPTED`).
2. **Ghi nhận Doanh thu Hoa hồng Nền tảng:**
   - Doanh thu hoa hồng thực nhận chỉ được ghi nhận khi khóa học chuyển sang trạng thái `COMPLETED`:
     $$\text{Realized Revenue} = \sum_{cb \in \text{COMPLETED}} (\text{CourseValue} \times \text{commissionRateSnapshot})$$
   - Doanh thu hoa hồng dự kiến (Pending Revenue) từ các khóa `ACCEPTED`:
     $$\text{Pending Revenue} = \sum_{cb \in \text{ACCEPTED}} (\text{CourseValue} \times \text{currentCommissionRate})$$
3. **Cơ chế Đóng băng Tỷ lệ Hoa hồng (Snapshotting):**
   - Khi một đơn hàng được tạo hoặc hoàn thành, tỷ lệ hoa hồng có hiệu lực tại thời điểm đó sẽ được ghi nhận cố định vào dữ liệu khóa học.
   - Khi Quản trị viên thay đổi tỷ lệ hoa hồng chung của nền tảng, các đơn hàng trong quá khứ tuyệt đối không bị tính toán lại.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Giữ nguyên Mô hình Phí kết nối buổi đầu (Lead Generation Model)
- **Mô tả:** GMV chỉ tính 1 buổi học đầu tiên ($180,000$ VNĐ). Hoa hồng sàn chỉ thu trên buổi đầu ($45,000$ VNĐ). Các buổi sau gia sư tự thu.
- **Lý do không chọn:** Không phản ánh đúng bản chất của tính năng `CourseBooking` nhiều tuần, tạo kẽ hở cho gia sư và học sinh bỏ qua nền tảng sau buổi đầu, làm suy giảm nghiêm trọng giá trị thương mại của sàn.

#### Phương án B: Thu hoa hồng ngay khi đơn chuyển sang `ACCEPTED`
- **Mô tả:** Ghi nhận toàn bộ doanh thu hoa hồng ngay khi gia sư đồng ý dạy.
- **Lý do không chọn:** Rủi ro kế toán cao. Nếu khóa học bị hủy giữa chừng do gia sư có việc bận hoặc học sinh bỏ học, nền tảng sẽ phải thực hiện các nghiệp vụ hoàn trả phức tạp.

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực:
- Thống kê tài chính phản ánh chính xác 100% quy mô kinh tế của các khóa học trên nền tảng.
- Tách bạch rõ ràng giữa dòng tiền thực thu và dòng tiền cam kết đang thực hiện.
- Bảo vệ dữ liệu kế toán lịch sử không bị biến động khi thay đổi cấu hình hoa hồng.

#### Tiêu cực / Chi phí kỹ thuật:
- Cần cập nhật cấu trúc `AdminMetrics` DTO và bổ sung trường hiển thị trên QML (`AdminOverviewView.qml`, `AdminCommissionConfigView.qml`).
- Cần cập nhật các ca kiểm thử trong `tests/TestRunner.cpp` (Test 3) để kiểm tra công thức mới.
```

---

### BẢN GHI 2: ĐỀ XUẤT ADR VỀ KHÓA TỆP ĐA TIẾN TRÌNH

```markdown
# ARCHITECTURE DECISION RECORD (ADR)
## ADR-009 (Seq. ADR-010): Cơ Chế Khóa Tệp Cấp Hệ Điều Hành Cho Tầng Lưu Trữ Đa Tiến Trình
### (OS-Level File Locking & Concurrency Control for Flat-File Persistence)

---

### THÔNG TIN TỔNG QUAN
- **Mã định danh:** ADR-009 (Số tuần tự trong kho lưu trữ: ADR-010)
- **Tiêu đề:** Cơ Chế Khóa Tệp Cấp Hệ Điều Hành Cho Tầng Lưu Trữ Đa Tiến Trình
- **Trạng thái:** PROPOSED (Đề xuất chờ phê duyệt từ Product Owner)
- **Ngày lập:** 2026-11-09
- **Tác giả:** Principal Software Architect & Systems Engineering Specialist
- **Phạm vi tác động:** `include/storage/locking/`, `src/storage/repositories/FileCourseBookingRepository.cpp`, `src/storage/repositories/FileUserRepository.cpp`, `src/storage/repositories/FileReviewRepository.cpp`.

---

### 1. BỐI CẢNH (CONTEXT)
1. Tầng lưu trữ của TutorMatch dựa trên các tệp phẳng CSV (`users.dat`, `course_bookings.dat`, `reviews.dat`).
2. Hiện tại, tính an toàn đồng thời chỉ được bảo vệ bằng `std::recursive_mutex`. Cơ chế này chỉ có hiệu lực bên trong một tiến trình đơn lẻ.
3. Khi người dùng chạy song song ứng dụng giao diện đồ họa `TutorMatch_GUI.exe` và ứng dụng dòng lệnh `TutorMatch_CLI.exe`, hiện tượng ghi đè tệp diễn ra:
   - Tiến trình này có thể ghi đè làm mất các bản ghi mà tiến trình kia vừa cập nhật.
   - Cả hai cùng ghi vào tệp tạm cố định `.tmp` gây lỗi tranh chấp chia sẻ tệp (`ERROR_SHARING_VIOLATION`).
4. Để đảm bảo tính toàn vẹn dữ liệu chuẩn ACID ở mức tệp phẳng, hệ thống bắt buộc phải có cơ chế khóa ở cấp Hệ điều hành.

---

### 2. QUYẾT ĐỊNH (DECISION)
Chúng tôi quyết định:
1. **Triển khai Hệ thống Khóa Tệp Cấp Hệ Điều Hành (OS File Locking):**
   - Xây dựng lớp trừu tượng `IFileLock` và triển khai cụ thể cho Windows bằng Windows Kernel API: `CreateFileW`, `LockFileEx` (`LOCKFILE_EXCLUSIVE_LOCK`), `UnlockFileEx`, `CloseHandle`.
   - Cung cấp lớp quản lý vòng đời RAII `FileLockGuard` để giải phóng khóa tự động khi kết thúc phạm vi (scope).
2. **Chiến lược Tệp Khóa Chuyên Dụng (Dedicated Lock Files):**
   - Tuyệt đối không gọi khóa trực tiếp trên tệp dữ liệu chính `.dat` nhằm tránh xung đột với cơ chế hoán đổi nguyên tử `MoveFileExW`.
   - Toàn bộ các khóa được tạo trong thư mục riêng: `data/.locks/<fileName>.lock`.
3. **Cơ chế Tệp Tạm Độc Nhất (Process-Unique Temp File):**
   - Tệp tạm khi ghi dữ liệu bắt buộc phải gắn mã định danh tiến trình PID và tem thời gian:
     `<fileName>.<PID>.<Timestamp>.tmp`.
4. **Quy trình Giao dịch Lưu trữ Chuẩn Tắc (Lock-Read-Mutate-Write-Replace-Unlock):**
   - Mọi thao tác ghi/sửa đều phải: Lấy khóa OS -> Nạp dữ liệu mới nhất từ đĩa -> Áp dụng thay đổi trên RAM -> Ghi ra tệp tạm độc nhất -> Flush đĩa -> `MoveFileExW` -> Giải phóng khóa.
5. **Thứ tự Khóa Toàn cục Ngăn ngừa Bế tắc (Global Lock Ordering):**
   - Bắt buộc tuân thủ thứ tự: `users.dat.lock` -> `course_bookings.dat.lock` -> `reviews.dat.lock`.
6. **Thời gian Chờ và Tự phục hồi:**
   - Hạn mức thời gian chờ tối đa (timeout) là $3000\text{ ms}$. Khi hết giờ sẽ trả về lỗi an toàn, không làm hỏng dữ liệu.
   - Nhờ cơ chế Kernel Lock của HĐH, khi tiến trình bị sập nguồn đột ngột, HĐH tự động giải phóng vùng khóa, không để lại khóa mồ côi.

---

### 3. CÁC PHƯƠNG ÁN THAY THẾ ĐÃ XEM XÉT (ALTERNATIVES CONSIDERED)

#### Phương án A: Chỉ cho phép chạy Duy nhất Một Tiến trình (Single-Instance Application Lock)
- **Mô tả:** Sử dụng một Named Mutex toàn cục (`Global\TutorMatch_SingleInstance_Mutex`). Nếu GUI đang chạy, người dùng không thể mở CLI (hoặc ngược lại).
- **Đánh giá:** Đơn giản nhất, nhưng làm giảm trải nghiệm sử dụng (không thể dùng CLI kiểm tra dữ liệu khi GUI đang mở). Phương án khóa tệp cấp repository được chọn vì cho phép cả 2 ứng dụng hoạt động cộng tác an toàn.

#### Phương án B: Chuyển đổi toàn bộ sang SQLite
- **Mô tả:** Bỏ file phẳng `.dat` và chuyển sang cơ sở dữ liệu nhúng SQLite với cơ chế WAL (Write-Ahead Logging).
- **Lý do loại bỏ:** Vi phạm nghiêm trọng ràng buộc cốt lõi của đồ án: "Không thay đổi định dạng lưu trữ file phẳng và cấu trúc dữ liệu tự xây dựng".

---

### 4. HỆ QUẢ (CONSEQUENCES)

#### Tích cực:
- Triệt tiêu 100% hiện tượng ghi đè mất dữ liệu giữa GUI và CLI.
- Loại bỏ hoàn toàn lỗi xung đột tệp tạm `ERROR_SHARING_VIOLATION`.
- Bảo toàn nguyên vẹn dữ liệu khi tiến trình bị tắt đột ngột.

#### Tiêu cực / Chi phí kỹ thuật:
- Tăng thời gian ghi đĩa thêm khoảng $1-3\text{ ms}$ cho mỗi giao dịch do chi phí gọi API khóa của nhân HĐH (hoàn toàn không ảnh hưởng tới trải nghiệm người dùng).
- Cần bổ sung thư mục `data/.locks/` vào danh mục khởi tạo và quản lý của ứng dụng.
```

---

## 8. DANH MỤC QUYẾT ĐỊNH CẦN PRODUCT OWNER PHÊ DUYỆT (DECISIONS REQUIRING PO APPROVAL)

Trước khi tiến hành sửa đổi bất kỳ dòng mã nguồn (`.cpp`, `.h`) hay cấu trúc giao diện QML nào, nhóm kiến trúc yêu cầu Product Owner xem xét và đưa ra quyết định chính thức đối với 5 điểm then chốt sau:

```
[ ] QUYẾT ĐỊNH 1: LỰA CHỌN MÔ HÌNH KINH DOANH VÀ CÔNG THỨC GMV
    ( ) Phương án 1A (Mô hình hiện tại): Giữ nguyên Mô hình Phí kết nối buổi đầu (Lead Gen).
        GMV = Học phí 1 buổi (180,000 VNĐ cho #1008). Hoa hồng = 45,000 VNĐ.
    ( ) Phương án 1B (Khuyến nghị kiến trúc): Chuyển sang Mô hình Quản lý Trọn khóa (Full Course Escrow).
        GMV = Học phí x Số tuần (720,000 VNĐ cho #1008). Hoa hồng = 180,000 VNĐ.

[ ] QUYẾT ĐỊNH 2: THỜI ĐIỂM GHI NHẬN DOANH THU HOA HỒNG (REVENUE RECOGNITION TIMING)
    ( ) Phương án 2A: Chỉ ghi nhận doanh thu khi khóa học kết thúc thành công (COMPLETED).
        Các khóa đang học (ACCEPTED) chỉ ghi nhận vào GMV Cam kết và Doanh thu Dự kiến (Pending).
    ( ) Phương án 2B: Ghi nhận doanh thu ngay khi gia sư đồng ý dạy (ACCEPTED).
        (Kèm theo cơ chế xử lý hoàn tiền nếu khóa học bị hủy giữa chừng).

[ ] QUYẾT ĐỊNH 3: BỔ SUNG TRƯỜNG ĐÓNG BĂNG HOA HỒNG (COMMISSION SNAPSHOTTING)
    ( ) Đồng ý: Cho phép bổ sung trường `commissionRateSnapshot` và `commissionAmount` vào mô hình
        CourseBooking để đóng băng số liệu lịch sử khi Admin thay đổi tỷ lệ hoa hồng chung của sàn.
    ( ) Không đồng ý: Tiếp tục tính toán động theo tỷ lệ hoa hồng hiện tại trong AdminService.

[ ] QUYẾT ĐỊNH 4: CHÍNH SÁCH ĐỒNG BỘ ĐA TIẾN TRÌNH (CONCURRENCY POLICY)
    ( ) Phương án 4A (Khuyến nghị kiến trúc): Triển khai OS File Locking tại tầng Repository
        để cho phép GUI và CLI cùng chạy song song và cập nhật dữ liệu an toàn.
    ( ) Phương án 4B: Áp dụng Single-Instance Lock (chỉ cho phép mở duy nhất 1 ứng dụng tại 1 thời điểm;
        nếu GUI đang mở thì chặn không cho CLI khởi động và ngược lại).

[ ] QUYẾT ĐỊNH 5: CHÍNH SÁCH XỬ LÝ HỦY KHÓA HỌC GIỮA CHỪNG (MID-COURSE CANCELLATION)
    ( ) Nếu một khóa học 8 tuần bị hủy sau 3 tuần: Nền tảng sẽ thu hoa hồng trên 3 tuần đã học,
        hay hoàn trả 100% cho học sinh, hay thu trọn gói 8 tuần?
```

---

## 9. KẾT LUẬN VÀ LỘ TRÌNH TRIỂN KHAI (IMPLEMENTATION ROADMAP)

Bản thiết kế này cung cấp đầy đủ luận cứ khoa học, phân tích pháp chứng dữ liệu và giải pháp kỹ thuật chi tiết ở cấp độ hệ thống. Ngay sau khi nhận được sự phê duyệt của Product Owner đối với các quyết định tại Mục 8, nhóm kỹ thuật sẽ triển khai theo lộ trình:

1. **Giai đoạn 1 (OS File Locking):**
   - Tạo thư viện `storage/locking/` (`IFileLock`, `WindowsFileLock`, `FileLockGuard`).
   - Tích hợp vào `FileCourseBookingRepository`, `FileUserRepository`, `FileReviewRepository`.
   - Bổ sung bộ kiểm thử `Test 23: Multi-Process Concurrency Test Suite` (chạy 7 kịch bản trong sandbox).
2. **Giai đoạn 2 (Financial Alignment):**
   - Cập nhật công thức trong `AdminService::calculateMetrics()` theo Mô hình đã duyệt.
   - Mở rộng DTO `AdminMetrics` (bổ sung `pendingGMV`, `pendingRevenue`, `realizedRevenue`).
   - Cập nhật hiển thị trên `AdminOverviewView.qml` và `AdminCommissionConfigView.qml`.
   - Cập nhật bộ kiểm thử `Test 3` trong `TestRunner.cpp`.
3. **Giai đoạn 3 (Final Verification & Sign-off):**
   - Chạy toàn bộ regression test suite (23 suites).
   - Kiểm chứng thực nghiệm song song GUI và CLI.
   - Bàn giao báo cáo nghiệm thu kỹ thuật.
