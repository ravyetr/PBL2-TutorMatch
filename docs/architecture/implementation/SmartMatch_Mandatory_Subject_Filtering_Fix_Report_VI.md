# BÁO CÁO KỸ THUẬT: LỌC MÔN HỌC BẮT BUỘC TRƯỚC KHI XẾP HẠNG SMARTMATCH (TUTORMATCH)

> **Dự án:** TutorMatch — Hệ thống kết nối Gia sư và Học viên  
> **Phiên bản:** Phase D / E (SmartMatch Criteria-Based Ranking Enhancement)  
> **Môi trường:** Desktop Qt 6.11.2 MinGW 64-bit / C++17  
> **Tài liệu tham chiếu:** [`include/algorithms/MatchingEngine.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/algorithms/MatchingEngine.h), [`src/algorithms/MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/MatchingEngine.cpp), [`src/models/SubjectTaxonomy.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp), [`ui/qml/StudentSmartMatchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentSmartMatchView.qml)  
> **Ngày hoàn thành:** 11/11/2026

---

## 1. NGUYÊN NHÂN GỐC RỄ (ROOT CAUSE ANALYSIS)

### 1.1. Hiện tượng lỗi được báo cáo
Trước khi sửa đổi, trong tính năng **Gợi ý thông minh (SmartMatch)**:
- Một gia sư giảng dạy môn học hoàn toàn khác (ví dụ: gia sư dạy *Lịch sử* hoặc *Toán*) vẫn có thể xuất hiện trên bảng kết quả đề xuất, thậm chí xếp hạng cao hơn cả gia sư dạy môn học học viên đang tìm kiếm (ví dụ: *Vật lý*).
- Nguyên nhân là do gia sư trái môn có điểm đánh giá sao cao (5.0★), học phí rẻ hơn (50.000 VNĐ) hoặc lịch rảnh nhiều hơn, từ đó tích lũy tổng điểm xếp hạng (composite score) vượt trội so với gia sư đúng môn.
- Điều này vi phạm nghiêm trọng yêu cầu nghiệp vụ cốt lõi: **Sự tương thích môn học phải là điều kiện tiên quyết bắt buộc (Mandatory Hard Filter), tuyệt đối không phải là điểm thưởng hay trọng số xếp hạng mềm.**

### 1.2. Phân tích chi tiết luồng xử lý và các lỗ hổng kỹ thuật
Qua quá trình rà soát mã nguồn từ GUI xuống Data Access:

1. **Thứ tự kiểm tra trong `MatchingEngine::isEligible` chưa đúng chuẩn nghiệp vụ:**
   - Trước đây, thứ tự kiểm tra trong `isEligible` đặt `tutor.getIsActive()` lên trước, và điều kiện môn học chỉ được kích hoạt nếu `!criteria.subject.empty()`.
   - Nếu dữ liệu môn học của gia sư bị trống (`""`), chứa khoảng trắng, hoặc dữ liệu không hợp lệ (`"???###"`), hàm kiểm tra cũ không loại trừ dứt khoát mà có thể bỏ qua hoặc xử lý lỏng lẻo.

2. **Hạn chế trong `SubjectTaxonomy::matchesFilter` và phân tích đa môn (Multi-Subject):**
   - Hàm `parseSubject` chỉ nhận diện môn học đơn đầu tiên tìm thấy trong chuỗi (`res.code = CanonicalSubject::MATH`). Với các gia sư dạy nhiều môn (ví dụ: `"Toán, Vật lý 10-12"`, `"Toán & Tin học"`, `"Chemistry and History"`), chỉ môn đầu tiên được lưu vào `code`. Khi học viên tìm kiếm môn thứ hai (như `"Vật lý"`), phép so sánh canonical `filterParsed.code == tutorParsed.code` thất bại. Sau đó, fallback so sánh chuỗi con `normTutor.find(normFilter)` chỉ hoạt động nếu từ khóa tiếng Việt/Anh trùng khớp hoàn toàn, nhưng thất bại nếu truy vấn dùng tên tiếng Anh (`"Physics"`) trong khi hồ sơ dùng tiếng Việt (`"Vật lý"`).
   - Danh mục chuẩn `CanonicalSubject` thiếu các môn học phổ thông quan trọng như `HISTORY` (Lịch sử) và `GEOGRAPHY` (Địa lý), khiến các môn này bị gán về `CanonicalSubject::OTHER` và xử lý không nhất quán.

3. **Tách biệt chưa triệt để giữa Giai đoạn A (Lọc cứng) và Giai đoạn B (Xếp hạng):**
   - Gia sư không đủ điều kiện môn học bắt buộc phải bị loại bỏ ngay từ bước đầu tiên của Giai đoạn A, không bao giờ được cấp điểm, không được đẩy vào hàng đợi ưu tiên (`MaxHeap`), và không có cơ hội tái xuất hiện trong kết quả thông qua bất kỳ cơ chế fallback hay cộng điểm nào.

---

## 2. KIẾN TRÚC & GIẢI PHÁP THỰC HIỆN (IMPLEMENTATION)

Kiến trúc lọc và xếp hạng SmartMatch được tái cấu trúc thành 2 giai đoạn độc lập, tuần tự và nghiêm ngặt:

```
[SmartMatch Search Request: criteria.subject]
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│ STAGE A: HARD ELIGIBILITY FILTERING (Điều kiện bắt buộc)│
│                                                         │
│ 1. Môn học yêu cầu (Mandatory First Condition)          │
│    -> Gia sư dạy sai môn / trống / vô hiệu: LOẠI BỎ!   │
│ 2. Trạng thái tài khoản hoạt động (isActive == true)     │
│ 3. Khối lớp / cấp học phù hợp (Grade level fit)         │
│ 4. Giới hạn học phí tối đa (hourlyRate <= maxBudget)     │
│ 5. Trùng khớp ca học tuần (Weekly Schedule compatibility)│
│ 6. Không xung đột lịch khóa học đã duyệt (No conflicts)  │
└──────────────────────────┬──────────────────────────────┘
                           │ (Chỉ gia sư THỎA MÃN TOÀN BỘ)
                           ▼
┌─────────────────────────────────────────────────────────┐
│ STAGE B: DETERMINISTIC 100-POINT RANKING (Xếp hạng)     │
│                                                         │
│ - Schedule Availability (35%): Độ trùng khớp ca học     │
│ - Price Competitiveness (25%): Tính cạnh tranh học phí   │
│ - Bayesian Rating Score (15%): Đánh giá học viên        │
│ - Experience & Grade Fit (15%): Kinh nghiệm & khối lớp  │
│ - Credibility (10%): Hồ sơ kiểm định quản trị viên       │
│                                                         │
│ (*) Không cộng điểm thưởng môn học vì đã là ĐIỀU KIỆN! │
└──────────────────────────┬──────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│ PRIORITY QUEUE / MAX-HEAP: Top K Kết quả                │
│ (Nếu 0 gia sư -> Trả về danh sách rỗng + Thông báo UI)  │
└─────────────────────────────────────────────────────────┘
```

### 2.1. Nâng cấp bộ phân loại môn học `SubjectTaxonomy`
- Mở rộng `CanonicalSubject` bổ sung đầy đủ: `HISTORY` (Lịch sử) và `GEOGRAPHY` (Địa lý).
- Bổ sung hàm trích xuất toàn bộ môn học chuẩn hóa:
  ```cpp
  static std::vector<CanonicalSubject> getCanonicalSubjects(const std::string& subjectStr);
  ```
  Hàm này quét và bóc tách chính xác tất cả các môn học mà gia sư đăng ký (hỗ trợ cả tiếng Việt có dấu, không dấu, viết tắt, và tiếng Anh như `"Mathematics and Physics"`, `"Toán & Vật lý"`, `"Lịch sử - Địa lý"`), đồng thời bảo vệ chống nhận diện sai (ví dụ: không nhầm `"dia ly"` sang môn Lý).
- Tái cấu trúc hàm `matchesFilter`:
  ```cpp
  bool SubjectTaxonomy::matchesFilter(const std::string& tutorSubjectStr, const std::string& filterSubjectStr) {
      if (tutorSubjectStr.empty() || filterSubjectStr.empty()) return false;
      auto filterSubjects = getCanonicalSubjects(filterSubjectStr);
      if (filterSubjects.empty()) return false;
      auto tutorSubjects = getCanonicalSubjects(tutorSubjectStr);
      if (tutorSubjects.empty()) return false;

      // Gia sư đa môn đủ điều kiện nếu ít nhất 1 môn khớp môn yêu cầu
      for (const auto& fs : filterSubjects) {
          for (const auto& ts : tutorSubjects) {
              if (fs == ts) return true;
          }
      }
      return false;
  }
  ```

### 2.2. Áp dụng quy tắc lọc cứng Stage A1 trong `MatchingEngine::isEligible`
Theo đúng thứ tự bắt buộc:
1. **Stage A1 (Môn học yêu cầu):** Kiểm tra đầu tiên. Nếu `criteria.subject` trống, hoặc chuyên môn của gia sư trống/không hợp lệ, hoặc không thỏa `matchesFilter` -> Lập tức trả về `false`.
2. **Stage A2 (Trạng thái hoạt động):** `if (!tutor.getIsActive()) return false;`.
3. **Stage A3 (Khối lớp):** Kiểm tra khoảng lớp của gia sư với lớp học viên.
4. **Stage A4 (Ngân sách học phí):** `if (criteria.maxBudget > 0 && tutor.getHourlyRate() > criteria.maxBudget) return false;`.
5. **Stage A5 & A6 (Lịch học & Xung đột khóa học active):** Kiểm tra trùng khớp bitmask và loại trừ xung đột thời gian với các khóa học đã được duyệt (`ACCEPTED`).

### 2.3. Bổ sung giải trình minh bạch (Explainability)
Trong `calculateScoreWithCriteria`:
- Tự động bổ sung dòng giải trình đầu tiên xác nhận môn học được khớp:
  ```cpp
  if (!criteria.subject.empty()) {
      res.explanations.push_back("Chuyên môn phù hợp môn học yêu cầu (" + tutor.getSubjectSpecialty() + ").");
  }
  ```
- Duy trì đầy đủ các giải trình về ca học khớp, học phí, khối lớp, đánh giá sao, và chứng nhận kiểm duyệt.

### 2.4. Xử lý trường hợp không có kết quả trên giao diện QML
Trong [`ui/qml/StudentSmartMatchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentSmartMatchView.qml):
- Khi `studentSmartMatchView.matchesList.length === 0`, hiển thị thẻ cảnh báo thân thiện với thông điệp tiếng Việt chuẩn xác:
  ```qml
  Text {
      anchors.horizontalCenter: parent.horizontalCenter
      text: "Không tìm thấy gia sư phù hợp với môn học bạn đã chọn."
      font.bold: true
      font.pixelSize: 14
      color: "#d48806"
  }
  ```
- Tuyệt đối không tự ý hiển thị gia sư môn khác để lấp đầy danh sách, không âm thầm thay đổi môn học mục tiêu của học viên.

---

## 3. DANH SÁCH TẬP TIN THAY ĐỔI (FILES CHANGED)

| STT | Đường dẫn tập tin | Nội dung chỉnh sửa |
|---|---|---|
| 1 | [`include/models/SubjectTaxonomy.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/models/SubjectTaxonomy.h) | Bổ sung `CanonicalSubject::HISTORY`, `GEOGRAPHY`; khai báo hàm `getCanonicalSubjects`. |
| 2 | [`src/models/SubjectTaxonomy.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/models/SubjectTaxonomy.cpp) | Triển khai `getCanonicalSubjects` bóc tách đa môn chuẩn xác; sửa `matchesFilter` so khớp tập hợp môn học; bổ sung danh mục Lịch sử, Địa lý. |
| 3 | [`src/algorithms/MatchingEngine.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/algorithms/MatchingEngine.cpp) | Đưa lọc môn học thành bước Stage A1 bắt buộc trong `isEligible`; thêm giải trình môn học vào `explanations`. |
| 4 | [`ui/qml/StudentSmartMatchView.qml`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/ui/qml/StudentSmartMatchView.qml) | Chuẩn hóa thông báo trạng thái rỗng: `"Không tìm thấy gia sư phù hợp với môn học bạn đã chọn."`. |
| 5 | [`tests/TestRunner.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp) | Thêm Test Suite 27 (`run_test_smartmatch_mandatory_subject_filtering`) bao phủ toàn diện 10 kịch bản kiểm thử bắt buộc. |

---

## 4. KẾT QUẢ KIỂM THỬ (TEST RESULTS)

Đã bổ sung **Test Suite 27** trong [`tests/TestRunner.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp) bao phủ đầy đủ 10 yêu cầu kiểm thử được chỉ định:

1. **Kịch bản 1:** Tìm môn `Physics` (hoặc `Vật lý`); gia sư chỉ dạy `Lịch sử` (`"Lịch sử 10-12"`) -> **Bị loại bỏ ngay lập tức (assert !isEligible).**
2. **Kịch bản 2:** Tìm môn `Physics`; gia sư dạy `Vật lý` (`"Vật lý 10-12"`) -> **Đủ điều kiện (assert isEligible).**
3. **Kịch bản 3:** Tìm môn `Physics`; gia sư dạy đa môn `Toán & Vật lý` (hoặc `Mathematics and Physics`) -> **Đủ điều kiện (assert isEligible).**
4. **Kịch bản 4:** Tìm môn `Physics`; gia sư dạy `Chemistry and History` (hoặc `Hóa học & Lịch sử`) -> **Bị loại bỏ hoàn toàn (assert !isEligible).**
5. **Kịch bản 5 (Xếp hạng với gia sư xuất sắc khác môn):**
   - Gia sư Sử (ID 107): Đánh giá 5.0★, học phí siêu rẻ 50.000 VNĐ, rảnh 100% thời gian (56/56 ca).
   - Gia sư Lý (ID 108): Đánh giá khiêm tốn 4.0★, học phí 200.000 VNĐ, chỉ rảnh 8 ca.
   - Tìm kiếm: Môn `Vật lý`.
   - Kết quả: **Chỉ có Gia sư Lý (ID 108) được trả về trong danh sách kết quả! Gia sư Sử (ID 107) bị loại hoàn toàn từ Stage A, không bao giờ xuất hiện dù có điểm số các tiêu chí khác cao vượt trội.**
6. **Kịch bản 6:** Không có gia sư nào dạy môn yêu cầu (`Sinh học` trong nhóm chỉ có Toán và Lý) -> **Trả về danh sách rỗng (size == 0).**
7. **Kịch bản 7:** Gia sư có chuỗi chuyên môn rỗng (`""`) hoặc chỉ chứa khoảng trắng (`"   "`) -> **Bị loại bỏ an toàn.**
8. **Kịch bản 8:** Gia sư có chuyên môn dị thường/không xác định (`"???###InvalidXYZ"`), hoặc tìm kiếm tiêu chí môn không xác định (`"???###UnknownCriteria"`) -> **Bị loại bỏ an toàn.**
9. **Kịch bản 9:** Tính nhất quán giữa `GUIBridge`, `MatchingService` và `MatchingEngine` -> **Khớp 100% danh sách gia sư và thứ tự xếp hạng; giải trình chứa xác nhận môn học tương thích.**
10. **Kịch bản 10:** Toàn bộ 26 bộ kiểm thử hồi quy trước đó tiếp tục vượt qua 100%.

### Nhật ký chạy TestRunner thực tế:
```
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine...
  -> PASS: MatchingEngine seamlessly adopted CustomRatingOnlyMatchingStrategy!
...
[TEST] 25. Verifying Student Profile Simplification & SmartMatch Criteria-Based Ranking...
  -> PASS: Student Profile Simplification & SmartMatch Criteria-Based Ranking verified 100%!
[TEST] 26. Verifying Persistence Schema & Migration...
  -> PASS: Persistence Schema & Migration verified 100%!
[TEST] 27. Verifying SmartMatch Mandatory Subject Filtering Before Ranking...
  -> PASS: SmartMatch Mandatory Subject Filtering Before Ranking verified 100%!
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

---

## 5. VÍ DỤ ĐỐI CHIẾU TRƯỚC VÀ SAU KHI SỬA (BEFORE / AFTER COMPARISON)

### Kịch bản A: Học viên tìm gia sư môn "Vật lý", lớp 10, ngân sách 200.000 VNĐ

| Tiêu chí | Trước khi sửa (Lỗi) | Sau khi sửa (Đã khắc phục chuẩn) |
|---|---|---|
| **Hạng #1** | **Cô Bùi Thị Sử (ID 15)** — Môn: *Lịch sử - Địa lý*<br>• Học phí: 90.000 VNĐ (Rẻ nhất)<br>• Đánh giá: 4.5★<br>⚠️ *Sai môn hoàn toàn nhưng đứng đầu do học phí rẻ!* | **TS. Trần Thị Vật Lý (ID 3)** — Môn: *Vật lý 10-12*<br>• Học phí: 200.000 VNĐ<br>• Đánh giá: 4.0★<br>✅ Đúng môn Vật lý yêu cầu. |
| **Hạng #2** | **ThS. Lê Văn Toàn (ID 2)** — Môn: *Toán 12*<br>• Học phí: 150.000 VNĐ<br>• Đánh giá: 5.0★ (2 đánh giá)<br>⚠️ *Gia sư dạy Toán nhưng lọt vào danh sách tìm Vật lý!* | **TS. Ngô Quang Lực (ID 16)** — Môn: *Vật lý 10-11*<br>• Học phí: 170.000 VNĐ<br>• Đánh giá: 5.0★ (1 đánh giá)<br>✅ Đúng môn Vật lý yêu cầu. |
| **Gia sư khác môn** | Xuất hiện tràn lan nếu có điểm sao cao hoặc giá rẻ. | **BỊ LOẠI HOÀN TOÀN TỪ VÒNG LỌC CỨNG.** Tuyệt đối không lọt vào danh sách kết quả. |

### Kịch bản B: Gia sư dạy đa môn "Toán & Vật lý" (Multi-subject tutor)
- Khi tìm `"Vật lý"`: Gia sư **HỢP LỆ** và được xếp hạng.
- Khi tìm `"Toán"`: Gia sư **HỢP LỆ** và được xếp hạng.
- Khi tìm `"Hóa học"`: Gia sư **BỊ LOẠI BỎ** dứt khoát.

### Kịch bản C: Không có gia sư nào dạy môn yêu cầu
- Trước đây: Hệ thống có thể fallback gợi ý các gia sư môn khác có điểm tổng hợp cao.
- Sau khi sửa: Trả về danh sách rỗng `[]`, giao diện hiển thị thông báo rõ ràng:  
  `"Không tìm thấy gia sư phù hợp với môn học bạn đã chọn."`

---

## 6. XÁC MINH GIAO DIỆN & BUILD WORKFLOW

Tất cả 4 target trong project đều được biên dịch và liên kết thành công với mã thoát 0 (Exit Code 0) trên Qt Creator / MinGW:
1. `TutorMatch_GUI.exe` — [PASS - 100% Build]
2. `TutorMatch_CLI.exe` — [PASS - 100% Build]
3. `TutorMatch_Tests.exe` — [PASS - 27/27 Suites PASS]
4. `TutorMatch_Inspector.exe` — [PASS - 100% Build]

Dữ liệu sản xuất `data/users.dat` (50 tài khoản), `data/bookings.dat` và `data/reviews.dat` được bảo toàn nguyên vẹn tính toàn vẹn và cấu trúc dữ liệu, không có bất kỳ sự xáo trộn hoặc giả mạo thông tin nào.

---

## 7. KẾT LUẬN & GIỚI HẠN CÒN LẠI (LIMITATIONS)

- **Mục tiêu hoàn thành:** Yêu cầu lọc môn học bắt buộc trước khi xếp hạng đã được triển khai triệt để và chứng minh qua 27 bộ kiểm thử tự động.
- **Giới hạn còn lại:** 
  - Hệ thống hiện tại phân loại môn học dựa trên danh mục môn học phổ thông chuẩn hóa (Toán, Lý, Hóa, Sinh, Anh, Văn, Tin, Sử, Địa). Các môn ngoại ngữ chuyên sâu ngoài Tiếng Anh (như Tiếng Nhật N5-N3 của Tutor ID 17) hiện được xếp vào nhóm `OTHER` và chưa nằm trong danh sách chọn nhanh của bộ lọc SmartMatch phổ thông. Trong tương lai nếu mở rộng sang phân hệ Ngoại ngữ / Năng khiếu, có thể mở rộng thêm enum môn học tương ứng.
