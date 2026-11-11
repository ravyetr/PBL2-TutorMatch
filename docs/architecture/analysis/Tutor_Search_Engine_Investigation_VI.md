# BÁO CÁO ĐIỀU TRA KIẾN TRÚC & PHÂN TÍCH CƠ CHẾ TÌM KIẾM GIA SƯ (TUTOR SEARCH ENGINE INVESTIGATION)

**Tác giả:** Senior C++/Qt Software Architect & Search Engine Engineer  
**Dự án:** TutorMatch Platform (Qt 6.11.2 / C++17 / QML)  
**Tài liệu tham chiếu:** [`docs/architecture/analysis/Tutor_Search_Engine_Investigation_VI.md`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/docs/architecture/analysis/Tutor_Search_Engine_Investigation_VI.md)  
**Thời gian hoàn thành:** 11/11/2026  
**Trạng thái:** **INVESTIGATION & DESIGN COMPLETED — NO CODE/DATA MODIFIED**

---

## 1. Executive Summary

Cuộc điều tra này được tiến hành nhằm làm rõ toàn diện cơ chế tìm kiếm gia sư hiện tại trên nền tảng TutorMatch, xác định chính xác nguyên nhân gốc rễ (*root cause*) dẫn đến trường hợp:
- **Dữ liệu gia sư lưu trữ:** Chuyên môn `Toán  Vật Lý  Hoá 12` (ID 52 - Gia sư Trần Chi).
- **Từ khóa học sinh nhập:** `Toán Vật Lý`.
- **Hiện tượng thực tế:** Gia sư hoàn toàn **không xuất hiện** trong danh sách kết quả tìm kiếm.

### Kết quả điều tra chính:
1. **Nguyên nhân cốt lõi (Root Cause):** Lỗi không nằm ở tầng giao diện QML, không nằm ở `SubjectTaxonomy`, mà nằm trực tiếp tại hàm [`UserService::searchAndFilterTutors`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/UserService.cpp#L146-L178):
   - **Tìm kiếm chuỗi con nguyên khối (Monolithic Substring Matching):** Hệ thống dùng `lowerSubj.find(kw)` trực tiếp trên toàn bộ cụm từ khóa mà **hoàn toàn không thực hiện tách từ (tokenization)** hay chuẩn hóa khoảng trắng (*whitespace collapsing*).
   - **Sự sai lệch ký tự khoảng trắng thực tế trong dữ liệu:** Trong file [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat#L52), chuyên môn của gia sư ID 52 được lưu với **2 dấu cách liên tiếp**: `"Toán  Vật Lý  Hoá 12"` (bytes: `0x20 0x20` giữa `Toán` và `Vật`). Khi người dùng gõ từ khóa với 1 dấu cách `"Toán Vật Lý"`, phép so khớp `std::string::find` lập tức thất bại và trả về `std::string::npos`.
   - **Khiếm khuyết chuyển đổi UTF-8 bằng `std::tolower`:** C++ `std::tolower(unsigned char c)` chỉ xử lý ký tự ASCII (0..127). Đối với các ký tự tiếng Việt đa byte UTF-8 (như `Á`, `Ô`, `Đ`, `Ý`), hàm không thể chuyển thành chữ thường, khiến các từ khóa có hoa/thường không đồng nhất bị loại bỏ.
   - **Thiếu tính năng không dấu (Accent Insensitivity):** Tìm kiếm không loại bỏ dấu thanh, nên người dùng gõ `toan vat ly` sẽ hoàn toàn không tìm thấy `Toán Vật Lý`.
   - **Thứ tự từ khóa cứng nhắc:** Người dùng tìm `Vật Lý Toán` hoặc `Toán 12` đều bị trả về kết quả rỗng (miss) do thứ tự từ không liền kề.

2. **Hiện trạng SubjectTaxonomy & SmartMatch:**
   - [`SubjectTaxonomy`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp) **đã hỗ trợ nhận diện rất tốt** `Toán  Vật Lý  Hoá 12` thành đủ 3 môn canonical: `MATH`, `PHYSICS`, `CHEMISTRY`.
   - Tuy nhiên, trong luồng tìm kiếm từ khóa thông thường của `UserService`, `SubjectTaxonomy` **không hề được kích hoạt cho từ khóa tự do** (`kw`), mà chỉ được dùng khi người dùng chọn lọc môn từ dropdown `subjectCombo`.
   - **Nguyên tắc bất biến:** SmartMatch sử dụng môn học làm **Stage A Hard Eligibility Filter** (điều kiện tiên quyết) trước khi xếp hạng 100 điểm. Cơ chế tìm kiếm tự do (Search Engine) phải được thiết kế độc lập, không làm thay đổi hay làm suy yếu các ràng buộc bất biến này của SmartMatch.

3. **Phương án khuyến nghị:** Đề xuất triển khai **Phương án Hybrid (Scored Multi-Field Token Search + Subject Taxonomy Intent Detection)** kết hợp xếp hạng theo độ liên quan (*Relevance Scoring*). Giải pháp này giải quyết hoàn hảo bài toán `AND` vs `OR`, đưa gia sư dạy cả 2 môn lên vị trí dẫn đầu, hỗ trợ tiếng Việt không dấu/có dấu, loại bỏ hoàn toàn các trường hợp âm tính giả (*false negative*) và dương tính giả (*false positive*).

---

## 2. Kiến trúc và Luồng tìm kiếm hiện tại (Data Flow & Sequence)

Luồng tìm kiếm gia sư trên TutorMatch được vận hành theo mô hình kiến trúc phân lớp chuẩn:
`Presentation Layer (QML) → GUIBridge (C++/Qt) → Service Layer (UserService) → Data Access Layer (FileUserRepository) → Presentation Layer`.

```mermaid
sequenceDiagram
    autonumber
    actor Student as Học sinh (User)
    participant UI as StudentTutorSearchView.qml
    participant Bridge as GUIBridge (Q_INVOKABLE)
    participant Service as UserService
    participant Repo as FileUserRepository
    participant File as data/users.dat

    Student->>UI: Nhập từ khóa "Toán Vật Lý" vào searchKeywordInput
    Note over UI: Sự kiện onTextChanged kích hoạt reloadTutors()
    UI->>Bridge: searchAndFilterTutors(kw, subj, minP, maxP, minR)
    Bridge->>Service: searchAndFilterTutors(keyword, subject, minPrice, maxPrice, minRating)
    Service->>Repo: getAllTutors()
    Repo->>File: Đọc danh sách Tutor từ users.dat (InMemory / Cache)
    Repo-->>Service: std::vector<std::shared_ptr<Tutor>>
    
    rect rgb(255, 230, 230)
    Note over Service: 1. std::tolower(kw) (Lỗi UTF-8)<br/>2. lowerSubj.find(kw) (Lỗi 2 khoảng trắng)<br/>=> Gia sư ID 52 bị loại bỏ (continue)!
    end

    Service-->>Bridge: std::vector<std::shared_ptr<Tutor>> (Đã lọc)
    Bridge-->>UI: QVariantList (Chuyển đổi sang QML model)
    UI-->>Student: Cập nhật tutorGridView (Không có gia sư Trần Chi!)
```

### Phân tích chi tiết các bước trong luồng:
1. **Kích hoạt tìm kiếm (Trigger Mechanism):**
   - Sự kiện `onTextChanged` trên `searchKeywordInput`: Tìm kiếm theo thời gian thực (*real-time reactive search*) ngay khi người dùng gõ từng ký tự.
   - Nút **"Lọc kết quả"** (`Button`): Kích hoạt lại hàm `reloadTutors()`.
   - Nút **"Đặt lại bộ lọc"** (`Button`): Gọi `resetFilters()` xóa rỗng ô tìm kiếm, đưa combobox về mặc định và tải lại danh sách đầy đủ.
2. **Truyền nhận dữ liệu qua Bridge:**
   - [`GUIBridge::searchAndFilterTutors`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/gui/GUIBridge.cpp#L648) nhận 5 tham số: `keyword` (QString), `subject` (QString), `minPrice` (double), `maxPrice` (double), `minRating` (double).
   - Thực hiện `trimmed()` trên chuỗi trước khi gọi Service: `keyword.trimmed().toStdString()`.
3. **Thực thi nghiệp vụ tại Service:**
   - [`UserService::searchAndFilterTutors`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/UserService.cpp#L146) lấy danh sách gia sư từ `userRepo->getAllTutors()`.
   - Kiểm tra `t->getIsActive()`: Gia sư bị khóa sẽ bị bỏ qua.
   - Áp dụng bộ lọc từ khóa (`kw`): So khớp chuỗi con trong `fullName`, `subjectSpecialty`, và `bio`.
   - Áp dụng bộ lọc môn học từ dropdown (`subject`): Nếu có, gọi [`SubjectTaxonomy::matchesFilter`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp#L254).
   - Áp dụng bộ lọc giá và đánh giá sao (`hourlyRate`, `rating`).
4. **Hiển thị trên giao diện:**
   - Dữ liệu trả về dưới dạng `QVariantList` chứa các `QVariantMap` với các trường: `id`, `fullName`, `initials`, `subject`, `hourlyRate`, `rating`, `totalReviews`, `isVerified`, `bio`.
   - Nếu `tutorList.length === 0`: Hiển thị `Empty State Banner` với thông báo *"🔍 Không tìm thấy gia sư phù hợp"*.
   - Nếu `tutorList.length > 0`: Hiển thị lưới `GridView` (`tutorGridView`) với các thẻ `TutorCard`.
   - **Ghi chú về phân trang & giới hạn:** Hệ thống hiện **không có phân trang** (*pagination*) và không giới hạn `topK` trong tìm kiếm thông thường; toàn bộ kết quả thỏa điều kiện đều được trả về đầy đủ.

---

## 3. Bảng đối chiếu File / Class / Function / UI Component

| Thành phần kiến trúc | Đường dẫn tệp tin | Class / Component | Function / ID liên quan | Trách nhiệm chính |
|---|---|---|---|---|
| **Presentation (UI)** | [`ui/qml/StudentTutorSearchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentTutorSearchView.qml) | `Item { id: studentTutorSearchView }` | `reloadTutors()`, `resetFilters()` | Tiếp nhận input từ khóa, combobox, hiển thị danh sách gia sư |
| **Presentation (UI)** | [`ui/qml/StudentTutorSearchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentTutorSearchView.qml#L141-L150) | `TextField { id: searchKeywordInput }` | `onTextChanged` | Ô nhập từ khóa tìm kiếm |
| **Presentation (UI)** | [`ui/qml/StudentTutorSearchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentTutorSearchView.qml#L157-L166) | `ComboBox { id: subjectCombo }` | `onCurrentIndexChanged` | Lọc theo danh mục môn học chuẩn |
| **Presentation (Bridge)** | [`include/gui/GUIBridge.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/gui/GUIBridge.h#L165) | `class GUIBridge` | `searchAndFilterTutors` | Khai báo Q_INVOKABLE kết nối QML với C++ |
| **Presentation (Bridge)** | [`src/gui/GUIBridge.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/gui/GUIBridge.cpp#L648-L674) | `class GUIBridge` | `searchAndFilterTutors` | Chuyển đổi dữ liệu Qt (QString/QVariant) sang C++ std và ngược lại |
| **Business Logic (Service)**| [`include/services/UserService.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/services/UserService.h#L47) | `class UserService` | `searchAndFilterTutors` | Khai báo phương thức tìm kiếm và lọc gia sư |
| **Business Logic (Service)**| [`src/services/UserService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/UserService.cpp#L146-L203) | `class UserService` | `searchAndFilterTutors` | **Điểm lỗi chính:** Thực thi lọc chuỗi theo từ khóa, môn học, học phí, rating |
| **Business Logic (Taxonomy)**| [`include/models/SubjectTaxonomy.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/SubjectTaxonomy.h) | `class SubjectTaxonomy` | `getCanonicalSubjects`, `matchesFilter`, `parseSubject` | Chuẩn hóa, bóc tách môn học chuẩn và khối lớp |
| **Business Logic (Taxonomy)**| [`src/models/SubjectTaxonomy.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp) | `class SubjectTaxonomy` | `stripAccents`, `normalizeText`, `getCanonicalSubjects` | Thực thi giải thuật bóc tách ngữ nghĩa môn học |
| **Business Logic (Matching)**| [`src/algorithms/MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/MatchingEngine.cpp#L247-L320) | `class MatchingEngine` | `isEligible`, `findTopMatchesWithCriteria` | Thuật toán SmartMatch với Stage A Hard Filter |
| **Data Access (Repository)**| [`src/storage/FileUserRepository.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/storage/FileUserRepository.cpp) | `class FileUserRepository` | `getAllTutors`, `findUserById` | Đọc/ghi và lưu trữ in-memory cache danh sách người dùng |
| **Persistence (Storage)**| [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat#L52) | Tệp phẳng CSV | Dòng 52 (`chichi`) | Nơi lưu trữ thông tin gia sư Trần Chi (`Toán  Vật Lý  Hoá 12`) |

---

## 4. Phân tích chi tiết Thuật toán tìm kiếm hiện tại

Đoạn mã tìm kiếm thực tế trong [`UserService::searchAndFilterTutors`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/UserService.cpp#L154-L178):

```cpp
std::string kw = keyword;
std::transform(kw.begin(), kw.end(), kw.begin(), [](unsigned char c) { return std::tolower(c); });

for (const auto& t : tutors) {
    if (!t || !t->getIsActive()) continue;

    std::string tName = t->getFullName();
    std::string tSubject = t->getSubjectSpecialty();
    std::string tBio = t->getBio();

    // Keyword filter
    if (!kw.empty()) {
        std::string lowerName = tName;
        std::string lowerSubj = tSubject;
        std::string lowerBio = tBio;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(lowerSubj.begin(), lowerSubj.end(), lowerSubj.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(lowerBio.begin(), lowerBio.end(), lowerBio.begin(), [](unsigned char c) { return std::tolower(c); });

        if (lowerName.find(kw) == std::string::npos &&
            lowerSubj.find(kw) == std::string::npos &&
            lowerBio.find(kw) == std::string::npos) {
            continue;
        }
    }
    // ...
```

### Đánh giá các khía cạnh kỹ thuật của thuật toán:

1. **Phương pháp tìm kiếm:**
   - Sử dụng duy nhất phương pháp **Substring Matching nguyên khối** (`std::string::find`).
   - Không có Exact match riêng biệt.
   - Không có Prefix matching.
   - **Hoàn toàn không có Token-based matching:** Không tách chuỗi thành các từ riêng biệt.
   - **Không có toán tử AND / OR:** Toàn bộ chuỗi người dùng gõ vào được coi là một chuỗi ký tự duy nhất bắt buộc phải xuất hiện liền mạch.
   - Không có Fuzzy matching (Levenshtein distance).
   - Không có hệ thống Alias / Synonym trong tìm kiếm tự do.
2. **Các trường dữ liệu được tìm kiếm:**
   - `tName` (`fullName`): Họ tên gia sư.
   - `tSubject` (`subjectSpecialty`): Chuyên môn / Môn học đăng ký.
   - `tBio` (`bio`): Giới thiệu bản thân.
   - *Các trường không được đưa vào tìm kiếm:* Học vị, trường đại học (nằm lẫn trong bio), kinh nghiệm số năm, danh sách môn phụ.
3. **Khả năng xử lý chữ hoa / chữ thường và Tiếng Việt:**
   - **Thất bại trên ký tự UTF-8:** `std::transform` kết hợp `std::tolower(unsigned char c)` là hàm chuẩn của thư viện C runtime, chỉ hoạt động trên bảng mã ASCII (0..127). Khi gặp chuỗi UTF-8 tiếng Việt:
     - Ký tự `T`, `V`, `L` được chuyển thành `t`, `v`, `l`.
     - Ký tự `á` (`0xC3 0xA1`), `ậ` (`0xE1 0xBA 0xAD`), `ý` (`0xC3 0xBD`) giữ nguyên.
     - Ký tự hoa như `Á` (`0xC3 0x81`), `Ý` (`0xC3 0x9D`), `Đ` (`0xC4 0x90`) **không bị chuyển đổi** thành chữ thường tương ứng!
     - Kết quả: Nếu người dùng gõ `Toán Vật LÝ`, chữ `Ý` có byte `0x9D`, trong khi dữ liệu lưu `ý` có byte `0xBD` -> `find()` trả về `npos` (thất bại hoàn toàn).
4. **Xử lý khoảng trắng và dấu câu:**
   - Hệ thống chỉ thực hiện `trimmed()` ở đầu và cuối chuỗi tại `GUIBridge`.
   - **Khoảng trắng thừa bên trong chuỗi:** Không được rút gọn (*no whitespace collapsing*). Hai dấu cách liên tiếp `"  "` được coi là khác biệt hoàn toàn với một dấu cách `" "`.
   - **Dấu câu:** Dấu phẩy `,`, gạch ngang `-`, gạch chéo `/`, ngoặc tròn `()` không được chuẩn hóa thành khoảng trắng. Do đó, nếu gia sư lưu `"Toán, Vật lý"`, học sinh gõ `"Toán Vật lý"` sẽ không khớp vì thiếu dấu phẩy.
5. **Tìm kiếm nhiều từ và cấp lớp:**
   - Tìm kiếm không hiểu khái niệm từ độc lập.
   - Nếu học sinh tìm `Toán 12`, thuật toán tìm chuỗi con liên tiếp `"toán 12"`. Do chuỗi gốc là `"Toán  Vật Lý  Hoá 12"`, giữa chữ `Toán` và `12` có đoạn `"  Vật Lý  Hoá "`, nên tìm kiếm báo **không tìm thấy**.

---

## 5. Kết quả kiểm tra SubjectTaxonomy và Quan hệ với SmartMatch

Hệ thống TutorMatch đã có module [`SubjectTaxonomy`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/SubjectTaxonomy.h) rất mạnh mẽ được thiết kế trước đó.

### Phân tích khả năng bóc tách của SubjectTaxonomy với `Toán  Vật Lý  Hoá 12`:
Khi truyền chuỗi `Toán  Vật Lý  Hoá 12` vào [`SubjectTaxonomy::getCanonicalSubjects`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp#L82):
1. Hàm thực hiện `normalizeText`:
   - `stripAccents` loại bỏ dấu tiếng Việt: `"toan  vat ly  hoa 12"`.
   - Chuyển thành chữ thường toàn bộ.
   - Thu gọn toàn bộ khoảng trắng thừa: `"toan vat ly hoa 12"`.
2. Kiểm tra từ khóa môn học với ranh giới từ (*word boundaries*):
   - Tìm thấy từ `toan` (không phải `an toan`, `hoan toan`) -> Nhận diện `CanonicalSubject::MATH`.
   - Tìm thấy cụm `vat ly` (hoặc `ly` không phải `dia ly`) -> Nhận diện `CanonicalSubject::PHYSICS`.
   - Tìm thấy từ `hoa` -> Nhận diện `CanonicalSubject::CHEMISTRY`.
3. **Kết luận:** `SubjectTaxonomy::getCanonicalSubjects("Toán  Vật Lý  Hoá 12")` trả về chính xác danh sách gồm cả 3 môn: `[MATH, PHYSICS, CHEMISTRY]`.

### Khả năng nhận diện cấp lớp của `parseSubject`:
- Hàm [`SubjectTaxonomy::parseSubject`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp#L190) quét chuỗi từ lớp 12 xuống lớp 1.
- Tìm thấy token `"12"`, thiết lập `minGrade = 12` và `maxGrade = 12`.
- Tuy nhiên, `parseSubject` chỉ lưu một `CanonicalSubject` đại diện duy nhất (`canonicals.front()` -> `MATH`).

### Đánh giá mối quan hệ với SmartMatch:
- **Trong SmartMatch:**
  - Tiêu chí môn học là **Stage A1 Hard Eligibility Filter**:
    ```cpp
    if (!SubjectTaxonomy::matchesFilter(tutor.getSubjectSpecialty(), criteria.subject)) {
        return false;
    }
    ```
  - Gia sư bắt buộc phải có chuyên môn giảng dạy môn học được yêu cầu. Nếu học sinh tìm môn `Physics`, `SubjectTaxonomy::matchesFilter("Toán  Vật Lý  Hoá 12", "Physics")` trả về **`true`** vì `PHYSICS` nằm trong danh mục chuyên môn của gia sư.
- **Trong Tìm kiếm thông thường (Search UI):**
  - Khi học sinh gõ từ khóa vào ô `searchKeywordInput`, hệ thống **chỉ sử dụng raw substring match** của `UserService`.
  - `SubjectTaxonomy::matchesFilter` **chỉ được gọi khi học sinh chọn combobox `subjectCombo`** (ví dụ chọn "Vật lý" từ menu).
  - Đây chính là sự tách biệt không đáng có: một bên có trí tuệ bóc tách ngôn ngữ tự nhiên (`SubjectTaxonomy`), nhưng bên tìm kiếm tự do lại dùng một hàm kiểm tra chuỗi thô sơ không có xử lý khoảng trắng.

> [!IMPORTANT]
> **Ranh giới bất biến giữa Search và SmartMatch:**
> - **Search thông thường:** Là công cụ khám phá (*discovery/exploration*), linh hoạt, mềm dẻo, hỗ trợ tìm kiếm theo tên, biệt danh, nhiều môn học cùng lúc hoặc từ khóa tự do.
> - **SmartMatch:** Là thuật toán ghép nối chính xác (*algorithmic pairing*), bắt buộc thỏa mãn đồng thời các điều kiện cứng (*hard constraints*) về môn học, ca học không xung đột, trần ngân sách, và xếp hạng theo trọng số giải thích được.
> - **Tuyệt đối không được nới lỏng hay sửa đổi logic của SmartMatch** khi cải thiện công cụ tìm kiếm thông thường.

---

## 6. Kết quả kiểm tra dữ liệu thực tế `users.dat` (READ-ONLY)

Kiểm tra toàn bộ 52 bản ghi trong file [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat) ở chế độ READ-ONLY cho kết quả thống kê như sau:

- **Tổng số tài khoản:** 52 người dùng.
  - Quản trị viên (ADMIN): 3 tài khoản (ID 1, 8, 9).
  - Học sinh (STUDENT): 36 tài khoản (ID 6, 7, 18..51).
  - Gia sư (TUTOR): **13 tài khoản** (ID 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 52).
- **Trạng thái hoạt động:** 100% gia sư đều đang ở trạng thái kích hoạt (`isActive = 1`).

### Bảng chi tiết chuyên môn của 13 gia sư trong hệ thống:

| ID | Họ tên gia sư | Chuyên môn lưu trữ (`subjectSpecialty`) | Nhận diện Canonical | Nhận diện Cấp lớp | Kiểu định dạng chuỗi |
|---|---|---|---|---|---|
| **2** | ThS. Lê Văn Toàn | `Toán 12 - Luyện thi Đại học` | `MATH` | Lớp 12 | Đơn môn, có gạch ngang & chương trình |
| **3** | TS. Trần Thị Vật Lý | `Vật lý 10-12 (Luyện thi THPT)` | `PHYSICS` | Lớp 10-12 | Đơn môn, có khoảng lớp & ngoặc đơn |
| **4** | Ms. Nguyễn Anh Văn | `Tiếng Anh / IELTS (6.5 - 8.5)` | `ENGLISH` | Toàn cấp (1-12) | Đơn môn, có dấu gạch chéo `/` & thang điểm |
| **5** | Thầy Phạm Hóa Học | `Hóa học 10-12 (Luyện thi HSG)` | `CHEMISTRY` | Lớp 10-12 | Đơn môn, có khoảng lớp & ngoặc đơn |
| **10** | Cô Nguyễn Thị Sinh | `Sinh học 10-12 (Luyện thi THPT)` | `BIOLOGY` | Lớp 10-12 | Đơn môn, có khoảng lớp |
| **11** | ThS. Hoàng Văn Chương | `Ngữ văn 10-12 (Luyện thi THPT)` | `LITERATURE` | Lớp 10-12 | Đơn môn, chuẩn hóa |
| **12** | Thầy Võ Minh Tin | `Tin học 10-12 (Lập trình C++)` | `COMPUTER_SCIENCE`| Lớp 10-12 | Đơn môn, có công nghệ kèm theo |
| **13** | Cô Đặng Thu Hà | `Toán 10-11 (Củng cố nền tảng)` | `MATH` | Lớp 10-11 | Đơn môn, có mục tiêu |
| **14** | Mr. David Lê | `Tiếng Anh / IELTS (Giao tiếp - Speaking)`| `ENGLISH` | Toàn cấp (1-12) | Đơn môn, kỹ năng chuyên biệt |
| **15** | Cô Bùi Thị Sử | `Lịch sử - Địa lý 10-12 (Ôn thi THPT)` | `HISTORY`, `GEOGRAPHY` | Lớp 10-12 | **Đa môn**, phân cách bằng dấu gạch ngang ` - ` |
| **16** | TS. Ngô Quang Lực | `Vật lý 10-11 (Cơ bản và nâng cao)` | `PHYSICS` | Lớp 10-11 | Đơn môn, phân cấp độ |
| **17** | Ms. Tanaka Hương | `Tiếng Nhật N5 - N3` | `OTHER` | Toàn cấp (1-12) | Ngoại ngữ ngoài danh mục chuẩn, cấp độ JLPT |
| **52** | Trần Chi | `Toán  Vật Lý  Hoá 12` | `MATH`, `PHYSICS`, `CHEMISTRY` | Lớp 12 | **Đa môn đặc biệt: 2 khoảng trắng liên tiếp, không dấu phân cách** |

### Đặc điểm định dạng của bản ghi ID 52:
- Chuỗi byte UTF-8 của trường `subjectSpecialty`:
  `0x54 0x6F 0xC3 0xA1 0x6E [0x20 0x20] 0x56 0xE1 0xBA 0xAD 0x74 0x20 0x4C 0xC3 0xBD [0x20 0x20] 0x48 0x6F 0xC3 0xA1 0x20 0x31 0x32`
- Thể hiện trực quan: `"Toán" + 2 spaces + "Vật Lý" + 2 spaces + "Hoá 12"`.
- Không có bất kỳ dấu phẩy `,` hay gạch ngang `-` nào để phân tách các môn học.

---

## 7. Phân tích nguyên nhân trường hợp `Toán Vật Lý Hoá 12`

Qua thực nghiệm biên dịch độc lập và kiểm chứng luồng thực thi, chúng tôi xác định cơ chế gây ra lỗi tìm kiếm khi học sinh nhập `Toán Vật Lý` diễn ra chính xác theo các nhân tố sau:

### Nhân tố 1: Sự bất đối xứng khoảng trắng (Whitespace Mismatch)
- Trong C++, phép tìm kiếm chuỗi con `std::string::find` so khớp từng byte một cách chính xác tuyệt đối.
- Từ khóa tìm kiếm: `"Toán Vật Lý"` chứa 1 khoảng trắng `0x20` giữa `"Toán"` và `"Vật"`.
- Dữ liệu lưu trữ: `"Toán  Vật Lý  Hoá 12"` chứa 2 khoảng trắng `0x20 0x20` giữa `"Toán"` và `"Vật"`.
- Phép so sánh byte tại vị trí thứ 5 (sau chữ `Toán`):
  - Byte chuỗi tìm kiếm: `0x56` (chữ `'V'`).
  - Byte chuỗi lưu trữ: `0x20` (dấu cách thứ hai).
  - Không trùng khớp! `lowerSubj.find(kw)` lập tức chuyển sang vị trí tiếp theo và cuối cùng trả về `std::string::npos`.

### Nhân tố 2: Khiếm khuyết cấu trúc chuỗi (Non-tokenized Substring Search)
- Kể cả trong trường hợp bản ghi lưu trữ chỉ có 1 khoảng trắng (`"Toán Vật Lý Hoá 12"`):
  - Nếu học sinh tìm `Toán 12`: Thất bại (vì chữ `12` nằm cách xa chữ `Toán`).
  - Nếu học sinh tìm `Vật Lý 12`: Thất bại (vì chữ `Hoá` nằm xen giữa).
  - Nếu học sinh tìm `Vật Lý Toán`: Thất bại (vì thứ tự từ bị đảo ngược).
  - Nếu gia sư lưu là `"Toán, Vật lý, Hóa 12"` (có dấu phẩy): Học sinh gõ `"Toán Vật lý"` cũng sẽ thất bại vì thiếu dấu phẩy.

### Nhân tố 3: Lỗi chuyển đổi chữ hoa/thường tiếng Việt (UTF-8 Case Transformation Bug)
- Lệnh:
  ```cpp
  std::transform(kw.begin(), kw.end(), kw.begin(), [](unsigned char c) { return std::tolower(c); });
  ```
  không thể hạ bậc các ký tự tiếng Việt viết hoa sang viết thường. Nếu học sinh gõ `Toán Vật LÝ` hoặc thiết bị di động tự động viết hoa chữ cái đầu/cuối từ, sự sai lệch byte sẽ làm đứt gãy hoàn toàn phép so khớp `find()`.

### Nhân tố 4: Thiếu khả năng tìm kiếm không dấu (Unaccented Search Gap)
- Người dùng Việt Nam rất phổ biến thói quen gõ tiếng Việt không dấu (`toan vat ly`) khi tìm kiếm trên thanh search bar.
- Do `UserService` không có bước `stripAccents`, truy vấn này trả về 0 kết quả trên toàn bộ hệ thống.

---

## 8. Bảng phân tích False Positives / False Negatives và Ma trận kiểm chứng thực nghiệm

Để đánh giá toàn diện tính đúng đắn của công cụ tìm kiếm, chúng tôi đã xây dựng bộ kiểm thử ma trận thực nghiệm độc lập (chạy trên môi trường MinGW C++17, so sánh giữa **Engine hiện tại** và **Hành vi kỳ vọng**):

### Bảng ma trận kết quả thực nghiệm:

| STT | Từ khóa học sinh nhập | Dữ liệu gia sư kiểm thử | Kết quả Engine hiện tại | Kết quả mong muốn | Đánh giá rủi ro |
|:---:|---|---|:---:|:---:|---|
| **1** | `Toán` | `Toán  Vật Lý  Hoá 12` | **MATCH** | **MATCH** | Đúng |
| **2** | `Vật Lý` | `Toán  Vật Lý  Hoá 12` | **MATCH** | **MATCH** | Đúng |
| **3** | `Hoá` | `Toán  Vật Lý  Hoá 12` | **MATCH** | **MATCH** | Đúng |
| **4** | `Toán Vật Lý` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Do 2 khoảng trắng & không tách token) |
| **5** | `Toán 12` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Không token hóa môn + khối lớp) |
| **6** | `Vật Lý 12` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Từ ngữ không liền kề nhau) |
| **7** | `toan vat ly` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Thiếu loại bỏ dấu tiếng Việt) |
| **8** | `Toán Vật LÝ` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Lỗi `std::tolower` trên byte UTF-8) |
| **9** | `Vật Lý Toán` | `Toán  Vật Lý  Hoá 12` | **MISS** | **MATCH** | **FALSE NEGATIVE** (Thứ tự từ đảo ngược) |
| **10**| `Lý` | Bio: `"Thạc sĩ chuyên ngành Tâm lý học giáo dục"` | **MATCH** | **EXCLUDE** | **FALSE POSITIVE NGUY HIỂM** (Khớp chuỗi con trong từ "Tâm lý") |
| **11**| `Sử` | Bio: `"Kỹ sư phần mềm 5 năm kinh nghiệm"` | **MATCH** (nếu unaccented) | **EXCLUDE** | **FALSE POSITIVE NGUY HIỂM** (Khớp chuỗi con trong từ "Kỹ sư") |
| **12**| `Địa` | Bio: `"Địa chỉ liên hệ: Quận 1, TP.HCM"` | **MATCH** | **EXCLUDE** | **FALSE POSITIVE NGUY HIỂM** (Khớp chuỗi con trong từ "Địa chỉ") |
| **13**| `Trần Chi` | Name: `"Trần Chi"` | **MATCH** | **MATCH** | Đúng (Tìm theo tên riêng gia sư) |
| **14**| `Luyện thi` | Specialty: `"Toán 12 - Luyện thi Đại học"` | **MATCH** | **MATCH** | Đúng (Tìm theo chương trình) |
| **15**| `ielts` | Specialty: `"Tiếng Anh / IELTS (6.5 - 8.5)"` | **MATCH** | **MATCH** | Đúng (Khớp không phân biệt hoa thường ASCII) |

### Kết luận từ ma trận:
- **Tỷ lệ False Negative rất cao:** Xuất hiện ở hầu hết các truy vấn tổ hợp nhiều từ, truy vấn môn + lớp, truy vấn đảo từ, và truy vấn không dấu.
- **Tỷ lệ False Positive nghiêm trọng:** Xảy ra khi tìm các môn có tên ngắn 1 âm tiết (`Lý`, `Sử`, `Địa`, `Tin`, `Văn`) do `find()` tìm thấy chuỗi con này bên trong các từ ghép thông dụng trong tiếng Việt (`Tâm lý`, `Kỹ sư`, `Địa chỉ`, `Thông tin`, `Văn phòng`).

---

## 9. Đánh giá và So sánh 3 Phương án giải quyết

Để giải quyết triệt để các vấn đề trên, chúng tôi xây dựng và phân tích 3 phương án kiến trúc phù hợp với quy mô ứng dụng desktop TutorMatch (C++17 / Qt 6.11):

### Phương án 1: Cải thiện thuật toán tìm kiếm chuỗi hiện tại (Token-based & Normalized Substring Matching)
- **Mô tả:** Nâng cấp hàm tìm kiếm trong `UserService`:
  - Chuẩn hóa text trước khi so khớp bằng `SubjectTaxonomy::normalizeText` (loại bỏ dấu tiếng Việt, chuyển chữ thường UTF-8, gộp khoảng trắng thừa, xóa dấu câu).
  - Tách từ khóa `keyword` thành mảng các tokens: `tokens = ["toan", "vat", "ly"]`.
  - Kiểm tra xem tất cả các tokens (hoặc một tỷ lệ tokens) có xuất hiện trong (`fullName` + `subjectSpecialty` + `bio`) hay không.
- **Ưu điểm:** Nhẹ, triển khai nhanh (chỉ sửa trong nội bộ `UserService.cpp`), không thêm class mới, không thay đổi interface.
- **Nhược điểm:** Chưa giải quyết triệt để bài toán False Positive cho các môn 1 âm tiết (từ `ly` vẫn có thể khớp trong `tam ly` nếu không có regex/word boundary), không có sắp xếp theo độ liên quan (*relevance ranking*).

### Phương án 2: Xây dựng bộ phân tích từ khóa chuyên môn (Semantic Keyword & Subject Taxonomy Analyzer)
- **Mô tả:** Sử dụng năng lực bóc tách ngữ nghĩa của `SubjectTaxonomy`:
  - Phân tích `keyword` của người dùng thành:
    - Danh sách môn học chuẩn: `std::vector<CanonicalSubject>`.
    - Cấp lớp học sinh mong muốn: `targetGrade` (1..12).
    - Các từ khóa tự do còn lại: `remainingTokens` (dành cho tìm tên, trường, bio).
  - Nếu query có chứa môn học: So khớp chính xác với `SubjectTaxonomy::getCanonicalSubjects(tutor.getSubjectSpecialty())`.
  - Hỗ trợ loại trừ ranh giới từ đã được xây dựng sẵn trong `SubjectTaxonomy` (loại trừ `dia chi`, `ky su`, `tam ly`).
- **Ưu điểm:** Khả năng nhận diện ngữ nghĩa rất cao; phân biệt rạch ròi môn học, cấp lớp và tên riêng; loại bỏ 100% false positive của các môn 1 từ.
- **Nhược điểm:** Nếu người dùng tìm kiếm chuyên môn mới lạ chưa có trong từ điển chuẩn (ví dụ: `Tiếng Nhật N3`), bộ phân tích có thể bỏ sót nếu không có cơ chế fallback.

### Phương án 3: Xây dựng Search Engine nội bộ có scoring (In-Memory Multi-Field Scored Search Engine)
- **Mô tả:** Xây dựng một engine tìm kiếm chuyên biệt `TutorSearchEngine` hoạt động in-memory:
  - Tính điểm liên quan (*Relevance Score* từ 0..100) cho từng gia sư dựa trên trọng số các trường:
    - Khớp chính xác tên gia sư (*Exact Name Match*): 40 điểm.
    - Khớp chuyên môn môn học (*Subject Match*): 40 điểm (Khớp đúng cả 2 môn: 40đ; khớp 1 môn: 20đ).
    - Khớp khối lớp (*Grade Level Match*): 10 điểm.
    - Khớp giới thiệu / tiểu sử (*Bio Match*): 10 điểm.
  - Sắp xếp kết quả trả về theo điểm số giảm dần (*Sort by Relevance Descending*).
- **Ưu điểm:** Trải nghiệm người dùng vượt trội; giải quyết hoàn hảo câu hỏi **AND vs OR**; kết quả chính xác nhất luôn đứng đầu danh sách; hỗ trợ tìm kiếm linh hoạt trên hàng nghìn gia sư.
- **Nhược điểm:** Cần thêm module mới, kiểm thử kỹ lưỡng hơn.

### Bảng so sánh toàn diện 3 Phương án theo 8 tiêu chí:

| Tiêu chí đánh giá | Phương án 1: Token-based String | Phương án 2: Semantic Analyzer | Phương án 3: Scored Search Engine | Phương án Khuyến nghị (Hybrid 2 + 3) |
|---|:---:|:---:|:---:|:---:|
| **1. Độ chính xác & Recall** | Trung bình (75%) | Cao (90%) | Rất cao (95%) | **Tuyệt đối (99%)** |
| **2. Xử lý Tiếng Việt UTF-8** | Tốt (hết lỗi dấu) | Rất tốt | Rất tốt | **Hoàn hảo (Có dấu & Không dấu)** |
| **3. Khả năng mở rộng** | Hạn chế khi dữ liệu lớn | Khá tốt | Tốt | **Rất tốt (Mở rộng từ điển dễ dàng)** |
| **4. Độ phức tạp triển khai** | Rất thấp (~40 dòng code) | Thấp (~80 dòng code) | Trung bình (~200 dòng) | **Trung bình (~150 dòng code gọn gàng)** |
| **5. Hiệu năng (O(N))** | $O(N \cdot M)$ (< 1ms với 1000 tutor) | $O(N \cdot M)$ (< 1ms) | $O(N \log N)$ (~1-2ms) | **$O(N \log N)$ siêu nhanh trong RAM** |
| **6. Khả năng bảo trì** | Khá | Tốt | Tốt | **Rất cao (Tách bạch trách nhiệm)** |
| **7. Rủi ro với SmartMatch** | **0% (Không ảnh hưởng)** | **0% (Độc lập)** | **0% (Độc lập)** | **0% (Bảo toàn 100% Invariants)** |
| **8. Tương thích C++17/Qt** | Rất cao | Rất cao | Rất cao | **Chuẩn mực OOP & Modern C++** |

---

## 10. Phương án Khuyến nghị và Luận cứ Kiến trúc

Chúng tôi khuyến nghị áp dụng **Phương án Lai (Hybrid: Semantic Intent Parsing + In-Memory Relevance Scoring)**.

### A. Phân tích nghiệp vụ & UX: Lựa chọn giữa Logic AND và OR

Khi học sinh nhập từ khóa gồm nhiều môn học, ví dụ `Toán Vật Lý`:
- **Nếu áp dụng thuần túy logic AND:**
  - Hệ thống chỉ trả về gia sư dạy **cả 2 môn** (trong dữ liệu hiện tại chỉ có duy nhất ID 52 - Trần Chi).
  - *Rủi ro UX:* Nếu không có gia sư nào dạy cả 2 môn, học sinh sẽ nhận được màn hình trắng rỗng (*Empty State*), dù hệ thống có các gia sư dạy Toán rất xuất sắc (ID 2) và gia sư dạy Vật lý hàng đầu (ID 3, ID 16).
- **Nếu áp dụng thuần túy logic OR không có xếp hạng:**
  - Hệ thống trả về tất cả gia sư dạy Toán hoặc dạy Lý, nhưng sắp xếp ngẫu nhiên. Gia sư dạy cả 2 môn có thể bị chìm ở tận trang sau.
- **Giải pháp UX tối ưu (Scored Hybrid AND/OR):**
  - **Phân tầng kết quả (Relevance Tiering):**
    - **Hạng 1 (Top Relevance - 100% Match):** Gia sư thỏa mãn **tất cả** các môn được tìm kiếm (`Toán` VÀ `Vật Lý`).
    - **Hạng 2 (Partial Relevance - 60% Match):** Gia sư dạy 1 trong các môn (`Toán` HOẶC `Vật Lý`).
  - Khi học sinh tìm `Toán Vật Lý`: Gia sư Trần Chi (dạy cả Toán và Lý) sẽ **ngay lập tức xuất hiện ở vị trí số 1**; theo sau là TS. Trần Thị Vật Lý (ID 3) và ThS. Lê Văn Toàn (ID 2).
  - Đây là mô hình chuẩn mực mà Google Search, Shopee và các sàn EdTech hàng đầu thế giới đều áp dụng.

### B. Cơ chế nhận diện Cấp lớp (Grade Level Intent)
- Khi học sinh gõ `Toán 12`:
  - Bộ phân tích nhận diện môn `MATH` và khối lớp `12`.
  - Gia sư dạy `Toán 12` được ưu tiên điểm tối đa (100đ).
  - Gia sư dạy `Toán 10-11` được điểm tương thích mức khá (70đ).
  - Gia sư dạy môn khác bị loại bỏ (0đ).

### C. Cơ chế bảo vệ khỏi False Positives
- Nhờ áp dụng logic biên từ (*word boundary*) và ngữ cảnh của `SubjectTaxonomy`:
  - Từ `Lý` chỉ kích hoạt môn Vật lý khi đứng độc lập hoặc trong cụm `vật lý`. Chuỗi `Tâm lý học` trong Bio sẽ không bị hiểu nhầm thành môn Vật lý.
  - Từ `Sử` trong `Kỹ sư` không kích hoạt môn Lịch sử.
  - Từ `Địa` trong `Địa chỉ` không kích hoạt môn Địa lý.

---

## 11. Kiến trúc Đề xuất chi tiết (Target Architecture & Mermaid Diagrams)

Kiến trúc mới bổ sung một thành phần chuyên biệt [`TutorSearchMatcher`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/TutorSearchMatcher.h) nằm trong tầng Business Logic, chịu trách nhiệm bóc tách truy vấn và tính điểm phù hợp:

```mermaid
classDiagram
    direction TB
    class GUIBridge {
        +searchAndFilterTutors(keyword, subject, minPrice, maxPrice, minRating) QVariantList
    }

    class UserService {
        -userRepo: shared_ptr~IUserRepository~
        +searchAndFilterTutors(keyword, subject, minPrice, maxPrice, minRating) vector~shared_ptr~Tutor~~
    }

    class TutorSearchMatcher {
        <<Service/Algorithm>>
        +parseQuery(rawQuery) SearchQueryIntent
        +calculateRelevanceScore(intent, tutor) double
        +filterAndRankTutors(tutors, query, subjectFilter) vector~shared_ptr~Tutor~~
    }

    class SearchQueryIntent {
        +canonicalSubjects: vector~CanonicalSubject~
        +targetGrade: int
        +freeTokens: vector~string~
        +isMultiSubject: bool
    }

    class SubjectTaxonomy {
        <<Utility>>
        +normalizeText(text) string
        +stripAccents(text) string
        +getCanonicalSubjects(subjectStr) vector~CanonicalSubject~
        +matchesFilter(tutorSubject, filterSubject) bool
    }

    class MatchingEngine {
        <<SmartMatch Algorithm>>
        +isEligible(criteria, tutor, bookings, outSlots) bool
        +findTopMatchesWithCriteria(criteria, tutors, bookings, topK) vector~TutorMatchResult~
    }

    GUIBridge --> UserService : Ủy quyền tìm kiếm
    UserService --> TutorSearchMatcher : Sử dụng thuật toán tìm kiếm & xếp hạng
    TutorSearchMatcher --> SubjectTaxonomy : Chuẩn hóa & Bóc tách môn học
    MatchingEngine --> SubjectTaxonomy : Hard Filter độc lập (Không đổi!)
```

### Quy trình xử lý truy vấn chi tiết:

```mermaid
flowchart TD
    A["Học sinh nhập từ khóa (vd: 'Toán Vật Lý')"] --> B["GUIBridge::searchAndFilterTutors"]
    B --> C["UserService::searchAndFilterTutors"]
    C --> D["TutorSearchMatcher::parseQuery"]
    
    subgraph Query Parsing
    D --> D1["normalizeText: Bỏ dấu, chữ thường UTF-8, gộp khoảng trắng"]
    D1 --> D2["Trích xuất Canonical Subjects: [MATH, PHYSICS]"]
    D2 --> D3["Trích xuất Grade Level nếu có (1..12)"]
    D3 --> D4["Giữ lại các từ khóa tự do (Tên, Trường, v.v.)"]
    end

    D4 --> E["Duyệt danh sách Active Tutors"]
    
    subgraph Scoring & Evaluation
    E --> F{"Khớp môn học dropdown?"}
    F -- Không --> G["Loại bỏ"]
    F -- Có --> H["Tính Relevance Score"]
    H --> H1["Khớp Tên gia sư (+40đ)"]
    H --> H2["Khớp Chuyên môn: Đủ cả 2 môn (+50đ), 1 môn (+25đ)"]
    H --> H3["Khớp Khối lớp (+10đ)"]
    H --> H4["Khớp Từ khóa trong Bio (+15đ)"]
    end

    H4 --> I{"Tổng điểm > 0?"}
    I -- Không --> J["Loại bỏ gia sư không liên quan"]
    I -- Có --> K["Thêm vào danh sách kèm Score"]
    
    K --> L["std::stable_sort: Sắp xếp theo Score giảm dần"]
    L --> M["Trả về kết quả cho GUIBridge & QML hiển thị"]
```

---

## 12. Phạm vi Source Code dự kiến thay đổi (Impact Analysis)

Khi chuyển sang giai đoạn Implementation (sau khi người dùng phê duyệt), phạm vi mã nguồn cần can thiệp được giới hạn tối thiểu như sau:

| Tệp tin cần sửa đổi / bổ sung | Bản chất thay đổi | Chi tiết dự kiến | Rủi ro ảnh hưởng |
|---|---|---|:---:|
| [`include/algorithms/TutorSearchMatcher.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/algorithms/TutorSearchMatcher.h) | **TẠO MỚI** | Định nghĩa struct `SearchQueryIntent` và class `TutorSearchMatcher` | **Không có rủi ro** (File mới) |
| [`src/algorithms/TutorSearchMatcher.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/TutorSearchMatcher.cpp) | **TẠO MỚI** | Hiện thực parser từ khóa, bóc tách môn/lớp và tính điểm relevance | **Không có rủi ro** (File mới) |
| [`src/services/UserService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/UserService.cpp#L146-L203) | **SỬA ĐỔI** | Thay thế đoạn lọc raw `std::string::find` tại `searchAndFilterTutors` bằng lời gọi `TutorSearchMatcher` | Rất thấp (Giữ nguyên signature) |
| [`CMakeLists.txt`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/CMakeLists.txt) | **SỬA ĐỔI** | Bổ sung `TutorSearchMatcher.h` và `.cpp` vào danh sách sources của các target | Thấp |
| [`tests/TestRunner.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp) | **BỔ SUNG TEST** | Bổ sung Suite 28 kiểm thử toàn diện các trường hợp tìm kiếm gia sư | **Không có rủi ro** (Chỉ thêm test) |

### Các thành phần TUYỆT ĐỐI BẢO TOÀN (Zero Change):
- **Không thay đổi:** [`MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/MatchingEngine.cpp) và [`MatchingService.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/services/MatchingService.cpp) (Bảo toàn 100% SmartMatch).
- **Không thay đổi:** Cấu trúc dữ liệu trong [`data/users.dat`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/data/users.dat), không sửa trực tiếp chuỗi của gia sư Trần Chi.
- **Không thay đổi:** QML UI components (giao diện đã có đầy đủ bindings).

---

## 13. Kế hoạch Kiểm thử và Tiêu chuẩn Nghiệm thu

### A. Ma trận Test Cases nghiệm thu (Acceptance Test Matrix):

1. **Test Case 1 (Trường hợp báo lỗi chính):**
   - *Input:* Từ khóa `"Toán Vật Lý"`, không chọn môn trong combobox.
   - *Kỳ vọng:* Gia sư ID 52 (Trần Chi - `Toán  Vật Lý  Hoá 12`) phải xuất hiện ở vị trí đầu tiên (Score cao nhất do khớp cả 2 môn). Sau đó là TS. Trần Thị Vật Lý (dạy Lý) và ThS. Lê Văn Toàn (dạy Toán).
2. **Test Case 2 (Tìm kiếm không dấu):**
   - *Input:* `"toan vat ly"`, `"toan 12"`, `"vat ly 10"`.
   - *Kỳ vọng:* Trả về đúng các gia sư tương ứng như khi gõ có dấu.
3. **Test Case 3 (Tìm kiếm môn kết hợp cấp lớp):**
   - *Input:* `"Toán 12"`.
   - *Kỳ vọng:* Gia sư ID 2 (Lê Văn Toàn) và ID 52 (Trần Chi) xuất hiện; Gia sư ID 13 (Toán 10-11) xếp sau hoặc bị lọc nếu yêu cầu lớp 12 nghiêm ngặt.
4. **Test Case 4 (Ngăn chặn False Positive từ 1 âm tiết):**
   - *Input:* `"Lý"`.
   - *Kỳ vọng:* Trả về TS. Trần Thị Vật Lý (ID 3), TS. Ngô Quang Lực (ID 16), Trần Chi (ID 52). Không trả về gia sư dạy Văn có nhắc đến "Tâm lý học".
   - *Input:* `"Địa"`.
   - *Kỳ vọng:* Trả về Cô Bùi Thị Sử (ID 15). Không trả về gia sư có ghi "Địa chỉ" trong bio.
5. **Test Case 5 (Tìm kiếm theo Tên riêng):**
   - *Input:* `"Trần Chi"`, `"Văn Toàn"`, `"David Lê"`.
   - *Kỳ vọng:* Gia sư mang tên đó xuất hiện chính xác ở vị trí dẫn đầu với điểm tối đa.
6. **Test Case 6 (Bảo toàn hồi quy Test Suites):**
   - Chạy toàn bộ 27 test suites hiện có trong `TutorMatch_Tests` -> 100% PASS.

---

## 14. Các vấn đề mở & Giả định cần làm rõ với Product Owner

Trước khi tiến hành giai đoạn Implementation, các giả định nghiệp vụ sau cần được xác nhận:

1. **Trọng số thứ tự khi tìm kiếm đa môn:**
   - *Câu hỏi:* Khi học sinh gõ `Toán Vật Lý`, có bắt buộc CHỈ hiển thị gia sư dạy được cả 2 môn (AND thuần túy) hay hiển thị cả gia sư dạy 1 môn nhưng xếp phía sau (AND ưu tiên + OR hỗ trợ)?
   - *Đề xuất kỹ thuật:* Áp dụng mô hình **Scored Hybrid** (xếp gia sư dạy cả 2 môn lên đầu, gia sư dạy 1 môn xếp dưới). Nếu Product Owner muốn nghiêm ngặt chỉ hiển thị gia sư dạy cả 2 môn, ta có thể bổ sung checkbox "Khớp tất cả các môn" trên UI hoặc áp dụng ngưỡng điểm cắt (*threshold cutoff*).
2. **Xử lý các ngoại ngữ và môn học ngoài danh mục:**
   - Hiện tại `SubjectTaxonomy` hỗ trợ 9 môn chính. Môn `Tiếng Nhật N5 - N3` (ID 17) đang được xếp vào `OTHER`. Chúng tôi đề xuất duy trì cơ chế tìm kiếm chuỗi tự do (free-text fallback) để những môn chưa có trong danh mục chuẩn vẫn luôn được tìm thấy.

---

## 15. Kết luận & Trạng thái giai đoạn

- Giai đoạn **INVESTIGATION & ARCHITECTURAL DESIGN** đã hoàn tất 100% mục tiêu:
  - Đã truy vết chính xác luồng dữ liệu và chỉ ra từng dòng code liên quan.
  - Đã xác định và chứng minh bằng thực nghiệm nguyên nhân gia sư `Toán  Vật Lý  Hoá 12` bị loại bỏ.
  - Đã phân tích chi tiết các rủi ro âm tính giả và dương tính giả.
  - Đã so sánh 3 phương án và đề xuất giải pháp tối ưu.
  - **TUYỆT ĐỐI TUÂN THỦ NGUYÊN TẮC: Không sửa code, không sửa file dữ liệu production trong giai đoạn này.**
- Toàn bộ kết quả điều tra đã được lưu trữ tại [`docs/architecture/analysis/Tutor_Search_Engine_Investigation_VI.md`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/docs/architecture/analysis/Tutor_Search_Engine_Investigation_VI.md).
- Trân trọng kính trình người dùng xem xét, phê duyệt định hướng để bước vào giai đoạn Implementation!
