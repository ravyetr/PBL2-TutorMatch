# BÁO CÁO NGHIỆM THU ĐỘC LẬP: XÁC MINH TOÀN DIỆN BỘ LỌC MÔN HỌC BẮT BUỘC SMARTMATCH (TUTORMATCH)

> **Dự án:** TutorMatch — Hệ thống kết nối Gia sư và Học viên  
> **Chuyên mục:** SmartMatch Subject Filtering Final Validation  
> **Môi trường xác minh:** Desktop Qt 6.11.2 MinGW 64-bit / C++17 / Windows  
> **Tài liệu tham chiếu:** [`include/models/SubjectTaxonomy.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/SubjectTaxonomy.h), [`src/models/SubjectTaxonomy.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp), [`src/algorithms/MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/MatchingEngine.cpp), [`src/services/MatchingService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/MatchingService.cpp)  
> **Thời điểm xác minh:** 11/11/2026  
> **Kết luận nghiệm thu:** **PASS (ĐẠT 100%)**

---

## 1. MA TRẬN KIỂM THỬ PHÂN LOẠI MÔN HỌC (TAXONOMY TEST MATRIX)

Bộ phân loại môn học `SubjectTaxonomy` đã được kiểm thử trực tiếp trên các hàm sản xuất C++:
- `normalizeText(input)`
- `getCanonicalSubjects(input)`
- `parseSubject(input)`
- `matchesFilter(input, filterSubject)`

Kết quả thực nghiệm trên 15 tập dữ liệu đại diện và biên độ:

| STT | Chuỗi đầu vào | `normalizeText` | `getCanonicalSubjects` | `parseSubject` (Code & Display) | So khớp `matchesFilter` | Kết luận |
|:---:|---|---|:---:|:---:|---|:---:|
| 1 | `Toán` | `"toan"` | `[MATH]` | `MATH` ("Toán") | Toán: **TRUE**; Lý, Hóa, Sử, Địa: **FALSE** | **ĐẠT** |
| 2 | `Vật lý` | `"vat ly"` | `[PHYSICS]` | `PHYSICS` ("Vật lý") | Lý: **TRUE**; Toán, Hóa, Sử, Địa: **FALSE** | **ĐẠT** |
| 3 | `Physics` | `"physics"` | `[PHYSICS]` | `PHYSICS` ("Vật lý") | Lý: **TRUE**; Toán, Hóa, Sử, Địa: **FALSE** | **ĐẠT** |
| 4 | `Toán, Vật lý` | `"toan vat ly"` | `[MATH, PHYSICS]` | `MATH` ("Toán") | Toán: **TRUE**; Lý: **TRUE**; Hóa, Sử: **FALSE** | **ĐẠT** |
| 5 | `Toán & Vật lý` | `"toan vat ly"` | `[MATH, PHYSICS]` | `MATH` ("Toán") | Toán: **TRUE**; Lý: **TRUE**; Hóa, Sử: **FALSE** | **ĐẠT** |
| 6 | `Toán và Vật lý` | `"toan va vat ly"` | `[MATH, PHYSICS]` | `MATH` ("Toán") | Toán: **TRUE**; Lý: **TRUE**; Hóa, Sử: **FALSE** | **ĐẠT** |
| 7 | `Mathematics and Physics` | `"mathematics and physics"` | `[MATH, PHYSICS]` | `MATH` ("Toán") | Toán: **TRUE**; Lý: **TRUE**; Hóa, Sử: **FALSE** | **ĐẠT** |
| 8 | `Lịch sử` | `"lich su"` | `[HISTORY]` | `HISTORY` ("Lịch sử") | Sử: **TRUE**; Toán, Lý, Hóa, Địa: **FALSE** | **ĐẠT** |
| 9 | `Địa lý` | `"dia ly"` | `[GEOGRAPHY]` | `GEOGRAPHY` ("Địa lý") | Địa: **TRUE**; Toán, Lý, Hóa, Sử: **FALSE** | **ĐẠT** |
| 10 | `Khoa học tự nhiên` | `"khoa hoc tu nhien"` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** (Đã chặn dứt điểm false-positive) | **ĐẠT** |
| 11 | `""` (Rỗng) | `""` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** | **ĐẠT** |
| 12 | `"   "` (Khoảng trắng) | `""` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** | **ĐẠT** |
| 13 | `Âm nhạc` | `"am nhac"` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** | **ĐẠT** |
| 14 | `Mỹ thuật` | `"my thuat"` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** | **ĐẠT** |
| 15 | `RandomXYZ` | `"randomxyz"` | `[]` | `OTHER` ("Môn học khác") | Mọi môn: **FALSE** | **ĐẠT** |

### Đánh giá các khía cạnh an toàn thuật toán:
- **Tách từ (Tokenization & Splitting):** Các ký tự phân cách (`,`, `&`, `+`, `/`, `và`, `and`) được chuẩn hóa thành khoảng trắng đơn đồng nhất.
- **Không xảy ra trùng lặp (No duplicate canonicals):** Dù chuỗi có chứa nhiều từ khóa cùng một môn (ví dụ: `"Tiếng Anh / IELTS (6.5 - 8.5)"`), mỗi mã chuẩn chỉ được đẩy vào `result` một lần duy nhất.
- **Khử lỗi dương tính giả (False-positive prevention):** 
  - Khắc phục lỗi tiềm ẩn với `"Khoa học tự nhiên"`: Từ `"khoa hoc"` từng chứa chuỗi con `"hoa hoc"`. Sau khi áp dụng kiểm tra biên từ bắt buộc `containsWordOrPhrase(norm, "hoa hoc")`, `"khoa hoc tu nhien"` không còn bị nhận nhầm sang môn Hóa học.
  - Từ khóa `"su"` được kiểm tra bảo vệ loại trừ các chức danh như `"ky su"` (kỹ sư), `"giao su"` (giáo sư), `"luat su"` (luật sư), `"bac si"` (bác sĩ).
  - Từ khóa `"dia"` được kiểm tra bảo vệ loại trừ `"dia diem"`, `"dia chi"`, `"dia phuong"`.
  - Từ khóa `"ly"` được bảo vệ dứt khoát không nhận nhầm từ `"dia ly"` sang môn Vật lý.

---

## 2. THỐNG KÊ DỮ LIỆU GIA SƯ SẢN XUẤT (ACTUAL TUTOR DATA AUDIT)

Kiểm tra trực tiếp tệp dữ liệu hoạt động [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat) theo phương thức chỉ đọc (read-only):

### 2.1. Số liệu tổng quan
- **Tổng số tài khoản gia sư trong hệ thống:** 12 tài khoản.
- **Số gia sư chuyên một môn (Single-subject):** 10 tài khoản (83.33%).
- **Số gia sư đa môn (Multi-subject):** 1 tài khoản (8.33%).
- **Số gia sư có dữ liệu môn rỗng:** 0 tài khoản (0.00%).
- **Số hồ sơ có chuyên môn chưa nằm trong danh mục chuẩn phổ thông:** 1 tài khoản (8.33%).

### 2.2. Phân bố tần suất theo môn học chuẩn hóa (Canonical Subject Frequencies)
| Mã môn chuẩn | Tên hiển thị tiếng Việt | Số lượng gia sư giảng dạy | Danh sách Gia sư (Mã ID & Chuyên môn đăng ký) |
|---|---|:---:|---|
| `MATH` | Toán | 2 | • **ID 2**: "Toán 12 - Luyện thi Đại học"<br>• **ID 13**: "Toán 10-11 (Củng cố nền tảng)" |
| `PHYSICS` | Vật lý | 2 | • **ID 3**: "Vật lý 10-12 (Luyện thi THPT)"<br>• **ID 16**: "Vật lý 10-11 (Cơ bản và nâng cao)" |
| `CHEMISTRY` | Hóa học | 1 | • **ID 5**: "Hóa học 10-12 (Luyện thi HSG)" |
| `BIOLOGY` | Sinh học | 1 | • **ID 10**: "Sinh học 10-12 (Luyện thi THPT)" |
| `ENGLISH` | Tiếng Anh | 2 | • **ID 4**: "Tiếng Anh / IELTS (6.5 - 8.5)"<br>• **ID 14**: "Tiếng Anh / IELTS (Giao tiếp - Speaking)" |
| `LITERATURE` | Ngữ văn | 1 | • **ID 11**: "Ngữ văn 10-12 (Luyện thi THPT)" |
| `COMPUTER_SCIENCE` | Tin học | 1 | • **ID 12**: "Tin học 10-12 (Lập trình C++)" |
| `HISTORY` | Lịch sử | 1 | • **ID 15**: "Lịch sử - Địa lý 10-12 (Ôn thi THPT)" |
| `GEOGRAPHY` | Địa lý | 1 | • **ID 15**: "Lịch sử - Địa lý 10-12 (Ôn thi THPT)" |

### 2.3. Chi tiết hồ sơ đặc biệt
- **Hồ sơ đa môn hợp lệ:** Gia sư **ID 15** (`"Lịch sử - Địa lý 10-12 (Ôn thi THPT)"`) được phân giải chính xác thành cả hai môn `[HISTORY, GEOGRAPHY]`. Khi tìm môn Sử hoặc tìm môn Địa, gia sư ID 15 đều đủ điều kiện và được xếp hạng.
- **Hồ sơ ngoài danh mục phổ thông:** Gia sư **ID 17** (`"Tiếng Nhật N5 - N3"`) là giáo viên ngoại ngữ chuyên biệt ngoài chương trình THPT, được phân loại chính xác thành `CanonicalSubject::OTHER`. Khi học viên tìm kiếm các môn học phổ thông trong SmartMatch, gia sư ID 17 được bảo vệ loại trừ an toàn, không bị nhầm lẫn với bất kỳ môn nào.

> **Kết luận kiểm toán:** Không có bất kỳ tài khoản gia sư hợp lệ nào của chương trình phổ thông bị loại trừ sai bởi bộ phân loại mới.

---

## 3. XÁC MINH LUỒNG THI HÀNH THỰC TẾ (PRODUCTION PIPELINE VERIFICATION)

Đã kiểm tra đường truyền dữ liệu thực tế:  
`GUIBridge::qmlSearchSmartMatches → MatchingService::findMatchesWithCriteria → MatchingEngine::findTopMatchesWithCriteria → Result List`

```
┌────────────────────────────────────────────────────────────────────────┐
│ 1. GUIBridge::qmlSearchSmartMatches                                    │
│    - Bóc tách criteriaMap["subject"] chuẩn hóa chuỗi                  │
│    - Luôn có giá trị mặc định từ ComboBox ("Toán") hoặc học viên chọn  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ criteria
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 2. MatchingService::findMatchesWithCriteria                            │
│    - Truy xuất danh sách gia sư hoạt động từ IUserRepository          │
│    - Truy xuất các đặt chỗ hiện hữu từ ICourseBookingRepository       │
│    - Chuyển giao hoàn toàn cho MatchingEngine                         │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 3. MatchingEngine::findTopMatchesWithCriteria                          │
│    - DUYỆT TỪNG GIA SƯ: Gọi isEligible(criteria, tutor, bookings)      │
│      ┌──────────────────────────────────────────────────────────────┐  │
│      │ Stage A1: SubjectTaxonomy::matchesFilter(tutorSubj, reqSubj) │  │
│      │ -> Nếu FALSE: CONTINUE NGAY LẬP TỨC!                         │  │
│      │ -> Không chấm điểm, không đẩy vào Heap!                      │  │
│      └──────────────────────────────────────────────────────────────┘  │
│      Stage A2: isActive == true                                        │
│      Stage A3: Grade level compatibility                               │
│      Stage A4: hourlyRate <= maxBudget                                 │
│      Stage A5 & A6: Schedule bitmask & Active Booking Conflicts        │
│                                                                        │
│    - CHỈ GIA SƯ QUA TOÀN BỘ STAGE A MỚI ĐƯỢC:                          │
│      calculateScoreWithCriteria() -> maxHeap.push(res)                 │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ 4. Kết quả trả về (MaxHeap::extractSortedTopK)                         │
│    - Chỉ gồm các gia sư đạt chuẩn môn học và các tiêu chí bắt buộc.    │
│    - Tuyệt đối không có cơ chế fallback lôi kéo gia sư trái môn.       │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 4. KẾT QUẢ XÁC MINH CÁC KỊCH BẢN ĐẦU-CUỐI (END-TO-END SCENARIOS)

Đã thực thi bộ kiểm thử thực tế độc lập `build/test_end_to_end_cases.exe` kiểm chứng 5 ca nghiệp vụ:

### Case A: Học viên tìm môn Physics — Gia sư Sử xuất sắc vs Gia sư Lý khiêm tốn
- **Dữ liệu:**
  - Gia sư A (ID 101): Môn `Lịch sử`, đánh giá 5.0★ (50 đánh giá), học phí 50.000 VNĐ, rảnh 100% thời gian (56/56 ca).
  - Gia sư B (ID 102): Môn `Vật lý 10-12`, đánh giá 4.0★ (2 đánh giá), học phí 180.000 VNĐ, chỉ rảnh 8 ca.
- **Kết quả thực tế:**
  - Gia sư A bị loại bỏ triệt để tại Stage A1.
  - Gia sư B hợp lệ và được xếp hạng #1 trong kết quả trả về (`results.size() == 1`, `ID == 102`).
  - **Kết luận:** Đạt hoàn hảo yêu cầu cốt lõi.

### Case B: Học viên tìm môn Physics — Gia sư dạy Toán & Lý
- **Dữ liệu:** Gia sư (ID 201) chuyên môn `"Mathematics and Physics"`. Truy vấn: `"Vật lý"`.
- **Kết quả thực tế:** Gia sư ID 201 vượt qua bộ lọc Stage A1 và được tính điểm, xếp hạng thành công (`results.size() == 1`, `ID == 201`).
- **Kết luận:** Gia sư đa môn được công nhận hợp lệ.

### Case C: Học viên tìm môn Physics — Toàn bộ gia sư đều dạy môn khác
- **Dữ liệu:** Nhóm gia sư ứng viên chỉ gồm Toán, Hóa học, Lịch sử.
- **Kết quả thực tế:** `results.empty() == true` (danh sách rỗng).
- **Giao diện:** Thẻ thông báo QML hiển thị dòng tiếng Việt chính xác:  
  `"Không tìm thấy gia sư phù hợp với môn học bạn đã chọn."`
- **Kết luận:** Xử lý danh sách rỗng an toàn và thân thiện.

### Case D: Môn học yêu cầu bị thiếu, rỗng hoặc chứa dữ liệu vô hiệu
- **Dữ liệu:**
  - Subcase D1: `criteria.subject = ""` (chuỗi rỗng).
  - Subcase D2: `criteria.subject = "    "` (chỉ có khoảng trắng).
  - Subcase D3: `criteria.subject = "???###InvalidSubjectXYZ"` (chuỗi dị thường).
- **Kết quả thực tế:** Cả 3 trường hợp đều trả về danh sách rỗng (`size == 0`). Hệ thống không bao giờ trả về danh sách gia sư không bị kiểm soát.
- **Kết luận:** Bảo mật và an toàn dữ liệu đầu vào đạt chuẩn.

### Case E: Gia sư đúng môn nhưng vi phạm điều kiện cứng khác
- **Dữ liệu:** 5 gia sư đều dạy môn `Vật lý`:
  - Gia sư E1 (ID 501): `isActive = false` (Tài khoản bị khóa/chưa kích hoạt).
  - Gia sư E2 (ID 502): Học phí 300.000 VNĐ > ngân sách tối đa 200.000 VNĐ.
  - Gia sư E3 (ID 503): Chuyên dạy tiểu học (lớp 1-5), học viên yêu cầu lớp 12.
  - Gia sư E4 (ID 504): Lịch rảnh không trùng ca học yêu cầu (Thứ 4 ca 7).
  - Gia sư E5 (ID 505): Thỏa mãn toàn bộ các điều kiện.
- **Kết quả thực tế:** `results.size() == 1`, chỉ duy nhất Gia sư E5 (ID 505) được đưa vào xếp hạng. E1 đến E4 đều bị loại bỏ dứt khoát trước vòng tính điểm.
- **Kết luận:** Quy trình lọc cứng Stage A bảo đảm tính toàn vẹn 100%.

---

## 5. TOÀN VẸN DỮ LIỆU & KIỂM THỬ HỒI QUY (INTEGRITY & REGRESSION)

### 5.1. Mã băm toàn vẹn dữ liệu sản xuất (SHA-256 Hashes)
Tất cả các tệp dữ liệu lưu trữ gốc trong thư mục `data/` đều được giữ nguyên vẹn, không có sự sai khác hay biến đổi bất kỳ byte nào:

| Tập tin dữ liệu | Kích thước | Mã băm SHA-256 thực tế | Trạng thái toàn vẹn |
|---|:---:|---|:---:|
| [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat) | 6.247 bytes | `b40fa6a07d3de238bcd66bf29c507d29623aae72d1e2843f30f6c626f32d1d55` | **NGUYÊN VẸN 100%** |
| [`data/course_bookings.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/course_bookings.dat) | 4.676 bytes | `dfb6c026de7c0ba18e0d28797aa6c837d6c93054c98ba9086ad198f58514fb99` | **NGUYÊN VẸN 100%** |
| [`data/reviews.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/reviews.dat) | 1.650 bytes | `193edaa190734bc19656c4b5b88a05909dd0b0ab83111c8a60ebe5e6eafd338b` | **NGUYÊN VẸN 100%** |

### 5.2. Kết quả kiểm thử hồi quy toàn bộ hệ thống (27/27 Test Suites)
Thực thi kiểm thử tổng thể qua `TutorMatch_Tests.exe`:
- **Suite 1:** Open/Closed Principle (OCP) in MatchingEngine — **PASS**
- **Suite 2:** Liskov Substitution Principle (LSP) in User Hierarchy — **PASS**
- **Suite 3:** DIP, ISP & SRP across decoupled Services — **PASS**
- **Suite 4:** ScheduleGuard Bitmask Collision Math — **PASS**
- **Suite 5:** Sidebar Navigation Labels & Zero Numeric Prefixes — **PASS**
- **Suite 6:** Tutor Profile Editing, Validation & Persistence — **PASS**
- **Suite 7:** Bidirectional Timetable Synchronization — **PASS**
- **Suite 8:** Admin User Card Data Contract — **PASS**
- **Suite 9:** User Detail Modal Contract & Error State — **PASS**
- **Suite 10:** Verification & Lock Sync Across Admin/Student Views — **PASS**
- **Suite 11:** AdminService Filters & Subject Keyword Search — **PASS**
- **Suite 12:** AVL Tree Multi-Value Support & Deletion — **PASS**
- **Suite 13:** 56-Bit Boundary & Phantom Bit Sanitization — **PASS**
- **Suite 14:** MockDatabaseEngine Atomic Persistence & Zero-Duplication — **PASS**
- **Suite 15:** DataFileInspector Read-Only Inspection & Redaction — **PASS**
- **Suite 16:** Course Booking Conflict Detection & Synchronization — **PASS**
- **Suite 17:** Dedicated Entity Serializers (Phase 2 Step 2) — **PASS**
- **Suite 18:** Concrete File Repositories (Phase 2 Step 3) — **PASS**
- **Suite 19:** Phase 3 End-to-End Workflows (E2E-01 to E2E-10) — **PASS**
- **Suite 20:** Recurring Course Booking Invariants — **PASS**
- **Suite 21:** Weekly Timetable Integration for Recurring Course Booking — **PASS**
- **Suite 22:** Comprehensive Data Audit, Parsing & Migration Invariants — **PASS**
- **Suite 23:** Multi-Slot Course Booking, Consensual Termination & Auto-Completion — **PASS**
- **Suite 24:** Commitment Acknowledgment & Review Eligibility Matrix — **PASS**
- **Suite 25:** Student Profile Simplification & Criteria-Based Ranking — **PASS**
- **Suite 26:** Persistence Schema & Migration — **PASS**
- **Suite 27:** SmartMatch Mandatory Subject Filtering Before Ranking — **PASS**

**Tổng kết:** **27/27 Suites PASS (100% Success, 0 failures, 0 regressions).**

---

## 6. KẾT LUẬN NGHIỆM THU (FINAL VERDICT)

| Hạng mục kiểm tra | Tiêu chuẩn đánh giá | Kết quả thực tế | Trạng thái |
|---|---|---|:---:|
| 1. Kiểm tra môn học bắt buộc | Môn học là điều kiện tiên quyết Stage A1 | Gia sư sai môn bị loại 100% trước vòng tính điểm | **PASS** |
| 2. Gia sư đa môn | Đủ điều kiện khi có ít nhất 1 môn khớp | Khớp chính xác cả tiếng Việt và tiếng Anh | **PASS** |
| 3. Xử lý trạng thái rỗng | Thông báo tiếng Việt chính xác, không tự ý gợi ý sai môn | Trả về `[]` và hiển thị thông điệp chuẩn xác trên QML | **PASS** |
| 4. Toàn vẹn dữ liệu sản xuất | Không chỉnh sửa dữ liệu, SHA-256 không đổi | Khớp 100% mã băm gốc của cả 3 tệp `.dat` | **PASS** |
| 5. Độ tin cậy biên dịch | Biên dịch sạch 4/4 target trên MinGW 64-bit | Exit Code 0, không có lỗi liên kết | **PASS** |
| 6. Kiểm thử hồi quy | Tất cả các suite cũ và mới tiếp tục hoạt động | 27/27 Suites PASS | **PASS** |

> **KẾT LUẬN CHUNG:** **PASS (CHẤP THUẬN NGHIỆM THU HOÀN TOÀN)**
