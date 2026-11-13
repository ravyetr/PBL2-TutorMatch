# DANH MỤC 30 BỘ KIỂM THỬ TỰ ĐỘNG (TEST SUITES INVENTORY)
## Khảo Sát Chi Tiết Toàn Bộ 30 Test Suites, Phạm Vi Kiểm Thử, Điều Kiện Khẳng Định và Kết Quả Thực Thi

---

## 1. TỔNG QUAN DANH MỤC KIỂM THỬ TOÀN DIỆN `[VERIFIED]`

Tệp kiểm thử duy nhất [`tests/TestRunner.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp) bao gồm **30 Test Suites độc lập**, bao phủ toàn bộ 3 tầng kiến trúc, từ cấu trúc dữ liệu cơ sở đến các kịch bản người dùng đầu cuối.

| Suite # | Tên hàm kiểm thử trong mã nguồn | Dòng bắt đầu | Trọng tâm kiểm định | Trạng thái |
| :---: | :--- | :---: | :--- | :---: |
| **01** | `run_test_ocp` | 292 | Khả năng mở rộng chiến lược ghép đôi `IMatchingStrategy` tại runtime. | **100% PASS** |
| **02** | `run_test_lsp` | 315 | Tính khả thay thế của cây kế thừa `User` (`Student`, `Tutor`, `Admin`). | **100% PASS** |
| **03** | `run_test_dip_isp_and_srp` | 347 | Sự tuân thủ DIP/ISP/SRP khi tiêm `FakeUserRepository` vào các Services. | **100% PASS** |
| **04** | `run_test_schedule_guard` | 440 | Đại số bitmask 56 ca, phát hiện xung đột và đếm số ca trùng khớp. | **100% PASS** |
| **05** | `run_test_sidebar_labels` | 465 | Tính nhất quán của nhãn điều hướng và chuỗi vai trò người dùng. | **100% PASS** |
| **06** | `run_test_tutor_profile_editing` | 514 | Chỉnh sửa hồ sơ gia sư, cập nhật học phí, chuyên môn và tiểu sử. | **100% PASS** |
| **07** | `run_test_schedule_synchronization` | 605 | Đồng bộ hóa thời khóa biểu đa người dùng và chuyển đổi ca rảnh. | **100% PASS** |
| **08** | `run_test_admin_user_data_contract` | 736 | Hợp đồng dữ liệu người dùng phục vụ bảng điều hành của Admin. | **100% PASS** |
| **09** | `run_test_admin_user_detail_modal_contract` | 806 | Hợp đồng dữ liệu chi tiết người dùng trên hộp thoại modal của Admin. | **100% PASS** |
| **10** | `run_test_admin_verification_and_lock_sync` | 860 | Đồng bộ hóa trạng thái khóa/mở khóa tài khoản người dùng (`isActive`). | **100% PASS** |
| **11** | `run_test_admin_service_filters_and_search` | 953 | Bộ lọc người dùng theo vai trò (`STUDENT`, `TUTOR`, `LOCKED`) và từ khóa. | **100% PASS** |
| **12** | `run_test_avl_tree_multi_value_and_deletion` | 981 | Cây AVL: chèn trùng khóa, truy vấn khoảng và cân bằng sau khi xóa. | **100% PASS** |
| **13** | `run_test_schedule_guard_sanitization_and_56_bit_boundary` | 1051 | Khử nhiễu bit ma ngoài phạm vi 56 bit qua `VALID_SCHEDULE_MASK`. | **100% PASS** |
| **14** | `run_test_mock_db_atomic_persistence_and_deduplication` | 1096 | Tính bền vững nguyên tử và chống trùng lặp bản ghi trong Mock DB. | **100% PASS** |
| **15** | `run_test_data_file_inspector` | 1182 | Công cụ thanh tra pháp chứng đọc-chỉ cấu trúc 3 tệp `.dat`. | **100% PASS** |
| **16** | `run_test_booking_conflict_detection` | 1259 | Thuật toán phát hiện xung đột lịch đơn ca và đa ca theo khoảng ngày. | **100% PASS** |
| **17** | `run_test_entity_serializers` | 1406 | Đọc ghi CSV chuẩn xác cho `User`, `CourseBooking`, `Review`. | **100% PASS** |
| **18** | `run_test_concrete_file_repositories` | 1700 | Thao tác I/O tệp thực tế của 3 kho lưu trữ `File...Repository`. | **100% PASS** |
| **19** | `run_test_phase3_end_to_end_scenarios` | 1876 | Kịch bản đầu cuối: Đăng ký $\to$ Đặt lịch $\to$ Duyệt $\to$ Hoàn tất $\to$ Đánh giá. | **100% PASS** |
| **20** | `run_test_recurring_course_booking` | 2215 | Vòng đời khóa học định kỳ nhiều tuần (4, 8, 12 tuần). | **100% PASS** |
| **21** | `run_test_timetable_recurring_course_integration` | 2494 | Mở rộng ca học tuần và hợp nhất động trên ma trận thời khóa biểu. | **100% PASS** |
| **22** | `run_test_data_audit_cleanup_and_migration` | 2713 | Dọn dẹp dữ liệu rác, di chuyển cấu trúc dữ liệu và kiểm toán toàn vẹn. | **100% PASS** |
| **23** | `run_test_multi_slot_course_and_consensual_termination` | 3013 | Lịch học đa ca ("dow:slot;dow:slot") và quy trình đồng thuận kết thúc sớm. | **100% PASS** |
| **24** | `run_test_commitment_acknowledgment_and_review_matrix` | 3146 | Điều khoản cam kết `COMMITMENT_V1` và ma trận điều kiện đánh giá. | **100% PASS** |
| **25** | `run_test_student_profile_and_smartmatch_criteria_ranking` | 3360 | Thuật toán gợi ý SmartMatch 100 điểm giải trình (35/25/15/15/10). | **100% PASS** |
| **26** | `run_test_persistence_schema_and_migration` | 3560 | Tương thích ngược schema CSV (từ 13 cột lên 23 cột khóa học). | **100% PASS** |
| **27** | `run_test_smartmatch_mandatory_subject_filtering` | 3786 | Ràng buộc lọc môn học bắt buộc trong `isEligible()`, zero false positives. | **100% PASS** |
| **28** | `run_test_tutor_search_engine_normalization_semantic_and_ranking` | 3958 | Động cơ tìm kiếm gia sư `TutorSearchMatcher` 4 tầng độ liên quan. | **100% PASS** |
| **29** | `run_test_tutor_verification_decommission` | 4162 | Xác nhận loại bỏ hoàn toàn thuộc tính xác thực gia sư khỏi hệ thống. | **100% PASS** |
| **30** | `run_test_generic_data_structures_invariants` | 4256 | Bất biến giải thuật của HashTable, AVLTree, LRUCache, MaxHeap, List. | **100% PASS** |

---

## 2. ĐẶC TẢ CHI TIẾT CÁC BỘ KIỂM THỬ NỔI BẬT `[VERIFIED]`

### 2.1. Test Suite 24: Ma Trận Cam Kết V1 & Điều Kiện Đánh Giá
- **Vị trí mã nguồn:** [`tests/TestRunner.cpp:3146-3358`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp#L3146)
- **Kịch bản kiểm thử:**
  1. Thử duyệt khóa học khi chỉ có Gia sư xác nhận cam kết $\implies$ Phải từ chối.
  2. Thử duyệt khóa học khi chỉ có Học viên xác nhận cam kết $\implies$ Phải từ chối.
  3. Cả hai cùng xác nhận cam kết `COMMITMENT_V1` $\implies$ Duyệt thành công, chuyển sang `ACCEPTED`.
  4. Thử gửi đánh giá khi khóa học đang `PENDING` hoặc `ACCEPTED` $\implies$ Bị từ chối.
  5. Khóa học chuyển sang `COMPLETED` hoặc `EARLY_TERMINATED` $\implies$ Cho phép gửi đánh giá, cập nhật rating gia sư và ghi nhận giao dịch bù nếu lỗi.

---

### 2.2. Test Suite 27: Ràng Buộc Môn Học Bắt Buộc Trong SmartMatch
- **Vị trí mã nguồn:** [`tests/TestRunner.cpp:3786-3956`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp#L3786)
- **Kịch bản kiểm thử:**
  - Tạo ứng viên gia sư có điểm số hoàn hảo về lịch (khớp 100%), học phí siêu rẻ (25/25 điểm), số sao tuyệt đối (5.0 sao), nhưng chuyên môn dạy "Vật lý".
  - Học viên tìm kiếm môn "Toán học".
  - Khẳng định: `MatchingEngine::isEligible` phải trả về `false`, gia sư hoàn toàn bị loại bỏ ngay từ Stage 1, không được xuất hiện trong danh sách kết quả dù tổng điểm các tiêu chí khác rất cao.

---

### 2.3. Test Suite 29: Chứng Thực Loại Bỏ Xác Thực Gia Sư (Decommission Verification)
- **Vị trí mã nguồn:** [`tests/TestRunner.cpp:4162-4254`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/tests/TestRunner.cpp#L4162)
- **Kịch bản kiểm thử:**
  - Kiểm tra hàm khởi tạo của `Tutor`: Không còn tham số `isVerified`.
  - Kiểm tra `UserSerializer`: Dòng CSV gia sư chỉ chứa chính xác 14 trường dữ liệu, không có trường xác thực nào.
  - Kiểm tra `AdminService`: Không còn phương thức duyệt xác thực hay toggle xác thực gia sư.
