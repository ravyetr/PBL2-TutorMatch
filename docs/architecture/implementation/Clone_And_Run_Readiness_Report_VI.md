# BÁO CÁO HOÀN THIỆN TÍNH NĂNG CLONE-AND-RUN TRÊN GITHUB CHO DỰ ÁN TUTORMATCH

---

## 1. Mục tiêu và Bối cảnh

### 1.1. Mục tiêu nhiệm vụ
Chuẩn bị và hoàn thiện toàn bộ mã nguồn, cấu hình xây dựng (CMake, CMakePresets) và cơ chế khởi tạo dữ liệu mẫu cho repository GitHub của dự án TutorMatch:
`https://github.com/ravyetr/PBL2-TutorMatch`

Mục tiêu cốt lõi: **Đảm bảo tính sẵn sàng 100% cho trải nghiệm "Clone-and-Run"**. Bất kỳ sinh viên, giảng viên hoặc cộng tác viên nào khi thực hiện:
1. `git clone` mã nguồn từ GitHub về máy tính cá nhân.
2. Khởi động Qt Creator và mở tệp `CMakeLists.txt`.
3. Lựa chọn bộ công cụ Kit chuẩn (ví dụ `Desktop Qt 6.11.2 MinGW 64-bit`).
4. Nhấn **Build** và nhấn **Run** (chạy target mặc định `TutorMatch_GUI`).
5. Ngay lập tức mở ra giao diện TutorMatch hoàn chỉnh, hiển thị đầy đủ dữ liệu mẫu chuẩn hóa (52 người dùng, 23 khóa học, 15 đánh giá), không gặp bất kỳ lỗi thiếu tệp, lỗi linker, hay yêu cầu cấu hình thủ công nào.

---

## 2. Kiểm tra Độc lập & Cấu hình Di động (Portability Audit)

### 2.1. Loại bỏ hoàn toàn đường dẫn cục bộ (Zero Hardcoded Paths)
Đã tiến hành rà soát tự động và thủ công toàn bộ cây mã nguồn (`src/`, `include/`, `ui/`, `tests/`, `CMakeLists.txt`):
- Không có bất kỳ đường dẫn tuyệt đối nào liên quan đến máy tính phát triển (ví dụ: `C:\Users\...`, `D:\CODE\...`).
- Cơ chế phân giải đường dẫn dữ liệu dựa trên lớp tiện ích `ApplicationPaths`:
  - Trong chế độ phát triển: Sử dụng macro biên dịch `PROJECT_SOURCE_DIR` được truyền động từ CMake (`target_compile_definitions`).
  - Trong chế độ thực thi độc lập: Tự động phân giải thư mục `data/` và `data/seed/` liền kề tệp thực thi hoặc trong các thư mục tổ tiên (hỗ trợ tối đa 4 cấp thư mục cha).

### 2.2. Chuẩn hóa CMake & CMakePresets.json
- Tệp `CMakeLists.txt` tương thích hoàn toàn từ CMake 3.16 trở lên, hỗ trợ cả Qt 6.2 LTS đến Qt 6.11+ mới nhất.
- Bổ sung tệp `CMakePresets.json` chuẩn v3 với 2 preset cấu hình:
  - `default`: Cấu hình Debug với generator `MinGW Makefiles`.
  - `release`: Cấu hình Release tối ưu hóa tốc độ chạy.
  - Cả hai preset không chứa đường dẫn môi trường cá nhân, sử dụng biến quy ước tiêu chuẩn `${sourceDir}` và `${sourceDir}/build/${presetName}`.

---

## 3. Kiến trúc Lưu trữ Dữ liệu: Phân tách Dữ liệu Gốc (Seed) và Dữ liệu Chạy (Runtime)

### 3.1. Vấn đề cốt lõi của cơ chế Flat-File khi đưa lên Git
Trong các dự án lưu trữ dạng tệp phẳng (flat-file persistence):
- Nếu commit các tệp `data/*.dat` đang chạy lên Git: Mỗi lần người dùng chạy thử hoặc test sẽ phát sinh thay đổi diff rác (timestamp, bitmask, lượt đánh giá), gây xung đột commit liên tục.
- Nếu ignore toàn bộ `data/` trong `.gitignore`: Khi clone mới về, thư mục `data/` trống rỗng, ứng dụng sẽ khởi động không có dữ liệu hoặc báo lỗi không tìm thấy tệp.

### 3.2. Giải pháp Kiến trúc Đã Triển khai
Đã thiết kế kiến trúc phân tách 2 tầng dữ liệu:
1. **Bộ Dữ liệu Hạt giống Được Quản lý Phiên bản (Version-Controlled Seed Dataset) tại `data/seed/`:**
   - `data/seed/users.dat`: 52 tài khoản chuẩn hóa (3 Admin, 13 Gia sư, 36 Học sinh).
   - `data/seed/course_bookings.dat`: 23 hợp đồng khóa học định kỳ đa ca với đầy đủ khóa ngoại hợp lệ.
   - `data/seed/reviews.dat`: 15 đánh giá lớp học được ánh xạ 1:1 với các khóa học đã hoàn thành.
   - Thư mục `data/seed/` được theo dõi vĩnh viễn trên Git (`!/data/seed/`, `!/data/seed/*.dat`).
2. **Cơ chế Tự Động Khởi Tạo Khi Chạy Lần Đầu (`MockDatabaseEngine::seedDefaultDataIfEmpty()`):**
   - Khi ứng dụng khởi động, `MockDatabaseEngine` kiểm tra chỉ mục người dùng trong bộ nhớ RAM (`getIndexCount() == 0`).
   - Nếu phát hiện thư mục `data/` chưa có dữ liệu hoặc trống rỗng:
     - Tự động gọi `ApplicationPaths::resolveSeedDirectory()`.
     - Sử dụng luồng I/O nhị phân an toàn (`std::ifstream` / `std::ofstream`) để sao chép nguyên vẹn dữ liệu từ `data/seed/*.dat` sang `data/*.dat`.
     - Nếu không tìm thấy thư mục seed: Tự động kích hoạt cơ chế fallback khởi tạo 7 tài khoản cơ bản trong bộ nhớ để hệ thống không bao giờ bị crash.
   - Khi các tệp `data/*.dat` đã tồn tại và có dữ liệu: Hệ thống tuyệt đối không ghi đè, bảo toàn 100% dữ liệu tạo mới của người dùng trong phiên làm việc.
3. **Cấu hình `.gitignore` Tinh gọn:**
   - Bỏ qua các tệp runtime `data/*.dat`, tệp tạm `data/*.tmp`, các bản sao lưu cục bộ `backup_*/`, `staging_*/`.
   - Giữ lại `data/.gitkeep`, `data/seed/` và `data/archive/` (phục vụ các test hồi quy lịch sử).

---

## 4. Chuẩn Hóa Thông Tin Đăng Nhập & Bảo Mật Dữ Liệu

Tất cả các tài khoản thử nghiệm trong dữ liệu mẫu đã được đồng bộ mật khẩu minh bạch, không lưu trữ bất kỳ chuỗi mật khẩu phức tạp hay thông tin cá nhân thực tế nào:
- **Tài khoản Quản trị viên (Admin):**
  - `admin` / `admin123` (Quản trị viên hệ thống tối cao)
  - `qt_phuongt92` / `admin123`
  - `qt_chaul50` / `admin123`
- **Tài khoản Gia sư (Tutor):**
  - `tutor_math` / `pass123` (ThS. Lê Văn Toàn - Toán 12)
  - `tutor_phys` / `pass123` (TS. Trần Thị Hồng Nhung - Vật lý 10-12)
  - `tutor_eng` / `pass123` (Ms. Nguyễn Anh Văn - Tiếng Anh IELTS)
  - `tutor_chem` / `pass123` (Thầy Phạm Hóa Học - Hóa học 10-12)
  - `sinh_thng84` / `pass123` (Cô Nguyễn Thị Sinh - Sinh học)
  - `tinvo.edu` / `pass123` (Thầy Võ Minh Tin - Tin học C++)
- **Tài khoản Học sinh (Student):**
  - `student_nam` / `pass123` (Nguyễn Văn Nam - Lớp 12)
  - `student_thanh` / `pass123` (Phạm Tuấn Thành - Lớp 11)
  - `tmanh964` / `pass123` (Trần Minh Anh - Lớp 11)

---

## 5. Kết Quả Kiểm Tra Mô Phỏng Clean-Clone (Simulation Verification)

Để đảm bảo tính khách quan tuyệt đối, đã thực hiện quy trình mô phỏng người dùng mới tải mã nguồn về một thư mục độc lập hoàn toàn ngoài cây thư mục phát triển:
- **Thư mục sandbox:** `C:\Users\DELL\AppData\Local\Temp\tutormatch_clean_clone_sim`
- **Tệp nguồn được copy vào sandbox:** Chỉ bao gồm các tệp được Git theo dõi (loại bỏ toàn bộ tệp build, `.obj`, `.exe`, và không có bất kỳ tệp runtime `data/*.dat` nào).

### Kết quả chi tiết:
1. **Cấu hình CMake từ đầu:**
   ```bash
   cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
   ```
   *Kết quả:* `-- Configuring done (5.8s), -- Generating done (1.0s)`. Thành công 100%.

2. **Biên dịch toàn bộ 4 Targets:**
   ```bash
   cmake --build build -j4
   ```
   *Kết quả:*
   - `TutorMatch_GUI.exe`: Biên dịch & Link thành công 100%.
   - `TutorMatch_CLI.exe`: Biên dịch & Link thành công 100%.
   - `TutorMatch_Tests.exe`: Biên dịch & Link thành công 100%.
   - `TutorMatch_Inspector.exe`: Biên dịch & Link thành công 100%.

3. **Chạy Bộ Test Toàn Diện (`TutorMatch_Tests.exe`):**
   - Khởi tạo lần đầu: Tự động seed 52 bản ghi từ `data/seed/` sang `data/users.dat`.
   - Kết quả thực thi: **29/29 Test Suites vượt qua xuất sắc (100% Pass)**.
   - Bao gồm toàn bộ các bài kiểm tra chuyên sâu: OCP, LSP, DIP, ScheduleGuard 56-bit, DataFileInspector, Persistence Schema Migration, SmartMatch Subject Filtering, Search Engine Semantic Ranking, và Tutor Verification Decommission Invariants.

4. **Kiểm tra Tính Toàn Vẹn Bằng Forensic Inspector (`TutorMatch_Inspector.exe --all`):**
   - Tổng số bản ghi đọc được: 90/90 bản ghi hợp lệ.
   - Số bản ghi lỗi/hỏng: 0.
   - Tính toàn vẹn dữ liệu: **100% Hoàn hảo**.
   - Cơ chế che dấu mật khẩu (`[REDACTED]`): Hoạt động chính xác.

5. **Khởi Chạy Giao Diện Đồ Họa (`TutorMatch_GUI.exe`):**
   - Ứng dụng nạp thành công module QML `TutorMatch`.
   - Giao diện đăng nhập và dashboard chính hiển thị đầy đủ, không phát sinh cảnh báo thiếu dữ liệu.

---

## 6. Hướng Dẫn Chi Tiết Trong README.md

Tệp `README.md` tại gốc dự án đã được viết lại toàn diện và chuyên nghiệp:
- Cung cấp bối cảnh học thuật rõ ràng (Dự án PBL2, Khoa CNTT, Đại học Bách Khoa - ĐH Đà Nẵng).
- Bảng ma trận yêu cầu công nghệ (Qt 6.2+, MinGW 13.1+, CMake 3.16+).
- Hướng dẫn từng bước với Qt Creator bằng hình ảnh và quy trình tối giản.
- Bảng tài khoản mẫu chi tiết cho người đánh giá.
- Hướng dẫn build bằng dòng lệnh và chạy test.
- Mục Xử lý sự cố (FAQ & Troubleshooting) dự phòng cho các trường hợp cấu hình máy khác nhau.

---

## 7. Tổng Kết Trạng Thái Git & Sẵn Sàng Đẩy Lên GitHub

- **Cấu hình Git:** Nhánh `main`, đồng bộ với remote `origin/main`.
- **An toàn mã nguồn:** Không chứa bất kỳ thông tin nhạy cảm, token, mật khẩu thật hay binary rác nào.
- **Cam kết:** Sẵn sàng thực hiện commit và push sạch sẽ lên `origin/main` qua giao thức an toàn.
