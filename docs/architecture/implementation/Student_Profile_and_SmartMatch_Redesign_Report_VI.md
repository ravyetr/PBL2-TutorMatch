# BÁO CÁO KIẾN TRÚC & TRIỂN KHAI: TINH GỌN HỒ SƠ HỌC VIÊN & CƠ CHẾ XẾP HẠNG GIA SƯ THEO TIÊU CHÍ TÌM KIẾM SMARTMATCH

> **Dự án:** TutorMatch — Nền tảng kết nối & ghép gia sư - học viên theo mô hình Three-Layer Architecture  
> **Thời điểm hoàn thành:** 11/11/2026  
> **Trạng thái xác thực:** Đã kiểm thử tự động toàn diện (26/26 Test Suites PASS — 100% thành công) & Đã biên dịch hoàn tất `TutorMatch_GUI.exe`

---

## 1. TỔNG QUAN & NGUYÊN NHÂN CỐT LÕI (PROBLEM DIAGNOSIS)

### 1.1. Hiện trạng trước khi cải tổ
Trong thiết kế ban đầu của TutorMatch, thông tin `preferredSubject` (môn học mục tiêu) và `maxBudget` (ngân sách học phí tối đa) được lưu trữ cố định như các thuộc tính vĩnh viễn trong thực thể `Student` và tệp cơ sở dữ liệu `users.dat`. 

Thiết kế này dẫn đến những bất cập nghiệp vụ và kiến trúc nghiêm trọng:
1. **Sai lệch mô hình thực tế (Domain Mismatch):** Một học viên trong thực tế không chỉ học một môn duy nhất hay có một mức ngân sách duy nhất. Cùng một học viên có thể tìm gia sư môn *Toán 12 Luyện thi Đại học* với ngân sách 250.000 VNĐ/buổi, nhưng đồng thời muốn tìm gia sư *Tiếng Anh Giao tiếp* với ngân sách 150.000 VNĐ/buổi, hoặc *Tin học Lập trình C++* vào các khung giờ khác nhau.
2. **Xung đột trạng thái giữa các lần tìm kiếm (State Coupling):** Mỗi lần học viên thực hiện tìm kiếm SmartMatch, việc ghi đè môn học hoặc ngân sách vào hồ sơ học viên làm biến đổi dữ liệu cá nhân ổn định, gây mất dữ liệu của các nhu cầu học tập khác.
3. **Vi phạm nguyên tắc Single Responsibility (SRP):** Thực thể `Student` phải gánh vác cả thông tin định danh cá nhân ổn định lẫn các tham số truy vấn tìm kiếm nhất thời.

### 1.2. Mục tiêu tái thiết kế
- **Hồ sơ học viên (Student Profile):** Chỉ lưu trữ các thông tin cá nhân ổn định gồm: Họ và tên, Email, Khối lớp hiện tại (`gradeLevel`), Tên đăng nhập, Mật khẩu, Trạng thái tài khoản (`isActive`), và Thời khóa biểu cá nhân (`scheduleBitmask`).
- **Thu thập tiêu chí tìm kiếm linh hoạt (SmartMatch Dynamic Criteria):** Mỗi lượt tìm kiếm gia sư SmartMatch thu thập một tập tiêu chí độc lập (`SmartMatchCriteria`), không ghi đè vào hồ sơ học viên. Các lượt tìm kiếm khác nhau hoàn toàn độc lập.
- **Bộ lọc cứng & Thang điểm xếp hạng minh bạch 100 điểm:** Loại bỏ các gia sư không đủ điều kiện và tính điểm giải thích được (Explainable Scoring) dựa trên mức độ phù hợp lịch học, tính cạnh tranh học phí, uy tín đánh giá (Bayesian), độ phủ khối lớp và sự hoàn thiện hồ sơ.
- **Đồng bộ Schema lưu trữ & Migration dữ liệu:** Cập nhật mô hình miền, serializer, repository và di chuyển tệp vật lý `data/users.dat` sang định dạng chuẩn tắc mới một cách an toàn và có sao lưu bảo toàn.

---

## 2. PHÂN TÍCH KIẾN TRÚC & PHÂN TÁCH LỚP (ARCHITECTURAL DECOUPLING)

Tuân thủ kiến trúc Three-Layer Architecture và các nguyên lý SOLID:

```
+-----------------------------------------------------------------------------------+
|                            PRESENTATION LAYER (GUI / QML)                         |
|  - StudentSmartMatchView.qml: Form tiêu chí 3 nhóm + Lưới 7x8 chọn ca + Kết quả    |
|  - StudentProfileCard.qml / ProfileEditModal.qml: Chỉ hiển thị/sửa info ổn định   |
|  - UserDetailModal.qml: Modal chi tiết người dùng chuẩn hóa                       |
+-----------------------------------------------------------------------------------+
                                        | (QVariantMap / Criteria DTO)
                                        v
+-----------------------------------------------------------------------------------+
|                             APPLICATION BRIDGE / ADAPTER                          |
|  - GUIBridge::qmlSearchSmartMatches(criteriaMap, topK): Nhận tiêu chí động        |
|  - GUIBridge::getCurrentStudentProfile(): Không chứa môn/ngân sách lỗi thời       |
|  - GUIBridge::updateStudentProfile(name, email, grade): Cập nhật hồ sơ thuần túy  |
+-----------------------------------------------------------------------------------+
                                        | (SmartMatchCriteria struct)
                                        v
+-----------------------------------------------------------------------------------+
|                                 SERVICES LAYER                                    |
|  - MatchingService::findMatchesWithCriteria(criteria, topK): Điều phối ghép cặp   |
|  - UserService::updateStudentProfile(student, name, email, grade): Nghiệp vụ user|
|  - AuthService::registerStudent(..., grade): Đăng ký tài khoản học viên mới       |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                              DOMAIN & ALGORITHM LAYER                             |
|  - SmartMatchCriteria: DTO tiêu chí tìm kiếm 3 nhóm độc lập                       |
|  - MatchingEngine::isEligible(criteria, tutor, activeBookings): 6 bộ lọc cứng     |
|  - WeightedScoreMatchingStrategy::calculateScoreWithCriteria(...): Thang 100đ     |
|  - Student Domain Model: Bỏ preferredSubject, maxBudget; giữ gradeLevel, mask     |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                           STORAGE & PERSISTENCE LAYER                             |
|  - UserSerializer::toCsv(Student): Xuất 9 trường chuẩn tắc (Canonical Schema)     |
|  - UserSerializer::fromCsv(...): Tương thích 9 trường (mới) và 11 trường (cũ)     |
|  - FileUserRepository: RAM Hash Index O(1), AVL Tree, Atomic Rewrite              |
|  - users.dat: 50 bản ghi dữ liệu sạch đã được migrate                             |
+-----------------------------------------------------------------------------------+
```

---

## 3. CHI TIẾT TINH GỌN HỒ SƠ HỌC VIÊN (STUDENT PROFILE SIMPLIFICATION)

### 3.1. Các trường được lưu giữ và loại bỏ
- **Các trường ổn định được giữ lại trong `Student`:**
  - `id`: Mã định danh duy nhất (int).
  - `username`: Tên tài khoản đăng nhập (string).
  - `passwordHash`: Mật khẩu băm (string).
  - `email`: Địa chỉ email liên hệ (string).
  - `fullName`: Họ và tên học viên (string).
  - `role`: Vai trò người dùng (`UserRole::STUDENT`).
  - `isActive`: Trạng thái hoạt động / khóa mềm (bool).
  - `gradeLevel`: Khối lớp hiện tại (1–12) (int).
  - `scheduleBitmask`: Mặt nạ nhị phân 56 ca học trong tuần (uint64_t).
- **Các trường bị loại bỏ hoàn toàn khỏi mô hình hồ sơ:**
  - `preferredSubject`: Không còn lưu trữ cố định.
  - `maxBudget`: Không còn lưu trữ cố định.

### 3.2. Cập nhật giao diện người dùng
1. **[StudentProfileCard.qml](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentProfileCard.qml):**
   - Loại bỏ các dòng hiển thị `Môn học mục tiêu` và `Ngân sách tối đa`.
   - Bố cục lại thành 4 trường cá nhân rõ ràng: *Họ và tên, Địa chỉ Email, Khối lớp học tập, Tên tài khoản đăng nhập & Trạng thái hoạt động*.
2. **[ProfileEditModal.qml](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/ProfileEditModal.qml):**
   - Biểu mẫu chỉnh sửa hồ sơ chỉ gồm 3 trường: *Họ và tên, Email, Khối lớp* (ComboBox từ Lớp 1 đến Lớp 12).
   - Loại bỏ hoàn toàn ô nhập `Môn học mục tiêu` và `Ngân sách`.
3. **[UserDetailModal.qml](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/UserDetailModal.qml):**
   - Cập nhật phần thông tin dành riêng cho học viên trong giao diện quản trị: hiển thị `Khối lớp học` và ghi chú cơ chế ghép gia sư: *Tìm kiếm SmartMatch độc lập theo từng lượt*.

---

## 4. BỘ TIÊU CHÍ VÀ ĐỘNG CƠ GHÉP SMARTMATCH (CRITERIA-BASED ENGINE)

### 4.1. Cấu trúc tiêu chí tìm kiếm 3 nhóm (`SmartMatchCriteria`)
Mỗi lần học viên tìm kiếm SmartMatch, hệ thống thu thập một cấu trúc `SmartMatchCriteria` độc lập:

#### Nhóm 1: Tiêu chí bắt buộc (Required Criteria)
- `subject` (string): Môn học mục tiêu lựa chọn từ danh mục chuẩn hóa (`Toán`, `Vật lý`, `Hóa học`, `Sinh học`, `Tiếng Anh`, `Ngữ văn`, `Tin học`, `Lịch sử - Địa lý`).
- `gradeLevel` (int): Khối lớp cần học (1–12), mặc định điền khối lớp từ hồ sơ học viên nhưng cho phép thay đổi theo từng lượt tìm kiếm mà không làm thay đổi hồ sơ.
- `maxBudget` (double): Học phí tối đa / buổi (VNĐ), bắt buộc $> 0$.

#### Nhóm 2: Lịch học & Khóa học (Schedule & Course Requirements)
- `preferredScheduleMask` (uint64_t): Mặt nạ 56 ca chọn trực tiếp từ bảng 7×8 ca học (gồm các phím tắt chọn nhanh: *Tối 2-4-6 Ca 7 (18:45)*, *Tối 3-5-7 Ca 7 (18:45)*, *Cuối tuần sáng Ca 2 (09:15)*).
- `startDate` (string): Ngày dự kiến bắt đầu khóa học (định dạng `YYYY-MM-DD`).
- `weeksCount` (int): Thời lượng khóa học đề xuất (4, 8, hoặc 12 tuần).
- `strictSchedule` (bool): 
  - `true` (*Nghiêm ngặt*): Gia sư bắt buộc phải có đủ 100% các ca học đã chọn và không bị trùng lịch khóa học đang hoạt động.
  - `false` (*Linh hoạt*): Gia sư chỉ cần trùng khớp ít nhất 1 ca, ưu tiên ca trùng nhiều nhất và tính điểm tỷ lệ.

#### Nhóm 3: Tùy chọn nâng cao (Optional Preferences)
- `requireVerified` (bool): Chỉ lọc gia sư đã được Quản trị viên duyệt xác minh hồ sơ (`isVerified == true`).
- `minRating` (double): Ngưỡng đánh giá tối thiểu (ví dụ: `4.0`, `4.5`, `4.8`).

---

### 4.2. 6 Bộ lọc điều kiện cứng (Hard Eligibility Filters)
Gia sư bị loại bỏ hoàn toàn khỏi danh sách đề xuất nếu không thỏa mãn một trong 6 điều kiện:
1. **Trạng thái tài khoản:** `tutor.getIsActive() == true`.
2. **Chuyên môn môn học:** `SubjectTaxonomy::matchesFilter(tutor.getSubjectSpecialty(), criteria.subject) == true`.
3. **Phù hợp khối lớp:** `criteria.gradeLevel >= tutorParsed.minGrade && criteria.gradeLevel <= tutorParsed.maxGrade`.
4. **Học phí trong ngân sách:** `tutor.getHourlyRate() <= criteria.maxBudget`.
5. **Khả năng đáp ứng lịch rảnh:**
   - Khi `strictSchedule == true`: Gia sư phải rảnh 100% các ca yêu cầu (`(reqMask & ~tutorMask) == 0`).
   - Khi `strictSchedule == false`: Gia sư phải rảnh ít nhất 1 ca yêu cầu (`(reqMask & tutorMask) != 0`).
6. **Không trùng khóa học đang diễn ra (Active Reservation Conflict Check):**
   - Kiểm tra toàn bộ các khóa học đang ở trạng thái `ACCEPTED` của gia sư trong khoảng thời gian `[startDate, startDate + weeksCount]`.
   - Nếu bất kỳ ca học nào trùng với ca của khóa học đã nhận, gia sư bị loại khỏi danh sách đủ điều kiện.

---

### 4.3. Mô hình chấm điểm minh bạch 100 điểm (100-Point Scoring Model)

Tổng điểm tối đa là **100.0 điểm**, phân bổ thành 5 tiêu chí:

$$\text{CompositeScore} = S_{\text{schedule}} + S_{\text{price}} + S_{\text{rating}} + S_{\text{experience}} + S_{\text{credibility}}$$

#### 1. Mức độ phù hợp lịch học ($S_{\text{schedule}}$: Tối đa 35.0 điểm)
- Nếu học viên không chỉ định ca cụ thể (`reqMask == 0`): Cho 35.0 điểm.
- Ở chế độ `strictSchedule == true`: Đạt 35.0 điểm (do đã qua bộ lọc cứng 100%).
- Ở chế độ `strictSchedule == false`:
  $$S_{\text{schedule}} = 35.0 \times \frac{\text{Số ca trùng}}{\text{Số ca yêu cầu}}$$

#### 2. Tính cạnh tranh học phí ($S_{\text{price}}$: Tối đa 25.0 điểm)
Hàm giảm đơn điệu đối với giá (học phí càng thấp so với ngân sách càng đạt điểm cao hơn):
$$S_{\text{price}} = 10.0 + 15.0 \times \max\left(0.0, 1.0 - \frac{\text{hourlyRate}}{\text{maxBudget}}\right)$$
- Gia sư có học phí bằng đúng ngân sách: Đạt 10.0 điểm.
- Gia sư có học phí thấp hơn ngân sách: Đạt từ 10.0 đến 25.0 điểm. Gia sư học phí 0đ (miễn phí): Đạt 25.0 điểm.

#### 3. Uy tín từ đánh giá học viên ($S_{\text{rating}}$: Tối đa 15.0 điểm)
Sử dụng công thức làm trơn Bayes (Bayesian Prior Smoothing) với giá trị tiên nghiệm 4.0 sao và trọng số $m = 3$:
$$\bar{R} = \frac{3 \times 4.0 + n \times R}{3 + n}$$
$$S_{\text{rating}} = 15.0 \times \left(\frac{\bar{R}}{5.0}\right)$$
Đảm bảo gia sư mới (chưa có đánh giá) nhận điểm hợp lý tương đương 4.0 sao (12.0 điểm), tránh thiên vị quá mức cho gia sư chỉ có 1 đánh giá 5 sao so với gia sư có 50 đánh giá 4.9 sao.

#### 4. Phù hợp kinh nghiệm & độ phủ khối lớp ($S_{\text{experience}}$: Tối đa 15.0 điểm)
- Phù hợp chính xác khối lớp yêu cầu: **10.0 điểm**.
- Chiều sâu chuyên môn nâng cao (tiểu sử hoặc chuyên môn chứa các từ khóa chuyên sâu: *luyện thi, chuyên, đại học, hsg, nâng cao, ielts, thpt*): **+5.0 điểm**.

#### 5. Mức độ hoàn thiện hồ sơ & độ tin cậy ($S_{\text{credibility}}$: Tối đa 10.0 điểm)
- Hồ sơ đã được Quản trị viên phê duyệt xác minh (`isVerified == true`): **6.0 điểm**.
- Có phần giới thiệu tiểu sử chi tiết ($\ge 20$ ký tự): **4.0 điểm**.

---

### 4.4. Quy tắc xếp hạng & Phá vỡ thế hòa (Tie-Breaking Rules)
Sắp xếp danh sách gia sư theo tiêu chuẩn tất định (Deterministic Strict Weak Ordering):
1. Ưu tiên **Tổng điểm (`compositeScore`)** giảm dần.
2. Nếu tổng điểm bằng nhau: Ưu tiên **Điểm lịch học (`scheduleScore`)** giảm dần.
3. Nếu điểm lịch bằng nhau: Ưu tiên **Điểm học phí (`priceScore`)** giảm dần.
4. Nếu vẫn bằng nhau: Ưu tiên **Mã số ID gia sư (`tutorId`)** tăng dần.

---

### 4.5. Giải thích đề xuất bằng tiếng Việt (Explainable Bullets)
Mỗi kết quả đề xuất hiển thị các gạch đầu dòng giải thích trực quan:
- `✓ Khớp toàn bộ 3 ca học mong muốn trong tuần (Chế độ nghiêm ngặt)`
- `✓ Trùng khớp 2/3 ca học mong muốn (Tối 2, Tối 4)`
- `✓ Học phí 150.000 VNĐ/buổi thấp hơn 25% so với ngân sách tối đa`
- `✓ Chuyên môn đúng Khối 12 và có định hướng Luyện thi Đại học / HSG`
- `✓ Đánh giá uy tín 4.9⭐ từ 12 học viên đã hoàn thành khóa học`
- `✓ Đã được quản trị viên phê duyệt xác minh hồ sơ`

---

## 5. COMPLETE users.dat MIGRATION REPORT (BÁO CÁO TOÀN DIỆN DI CHUYỂN DỮ LIỆU users.dat)

### 5.1. Bối cảnh & Mục tiêu di chuyển
Theo yêu cầu bắt buộc, toàn bộ tệp cơ sở dữ liệu `data/users.dat` đang hoạt động trong môi trường production phải được chuyển đổi hoàn toàn sang **Schema chuẩn tắc mới (Canonical Schema)**. Sau khi chuyển đổi, **tuyệt đối không còn bất kỳ bản ghi legacy (11 trường) nào tồn tại trong tệp `users.dat` sản xuất**. Toàn bộ 35 bản ghi học viên phải tuân thủ chuẩn 9 trường; các giá trị `targetSubject` và `maximumTuition` bị loại bỏ triệt để khỏi lưu trữ cố định mà không gây ảnh hưởng đến bất kỳ thông tin nào khác.

---

### 5.2. Đặc tả Schema: Gốc (Legacy) vs Chuẩn tắc mới (Canonical)

#### Schema Gốc (Legacy 11-field Schema for Student):
```csv
id,username,passwordHash,email,fullName,STUDENT,isActive,gradeLevel,preferredSubject,maxBudget,scheduleBitmask
```
- Độ dài: 11 tokens
- Trường 8: `preferredSubject` (Môn học mục tiêu - thông số tìm kiếm cũ)
- Trường 9: `maxBudget` (Ngân sách tối đa - thông số tìm kiếm cũ)
- Trường 10: `scheduleBitmask` (Mặt nạ 56 ca học)

#### Schema Chuẩn tắc Mới (Canonical 9-field Schema for Student):
```csv
id,username,passwordHash,email,fullName,STUDENT,isActive,gradeLevel,scheduleBitmask
```
- Độ dài: **Đúng 9 tokens**
- Các trường đã loại bỏ: `preferredSubject` (trường 8 cũ) và `maxBudget` (trường 9 cũ).
- Trường thứ 8 (index 8): `scheduleBitmask` (được đưa liền kề sau `gradeLevel`).
- Tuyệt đối không sinh giá trị thay thế nhân tạo, không trượt cột hay tái diễn giải nhầm lẫn sang thuộc tính khác.

#### Schema TUTOR (15 trường) và ADMIN (8 trường):
Giữ nguyên vẹn 100% định dạng, độ dài token và dữ liệu nghiệp vụ:
- `ADMIN` (8 tokens): `id,username,passwordHash,email,fullName,ADMIN,isActive,permissionLevel`
- `TUTOR` (15 tokens): `id,username,passwordHash,email,fullName,TUTOR,isActive,bio,subjectSpecialty,hourlyRate,rating,totalReviews,isVerified,totalEarnings,scheduleBitmask`

---

### 5.3. Bảng tổng hợp số liệu di chuyển (Migration Metrics & Verification)

| Chỉ số / Thuộc tính | Tệp Gốc (Pre-Migration / Backup) | Tệp Đã Di Chuyển (Post-Migration Production) | Trạng thái & Độ lệch |
|:---|:---|:---|:---:|
| **Đường dẫn tệp** | `data/archive/legacy_users/users_legacy_11fields_pre_migration.dat` | `data/users.dat` | Độc lập & An toàn |
| **Dung lượng tệp (Bytes)** | **7,008 bytes** | **6,247 bytes** | $\Delta = -761$ bytes (Đã tinh gọn sạch) |
| **Mã băm SHA-256** | `8b8c9c2489a9c94b74c57339cfdffa004f30e43c12f809c181f7ddc457bd49cf` | `b40fa6a07d3de238bcd66bf29c507d29623aae72d1e2843f30f6c626f32d1d55` | **Xác thực toàn vẹn** |
| **Tổng số bản ghi** | **50** | **50** | **1:1 Khớp chính xác 100%** |
| - Quản trị viên (ADMIN) | 3 bản ghi (Mỗi bản ghi 8 trường) | 3 bản ghi (Mỗi bản ghi 8 trường) | Bảo toàn 100% |
| - Gia sư (TUTOR) | 12 bản ghi (Mỗi bản ghi 15 trường) | 12 bản ghi (Mỗi bản ghi 15 trường) | Bảo toàn 100% |
| - Học viên (STUDENT) | 35 bản ghi (Mỗi bản ghi 11 trường) | **35 bản ghi (Mỗi bản ghi đúng 9 trường)** | Chuyển đổi chuẩn tắc 100% |
| **Số bản ghi Legacy còn lại trong file sản xuất** | 50 (35 học viên 11 trường) | **0 (Không còn bản ghi legacy nào)** | **Đạt tuyệt đối (ZERO)** |
| **Số bản ghi di chuyển thành công** | — | **50 / 50 (100%)** | Hoàn tất |
| **Số bản ghi lỗi hoặc bị bỏ qua** | — | **0 (Không có bản ghi nào lỗi)** | **ZERO Failure** |

---

### 5.4. Quy trình di chuyển an toàn & Chống mất mát dữ liệu (Safe Atomic Migration)
Quy trình được thực thi tự động qua script quản trị chuyên dụng [`scripts/migrate_users_canonical_schema.py`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/scripts/migrate_users_canonical_schema.py):
1. **Kiểm tra tiền kiểm (Pre-flight Inspection):** Phân tích từng dòng của tệp nguồn; kiểm tra định dạng và tính duy nhất của mã ID người dùng (IDs từ 1 đến 50).
2. **Tạo bản sao lưu có xác thực (Verified Backup):**
   - Lưu trữ bản sao nguyên trạng tại: `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/archive/legacy_users/users_legacy_11fields_pre_migration.dat`
   - Ghi nhận mã SHA-256: `8b8c9c2489a9c94b74c57339cfdffa004f30e43c12f809c181f7ddc457bd49cf`
3. **Chuyển đổi dữ liệu chuẩn tắc (Data Transformation):**
   - Lọc bỏ chính xác hai trường `tokens[8]` và `tokens[9]` ở các bản ghi `STUDENT`.
   - Giữ nguyên toàn bộ 7 trường đầu (ID, username, passwordHash, email, fullName, role, isActive) và trường `gradeLevel`, `scheduleBitmask`.
4. **Ghi tệp tạm thời (Staging File):** Ghi toàn bộ 50 dòng đã chuyển đổi vào tệp tạm thời an toàn `users_migrated_*.dat` bằng cơ chế `tempfile.mkstemp`.
5. **Kiểm tra hậu kiểm tệp tạm thời:** Đọc lại tệp tạm; kiểm tra tổng số dòng, cấu trúc token và xác minh không có dòng nào bị cắt cụt.
6. **Thay thế nguyên tử (Atomic In-Place Replacement):** Sử dụng `shutil.move` / atomic file rename để hoán đổi tệp tạm thời thành `data/users.dat`. Nếu có sự cố bất ngờ xảy ra, tệp sản xuất gốc không bị ảnh hưởng.
7. **Đồng bộ môi trường thực thi:** Đồng bộ tệp sản xuất sang thư mục build `build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/data/users.dat`.

---

### 5.5. Xác minh hậu kiểm tệp sản xuất (Post-Migration Verification on Production Disk)
Quá trình xác thực được kiểm tra trực tiếp trên đĩa vật lý (không phụ thuộc vào biểu diễn RAM):
- **Bảo toàn ID người dùng:** Tất cả 50 mã số ID (1 đến 50) tồn tại duy nhất, liên tục và không bị hoán đổi vai trò.
- **Bảo toàn mật khẩu và thông tin cá nhân:** Các mật khẩu (kể cả mật khẩu phức tạp chứa ký tự đặc biệt như `VyM5Gv$eZUOi`, `1tK9YXAH%XUJ`), họ tên tiếng Việt có dấu, email đều được giữ nguyên 100%.
- **Bảo toàn mặt nạ lịch học (`scheduleBitmask`):**
  - Học viên 6 (Nguyễn Văn Nam): `71777214294589695ULL`
  - Học viên 7 (Phạm Tuấn Thành): `281470681808895ULL`
  - Học viên 18 (Trần Minh Anh): `20574066625762505ULL`
- **Kiểm thử khứ hồi 100% (Round-Trip Determinism Parity):**
  Thực hiện vòng lặp `Đọc dòng từ đĩa -> Phân giải đối tượng C++ (fromCsv) -> Xuất lại chuỗi CSV (toCsv)` đối với **toàn bộ 50 dòng**. Kết quả so sánh chuỗi đạt độ chính xác 100% từng byte:
  $$\forall \text{line} \in \text{users.dat}: \quad \text{UserSerializer::toCsv}(*\text{UserSerializer::fromCsv}(\text{line})) \equiv \text{line}$$

---

### 5.6. Chiến lược tương thích ngược (Legacy Compatibility Strategy)
- Mặc dù tệp `data/users.dat` sản xuất hiện tại chứa **100% bản ghi chuẩn tắc 9 trường**, phương thức `UserSerializer::fromCsv` vẫn duy trì khả năng đọc dữ liệu 11 trường để tương thích ngược khi khôi phục từ các bản sao lưu lịch sử (`archive/legacy_users/`) hoặc các hệ thống bên ngoài.
- Tất cả các thao tác ghi mới hoặc cập nhật người dùng (`FileUserRepository::saveUser`, `FileUserRepository::updateUser`, `GUIBridge`, `AuthService`, `ConsoleApp`) **bắt buộc và duy nhất chỉ ghi ra Schema chuẩn tắc 9 trường**.

---

## 6. MA TRẬN KIỂM THỬ TỰ ĐỘNG (VERIFICATION MATRIX)

Hệ thống kiểm thử tự động `TutorMatch_Tests.exe` đã được mở rộng thêm 2 bộ kiểm thử chuyên biệt:

### 6.1. Test Suite 25: Student Profile Simplification & SmartMatch Criteria-Based Ranking
- **Test 25.1 — Tinh gọn hồ sơ:** Cập nhật hồ sơ học viên thành công mà không đòi hỏi môn học và ngân sách; xác thực từ chối khối lớp không hợp lệ ($< 1$ hoặc $> 12$).
- **Test 25.2 — Tính độc lập của truy vấn:** Chạy hai tìm kiếm SmartMatch với hai môn học và ngân sách khác nhau; xác thực hồ sơ học viên không bị biến đổi.
- **Test 25.3 — 6 Bộ lọc điều kiện cứng:**
  - Gia sư bị khóa mềm $\to$ Loại.
  - Gia sư sai môn học $\to$ Loại.
  - Gia sư lệch khối lớp $\to$ Loại.
  - Gia sư học phí vượt quá ngân sách $\to$ Loại.
  - Gia sư không đủ ca rảnh ở chế độ nghiêm ngặt $\to$ Loại.
  - Gia sư bị trùng khóa học `ACCEPTED` trong cùng khoảng thời gian $\to$ Loại.
- **Test 25.4 — Chế độ lịch học:** Xác thực ở chế độ nghiêm ngặt chỉ chọn gia sư đạt 100% ca; ở chế độ linh hoạt chấp nhận gia sư đạt $\ge 1$ ca và tính điểm tỷ lệ.
- **Test 25.5 — Tính bất biến của thang điểm 100:** Tổng điểm mọi gia sư luôn $\in [0, 100]$.
- **Test 25.6 — Trọng số đánh giá Bayes:** Gia sư 5.0 sao (1 đánh giá) nhận 12.75đ; gia sư 4.9 sao (10 đánh giá) nhận 14.07đ (chính xác hơn nhờ cỡ mẫu).
- **Test 25.7 — Tính giảm đơn điệu của học phí:** Học phí 120k nhận điểm cao hơn 150k.
- **Test 25.8 — Tính tất định & Phá vỡ thế hòa:** Hai lần chạy cùng tham số cho ra cùng một thứ tự và cùng một số điểm.

### 6.2. Test Suite 26: Persistence Schema & Migration
- **Test 26.1 — Đọc bản ghi legacy (11 trường):** Đọc thành công từ chuỗi legacy 11 trường, loại bỏ an toàn môn/ngân sách cũ, giữ đúng khối lớp và mặt nạ lịch; re-serialization xuất ra đúng 9 trường chuẩn tắc.
- **Test 26.2 — Đọc bản ghi chuẩn tắc mới (9 trường):** Đọc chính xác 9 trường của học viên mới.
- **Test 26.3 — Lưu và đọc lại (Round-trip determinism):** Ghi bản ghi mới qua `FileUserRepository`, đọc lại tệp kiểm tra đúng 9 tokens trên đĩa.
- **Test 26.4 — Bảo toàn bản ghi TUTOR (15 trường) và ADMIN (8 trường):** Không bị trượt cột hay méo mó dữ liệu giữa các vai trò khác nhau.
- **Test 26.5 — Tính độc lập của nhiều truy vấn SmartMatch:** Tìm kiếm với nhiều môn học và mức ngân sách khác nhau hoàn toàn độc lập, hồ sơ học viên trên bộ nhớ/đĩa không bị thay đổi.
- **Test 26.6 — Tính toàn vẹn sau di chuyển dữ liệu:** Khởi tạo `FileUserRepository` trên `data/users.dat` thực tế của production, xác thực 50/50 ID người dùng nguyên vẹn (3 Admin, 12 Tutor, 35 Student).
- **Test 26.7 — Bảo đảm tệp vật lý chuẩn tắc & Round-Trip Parity 100%:** Quét từng dòng của `data/users.dat` thực tế; xác thực 35/35 học viên có đúng 9 tokens, 0 bản ghi legacy 11 trường còn sót lại, xác nhận tệp backup lưu trữ tồn tại, và mỗi dòng đều đạt tính toàn vẹn tuyệt đối `toCsv(*fromCsv(line)) == line`.

### 6.3. Bảng tổng hợp trạng thái kiểm thử (Regression Results)

| STT | Tên Test Suite | Phạm vi kiểm tra | Trạng thái |
|:---:|:---|:---|:---:|
| 1 | Open/Closed Principle (OCP) | Khả năng mở rộng Strategy trong MatchingEngine | **PASS** |
| 2 | Liskov Substitution Principle (LSP) | Tính đa hình trong phân cấp thực thể User | **PASS** |
| 3 | DIP, ISP & SRP | Tách rời AuthService, BookingService, AdminService | **PASS** |
| 4 | ScheduleGuard Bitmask Math | Toán học xử lý 56 ca nhị phân & khử phantom bit | **PASS** |
| 5 | Sidebar Navigation Labels | Độ sạch của nhãn điều hướng không chứa tiền tố số | **PASS** |
| 6 | Tutor Profile Editing | Chỉnh sửa, xác thực và lưu trữ hồ sơ gia sư | **PASS** |
| 7 | Bidirectional Timetable Sync | Đồng bộ lịch học hai chiều giữa học viên và gia sư | **PASS** |
| 8 | Admin User Card Contract | Hợp đồng dữ liệu thẻ người dùng admin chuẩn hóa | **PASS** |
| 9 | User Detail Modal Contract | Modal chi tiết người dùng và xử lý người dùng không tồn tại | **PASS** |
| 10 | Verification & Lock Sync | Đồng bộ trạng thái khóa/xác minh giữa Admin và Student | **PASS** |
| 11 | AdminService Filters & Search | Lọc người dùng theo từ khóa và vai trò | **PASS** |
| 12 | AVL Tree Multi-Value Support | Cây cân bằng AVL hỗ trợ đa giá trị và xóa nút | **PASS** |
| 13 | Bitmask Boundary Sanitization | Khử bit rác ngoài phạm vi 56 ca | **PASS** |
| 14 | MockDatabaseEngine Atomic Persistence | Ghi file nguyên tử không trùng lặp | **PASS** |
| 15 | DataFileInspector Read-Only Inspection | Đọc tệp dữ liệu không ghi đè và ẩn mật khẩu | **PASS** |
| 16 | Course Booking Conflict Detection | Phát hiện xung đột lịch học định kỳ | **PASS** |
| 17 | Dedicated Entity Serializers | Serializer chuyên biệt cho User, Booking, Review | **PASS** |
| 18 | Concrete File Repositories | FileUserRepository, FileBookingRepo, FileReviewRepo | **PASS** |
| 19 | Phase 3 End-to-End Workflows | 10 kịch bản nghiệp vụ đầu-cuối (E2E-01 đến E2E-10) | **PASS** |
| 20 | Recurring Course Booking | Đặt khóa học định kỳ nhiều tuần | **PASS** |
| 21 | Weekly Timetable Integration | Hiển thị khóa học nhiều tuần trên thời khóa biểu | **PASS** |
| 22 | Comprehensive Data Audit | Toàn vẹn dữ liệu mẫu và tính nhất quán chéo | **PASS** |
| 23 | Multi-Slot Course & Termination | Khóa học nhiều ca/tuần, hủy thuận tình, tự hoàn thành | **PASS** |
| 24 | Commitment Acknowledgment Matrix | Cam kết học viên và điều kiện viết đánh giá | **PASS** |
| **25** | **Student Profile & SmartMatch Engine** | **Tinh gọn hồ sơ, tiêu chí động, lọc 6 bước, chấm 100đ** | **PASS** |
| **26** | **Persistence Schema & Migration** | **Tương thích 9/11 trường, di chuyển users.dat sạch sẽ** | **PASS** |

**Kết luận kiểm thử:** **26/26 Test Suites ĐẠT (100% PASS)**, thời gian chạy hoàn tất: ~1.8 giây.

---

## 7. GIAO DIỆN NGƯỜI DÙNG & TỐI ƯU CẢM NHẬN (UI POLISH)

### 7.1. Cải tiến giao diện SmartMatch (`StudentSmartMatchView.qml`)
- **Khung tiêu chí thu gọn (Search Criteria Card):** Được phân nhóm khoa học thành 3 vùng trực quan:
  1. *Nhóm 1 (Bắt buộc):* ComboBox chọn Môn học, ComboBox chọn Khối lớp, TextField nhập Ngân sách tối đa có kiểm tra số dương.
  2. *Nhóm 2 (Lịch học):* Ô nhập Ngày bắt đầu (`YYYY-MM-DD`), ComboBox Thời lượng khóa (4, 8, 12 tuần), Switch chuyển đổi chế độ Lịch học (Nghiêm ngặt / Linh hoạt).
  3. *Lưới chọn ca học tương tác 7×8:* Hiển thị 7 cột thứ (Thứ 2 đến Chủ nhật) và 8 hàng ca học. Nhấn chọn/bỏ chọn từng ca trực quan với mã màu đỏ rượu (`cBurgundy`).
  4. *Phím tắt nhanh (Presets):* Nút chọn nhanh *Tối 2-4-6*, *Tối 3-5-7*, *Cuối tuần sáng* và nút *Xóa chọn ca*.
  5. *Nhóm 3 (Nâng cao):* CheckBox *Chỉ chọn gia sư đã xác minh*, ComboBox *Đánh giá tối thiểu* (Tất cả, $\ge 4.0$, $\ge 4.5$, $\ge 4.8$).
- **Thẻ hiển thị kết quả đề xuất (Ranked Tutor Cards):**
  - Huy hiệu điểm tổng thể lớn, nổi bật (ví dụ: `88.5 / 100đ`).
  - Thanh tiến trình mini trực quan hiển thị điểm thành phần: Lịch học (35đ), Học phí (25đ), Đánh giá (15đ), Chuyên môn (15đ), Uy tín (10đ).
  - Khối danh sách các giải thích đề xuất (`✓ Khớp toàn bộ...`, `✓ Học phí...`, `✓ Chuyên môn...`) bằng tiếng Việt.
  - Nút *Đặt khóa học ngay* kết nối trực tiếp sang hộp thoại cam kết và đặt lịch định kỳ.

### 7.2. Tinh gọn giao diện Hồ sơ học viên (`StudentProfileCard.qml`)
- Đã loại bỏ các ô nhập môn học và ngân sách gây hiểu nhầm.
- Tinh chỉnh giao diện hiển thị các thông tin học tập căn bản và trạng thái tài khoản sạch sẽ, cân đối.

---

## 8. KẾT LUẬN

1. Dự án TutorMatch đã hoàn thành xuất sắc yêu cầu **Student Profile Simplification & SmartMatch Criteria-Based Ranking**:
   - Tách rời triệt để dữ liệu hồ sơ tĩnh và tham số tìm kiếm động.
   - Xây dựng động cơ ghép SmartMatch 100 điểm minh bạch, giải thích được.
   - Hoàn thành di chuyển dữ liệu vật lý an toàn cho `data/users.dat` sang Schema chuẩn tắc 9 trường.
   - Đảm bảo tính tương thích ngược toàn vẹn với dữ liệu và mã nguồn cũ.
2. Cả hai mục tiêu phần mềm `TutorMatch_Tests.exe` (26/26 test suites) và `TutorMatch_GUI.exe` đều đã được biên dịch thành công, chạy ổn định và sẵn sàng cho môi trường vận hành thực tế.
