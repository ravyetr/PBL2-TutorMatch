#include "../../include/ui/ConsoleApp.h"
#include "../../include/ds/ScheduleGuard.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <sstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#endif

namespace TutorMatch {

// ANSI Color Codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"

// Safe Input Buffer Clearing Helper to prevent std::getline skipping (Trôi dòng)
// Safe Input Buffer Clearing Helper to prevent std::getline skipping (Trôi dòng)
static void clearInputBuffer() {
    std::cin.clear();
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (GetFileType(hIn) == FILE_TYPE_CHAR) {
        FlushConsoleInputBuffer(hIn);
        return;
    }
#endif
    while (std::cin.rdbuf()->in_avail() > 0) {
        int ch = std::cin.get();
        if (ch == '\n' || ch == EOF) break;
    }
}

// UTF-8 Helper Functions for Perfect Monospace Table Alignment
static size_t utf8VisualLength(const std::string& str) {
    size_t len = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c < 0x80) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else i += 1;
        len++;
    }
    return len;
}

static std::string utf8Substr(const std::string& str, size_t maxLen) {
    size_t visLen = 0;
    size_t byteIdx = 0;
    while (byteIdx < str.length() && visLen < maxLen) {
        unsigned char c = static_cast<unsigned char>(str[byteIdx]);
        size_t charBytes = 1;
        if (c < 0x80) charBytes = 1;
        else if ((c & 0xE0) == 0xC0) charBytes = 2;
        else if ((c & 0xF0) == 0xE0) charBytes = 3;
        else if ((c & 0xF8) == 0xF0) charBytes = 4;

        if (byteIdx + charBytes > str.length()) break;
        byteIdx += charBytes;
        visLen++;
    }
    return str.substr(0, byteIdx);
}

static std::string padRightUTF8(const std::string& str, size_t targetWidth) {
    size_t vLen = utf8VisualLength(str);
    if (vLen >= targetWidth) return utf8Substr(str, targetWidth);
    return str + std::string(targetWidth - vLen, ' ');
}

static std::string padCenterUTF8(const std::string& str, size_t targetWidth) {
    size_t vLen = utf8VisualLength(str);
    if (vLen >= targetWidth) return utf8Substr(str, targetWidth);
    size_t padLeft = (targetWidth - vLen) / 2;
    size_t padRight = targetWidth - vLen - padLeft;
    return std::string(padLeft, ' ') + str + std::string(padRight, ' ');
}

static std::string translateStatus(const std::string& status) {
    if (status == "PENDING") return "CHỜ DUYỆT";
    if (status == "ACCEPTED") return "ĐÃ DUYỆT";
    if (status == "REJECTED") return "TỪ CHỐI";
    if (status == "CANCELLED") return "ĐÃ HỦY";
    if (status == "COMPLETED") return "HOÀN THÀNH";
    if (status == "ACTIVE") return "HOẠT ĐỘNG";
    if (status == "BLOCKED") return "BỊ KHÓA";
    return status;
}

ConsoleApp::ConsoleApp() : isLoggedIn(false) {}

ConsoleApp::~ConsoleApp() {}

bool ConsoleApp::initialize(std::shared_ptr<AuthService> auth,
                            std::shared_ptr<UserService> user,
                            std::shared_ptr<BookingService> booking,
                            std::shared_ptr<ReviewService> review,
                            std::shared_ptr<MatchingService> matching,
                            std::shared_ptr<AdminService> admin,
                            std::shared_ptr<MockDatabaseEngine> db) {
    authService = auth;
    userService = user;
    bookingService = booking;
    reviewService = review;
    matchingService = matching;
    adminService = admin;
    dbEngine = db;
    return (authService && userService && bookingService && reviewService && matchingService && adminService);
}

bool ConsoleApp::initialize(std::shared_ptr<MockDatabaseEngine> engine) {
    dbEngine = engine;
    if (!dbEngine) return false;
    auto uRepo = dbEngine->getUserRepository();
    auto rRepo = dbEngine->getReviewRepository();
    auto cbRepo = dbEngine->getCourseBookingRepository();
    authService = std::make_shared<AuthService>(uRepo);
    userService = std::make_shared<UserService>(uRepo);
    bookingService = std::make_shared<BookingService>(uRepo, rRepo, cbRepo);
    reviewService = std::make_shared<ReviewService>(rRepo, uRepo, cbRepo);
    matchingService = std::make_shared<MatchingService>(uRepo);
    adminService = std::make_shared<AdminService>(uRepo, rRepo, bookingService);
    return true;
}

void ConsoleApp::printHeader(const std::string& title) const {
    std::cout << "\n" << COLOR_CYAN << COLOR_BOLD
              << "==========================================================================" << "\n"
              << "  " << title << "\n"
              << "=========================================================================="
              << COLOR_RESET << "\n";
}

void ConsoleApp::printDivider() const {
    std::cout << COLOR_CYAN << "--------------------------------------------------------------------------" << COLOR_RESET << "\n";
}

void ConsoleApp::printSuccess(const std::string& msg) const {
    std::cout << COLOR_GREEN << COLOR_BOLD << " [THÀNH CÔNG] " << COLOR_RESET << COLOR_GREEN << msg << COLOR_RESET << "\n";
}

void ConsoleApp::printError(const std::string& msg) const {
    std::cout << COLOR_RED << COLOR_BOLD << " [LỖI] " << COLOR_RESET << COLOR_RED << msg << COLOR_RESET << "\n";
}

void ConsoleApp::printWarning(const std::string& msg) const {
    std::cout << COLOR_YELLOW << COLOR_BOLD << " [CẢNH BÁO] " << COLOR_RESET << COLOR_YELLOW << msg << COLOR_RESET << "\n";
}

void ConsoleApp::printInfo(const std::string& msg) const {
    std::cout << COLOR_BLUE << COLOR_BOLD << " [THÔNG TIN] " << COLOR_RESET << msg << "\n";
}

bool ConsoleApp::isValidEmail(const std::string& email) const {
    const std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, pattern);
}

void ConsoleApp::renderTimetableGrid(int userId, UserRole role) const {
    printHeader("THỜI KHÓA BIỂU TUẦN");

    // Get user to check scheduleBitmask for busy slots
    auto u = userService ? userService->getUserById(userId)
                         : (dbEngine ? dbEngine->findUserById(userId) : nullptr);
    uint64_t userBitmask = ScheduleGuard::VALID_SCHEDULE_MASK;
    if (u) {
        if (role == UserRole::STUDENT) {
            auto s = std::static_pointer_cast<Student>(u);
            userBitmask = s->getScheduleBitmask();
        } else if (role == UserRole::TUTOR) {
            auto t = std::static_pointer_cast<Tutor>(u);
            userBitmask = t->getScheduleBitmask();
        }
    }

    // Get recurring course bookings
    std::vector<CourseBooking> courseBookings;
    if (role == UserRole::STUDENT) {
        courseBookings = bookingService ? bookingService->getStudentCourseBookings(userId)
                                        : (dbEngine ? dbEngine->getCourseBookingsByStudentId(userId) : std::vector<CourseBooking>{});
    } else if (role == UserRole::TUTOR) {
        courseBookings = bookingService ? bookingService->getTutorCourseBookings(userId)
                                        : (dbEngine ? dbEngine->getCourseBookingsByTutorId(userId) : std::vector<CourseBooking>{});
    }

    // Map: [day 0..6][slot 0..7] -> Course Name
    std::string grid[7][8];
    for (const auto& cb : courseBookings) {
        if (cb.status == "ACCEPTED") {
            if (cb.dayOfWeek >= 0 && cb.dayOfWeek < 7 && cb.slotIndex >= 0 && cb.slotIndex < 8) {
                grid[cb.dayOfWeek][cb.slotIndex] = cb.subject;
            }
        }
    }

    static const char* SLOT_TIMES[] = {
        "07:30-09:00", "09:15-10:45", "13:30-15:00", "15:15-16:45",
        "17:00-18:30", "18:45-20:15", "20:30-22:00", "22:00-23:30"
    };

    std::cout << "+---------------+------------+------------+------------+------------+------------+------------+------------+\n";
    std::cout << "|   THỜI GIAN   |   THỨ 2    |   THỨ 3    |   THỨ 4    |   THỨ 5    |   THỨ 6    |   THỨ 7    |  CHỦ NHẬT  |\n";
    std::cout << "+---------------+------------+------------+------------+------------+------------+------------+------------+\n";

    for (int slot = 0; slot < 8; ++slot) {
        std::cout << "| " << std::setw(13) << SLOT_TIMES[slot] << " |";
        for (int day = 0; day < 7; ++day) {
            uint64_t slotMask = ScheduleGuard::getSlotMask(day, slot);
            std::string rawText = grid[day][slot];
            bool isAvailableBit = ((userBitmask & slotMask) != 0);

            if (!rawText.empty()) {
                std::cout << COLOR_GREEN << COLOR_BOLD << padCenterUTF8(rawText, 12) << COLOR_RESET << "|";
            } else if (!isAvailableBit) {
                std::cout << COLOR_RED << padCenterUTF8("[BẬN]", 12) << COLOR_RESET << "|";
            } else {
                std::cout << "            |"; // 12 spaces
            }
        }
        std::cout << "\n";
    }
    std::cout << "+---------------+------------+------------+------------+------------+------------+------------+------------+\n";
}

void ConsoleApp::renderTutorTable(const std::vector<std::shared_ptr<Tutor>>& tutors) const {
    if (tutors.empty()) {
        printInfo("Không tìm thấy Gia sư nào phù hợp.");
        return;
    }
    std::cout << "\n+----+----------------------+-------------------+--------------+----------+\n";
    std::cout << "| ID | Họ và Tên            | Chuyên môn        | Học phí (VND)| Đánh giá |\n";
    std::cout << "+----+----------------------+-------------------+--------------+----------+\n";
    for (const auto& t : tutors) {
        std::string ratingStr = (t->getTotalReviews() > 0 || t->getRating() > 0.0) 
                              ? (std::to_string(t->getRating()).substr(0, 3) + " *") 
                              : "0.0 *";
        std::cout << "| " << padRightUTF8(std::to_string(t->getId()), 2)
                  << " | " << padRightUTF8(t->getFullName(), 20)
                  << " | " << padRightUTF8(t->getSubjectSpecialty(), 17)
                  << " | " << padRightUTF8(std::to_string((long long)t->getHourlyRate()) + " VNĐ", 12)
                  << " | " << padCenterUTF8(ratingStr, 8) << " |\n";
    }
    std::cout << "+----+----------------------+-------------------+--------------+----------+\n";
}

int ConsoleApp::readIntInput(const std::string& prompt) {
    int val = 0;
    while (true) {
        std::cout << COLOR_BOLD << prompt << COLOR_RESET;
        if (std::cin >> val) {
            clearInputBuffer();
            return val;
        }
        if (std::cin.eof()) return 0;
        clearInputBuffer();
        printError("Định dạng số không hợp lệ. Vui lòng nhập số nguyên!");
    }
}

int ConsoleApp::getValidIntChoice(int minVal, int maxVal, const std::string& prompt) {
    int val = 0;
    while (true) {
        std::cout << COLOR_BOLD << prompt << COLOR_RESET;
        if (std::cin >> val) {
            if (val >= minVal && val <= maxVal) {
                clearInputBuffer();
                return val;
            }
        }
        if (std::cin.eof()) return 0;
        clearInputBuffer();
        printError("Lựa chọn không hợp lệ! Vui lòng nhập số nằm trong khoảng [" + std::to_string(minVal) + " - " + std::to_string(maxVal) + "].");
    }
}

double ConsoleApp::readDoubleInput(const std::string& prompt) {
    double val = 0.0;
    while (true) {
        std::cout << COLOR_BOLD << prompt << COLOR_RESET;
        if (std::cin >> val) {
            clearInputBuffer();
            return val;
        }
        if (std::cin.eof()) return 0.0;
        clearInputBuffer();
        printError("Định dạng số thực không hợp lệ. Vui lòng nhập số hợp lệ!");
    }
}

std::string ConsoleApp::readStringInput(const std::string& prompt) {
    std::cout << COLOR_BOLD << prompt << COLOR_RESET;

#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (GetFileType(hIn) == FILE_TYPE_CHAR) {
        wchar_t wbuf[1024];
        DWORD charsRead = 0;
        if (ReadConsoleW(hIn, wbuf, 1023, &charsRead, NULL)) {
            wbuf[charsRead] = L'\0';
            std::wstring wstr(wbuf);
            while (!wstr.empty() && (wstr.back() == L'\r' || wstr.back() == L'\n')) {
                wstr.pop_back();
            }
            if (wstr.empty()) return "";
            int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                std::string utf8str(len - 1, 0);
                WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], len, NULL, NULL);

                size_t first = utf8str.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) return "";
                size_t last = utf8str.find_last_not_of(" \t\r\n");
                return utf8str.substr(first, (last - first + 1));
            }
        }
    }
#endif

    std::string input;

    // Check if stdin buffer has residual '\n' from prior std::cin >>
    if (std::cin.peek() == '\n') {
        std::cin.get(); // consume residual newline
    }

    std::getline(std::cin, input);

    // Trim leading/trailing whitespace
    size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, (last - first + 1));
}

void ConsoleApp::run() {
    while (true) {
        if (!isLoggedIn) {
            printHeader("CỔNG XÁC THỰC VÀ ĐĂNG NHẬP HỆ THỐNG");
            std::cout << " 1. Đăng nhập (Học viên: student_nam, Gia sư: tutor_math, Admin: admin | Mật khẩu: pass123/admin123)\n";
            std::cout << " 2. Đăng ký tài khoản mới (Học viên / Gia sư)\n";
            std::cout << " 0. Thoát hệ thống\n";
            printDivider();

            int choice = getValidIntChoice(0, 2, "Chọn thao tác [0-2]: ");
            if (choice == 0) {
                printInfo("Đã thoát nền tảng TutorMatch. Tạm biệt quý khách!");
                break;
            }

            switch (choice) {
                case 1: handleLogin(); break;
                case 2: handleRegister(); break;
                default: printError("Lựa chọn không hợp lệ!"); break;
            }
        } else {
            switch (currentUser->getRole()) {
                case UserRole::STUDENT: renderStudentMenu(); break;
                case UserRole::TUTOR:   renderTutorMenu(); break;
                case UserRole::ADMIN:   renderAdminMenu(); break;
            }
        }
    }
}

void ConsoleApp::handleLogin() {
    std::string uName = readStringInput("Tên đăng nhập: ");
    std::string pPass = readStringInput("Mật khẩu: ");

    std::string errMsg;
    std::shared_ptr<User> user = nullptr;
    if (authService) {
        if (authService->login(uName, pPass, errMsg)) {
            user = authService->getCurrentUser();
        }
    } else if (dbEngine) {
        user = dbEngine->findUserByUsername(uName);
        if (user && user->getPasswordHash() != pPass) {
            user = nullptr;
            errMsg = "Tên đăng nhập hoặc mật khẩu không chính xác!";
        } else if (user && !user->getIsActive()) {
            user = nullptr;
            errMsg = "Tài khoản này đã BỊ KHÓA bởi Quản trị viên!";
        }
    }

    if (user) {
        currentUser = user;
        isLoggedIn = true;
        printSuccess("Đăng nhập thành công! Chào mừng, " + currentUser->getFullName() + " (" + currentUser->getRoleString() + ")");
    } else {
        if (!errMsg.empty()) {
            printError(errMsg);
        } else {
            printError("Tên đăng nhập hoặc mật khẩu không chính xác!");
        }
    }
}

void ConsoleApp::handleRegister() {
    printHeader("ĐĂNG KÝ TÀI KHOẢN MỚI");
    std::string uName = readStringInput("Tên đăng nhập mong muốn: ");
    if (dbEngine && dbEngine->findUserByUsername(uName) != nullptr) {
        printError("Tên đăng nhập đã tồn tại trong hệ thống!");
        return;
    }

    std::string pPass = readStringInput("Mật khẩu: ");
    std::string fullName = readStringInput("Họ và tên: ");

    // Email Format Validation
    std::string email;
    while (true) {
        email = readStringInput("Địa chỉ Email (Ví dụ: ten@domain.com, hoặc nhập 0 để hủy): ");
        if (email == "0") {
            printInfo("Đã hủy đăng ký tài khoản.");
            return;
        }
        if (isValidEmail(email)) {
            break;
        }
        printError("Định dạng email không hợp lệ (Ví dụ: ten@domain.com)!");
    }

    std::cout << "Chọn vai trò tài khoản:\n 1. Học viên\n 2. Gia sư\n";
    int rChoice = getValidIntChoice(1, 2, "Chọn vai trò [1-2]: ");

    if (rChoice == 2) {
        std::string subj = readStringInput("Môn học chuyên môn: ");
        double rate = readDoubleInput("Học phí mong muốn (VNĐ/buổi): ");
        std::string bio = readStringInput("Giới thiệu bản thân : ");

        std::string errMsg;
        bool ok = false;
        if (authService) {
            ok = authService->registerTutor(uName, pPass, fullName, email, subj, rate, bio, errMsg);
        } else if (dbEngine) {
            int newId = dbEngine->getNextUserId();
            Tutor tutor(newId, uName, pPass, email, fullName, bio, subj, rate, 5.0, 0, false, 0.0, 0x00FF00FF00FF00FFULL, true);
            ok = dbEngine->saveUser(tutor);
        }

        if (ok) {
            printSuccess("Đăng ký tài khoản Gia sư thành công! Vui lòng đăng nhập.");
        } else {
            printError(errMsg.empty() ? "Lưu tài khoản Gia sư thất bại!" : errMsg);
        }
    } else {
        int grade = getValidIntChoice(1, 12, "Khối lớp (1-12): ");
        std::string prefSubj = readStringInput("Môn học mục tiêu cần tìm Gia sư: ");
        double budget = readDoubleInput("Ngân sách tối đa (VNĐ/buổi): ");

        std::string errMsg;
        bool ok = false;
        if (authService) {
            ok = authService->registerStudent(uName, pPass, fullName, email, std::to_string(grade), prefSubj, budget, errMsg);
        } else if (dbEngine) {
            int newId = dbEngine->getNextUserId();
            Student student(newId, uName, pPass, email, fullName, grade, prefSubj, budget, 0x00FF00FF00FF00FFULL, true);
            ok = dbEngine->saveUser(student);
        }

        if (ok) {
            printSuccess("Đăng ký tài khoản Học viên thành công! Vui lòng đăng nhập.");
        } else {
            printError(errMsg.empty() ? "Lưu tài khoản Học viên thất bại!" : errMsg);
        }
    }
}

void ConsoleApp::handleBenchmarkStatus() {
    printHeader("KIỂM THỬ CẤU TRÚC DỮ LIỆU & BỘ NHỚ LƯU TRỮ HỆ THỐNG");
    size_t indexCount = dbEngine ? dbEngine->getIndexCount() : 0;
    size_t cacheSize = dbEngine ? dbEngine->getCacheSize() : 0;
    std::cout << COLOR_YELLOW << " [Chỉ mục Bảng băm RAM O(1)] : " << indexCount << " vị trí đối tượng được đánh chỉ mục trong RAM\n";
    std::cout << " [Bộ nhớ đệm LRU Cache]      : " << cacheSize << " hồ sơ truy cập gần đây trong bộ nhớ\n";
    std::cout << " [Chỉ mục Cây AVL]            : Tối ưu truy vấn khoảng Học phí & Đánh giá trong O(log N)\n" << COLOR_RESET;

    auto tutors = userService ? userService->getAllTutors()
                              : (dbEngine ? dbEngine->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{});
    std::cout << " [Số lượng Gia sư hoạt động] : " << tutors.size() << " gia sư đã/chưa xác minh\n";

    printDivider();
    std::cout << "Đang kiểm thử Truy vấn khoảng Cây AVL (100.000 VNĐ đến 200.000 VNĐ)...\n";
    std::vector<std::shared_ptr<Tutor>> avlTutors;
    if (userService) {
        avlTutors = userService->filterTutorsByPrice(100000.0, 200000.0);
    } else if (dbEngine) {
        auto tutorIds = dbEngine->findTutorIdsByPriceRange(100000.0, 200000.0);
        for (int tid : tutorIds) {
            auto u = dbEngine->findUserById(tid);
            if (u && u->getRole() == UserRole::TUTOR) avlTutors.push_back(std::static_pointer_cast<Tutor>(u));
        }
    }
    printSuccess("Tìm thấy " + std::to_string(avlTutors.size()) + " gia sư trong khoảng giá [100.000, 200.000] qua Cây AVL!");
    for (const auto& t : avlTutors) {
        std::cout << "  -> Mã ID " << t->getId() << ": " << t->getFullName() << "\n";
    }
}

void ConsoleApp::handleChangePassword() {
    std::string oldPass = readStringInput("Nhập Mật khẩu cũ: ");
    if (oldPass != currentUser->getPasswordHash()) {
        printError("Mật khẩu cũ không chính xác!");
        return;
    }
    std::string newPass = readStringInput("Nhập Mật khẩu mới: ");
    currentUser->setPasswordHash(newPass);
    bool ok = userService ? userService->updateUser(*currentUser)
                          : (dbEngine && dbEngine->updateUser(*currentUser));
    if (ok) {
        printSuccess("Đổi mật khẩu thành công!");
    } else {
        printError("Cập nhật mật khẩu thất bại!");
    }
}

void ConsoleApp::renderStudentOverview() const {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);
    printHeader("BẢNG ĐIỀU KHIỂN HỌC VIÊN | " + studentPtr->getFullName());
    studentPtr->displayProfile();
    renderTimetableGrid(studentPtr->getId(), UserRole::STUDENT);
}

void ConsoleApp::renderTutorOverview() const {
    auto tutorPtr = std::static_pointer_cast<Tutor>(currentUser);
    printHeader("BẢNG ĐIỀU KHIỂN GIA SƯ | " + tutorPtr->getFullName());
    auto reviews = reviewService ? reviewService->getReviewsByTutorId(tutorPtr->getId())
                                 : (dbEngine ? dbEngine->getReviewsByTutorId(tutorPtr->getId()) : std::vector<Review>{});
    tutorPtr->setTotalReviews((int)reviews.size());
    tutorPtr->displayProfile();
    renderTimetableGrid(tutorPtr->getId(), UserRole::TUTOR);
}

// =========================================================================
// STUDENT MENU IMPLEMENTATION
// =========================================================================
void ConsoleApp::renderStudentMenu() {
    renderStudentOverview();

    printDivider();
    std::cout << " 1. Chỉnh sửa thông tin hồ sơ\n";
    std::cout << " 2. Tìm kiếm & Lọc danh sách Gia sư\n";
    std::cout << " 3. " << COLOR_GREEN << COLOR_BOLD << "CHẠY THUẬT TOÁN GỢI Ý GIA SƯ THÔNG MINH " << COLOR_RESET << "\n";
    std::cout << " 4. " << COLOR_CYAN << COLOR_BOLD << "Chỉnh sửa Lịch Rảnh / Bận tương tác " << COLOR_RESET << "\n";
    std::cout << " 5. " << COLOR_YELLOW << COLOR_BOLD << "Quản lý Đơn đặt lịch & Đặt lịch học mới " << COLOR_RESET << "\n";
    std::cout << " 6. Viết Đánh giá & Phản hồi cho buổi học đã hoàn thành\n";
    std::cout << " 7. Đổi mật khẩu\n";
    std::cout << " 8. Đăng xuất\n";
    printDivider();

    int choice = getValidIntChoice(1, 8, "Chọn chức năng [1-8]: ");
    switch (choice) {
        case 1: handleStudentProfileUpdate(); break;
        case 2: handleTutorDiscoveryAVL(); break;
        case 3: handleSmartMatchingMaxHeap(); break;
        case 4: handleStudentScheduleMatrixEditor(); break;
        case 5: handleStudentBookings(); break;
        case 6: handleLeaveReviewAndRating(); break;
        case 7: handleChangePassword(); break;
        case 8:
            isLoggedIn = false;
            currentUser = nullptr;
            printInfo("Đã đăng xuất thành công.");
            break;
        default: printError("Lựa chọn không hợp lệ!"); break;
    }
}

void ConsoleApp::handleStudentProfileUpdate() {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);

    while (true) {
        printHeader("CHỌN THÔNG TIN CẦN CHỈNH SỬA");
        std::cout << " 1. Môn học mục tiêu [" << studentPtr->getPreferredSubject() << "]\n";
        std::cout << " 2. Ngân sách tối đa [" << (long long)studentPtr->getMaxBudget() << " VNĐ/buổi]\n";
        std::cout << " 3. Khối lớp [Lớp " << studentPtr->getGradeLevel() << "]\n";
        std::cout << " 4. Địa chỉ Email [" << studentPtr->getEmail() << "]\n";
        std::cout << " 5. Họ và tên [" << studentPtr->getFullName() << "]\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();

        int fieldChoice = getValidIntChoice(0, 5, "Chọn mục cần sửa [0-5]: ");
        if (fieldChoice == 0) {
            break;
        }

        bool updated = false;
        switch (fieldChoice) {
            case 1: {
                std::string newSubj = readStringInput("Nhập Môn học mục tiêu mới [" + studentPtr->getPreferredSubject() + "]: ");
                if (!newSubj.empty()) {
                    studentPtr->setPreferredSubject(newSubj);
                    updated = true;
                }
                break;
            }
            case 2: {
                double newBudget = readDoubleInput("Nhập Ngân sách tối đa mới (VNĐ/buổi) [" + std::to_string((long long)studentPtr->getMaxBudget()) + "]: ");
                if (newBudget > 0) {
                    studentPtr->setMaxBudget(newBudget);
                    updated = true;
                } else {
                    printError("Ngân sách tối đa phải lớn hơn 0!");
                }
                break;
            }
            case 3: {
                int newGrade = getValidIntChoice(1, 12, "Nhập Khối lớp mới (1-12) [" + std::to_string(studentPtr->getGradeLevel()) + "]: ");
                studentPtr->setGradeLevel(newGrade);
                updated = true;
                break;
            }
            case 4: {
                while (true) {
                    std::string newEmail = readStringInput("Nhập Địa chỉ Email mới (Ví dụ: ten@domain.com, hoặc 0 để hủy): ");
                    if (newEmail == "0") {
                        printInfo("Đã hủy cập nhật email.");
                        break;
                    }
                    if (isValidEmail(newEmail)) {
                        studentPtr->setEmail(newEmail);
                        updated = true;
                        break;
                    }
                    printError("Định dạng email không hợp lệ (Ví dụ: ten@domain.com)!");
                }
                break;
            }
            case 5: {
                std::string newName = readStringInput("Nhập Họ và tên mới [" + studentPtr->getFullName() + "]: ");
                if (!newName.empty()) {
                    studentPtr->setFullName(newName);
                    updated = true;
                }
                break;
            }
            default:
                printError("Lựa chọn mục sửa không hợp lệ!");
                break;
        }

        if (updated) {
            bool ok = userService ? userService->updateUser(*studentPtr)
                                  : (dbEngine && dbEngine->updateUser(*studentPtr));
            if (ok) {
                printSuccess("Cập nhật hồ sơ Học viên thành công!");
            } else {
                printError("Lưu hồ sơ đã cập nhật vào cơ sở dữ liệu thất bại!");
            }
        }
    }
}

void ConsoleApp::handleTutorDiscoveryAVL() {
    while (true) {
        printHeader("TÌM KIẾM & LỌC GIA SƯ");
        std::cout << " 1. Xem danh sách toàn bộ Gia sư\n";
        std::cout << " 2. Lọc Gia sư theo khoảng Học phí \n";
        std::cout << " 3. Lọc Gia sư theo Điểm đánh giá tối thiểu \n";
        std::cout << " 4. Xem chi tiết Hồ sơ Gia sư & Đánh giá của Học viên\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int choice = getValidIntChoice(0, 4, "Chọn chế độ lọc [0-4]: ");
        if (choice == 0) break;

        if (choice == 1) {
            auto tutors = userService ? userService->getAllTutors()
                                      : (dbEngine ? dbEngine->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{});
            renderTutorTable(tutors);
        } else if (choice == 2) {
            double minP = readDoubleInput("Nhập mức giá tối thiểu (VNĐ): ");
            double maxP = readDoubleInput("Nhập mức giá tối đa (VNĐ): ");
            std::vector<std::shared_ptr<Tutor>> filtered;
            if (userService) {
                filtered = userService->filterTutorsByPrice(minP, maxP);
            } else if (dbEngine) {
                auto ids = dbEngine->findTutorIdsByPriceRange(minP, maxP);
                for (int tid : ids) {
                    auto u = dbEngine->findUserById(tid);
                    if (u && u->getRole() == UserRole::TUTOR) {
                        filtered.push_back(std::static_pointer_cast<Tutor>(u));
                    }
                }
            }

            printSuccess("Tìm thấy " + std::to_string(filtered.size()) + " gia sư trong khoảng giá [" + std::to_string((long long)minP) + " VNĐ, " + std::to_string((long long)maxP) + " VNĐ]:");
            renderTutorTable(filtered);
        } else if (choice == 3) {
            double minR = readDoubleInput("Nhập điểm đánh giá tối thiểu (1.0 - 5.0): ");
            std::vector<std::shared_ptr<Tutor>> filtered;
            if (userService) {
                filtered = userService->filterTutorsByRating(minR);
            } else if (dbEngine) {
                auto ids = dbEngine->findTutorIdsByMinRating(minR);
                for (int tid : ids) {
                    auto u = dbEngine->findUserById(tid);
                    if (u && u->getRole() == UserRole::TUTOR) {
                        filtered.push_back(std::static_pointer_cast<Tutor>(u));
                    }
                }
            }

            printSuccess("Tìm thấy " + std::to_string(filtered.size()) + " gia sư có Điểm đánh giá >= " + std::to_string(minR) + " *:");
            renderTutorTable(filtered);
        } else if (choice == 4) {
            int tutorId = readIntInput("Nhập Mã ID Gia sư để xem chi tiết: ");
            auto u = userService ? userService->getUserById(tutorId)
                                 : (dbEngine ? dbEngine->findUserById(tutorId) : nullptr);
            if (!u || u->getRole() != UserRole::TUTOR) {
                printError("Mã ID Gia sư không tồn tại hoặc không hợp lệ!");
            } else {
                auto t = std::static_pointer_cast<Tutor>(u);
                auto reviews = reviewService ? reviewService->getReviewsByTutorId(t->getId())
                                             : (dbEngine ? dbEngine->getReviewsByTutorId(t->getId()) : std::vector<Review>{});

                printHeader("CHI TIẾT HỒ SƠ GIA SƯ");
                std::cout << " Mã ID Gia sư      : " << t->getId() << "\n";
                std::cout << " Họ và tên          : " << t->getFullName() << "\n";
                std::cout << " Chuyên môn giảng dạy: " << t->getSubjectSpecialty() << "\n";
                std::cout << " Mức học phí        : " << (long long)t->getHourlyRate() << " VNĐ/buổi\n";
                std::cout << " Điểm đánh giá      : " << std::fixed << std::setprecision(1) << t->getRating() << " * (" << reviews.size() << " lượt đánh giá)\n";
                std::cout << " Trạng thái xác minh: " << (t->getIsVerified() ? "ĐÃ XÁC MINH (CÓ)" : "CHƯA XÁC MINH (KHÔNG)") << "\n";
                std::cout << " Giới thiệu bản thân: " << t->getBio() << "\n";
                printDivider();

                std::cout << COLOR_CYAN << COLOR_BOLD << "PHẢN HỒI CỦA HỌC VIÊN (" << reviews.size() << " lượt phản hồi):" << COLOR_RESET << "\n";
                if (reviews.empty()) {
                    printInfo("Chưa có lượt phản hồi nào cho gia sư này.");
                } else {
                    for (const auto& r : reviews) {
                        std::cout << " - [" << std::fixed << std::setprecision(1) << (double)r.ratingScore << " *] "
                                  << COLOR_BOLD << r.studentName << COLOR_RESET
                                  << " (" << r.createdAt << "): \"" << r.comment << "\"\n";
                    }
                }
                printDivider();
            }
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

void ConsoleApp::handleSmartMatchingMaxHeap() {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);

    while (true) {
        auto tutors = userService ? userService->getAllTutors()
                                  : (dbEngine ? dbEngine->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{});
        auto topMatches = matchingService ? matchingService->findMatches(*studentPtr, 3)
                                          : matchingEngine.findTopMatches(*studentPtr, tutors, 3);

        printHeader("HỆ THỐNG GỢI Ý GIA SƯ THÔNG MINH");
        std::cout << COLOR_YELLOW << " Trọng số đa tiêu chí: Môn học (30%), Giá học phí (20%), Lịch dạy (25%), Đánh giá (15%), Xác minh (10%)\n" << COLOR_RESET;
        printDivider();

        int rank = 1;
        for (const auto& res : topMatches) {
            std::cout << COLOR_GREEN << COLOR_BOLD << " Hạng #" << rank++ << " | Điểm tương thích tổng hợp: "
                      << std::fixed << std::setprecision(1) << res.compositeScore << "%" << COLOR_RESET << "\n";
            std::cout << "   Gia sư: " << res.tutor->getFullName()
                      << " (" << (long long)res.tutor->getHourlyRate() << " VNĐ/buổi, " << std::fixed << std::setprecision(1) << res.tutor->getRating() << " *)\n";
            std::cout << "   Độ khớp môn học: " << res.subjectScore << "% | Phù hợp giá: " << res.priceScore
                      << "% | Trùng khớp lịch học: " << res.scheduleScore << "%\n";
            std::cout << "   Lịch dạy rảnh: " << ScheduleGuard::formatSchedule(res.tutor->getScheduleBitmask()) << "\n";
            printDivider();
        }

        std::cout << " 1. Chạy lại / Cập nhật thuật toán Gợi ý\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int choice = getValidIntChoice(0, 1, "Chọn thao tác [0-1]: ");
        if (choice == 0) break;
    }
}

void ConsoleApp::handleBookLessonScheduleGuard() {
    printHeader("ĐẶT LỊCH HỌC MỚI VỚI GIA SƯ");

    auto tutors = userService ? userService->getAllTutors()
                              : (dbEngine ? dbEngine->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{});
    std::cout << "\nDANH SÁCH GIA SƯ HIỆN CÓ:";
    renderTutorTable(tutors);
    std::cout << "\n";

    int tutorId = readIntInput("Nhập Mã ID Gia sư muốn đặt lịch (nhập 0 để hủy): ");
    if (tutorId == 0) return;

    auto u = userService ? userService->getUserById(tutorId)
                         : (dbEngine ? dbEngine->findUserById(tutorId) : nullptr);
    if (!u || u->getRole() != UserRole::TUTOR || !u->getIsActive()) {
        printError("Không tìm thấy Gia sư hoặc tài khoản đang bị khóa!");
        return;
    }

    auto tutor = std::static_pointer_cast<Tutor>(u);
    std::cout << "\nGia sư đã chọn: " << tutor->getFullName() << " (" << (long long)tutor->getHourlyRate() << " VNĐ/buổi)\n";
    std::cout << "Lịch dạy rảnh của Gia sư: " << ScheduleGuard::formatSchedule(tutor->getScheduleBitmask()) << "\n\n";

    std::cout << "Chọn thứ trong tuần:\n 0 = Thứ 2, 1 = Thứ 3, 2 = Thứ 4, 3 = Thứ 5, 4 = Thứ 6, 5 = Thứ 7, 6 = Chủ Nhật\n";
    int day = getValidIntChoice(0, 6, "Chọn thứ [0-6]: ");
    std::cout << "Chọn ca học:\n 0: 07:30-09:00, 1: 09:15-10:45, 2: 13:30-15:00, 3: 15:15-16:45\n 4: 17:00-18:30, 5: 18:45-20:15, 6: 20:30-22:00, 7: 22:00-23:30\n";
    int slot = getValidIntChoice(0, 7, "Chọn ca học [0-7]: ");

    std::cout << "Chọn thời lượng khóa học:\n 1: 4 tuần\n 2: 8 tuần\n 3: 12 tuần\n";
    int durationChoice = getValidIntChoice(1, 3, "Chọn thời lượng [1-3]: ");
    int weeksCount = (durationChoice == 1) ? 4 : ((durationChoice == 2) ? 8 : 12);

    std::string today = DateUtils::getCurrentDateString();
    int currentDow = DateUtils::dayOfWeek(today);
    int daysUntil = (day - currentDow + 7) % 7;
    std::vector<std::string> upcomingDates;
    for (int i = 0; i < 3; ++i) {
        upcomingDates.push_back(DateUtils::addDays(today, daysUntil + 7 * i));
    }
    std::cout << "Chọn ngày bắt đầu khóa học:\n";
    for (size_t i = 0; i < upcomingDates.size(); ++i) {
        std::cout << " " << (i + 1) << ": " << upcomingDates[i]
                  << (i == 0 && daysUntil == 0 ? " (Hôm nay)" : "") << "\n";
    }
    int dateChoice = getValidIntChoice(1, (int)upcomingDates.size(), "Chọn ngày bắt đầu [1-3]: ");
    std::string startDate = upcomingDates[dateChoice - 1];

    std::cout << "\n" << COLOR_CYAN << "--- CHÍNH SÁCH CAM KẾT KHÓA HỌC (" << CourseBooking::CURRENT_COMMITMENT_VERSION << ") ---" << COLOR_RESET << "\n";
    std::cout << CourseBooking::COMMITMENT_POLICY_TEXT << "\n";
    std::cout << "Bạn có đồng ý cam kết tham gia khóa học theo các điều khoản trên không? (Y/N): ";
    std::string ackStr = readStringInput("");
    bool ack = (ackStr == "Y" || ackStr == "y");
    if (!ack) {
        printError("Bạn cần đồng ý với chính sách cam kết để tạo yêu cầu khóa học!");
        return;
    }

    auto studentPtr = std::static_pointer_cast<Student>(currentUser);
    std::string outMsg;
    bool booked = false;
    int newCourseId = 0;

    if (bookingService) {
        booked = bookingService->bookCourse(studentPtr->getId(), tutor->getId(),
                                            studentPtr->getPreferredSubject(), day, slot,
                                            startDate, weeksCount, ack, outMsg, &newCourseId);
    } else if (dbEngine) {
        auto cbRepo = dbEngine->getCourseBookingRepository();
        if (cbRepo) {
            auto bService = std::make_shared<BookingService>(dbEngine->getUserRepository(), dbEngine->getReviewRepository(), cbRepo);
            booked = bService->bookCourse(studentPtr->getId(), tutor->getId(),
                                          studentPtr->getPreferredSubject(), day, slot,
                                          startDate, weeksCount, ack, outMsg, &newCourseId);
        }
    }

    if (booked) {
        printSuccess(outMsg);
    } else {
        printError(outMsg);
    }
}

void ConsoleApp::handleStudentScheduleMatrixEditor() {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);

    while (true) {
        renderTimetableGrid(studentPtr->getId(), UserRole::STUDENT);

        uint64_t mask = studentPtr->getScheduleBitmask();

        printDivider();
        std::cout << "Chọn thao tác:\n 1. Bật/Tắt trạng thái ca học (Rảnh/Bận)\n 2. Đặt lại toàn bộ các ca thành Rảnh\n 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int act = getValidIntChoice(0, 2, "Chọn thao tác [0-2]: ");
        if (act == 0) {
            break;
        }

        if (act == 1) {
            int day = getValidIntChoice(0, 6, "Chọn thứ [0=Thứ 2, 1=Thứ 3, 2=Thứ 4, 3=Thứ 5, 4=Thứ 6, 5=Thứ 7, 6=Chủ Nhật]: ");
            int slot = getValidIntChoice(0, 7, "Chọn ca học [0-7]: ");

            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
            bool isAvail = (mask & sMask) != 0;
            mask = ScheduleGuard::setSlot(mask, day, slot, !isAvail);

            studentPtr->setScheduleBitmask(mask);
            if (userService) userService->updateUser(*studentPtr);
            else if (dbEngine) dbEngine->updateUser(*studentPtr);
            printSuccess("Ca học " + ScheduleGuard::formatSlotSingle(day, slot) + " hiện đã chuyển thành " + (!isAvail ? "RẢNH" : "BẬN"));
        } else if (act == 2) {
            studentPtr->setScheduleBitmask(ScheduleGuard::VALID_SCHEDULE_MASK);
            if (userService) userService->updateUser(*studentPtr);
            else if (dbEngine) dbEngine->updateUser(*studentPtr);
            printSuccess("Đã đặt lại tất cả các ca học thành RẢNH!");
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

void ConsoleApp::handleStudentBookings() {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);

    while (true) {
        renderTimetableGrid(studentPtr->getId(), UserRole::STUDENT);

        auto bookings = bookingService ? bookingService->getStudentCourseBookings(studentPtr->getId())
                                      : (dbEngine ? dbEngine->getCourseBookingsByStudentId(studentPtr->getId()) : std::vector<CourseBooking>{});

        printHeader("QUẢN LÝ KHÓA HỌC ĐÃ ĐẶT (" + std::to_string(bookings.size()) + " KHÓA HỌC)");
        if (bookings.empty()) {
            printInfo("Bạn chưa có khóa học nào trong hệ thống.");
        } else {
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
            std::cout << "| ID  | Tên Gia sư          | Môn học           | Học phí (VND)| Bắt đầu    | Số tuần| Khung lịch học     | Trạng thái        |\n";
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
            for (const auto& b : bookings) {
                std::string sched = ScheduleGuard::formatSlotSingle(b.dayOfWeek, b.slotIndex);
                std::cout << "| " << padRightUTF8(std::to_string(b.id), 3)
                          << " | " << padRightUTF8(b.tutorName, 19)
                          << " | " << padRightUTF8(b.subject, 17)
                          << " | " << padRightUTF8(std::to_string((long long)b.hourlyRate) + " VND", 12)
                          << " | " << padRightUTF8(b.startDate, 10)
                          << " | " << padRightUTF8(std::to_string(b.weeksCount), 6)
                          << " | " << padRightUTF8(sched, 18)
                          << " | " << padCenterUTF8(translateStatus(b.status), 17) << " |\n";
            }
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
        }

        printDivider();
        std::cout << " 1. Đặt khóa học mới với Gia sư\n";
        std::cout << " 2. Hủy một khóa học\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();

        int act = getValidIntChoice(0, 2, "Chọn thao tác [0-2]: ");
        if (act == 0) {
            break;
        }

        if (act == 1) {
            handleBookLessonScheduleGuard();
        } else if (act == 2) {
            int bId = readIntInput("Nhập Mã ID khóa học muốn hủy: ");
            std::string outMsg;
            bool ok = false;
            if (bookingService) {
                ok = bookingService->cancelCourseBooking(studentPtr->getId(), UserRole::STUDENT, bId, outMsg);
            } else if (dbEngine) {
                auto cbRepo = dbEngine->getCourseBookingRepository();
                if (cbRepo) {
                    auto bService = std::make_shared<BookingService>(dbEngine->getUserRepository(), dbEngine->getReviewRepository(), cbRepo);
                    ok = bService->cancelCourseBooking(studentPtr->getId(), UserRole::STUDENT, bId, outMsg);
                }
            }
            if (ok) {
                printSuccess("Hủy khóa học #" + std::to_string(bId) + " thành công!");
            } else {
                printError(outMsg.empty() ? "Mã ID khóa học không hợp lệ hoặc không thể hủy!" : outMsg);
            }
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

void ConsoleApp::handleLeaveReviewAndRating() {
    auto studentPtr = std::static_pointer_cast<Student>(currentUser);

    while (true) {
        auto bookings = bookingService ? bookingService->getStudentCourseBookings(studentPtr->getId())
                                      : (dbEngine ? dbEngine->getCourseBookingsByStudentId(studentPtr->getId()) : std::vector<CourseBooking>{});

        printHeader("ĐÁNH GIÁ & VIẾT PHẢN HỒI CHO KHÓA HỌC ĐÃ HOÀN THÀNH");
        std::vector<CourseBooking> completedBookings;
        for (const auto& b : bookings) {
            if (b.status == "COMPLETED") {
                completedBookings.push_back(b);
            }
        }

        if (completedBookings.empty()) {
            printWarning("Bạn chưa có khóa học nào ở trạng thái HOÀN THÀNH để đánh giá!");
            break;
        }

        for (const auto& b : completedBookings) {
            std::cout << " Khóa #" << b.id << " | Gia sư: " << b.tutorName << " | Môn học: " << b.subject << " (" << b.weeksCount << " tuần)\n";
        }

        printDivider();
        std::cout << " 1. Viết Đánh giá & Phản hồi cho khóa học\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int opt = getValidIntChoice(0, 1, "Chọn thao tác [0-1]: ");
        if (opt == 0) break;

        if (opt == 1) {
            int bId = readIntInput("Nhập Mã ID khóa học đã HOÀN THÀNH để đánh giá: ");
            int score = getValidIntChoice(1, 5, "Nhập số điểm đánh giá (1 đến 5 Sao): ");

            // Clear input buffer explicitly before reading written comment line
            clearInputBuffer();

            std::string comment;
            while (true) {
                comment = readStringInput("Nhập nội dung nhận xét / phản hồi: ");
                if (!comment.empty()) {
                    break;
                }
                printError("Nội dung phản hồi không được để trống! Vui lòng nhập nhận xét.");
            }

            if (reviewService) {
                std::string outMsg;
                if (reviewService->submitReview(studentPtr->getId(), bId, score, comment, outMsg)) {
                    printSuccess("Cảm ơn bạn! Đã gửi đánh giá & phản hồi thành công.");
                } else {
                    printError(outMsg.empty() ? "Gửi đánh giá thất bại!" : outMsg);
                }
            } else if (dbEngine) {
                auto cbPtr = dbEngine->findCourseBookingById(bId);
                if (!cbPtr || cbPtr->studentId != studentPtr->getId() || cbPtr->status != "COMPLETED") {
                    printError("Mã ID khóa học đã hoàn thành không hợp lệ!");
                    continue;
                }

                Review review(dbEngine->getNextReviewId(), cbPtr->id, studentPtr->getId(), cbPtr->tutorId,
                              studentPtr->getFullName(), score, comment, DateUtils::getCurrentDateString());

                if (dbEngine->saveReview(review)) {
                    auto tUser = dbEngine->findUserById(cbPtr->tutorId);
                    if (tUser && tUser->getRole() == UserRole::TUTOR) {
                        auto tutorPtr = std::static_pointer_cast<Tutor>(tUser);
                        auto allRev = dbEngine->getReviewsByTutorId(tutorPtr->getId());
                        double sum = 0.0;
                        for (const auto& r : allRev) sum += r.ratingScore;
                        tutorPtr->setTotalReviews((int)allRev.size());
                        tutorPtr->setRating(allRev.empty() ? 0.0 : sum / allRev.size());
                        dbEngine->updateUser(*tutorPtr);
                    }
                    printSuccess("Cảm ơn bạn! Đã gửi đánh giá & phản hồi thành công.");
                }
            }
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

// =========================================================================
// TUTOR MENU IMPLEMENTATION
// =========================================================================
void ConsoleApp::renderTutorMenu() {
    renderTutorOverview();

    printDivider();
    std::cout << " 1. Chỉnh sửa thông tin giảng dạy & Bio\n";
    std::cout << " 2. " << COLOR_GREEN << COLOR_BOLD << "Chỉnh sửa Lịch Dạy Rảnh / Bận tương tác (Ma trận 7x8)" << COLOR_RESET << "\n";
    std::cout << " 3. " << COLOR_YELLOW << COLOR_BOLD << "Quản lý Đơn đặt lịch & Duyệt / Từ chối buổi học" << COLOR_RESET << "\n";
    std::cout << " 4. Thống kê số buổi dạy & Phản hồi từ học viên\n";
    std::cout << " 5. Đổi mật khẩu\n";
    std::cout << " 6. Đăng xuất\n";
    printDivider();

    int choice = getValidIntChoice(1, 6, "Chọn chức năng [1-6]: ");
    switch (choice) {
        case 1: handleTutorProfileUpdate(); break;
        case 2: handleTutorScheduleMatrixEditor(); break;
        case 3: handleTutorPendingBookings(); break;
        case 4: handleTutorEarningsStats(); break;
        case 5: handleChangePassword(); break;
        case 6:
            isLoggedIn = false;
            currentUser = nullptr;
            printInfo("Đã đăng xuất thành công.");
            break;
        default: printError("Lựa chọn không hợp lệ!"); break;
    }
}

void ConsoleApp::handleTutorProfileUpdate() {
    auto tutorPtr = std::static_pointer_cast<Tutor>(currentUser);

    while (true) {
        printHeader("CHỌN THÔNG TIN CẦN CHỈNH SỬA");
        std::cout << " 1. Môn học chuyên môn [" << tutorPtr->getSubjectSpecialty() << "]\n";
        std::cout << " 2. Học phí (VNĐ/buổi) [" << (long long)tutorPtr->getHourlyRate() << " VNĐ/buổi]\n";
        std::cout << " 3. Giới thiệu bản thân (Bio) [" << tutorPtr->getBio() << "]\n";
        std::cout << " 4. Địa chỉ Email [" << tutorPtr->getEmail() << "]\n";
        std::cout << " 5. Họ và tên [" << tutorPtr->getFullName() << "]\n";
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();

        int fieldChoice = getValidIntChoice(0, 5, "Chọn mục cần sửa [0-5]: ");
        if (fieldChoice == 0) {
            break;
        }

        bool updated = false;
        switch (fieldChoice) {
            case 1: {
                std::string newSubj = readStringInput("Nhập Môn học chuyên môn mới [" + tutorPtr->getSubjectSpecialty() + "]: ");
                if (!newSubj.empty()) {
                    tutorPtr->setSubjectSpecialty(newSubj);
                    updated = true;
                }
                break;
            }
            case 2: {
                double newRate = readDoubleInput("Nhập Mức học phí mới (VNĐ/buổi) [" + std::to_string((long long)tutorPtr->getHourlyRate()) + "]: ");
                if (newRate > 0) {
                    tutorPtr->setHourlyRate(newRate);
                    updated = true;
                } else {
                    printError("Mức học phí phải lớn hơn 0!");
                }
                break;
            }
            case 3: {
                std::string newBio = readStringInput("Nhập Giới thiệu bản thân mới [" + tutorPtr->getBio() + "]: ");
                if (!newBio.empty()) {
                    tutorPtr->setBio(newBio);
                    updated = true;
                }
                break;
            }
            case 4: {
                while (true) {
                    std::string newEmail = readStringInput("Nhập Địa chỉ Email mới (Ví dụ: ten@domain.com, hoặc 0 để hủy): ");
                    if (newEmail == "0") {
                        printInfo("Đã hủy cập nhật email.");
                        break;
                    }
                    if (isValidEmail(newEmail)) {
                        tutorPtr->setEmail(newEmail);
                        updated = true;
                        break;
                    }
                    printError("Định dạng email không hợp lệ (Ví dụ: ten@domain.com)!");
                }
                break;
            }
            case 5: {
                std::string newName = readStringInput("Nhập Họ và tên mới [" + tutorPtr->getFullName() + "]: ");
                if (!newName.empty()) {
                    tutorPtr->setFullName(newName);
                    updated = true;
                }
                break;
            }
            default:
                printError("Lựa chọn mục sửa không hợp lệ!");
                break;
        }

        if (updated) {
            bool ok = userService ? userService->updateUser(*tutorPtr)
                                  : (dbEngine && dbEngine->updateUser(*tutorPtr));
            if (ok) {
                printSuccess("Cập nhật hồ sơ Gia sư thành công!");
            } else {
                printError("Lưu hồ sơ đã cập nhật vào cơ sở dữ liệu thất bại!");
            }
        }
    }
}

void ConsoleApp::handleTutorScheduleMatrixEditor() {
    auto tutorPtr = std::static_pointer_cast<Tutor>(currentUser);

    while (true) {
        renderTimetableGrid(tutorPtr->getId(), UserRole::TUTOR);

        uint64_t mask = tutorPtr->getScheduleBitmask();

        printDivider();
        std::cout << "Chọn thao tác:\n 1. Bật/Tắt trạng thái ca học (Rảnh/Bận)\n 2. Đặt lại toàn bộ các ca thành Rảnh\n 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int act = getValidIntChoice(0, 2, "Chọn thao tác [0-2]: ");
        if (act == 0) {
            break;
        }

        if (act == 1) {
            int day = getValidIntChoice(0, 6, "Chọn thứ [0=Thứ 2, 1=Thứ 3, 2=Thứ 4, 3=Thứ 5, 4=Thứ 6, 5=Thứ 7, 6=Chủ Nhật]: ");
            int slot = getValidIntChoice(0, 7, "Chọn ca học [0-7]: ");

            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
            bool isAvail = (mask & sMask) != 0;
            mask = ScheduleGuard::setSlot(mask, day, slot, !isAvail);

            tutorPtr->setScheduleBitmask(mask);
            if (userService) userService->updateUser(*tutorPtr);
            else if (dbEngine) dbEngine->updateUser(*tutorPtr);
            printSuccess("Ca học " + ScheduleGuard::formatSlotSingle(day, slot) + " hiện đã chuyển thành " + (!isAvail ? "RẢNH (ƯU TIÊN)" : "BẬN (KHÔNG RẢNH)"));
        } else if (act == 2) {
            tutorPtr->setScheduleBitmask(ScheduleGuard::VALID_SCHEDULE_MASK);
            if (userService) userService->updateUser(*tutorPtr);
            else if (dbEngine) dbEngine->updateUser(*tutorPtr);
            printSuccess("Đã đặt lại tất cả các ca học thành RẢNH!");
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

void ConsoleApp::handleTutorPendingBookings() {
    auto tutorPtr = std::static_pointer_cast<Tutor>(currentUser);

    while (true) {
        renderTimetableGrid(tutorPtr->getId(), UserRole::TUTOR);

        auto bookings = bookingService ? bookingService->getTutorCourseBookings(tutorPtr->getId())
                                      : (dbEngine ? dbEngine->getCourseBookingsByTutorId(tutorPtr->getId()) : std::vector<CourseBooking>{});

        printHeader("QUẢN LÝ KHÓA HỌC GIA SƯ (" + std::to_string(bookings.size()) + " KHÓA HỌC)");
        if (bookings.empty()) {
            printInfo("Không có yêu cầu đặt khóa học nào.");
        } else {
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
            std::cout << "| ID  | Tên Học viên        | Môn học           | Học phí (VND)| Bắt đầu    | Số tuần| Khung lịch học     | Trạng thái        |\n";
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
            for (const auto& b : bookings) {
                std::string sched = ScheduleGuard::formatSlotSingle(b.dayOfWeek, b.slotIndex);
                std::cout << "| " << padRightUTF8(std::to_string(b.id), 3)
                          << " | " << padRightUTF8(b.studentName, 19)
                          << " | " << padRightUTF8(b.subject, 17)
                          << " | " << padRightUTF8(std::to_string((long long)b.hourlyRate) + " VND", 12)
                          << " | " << padRightUTF8(b.startDate, 10)
                          << " | " << padRightUTF8(std::to_string(b.weeksCount), 6)
                          << " | " << padRightUTF8(sched, 18)
                          << " | " << padCenterUTF8(translateStatus(b.status), 17) << " |\n";
            }
            std::cout << "+-----+---------------------+-------------------+--------------+------------+--------+--------------------+-------------------+\n";
        }

        printDivider();
        std::cout << " 1. Chấp nhận khóa học \n 2. Từ chối khóa học\n 3. Đánh dấu Hoàn thành khóa học \n 0. Quay lại Bảng điều khiển\n";
        printDivider();

        int act = getValidIntChoice(0, 3, "Chọn thao tác [0-3]: ");
        if (act == 0) {
            break;
        }

        if (act >= 1 && act <= 3) {
            int bId = readIntInput("Nhập Mã ID khóa học: ");
            std::string outMsg;
            bool ok = false;
            if (bookingService) {
                if (act == 1) {
                    std::cout << "\n" << COLOR_CYAN << "--- CHÍNH SÁCH CAM KẾT KHÓA HỌC (" << CourseBooking::CURRENT_COMMITMENT_VERSION << ") ---" << COLOR_RESET << "\n";
                    std::cout << CourseBooking::COMMITMENT_POLICY_TEXT << "\n";
                    std::cout << "Gia sư có đồng ý với điều khoản cam kết để chấp nhận khóa học này không? (Y/N): ";
                    std::string ackStr = readStringInput("");
                    bool ack = (ackStr == "Y" || ackStr == "y");
                    if (!ack) {
                        printError("Gia sư cần đồng ý với chính sách cam kết để chấp nhận khóa học!");
                        continue;
                    }
                    ok = bookingService->acceptCourseBooking(tutorPtr->getId(), bId, ack, outMsg);
                    if (ok) printSuccess("Đã CHẤP NHẬN khóa học #" + std::to_string(bId) + "!");
                    else printError(outMsg.empty() ? "Chấp nhận khóa học thất bại!" : outMsg);
                } else if (act == 2) {
                    ok = bookingService->rejectCourseBooking(tutorPtr->getId(), bId, outMsg);
                    if (ok) printSuccess("Đã TỪ CHỐI khóa học #" + std::to_string(bId) + ".");
                    else printError(outMsg.empty() ? "Từ chối khóa học thất bại!" : outMsg);
                } else if (act == 3) {
                    ok = bookingService->cancelCourseBooking(tutorPtr->getId(), UserRole::TUTOR, bId, outMsg);
                    if (ok) printSuccess("Đã HỦY khóa học #" + std::to_string(bId) + "!");
                    else printError(outMsg.empty() ? "Hủy khóa học thất bại!" : outMsg);
                }
            } else if (dbEngine) {
                auto cbRepo = dbEngine->getCourseBookingRepository();
                if (cbRepo) {
                    auto bService = std::make_shared<BookingService>(dbEngine->getUserRepository(), dbEngine->getReviewRepository(), cbRepo);
                    if (act == 1) {
                        std::cout << "\n" << COLOR_CYAN << "--- CHÍNH SÁCH CAM KẾT KHÓA HỌC (" << CourseBooking::CURRENT_COMMITMENT_VERSION << ") ---" << COLOR_RESET << "\n";
                        std::cout << CourseBooking::COMMITMENT_POLICY_TEXT << "\n";
                        std::cout << "Gia sư có đồng ý với điều khoản cam kết để chấp nhận khóa học này không? (Y/N): ";
                        std::string ackStr = readStringInput("");
                        bool ack = (ackStr == "Y" || ackStr == "y");
                        if (!ack) {
                            printError("Gia sư cần đồng ý với chính sách cam kết để chấp nhận khóa học!");
                            continue;
                        }
                        ok = bService->acceptCourseBooking(tutorPtr->getId(), bId, ack, outMsg);
                    }
                    else if (act == 2) ok = bService->rejectCourseBooking(tutorPtr->getId(), bId, outMsg);
                    else if (act == 3) ok = bService->cancelCourseBooking(tutorPtr->getId(), UserRole::TUTOR, bId, outMsg);
                    if (ok) printSuccess("Thao tác thành công trên khóa học #" + std::to_string(bId));
                    else printError(outMsg.empty() ? "Thao tác thất bại!" : outMsg);
                }
            }
        } else {
            printError("Lựa chọn không hợp lệ!");
        }
    }
}

void ConsoleApp::handleTutorEarningsStats() {
    auto tutorPtr = std::static_pointer_cast<Tutor>(currentUser);

    while (true) {
        auto reviews = reviewService ? reviewService->getReviewsByTutorId(tutorPtr->getId())
                                     : (dbEngine ? dbEngine->getReviewsByTutorId(tutorPtr->getId()) : std::vector<Review>{});
        auto bookings = bookingService ? bookingService->getTutorCourseBookings(tutorPtr->getId())
                                       : (dbEngine ? dbEngine->getCourseBookingsByTutorId(tutorPtr->getId()) : std::vector<CourseBooking>{});

        int completedCount = 0;
        int pendingCount = 0;
        for (const auto& b : bookings) {
            if (b.status == "COMPLETED") completedCount++;
            else if (b.status == "PENDING" || b.status == "ACCEPTED") pendingCount++;
        }

        printHeader("THỐNG KÊ SỐ BUỔI DẠY & ĐÁNH GIÁ TỪ HỌC VIÊN");
        std::cout << " Số buổi dạy đã hoàn thành  : " << completedCount << " buổi\n";
        std::cout << " Số buổi chờ duyệt / sắp tới: " << pendingCount << " buổi\n";
        std::cout << " Điểm đánh giá trung bình   : " << std::fixed << std::setprecision(1) << tutorPtr->getRating() << " * (" << reviews.size() << " lượt phản hồi)\n";
        printDivider();

        std::cout << "Danh sách Phản hồi từ Học viên:\n";
        if (reviews.empty()) {
            std::cout << "  Chưa có lượt phản hồi nào từ học viên.\n";
        } else {
            for (const auto& r : reviews) {
                std::cout << "  - [" << std::fixed << std::setprecision(1) << (double)r.ratingScore << " *] "
                          << COLOR_BOLD << r.studentName << COLOR_RESET << ": \"" << r.comment << "\" (" << r.createdAt << ")\n";
            }
        }
        printDivider();
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();

        int choice = getValidIntChoice(0, 0, "Chọn thao tác [0]: ");
        if (choice == 0) break;
    }
}

// =========================================================================
// ADMIN MENU IMPLEMENTATION
// =========================================================================
void ConsoleApp::renderAdminMenu() {
    printHeader("TRUNG TÂM QUẢN TRỊ VIÊN | " + currentUser->getFullName());
    std::cout << " Trạng thái hệ thống: BÌNH THƯỜNG\n";
    printDivider();

    std::cout << " 1. Tổng quan Thống kê & Báo cáo số liệu\n";
    std::cout << " 2. Quản lý Người dùng (Khóa/Mở khóa & Duyệt Gia sư)\n";
    std::cout << " 3. Đổi mật khẩu\n";
    std::cout << " 4. Đăng xuất\n";
    printDivider();

    int choice = getValidIntChoice(1, 4, "Chọn chức năng [1-4]: ");
    switch (choice) {
        case 1: handleAdminSystemMetrics(); break;
        case 2: handleAdminUserManagement(); break;
        case 3: handleChangePassword(); break;
        case 4:
            isLoggedIn = false;
            currentUser = nullptr;
            printInfo("Đã đăng xuất thành công.");
            break;
        default: printError("Lựa chọn không hợp lệ!"); break;
    }
}

void ConsoleApp::handleAdminSystemMetrics() {
    while (true) {
        int tutorCount = 0;
        int studentCount = 0;
        int totalBookingsCount = 0;
        int completedCount = 0;
        int activeMatches = 0;
        int pendingVerification = 0;

        if (adminService) {
            auto m = adminService->calculateMetrics();
            tutorCount = m.totalTutors;
            studentCount = m.totalStudents;
            totalBookingsCount = m.totalBookings;
            completedCount = m.completedBookings;
            activeMatches = m.activeMatches;
            pendingVerification = m.pendingVerification;
        } else {
            auto tutors = dbEngine ? dbEngine->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{};
            auto students = dbEngine ? dbEngine->getAllStudents() : std::vector<std::shared_ptr<Student>>{};
            auto bookings = dbEngine ? dbEngine->getAllCourseBookings() : std::vector<CourseBooking>{};
            tutorCount = (int)tutors.size();
            studentCount = (int)students.size();
            totalBookingsCount = (int)bookings.size();
            for (const auto& b : bookings) {
                if (b.status == "COMPLETED") {
                    completedCount++;
                } else if (b.status == "ACCEPTED") {
                    activeMatches++;
                }
            }
            for (const auto& t : tutors) {
                if (t && !t->getIsVerified()) pendingVerification++;
            }
        }

        printHeader("BẢNG THỐNG KÊ HOẠT ĐỘNG HỆ THỐNG");
        std::cout << " +-----------------------------------+-----------------------------------+\n";
        std::cout << " |  Tổng số Gia sư: " << padRightUTF8(std::to_string(tutorCount), 17) << "|  Tổng số Học viên: " << padRightUTF8(std::to_string(studentCount), 15) << "|\n";
        std::cout << " +-----------------------------------+-----------------------------------+\n";
        std::cout << " |  Tổng yêu cầu đăng ký: " << padRightUTF8(std::to_string(totalBookingsCount), 11) << "|  Khóa học đang diễn ra: " << padRightUTF8(std::to_string(activeMatches), 10) << "|\n";
        std::cout << " +-----------------------------------+-----------------------------------+\n";
        std::cout << " |  Khóa học đã hoàn thành: " << padRightUTF8(std::to_string(completedCount), 9) << "|  Gia sư chờ xác minh: " << padRightUTF8(std::to_string(pendingVerification), 12) << "|\n";
        std::cout << " +-----------------------------------+-----------------------------------+\n";

        printDivider();
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int choice = getValidIntChoice(0, 0, "Chọn thao tác [0]: ");
        if (choice == 0) break;
    }
}

void ConsoleApp::handleAdminUserManagement() {
    while (true) {
        auto users = adminService ? adminService->getAllUsers()
                                  : (userService ? userService->getAllUsers() : (dbEngine ? dbEngine->getAllUsers() : std::vector<std::shared_ptr<User>>{}));
        printHeader("QUẢN LÝ NGƯỜI DÙNG & HÀNG ĐỢI XÁC MINH");

        std::cout << "+----+----------------------+-------------------+-------------------+------------+-------------------+\n";
        std::cout << "| ID | Họ và Tên            | Vai trò           | Chuyên môn/Môn    | Xác minh   | Trạng thái        |\n";
        std::cout << "+----+----------------------+-------------------+-------------------+------------+-------------------+\n";
        for (const auto& u : users) {
            std::string subj = "N/A";
            std::string ver = "N/A";
            if (u->getRole() == UserRole::TUTOR) {
                auto t = std::static_pointer_cast<Tutor>(u);
                subj = t->getSubjectSpecialty();
                ver = t->getIsVerified() ? "ĐÃ XÁC MINH" : "CHƯA XÁC MINH";
            } else if (u->getRole() == UserRole::STUDENT) {
                auto s = std::static_pointer_cast<Student>(u);
                subj = s->getPreferredSubject();
            }

            std::cout << "| " << padRightUTF8(std::to_string(u->getId()), 2)
                      << " | " << padRightUTF8(u->getFullName(), 20)
                      << " | " << padRightUTF8(u->getRoleString(), 17)
                      << " | " << padRightUTF8(subj, 17)
                      << " | " << padCenterUTF8(ver, 10)
                      << " | " << padCenterUTF8(u->getIsActive() ? "HOẠT ĐỘNG" : "BỊ KHÓA", 17) << " |\n";
        }
        std::cout << "+----+----------------------+-------------------+-------------------+------------+-------------------+\n";

        std::cout << "\nThao tác Quản trị viên:\n 1. Xác minh tài khoản Gia sư\n 2. Khóa tài khoản Người dùng\n 3. Mở khóa tài khoản Người dùng\n 4. Xem Chi tiết Hồ sơ & Lịch sử Đánh giá / Phản hồi\n 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int act = getValidIntChoice(0, 4, "Chọn thao tác [0-4]: ");
        if (act == 0) break;

        if (act == 1) {
            int uId = readIntInput("Nhập Mã ID Gia sư muốn Xác minh: ");
            std::string msg;
            bool ok = false;
            if (adminService) {
                ok = adminService->verifyTutor(uId, true, msg);
            } else if (userService) {
                auto u = userService->getUserById(uId);
                if (u && u->getRole() == UserRole::TUTOR) {
                    auto t = std::static_pointer_cast<Tutor>(u);
                    t->setIsVerified(true);
                    ok = userService->updateUser(*t);
                }
            } else if (dbEngine) {
                auto u = dbEngine->findUserById(uId);
                if (u && u->getRole() == UserRole::TUTOR) {
                    auto t = std::static_pointer_cast<Tutor>(u);
                    t->setIsVerified(true);
                    ok = dbEngine->updateUser(*t);
                }
            }

            if (ok) {
                printSuccess("Gia sư đã được XÁC MINH thành công!");
            } else {
                printError(msg.empty() ? "Mã ID Gia sư không hợp lệ!" : msg);
            }
        } else if (act == 2) {
            int uId = readIntInput("Nhập Mã ID Người dùng muốn Khóa: ");
            std::string msg;
            bool ok = false;
            if (adminService) {
                ok = adminService->toggleUserLock(uId, msg);
            } else if (userService) {
                auto u = userService->getUserById(uId);
                if (u) {
                    u->setIsActive(false);
                    ok = userService->updateUser(*u);
                }
            } else if (dbEngine) {
                auto u = dbEngine->findUserById(uId);
                if (u) {
                    u->setIsActive(false);
                    ok = dbEngine->updateUser(*u);
                }
            }

            if (ok) {
                printSuccess("Tài khoản đã BỊ KHÓA!");
            } else {
                printError(msg.empty() ? "Mã ID Người dùng không hợp lệ!" : msg);
            }
        } else if (act == 3) {
            int uId = readIntInput("Nhập Mã ID Người dùng muốn Mở khóa: ");
            std::string msg;
            bool ok = false;
            if (adminService) {
                ok = adminService->toggleUserLock(uId, msg);
            } else if (userService) {
                auto u = userService->getUserById(uId);
                if (u) {
                    u->setIsActive(true);
                    ok = userService->updateUser(*u);
                }
            } else if (dbEngine) {
                auto u = dbEngine->findUserById(uId);
                if (u) {
                    u->setIsActive(true);
                    ok = dbEngine->updateUser(*u);
                }
            }

            if (ok) {
                printSuccess("Tài khoản đã được MỞ KHÓA!");
            } else {
                printError(msg.empty() ? "Mã ID Người dùng không hợp lệ!" : msg);
            }
        } else if (act == 4) {
            int targetId = readIntInput("Nhập Mã ID Người dùng cần tra cứu (hoặc 0 để quay lại): ");
            if (targetId == 0) continue;

            auto userPtr = userService ? userService->getUserById(targetId)
                                       : (dbEngine ? dbEngine->findUserById(targetId) : nullptr);
            if (!userPtr) {
                printError("Không tìm thấy Người dùng với Mã ID " + std::to_string(targetId) + "!");
                continue;
            }

            if (userPtr->getRole() == UserRole::TUTOR) {
                auto tutorPtr = std::static_pointer_cast<Tutor>(userPtr);
                auto bookings = bookingService ? bookingService->getTutorCourseBookings(tutorPtr->getId())
                                               : (dbEngine ? dbEngine->getCourseBookingsByTutorId(tutorPtr->getId()) : std::vector<CourseBooking>{});
                int completedCount = 0;
                for (const auto& b : bookings) {
                    if (b.status == "COMPLETED") completedCount++;
                }

                printHeader("CHI TIẾT HỒ SƠ & LỊCH SỬ ĐÁNH GIÁ GIA SƯ: " + tutorPtr->getFullName());
                std::cout << " Mã ID Gia sư           : " << tutorPtr->getId() << "\n";
                std::cout << " Tên tài khoản          : " << tutorPtr->getUsername() << "\n";
                std::cout << " Họ và tên              : " << tutorPtr->getFullName() << "\n";
                std::cout << " Địa chỉ Email          : " << tutorPtr->getEmail() << "\n";
                std::cout << " Chuyên môn             : " << tutorPtr->getSubjectSpecialty() << "\n";
                std::cout << " Mức học phí            : " << (long long)tutorPtr->getHourlyRate() << " VNĐ/buổi\n";
                std::cout << " Trạng thái xác minh    : " << (tutorPtr->getIsVerified() ? "ĐÃ XÁC MINH" : "CHƯA XÁC MINH") << "\n";
                std::cout << " Trạng thái tài khoản   : " << (tutorPtr->getIsActive() ? "HOẠT ĐỘNG" : "BỊ KHÓA") << "\n";
                std::cout << " Tổng số buổi đã dạy    : " << completedCount << " buổi hoàn thành\n";
                printDivider();

                while (true) {
                    auto reviews = reviewService ? reviewService->getReviewsByTutorId(tutorPtr->getId())
                                                 : (dbEngine ? dbEngine->getReviewsByTutorId(tutorPtr->getId()) : std::vector<Review>{});
                    std::cout << COLOR_CYAN << COLOR_BOLD << "LỊCH SỬ NHẬN ĐÁNH GIÁ CỦA GIA SƯ (" << reviews.size() << " lượt):" << COLOR_RESET << "\n";
                    if (reviews.empty()) {
                        printInfo("Gia sư này chưa nhận được phản hồi nào.");
                    } else {
                        std::cout << "+-----+---------------------+-----------+------------+--------------------------------------------------+\n";
                        std::cout << "| ID  | Học viên            | Điểm số   | Ngày       | Nội dung phản hồi                                |\n";
                        std::cout << "+-----+---------------------+-----------+------------+--------------------------------------------------+\n";
                        for (const auto& r : reviews) {
                            std::cout << "| " << padRightUTF8(std::to_string(r.id), 3)
                                      << " | " << padRightUTF8(r.studentName, 19)
                                      << " | " << padCenterUTF8(std::to_string(r.ratingScore) + " *", 9)
                                      << " | " << padRightUTF8(r.createdAt, 10)
                                      << " | " << padRightUTF8(r.comment, 48) << " |\n";
                        }
                        std::cout << "+-----+---------------------+-----------+------------+--------------------------------------------------+\n";
                    }

                    printDivider();
                    std::cout << "Thao tác Kiểm duyệt (Moderation):\n 1. Xóa một đánh giá vi phạm tiêu chuẩn\n 0. Quay lại\n";
                    printDivider();
                    int modAct = getValidIntChoice(0, 1, "Chọn thao tác [0-1]: ");
                    if (modAct == 0) break;

                    if (modAct == 1) {
                        int rId = readIntInput("Nhập Mã ID đánh giá cần xóa: ");
                        std::string msg;
                        bool ok = false;
                        if (adminService) {
                            ok = adminService->deleteViolatingReview(rId, msg);
                        } else if (reviewService) {
                            ok = reviewService->deleteReview(rId, msg);
                        } else if (dbEngine) {
                            ok = dbEngine->deleteReview(rId);
                            if (ok) {
                                auto remainingReviews = dbEngine->getReviewsByTutorId(tutorPtr->getId());
                                tutorPtr->setTotalReviews((int)remainingReviews.size());
                                if (remainingReviews.empty()) {
                                    tutorPtr->setRating(0.0);
                                } else {
                                    double sum = 0.0;
                                    for (const auto& rev : remainingReviews) sum += rev.ratingScore;
                                    tutorPtr->setRating(sum / remainingReviews.size());
                                }
                                dbEngine->updateUser(*tutorPtr);
                            }
                        }

                        if (ok) {
                            printSuccess("Đã xóa đánh giá #" + std::to_string(rId) + " và cập nhật lại điểm đánh giá trung bình cho gia sư!");
                        } else {
                            printError(msg.empty() ? ("Không tìm thấy Mã ID đánh giá #" + std::to_string(rId) + "!") : msg);
                        }
                    }
                }

            } else if (userPtr->getRole() == UserRole::STUDENT) {
                auto studentPtr = std::static_pointer_cast<Student>(userPtr);
                printHeader("CHI TIẾT HỒ SƠ & LỊCH SỬ ĐÁNH GIÁ HỌC VIÊN: " + studentPtr->getFullName());
                std::cout << " Mã ID Học viên         : " << studentPtr->getId() << "\n";
                std::cout << " Tên tài khoản          : " << studentPtr->getUsername() << "\n";
                std::cout << " Họ và tên              : " << studentPtr->getFullName() << "\n";
                std::cout << " Địa chỉ Email          : " << studentPtr->getEmail() << "\n";
                std::cout << " Khối lớp               : Lớp " << studentPtr->getGradeLevel() << "\n";
                std::cout << " Môn học mục tiêu       : " << studentPtr->getPreferredSubject() << "\n";
                std::cout << " Ngân sách tối đa       : " << (long long)studentPtr->getMaxBudget() << " VNĐ/buổi\n";
                std::cout << " Trạng thái tài khoản   : " << (studentPtr->getIsActive() ? "HOẠT ĐỘNG" : "BỊ KHÓA") << "\n";
                printDivider();

                auto studentReviews = reviewService ? reviewService->getReviewsByStudentId(studentPtr->getId())
                                                    : std::vector<Review>{};
                if (!reviewService && dbEngine) {
                    auto allReviews = dbEngine->getAllReviews();
                    for (const auto& r : allReviews) {
                        if (r.studentId == studentPtr->getId()) {
                            studentReviews.push_back(r);
                        }
                    }
                }

                if (studentReviews.empty()) {
                    printInfo("Học viên này chưa gửi đánh giá nào.");
                } else {
                    std::cout << COLOR_CYAN << COLOR_BOLD << "LỊCH SỬ ĐÁNH GIÁ ĐÃ GỬI CỦA HỌC VIÊN (" << studentReviews.size() << " lượt):" << COLOR_RESET << "\n";
                    std::cout << "+-----+---------------------+-------------------+-----------+------------------------------------+------------+\n";
                    std::cout << "| ID  | Gia sư được đánh giá | Môn học           | Điểm số   | Nội dung nhận xét                  | Ngày gửi   |\n";
                    std::cout << "+-----+---------------------+-------------------+-----------+------------------------------------+------------+\n";
                    for (const auto& r : studentReviews) {
                        std::string tName = "Gia sư #" + std::to_string(r.tutorId);
                        std::string subj = "N/A";
                        auto tUser = userService ? userService->getUserById(r.tutorId)
                                                 : (dbEngine ? dbEngine->findUserById(r.tutorId) : nullptr);
                        if (tUser && tUser->getRole() == UserRole::TUTOR) {
                            auto tPtr = std::static_pointer_cast<Tutor>(tUser);
                            tName = tPtr->getFullName();
                            subj = tPtr->getSubjectSpecialty();
                        }
                        std::cout << "| " << padRightUTF8(std::to_string(r.id), 3)
                                  << " | " << padRightUTF8(tName, 19)
                                  << " | " << padRightUTF8(subj, 17)
                                  << " | " << padCenterUTF8(std::to_string(r.ratingScore) + " *", 9)
                                  << " | " << padRightUTF8(r.comment, 34)
                                  << " | " << padRightUTF8(r.createdAt, 10) << " |\n";
                    }
                    std::cout << "+-----+---------------------+-------------------+-----------+------------------------------------+------------+\n";
                }

                printDivider();
                std::cout << " 0. Quay lại\n";
                printDivider();
                getValidIntChoice(0, 0, "Chọn thao tác [0]: ");

            } else if (userPtr->getRole() == UserRole::ADMIN) {
                auto adminPtr = std::static_pointer_cast<Admin>(userPtr);
                printHeader("CHI TIẾT HỒ SƠ QUẢN TRỊ VIÊN: " + adminPtr->getFullName());
                std::cout << " Mã ID                  : " << adminPtr->getId() << "\n";
                std::cout << " Tên tài khoản          : " << adminPtr->getUsername() << "\n";
                std::cout << " Họ và tên              : " << adminPtr->getFullName() << "\n";
                std::cout << " Cấp độ quản trị        : Cấp " << adminPtr->getPermissionLevel() << "\n";
                std::cout << " Trạng thái tài khoản   : " << (adminPtr->getIsActive() ? "HOẠT ĐỘNG" : "BỊ KHÓA") << "\n";
                printDivider();
                std::cout << " 0. Quay lại\n";
                printDivider();
                getValidIntChoice(0, 0, "Chọn thao tác [0]: ");
            }
        } else {
            printError("Lựa chọn thao tác không hợp lệ!");
        }
    }
}

void ConsoleApp::handleAdminDBStorageStatus() {
    while (true) {
        printHeader("KIỂM TRA BỘ NHỚ LƯU TRỮ FILE & CHỈ MỤC RAM");
        std::cout << " Đường dẫn thư mục dữ liệu : data/\n";
        size_t idxCount = dbEngine ? dbEngine->getIndexCount() : 0;
        size_t cacheSz = dbEngine ? dbEngine->getCacheSize() : 0;
        std::cout << " Kích thước chỉ mục RAM Hash: " << idxCount << " vị trí byte offset O(1)\n";
        std::cout << " Kích thước Bộ nhớ đệm LRU  : " << cacheSz << " hồ sơ lưu tạm trong bộ nhớ\n";
        printSuccess("Tất cả các chỉ mục lưu trữ File I/O đang hoạt động bình thường.");

        printDivider();
        std::cout << " 0. Quay lại Bảng điều khiển\n";
        printDivider();
        int choice = getValidIntChoice(0, 0, "Chọn thao tác [0]: ");
        if (choice == 0) break;
    }
}

} // namespace TutorMatch
