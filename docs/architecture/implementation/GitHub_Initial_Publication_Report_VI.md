# BÁO CÁO XUẤT BẢN DỰ ÁN LÊN GITHUB (GITHUB INITIAL PUBLICATION REPORT)
## TutorMatch | C++17 | Qt 6.11 | QML | Flat-File Persistence
**Mã tài liệu:** `TUTORMATCH-PUB-20261110-01`  
**Dự án:** TutorMatch (Hệ thống kết nối gia sư và quản lý thời khóa biểu trực tuyến)  
**Workspace:** `C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch`  
**Thời điểm thực hiện:** 10/11/2026  
**Tác giả thực hiện:** Senior C++/Qt Software Auditor & Release Engineer  

---

## 1. KẾT QUẢ XUẤT BẢN (PUBLICATION RESULT)

### **TRẠNG THÁI: SUCCESS (XUẤT BẢN THÀNH CÔNG 100%)**

- **Kho lưu trữ GitHub (Repository URL):** [https://github.com/ravyetr/PBL2-TutorMatch](https://github.com/ravyetr/PBL2-TutorMatch)
- **Chế độ hiển thị (Visibility):** Public (Công khai)
- **Nhánh đích (Target Branch):** `main`
- **Mã băm Commit gốc (Root Commit Hash):** `1508e6965ffb7c01b13e11b2a1211a3729084819` (`1508e69`)
- **Thông điệp Commit (Commit Message):** `Initial release: TutorMatch PBL2 project`
- **Tác giả Commit (Author):** `ravyetr <hongocyentrang83@gmail.com>`
- **Số lượng tệp được xuất bản:** **125 tệp** (36,420 dòng mã/tài liệu)

---

## 2. CẤU TRÚC THƯ MỤC TRÊN GITHUB (REPOSITORY DIRECTORY STRUCTURE)

Toàn bộ mã nguồn dự án được đặt trực tiếp tại thư mục gốc của repository theo chuẩn quy ước C++/Qt:

```
PBL2-TutorMatch/
├── .gitignore                   # Cấu hình loại trừ tệp biên dịch, thư viện, dữ liệu nhạy cảm
├── CMakeLists.txt               # Tập lệnh cấu hình build hệ thống 4 targets
├── README.md                    # Tài liệu giới thiệu, hướng dẫn biên dịch và vận hành (English)
├── data/
│   └── .gitkeep                 # Neo giữ thư mục lưu trữ dữ liệu trống trên GitHub
├── docs/                        # Tài liệu kiến trúc hệ thống, ADRs và báo cáo kiểm toán
│   └── architecture/
│       ├── decisions/           # 9 Architecture Decision Records (ADR-001 đến ADR-009)
│       ├── design/              # Đặc tả thiết kế hệ thống chi tiết (Design Specifications)
│       └── implementation/      # Báo cáo thực thi, kiểm toán dữ liệu và nghiệm thu
├── include/                     # File tiêu đề C++ (.h)
│   ├── algorithms/              # Thuật toán MatchingEngine và các chiến lược OCP
│   ├── ds/                      # Cấu trúc dữ liệu tự tạo (ScheduleGuard, AVLTree, HashTable, ...)
│   ├── gui/                     # GUIBridge tầng trung gian kết nối QML với Backend
│   ├── models/                  # Domain Models (User, Student, Tutor, Admin, CourseBooking, Review)
│   ├── services/                # Các dịch vụ nghiệp vụ (BookingService, AuthService, ReviewService, ...)
│   ├── storage/                 # Interfaces IRepository, MockDatabaseEngine, DataFileInspector
│   │   ├── repositories/        # File-based repositories
│   │   └── serializers/         # CSV Entity Serializers tương thích ngược
│   ├── ui/                      # Giao diện ConsoleApp CLI
│   └── utils/                   # Tiện ích thời gian DateUtils
├── scripts/                     # Kịch bản Python hỗ trợ di chuyển và kiểm tra dữ liệu
│   ├── archive_legacy_bookings.py
│   └── migrate_and_audit.py
├── src/                         # File mã nguồn triển khai C++ (.cpp)
│   ├── algorithms/
│   ├── ds/
│   ├── gui/
│   ├── models/
│   ├── services/
│   ├── storage/
│   ├── tools/                   # inspect_main.cpp (TutorMatch_Inspector)
│   ├── ui/                      # ConsoleApp.cpp
│   ├── utils/
│   └── main.cpp                 # Điểm khởi chạy CLI
├── tests/
│   └── TestRunner.cpp           # Bộ kiểm thử tự động 24 suites (SOLID & E2E Workflows)
└── ui/
    └── qml/                     # 26 tệp QML và qmldir cho giao diện máy tính Desktop GUI
```

---

## 3. DANH MỤC CÁC TỆP ĐƯỢC PHÉP XUẤT BẢN & PHÂN LOẠI

| Phân hệ (Component) | Số lượng tệp | Nội dung chi tiết |
| :--- | :---: | :--- |
| **Root Configurations** | 3 | `.gitignore`, `CMakeLists.txt`, `README.md` |
| **C++ Header Files (`include/`)** | 33 | Đầy đủ định nghĩa lớp, DTOs, interfaces, cấu trúc dữ liệu, dịch vụ |
| **C++ Source Files (`src/`)** | 26 | Mã nguồn triển khai chi tiết các dịch vụ, repository, GUIBridge, CLI |
| **QML UI Views & Assets (`ui/`)** | 27 | 26 tệp view/modal/theme QML và tệp `qmldir` |
| **Automated Tests (`tests/`)** | 1 | `TestRunner.cpp` chứa 24 test suites toàn diện |
| **Architecture Documentation (`docs/`)** | 32 | Toàn bộ 9 ADRs, thiết kế kiến trúc và báo cáo kiểm toán trước đó |
| **Utility Scripts (`scripts/`)** | 2 | Các công cụ script kiểm tra và di chuyển dữ liệu |
| **Data Directory Anchor (`data/`)** | 1 | `data/.gitkeep` để giữ cấu trúc thư mục rỗng |
| **TỔNG CỘNG** | **125 tệp** | **Không có tệp thừa, không thiếu tệp nguồn** |

---

## 4. DANH MỤC CÁC TỆP BỊ LOẠI TRỪ VÀ LÝ DO (INTENTIONALLY EXCLUDED FILES)

Tuân thủ nghiêm ngặt quy tắc an toàn dữ liệu và chuẩn phát hành phần mềm:

| Danh mục loại trừ | Các tệp / Thư mục cụ thể | Lý do loại trừ bắt buộc |
| :--- | :--- | :--- |
| **Dữ liệu người dùng thực tế & lịch sử** | `data/*.dat` (`users.dat`, `course_bookings.dat`, `reviews.dat`), `data/archive/`, `data/backup_*/`, `data/staging_*/` | **Tuyệt đối không đưa dữ liệu hoạt động, thông tin cá nhân, mật khẩu, đơn đặt lịch thực tế lên GitHub công khai.** Hệ thống đã có cơ chế tự động khởi tạo dữ liệu mẫu sạch (`MockDatabaseEngine::seedDefaultDataIfEmpty()`). |
| **Thư mục Build & Tệp trung gian** | `build/`, `*.obj`, `*.o`, `*.ninja`, `CMakeFiles/` | Tệp sinh ra trong quá trình biên dịch MinGW cục bộ, dung lượng lớn (>530 MB), phụ thuộc vào môi trường máy tính cụ thể. |
| **Tệp nhị phân thực thi & Thư viện** | `*.exe` (`TutorMatch_GUI.exe`, `TutorMatch_CLI.exe`, `TutorMatch_Tests.exe`, `TutorMatch_Inspector.exe`), `*.dll` (34 tệp DLL Qt và MinGW ở thư mục gốc) | Nguyên tắc Git không lưu trữ binaries đã biên dịch. Người dùng khác sẽ tự build từ mã nguồn theo hướng dẫn trong `README.md`. |
| **Tài nguyên Qt triển khai (Deployment Artifacts)** | `generic/`, `iconengines/`, `imageformats/`, `networkinformation/`, `platforms/`, `qml/`, `qmltooling/`, `tls/`, `translations/` | Các plugin động sinh ra từ lệnh `windeployqt` trên máy Windows cục bộ (hơn 1,300 tệp). |
| **Cấu hình IDE Cục bộ** | `.qtcreator/`, `CMakeLists.txt.user*` | Chứa đường dẫn tuyệt đối của máy tính cá nhân (`C:\Users\DELL\...`), gây xung đột nếu nhà phát triển khác mở dự án. |
| **Tệp nháp & Log phát triển** | `scratch/`, `*.log`, `*.tmp`, `test_*_data/` | Các tập lệnh thử nghiệm tạm thời và tệp dữ liệu sandbox sinh ra khi chạy test. |

---

## 5. KẾT QUẢ KIỂM TRA DỮ LIỆU NHẠY CẢM (SENSITIVE-DATA INSPECTION)

Đã thực hiện quét kiểm tra tự động trước khi commit:
1. **Kiểm tra khóa bí mật & Access Tokens:** Quét toàn bộ staged diff với regex kiểm tra API keys, GitHub Personal Access Tokens (`ghp_*`), Private Keys (`BEGIN RSA/OPENSSH PRIVATE KEY`).
   - **Kết quả:** Không tìm thấy bất kỳ mã khóa bí mật hoặc token truy cập nào.
2. **Kiểm tra tệp dữ liệu `.dat`:** Lệnh `git diff --cached --name-only` xác nhận không có bất kỳ tệp đuôi `.dat` nào lọt vào chỉ mục commit.
3. **Kiểm tra thông tin định danh tác giả (Git Author):**
   - Tên hiển thị: `ravyetr`
   - Email: `hongocyentrang83@gmail.com`
   - Khớp 100% với tài khoản sở hữu repository đích `https://github.com/ravyetr/PBL2-TutorMatch`.

---

## 6. KẾT QUẢ KIỂM TRA BIÊN DỊCH VÀ KIỂM THỬ (BUILD/TEST BASELINE)

- **Cơ sở kiểm thử đã xác minh (Baseline):** Dự án đã đạt chuẩn nghiệm thu kỹ thuật tại commit kiểm toán độc lập `TUTORMATCH-VERIF-20261109-01` với 24/24 Test Suites PASS (100% SUCCESS) và tính toàn vẹn 73/73 bản ghi dữ liệu hợp lệ.
- **Ghi chú về ràng buộc thời gian (Time-Dependent Test Invariant):**
  - Trong bộ kiểm thử `TestRunner.cpp`, một số ca kiểm thử tĩnh sử dụng ngày cố định `2026-11-09` để kiểm tra chức năng đặt lịch. Khi đồng hồ hệ thống bước sang ngày mới `2026-11-10`, logic nghiệp vụ thắt chặt của `BookingService` (`DateUtils::compareDates(startDate, today) < 0`) đã kích hoạt chính xác để từ chối ngày trong quá khứ.
  - Tuân thủ quy tắc nghiêm ngặt: **Không sửa mã nguồn ứng dụng hay assertion bài test chỉ để ép pass khi xuất bản GitHub**, mã nguồn được giữ nguyên vẹn ở trạng thái nghiệm thu gốc. Trong `README.md`, hiện tượng này đã được công khai minh bạch tại mục *"Known Technical Limitations"*.

---

## 7. BẰNG CHỨNG XÁC THỰC SAU KHI PUSH (POST-PUSH VERIFICATION)

Lệnh kiểm tra truy vấn remote repository:
```bash
git ls-remote origin
```
Kết quả trả về từ GitHub:
```text
1508e6965ffb7c01b13e11b2a1211a3729084819    HEAD
1508e6965ffb7c01b13e11b2a1211a3729084819    refs/heads/main
```

Kiểm tra cây tệp remote `origin/main` (`git ls-tree -r --name-only origin/main`):
- Tổng cộng: **125 tệp**.
- Có mặt đầy đủ: `README.md`, `.gitignore`, `CMakeLists.txt`, `data/.gitkeep`.
- Không có bất kỳ tệp `.dat`, `.dll`, `.exe` hay thư mục `build/`, `scratch/`, `.qtcreator/`.
- Repository công khai có thể truy cập ngay tại: [https://github.com/ravyetr/PBL2-TutorMatch](https://github.com/ravyetr/PBL2-TutorMatch).

---

## 8. HƯỚNG DẪN DÀNH CHO NGƯỜI DÙNG KHI SỬ DỤNG LẠI

1. **Clone repository về máy mới:**
   ```bash
   git clone https://github.com/ravyetr/PBL2-TutorMatch.git
   cd PBL2-TutorMatch
   ```
2. **Biên dịch với CMake:**
   ```bash
   cmake -B build -S . -G "MinGW Makefiles"
   cmake --build build -j4
   ```
3. **Khởi chạy ứng dụng:**
   - Ứng dụng khi chạy lần đầu sẽ tự động phát hiện thư mục `data/` chưa có dữ liệu và kích hoạt cơ chế `seedDefaultDataIfEmpty()` để tạo tài khoản mẫu (`admin`, `tutor_math`, `student_nam`).
