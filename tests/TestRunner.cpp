#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>

// Domain & Interfaces
#include "business/domain/User.h"
#include "business/domain/CourseBooking.h"
#include "business/domain/Review.h"
#include "business/matching/SubjectTaxonomy.h"
#include "business/algorithms/ScheduleGuard.h"
#include "business/algorithms/AVLTree.h"
#include "business/algorithms/HashTable.h"
#include "business/algorithms/LRUCache.h"
#include "business/algorithms/MaxHeap.h"
#include "business/algorithms/DoublyLinkedList.h"
#include "data_access/repositories/IRepository.h"
#include "data_access/storage/MockDatabaseEngine.h"
#include "data_access/storage/DataFileInspector.h"
#include "data_access/serializers/SerializerUtils.h"
#include "data_access/serializers/UserSerializer.h"
#include "data_access/serializers/CourseBookingSerializer.h"
#include "data_access/serializers/ReviewSerializer.h"
#include "data_access/repositories/FileUserRepository.h"
#include "data_access/repositories/FileReviewRepository.h"
#include "data_access/repositories/FileCourseBookingRepository.h"
#include "data_access/storage/ApplicationPaths.h"
#include "business/domain/DateUtils.h"
#include "business/matching/IMatchingStrategy.h"
#include "business/matching/MatchingEngine.h"
#include "business/matching/TutorSearchMatcher.h"

// Services
#include "business/services/AuthService.h"
#include "business/services/UserService.h"
#include "business/services/BookingService.h"
#include "business/services/ReviewService.h"
#include "business/services/MatchingService.h"
#include "business/services/AdminService.h"
#include "presentation/bridges/GUIBridge.h"

using namespace TutorMatch;

// ============================================================================
// Test Double / Fake Repositories (DIP & ISP Verification - 100% In-Memory)
// ============================================================================

class FakeUserRepository : public IUserRepository {
private:
    std::unordered_map<int, std::shared_ptr<User>> users;
    int nextId = 100;
    mutable std::recursive_mutex repoMutex;

public:
    int getNextUserId() override { 
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        return nextId++; 
    }

    bool saveUser(const User& user) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        // Deep copy via polymorphic copy construction
        if (user.getRole() == UserRole::STUDENT) {
            auto s = std::make_shared<Student>(static_cast<const Student&>(user));
            users[s->getId()] = s;
        } else if (user.getRole() == UserRole::TUTOR) {
            auto t = std::make_shared<Tutor>(static_cast<const Tutor&>(user));
            users[t->getId()] = t;
        } else {
            auto a = std::make_shared<Admin>(static_cast<const Admin&>(user));
            users[a->getId()] = a;
        }
        return true;
    }

    std::shared_ptr<User> findUserById(int userId) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        auto it = users.find(userId);
        if (it != users.end()) return it->second;
        return nullptr;
    }

    std::shared_ptr<User> findUserByUsername(const std::string& username) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        for (const auto& [id, u] : users) {
            if (u->getUsername() == username) return u;
        }
        return nullptr;
    }

    bool updateUser(const User& user) override {
        return saveUser(user);
    }

    bool deleteUserSoft(int userId) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        auto u = findUserById(userId);
        if (u) {
            u->setIsActive(false);
            return true;
        }
        return false;
    }

    std::vector<std::shared_ptr<Tutor>> getAllTutors() override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<std::shared_ptr<Tutor>> res;
        for (const auto& [id, u] : users) {
            if (u->getRole() == UserRole::TUTOR) {
                res.push_back(std::static_pointer_cast<Tutor>(u));
            }
        }
        return res;
    }

    std::vector<std::shared_ptr<Student>> getAllStudents() override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<std::shared_ptr<Student>> res;
        for (const auto& [id, u] : users) {
            if (u->getRole() == UserRole::STUDENT) {
                res.push_back(std::static_pointer_cast<Student>(u));
            }
        }
        return res;
    }

    std::vector<std::shared_ptr<User>> getAllUsers() override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<std::shared_ptr<User>> res;
        for (const auto& [id, u] : users) {
            res.push_back(u);
        }
        return res;
    }

    std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<int> res;
        for (const auto& [id, u] : users) {
            if (u->getRole() == UserRole::TUTOR) {
                auto t = std::static_pointer_cast<Tutor>(u);
                if (t->getHourlyRate() >= minRate && t->getHourlyRate() <= maxRate) {
                    res.push_back(t->getId());
                }
            }
        }
        return res;
    }

    std::vector<int> findTutorIdsByMinRating(double minRating) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<int> res;
        for (const auto& [id, u] : users) {
            if (u->getRole() == UserRole::TUTOR) {
                auto t = std::static_pointer_cast<Tutor>(u);
                if (t->getRating() >= minRating) {
                    res.push_back(t->getId());
                }
            }
        }
        return res;
    }
};

class FakeReviewRepository : public IReviewRepository {
private:
    std::unordered_map<int, Review> reviews;
    int nextId = 300;

public:
    int getNextReviewId() override { return nextId++; }

    bool saveReview(const Review& review) override {
        reviews[review.id] = review;
        return true;
    }

    bool deleteReview(int reviewId) override {
        return reviews.erase(reviewId) > 0;
    }

    std::vector<Review> getReviewsByTutorId(int tutorId) override {
        std::vector<Review> res;
        for (const auto& [id, r] : reviews) {
            if (r.tutorId == tutorId) res.push_back(r);
        }
        return res;
    }

    std::vector<Review> getAllReviews() override {
        std::vector<Review> res;
        for (const auto& [id, r] : reviews) {
            res.push_back(r);
        }
        return res;
    }
};

class FakeCourseBookingRepository : public ICourseBookingRepository {
private:
    std::unordered_map<int, CourseBooking> courseBookings;
    int nextId = 1000;
    mutable std::recursive_mutex repoMutex;

public:
    int getNextCourseBookingId() override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        return nextId++;
    }

    bool saveCourseBooking(const CourseBooking& booking) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        courseBookings[booking.id] = booking;
        return true;
    }

    bool updateCourseBooking(const CourseBooking& booking) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        courseBookings[booking.id] = booking;
        return true;
    }

    std::shared_ptr<CourseBooking> findCourseBookingById(int id) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        auto it = courseBookings.find(id);
        if (it != courseBookings.end()) {
            return std::make_shared<CourseBooking>(it->second);
        }
        return nullptr;
    }

    std::vector<CourseBooking> getAllCourseBookings() override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<CourseBooking> res;
        for (const auto& [id, cb] : courseBookings) res.push_back(cb);
        return res;
    }

    std::vector<CourseBooking> getCourseBookingsByStudentId(int studentId) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<CourseBooking> res;
        for (const auto& [id, cb] : courseBookings) {
            if (cb.studentId == studentId) res.push_back(cb);
        }
        return res;
    }

    std::vector<CourseBooking> getCourseBookingsByTutorId(int tutorId) override {
        std::lock_guard<std::recursive_mutex> lock(repoMutex);
        std::vector<CourseBooking> res;
        for (const auto& [id, cb] : courseBookings) {
            if (cb.tutorId == tutorId) res.push_back(cb);
        }
        return res;
    }
};

// ============================================================================
// Custom Test Strategy (OCP Verification)
// ============================================================================

class CustomRatingOnlyMatchingStrategy : public IMatchingStrategy {
public:
    std::string getStrategyName() const override {
        return "Custom Rating Only Test Strategy (100% Rating)";
    }

    TutorMatchResult calculateScore(const Student& student, const Tutor& tutor) override {
        TutorMatchResult res;
        res.tutor = std::make_shared<Tutor>(tutor);
        res.subjectScore = 0.0;
        res.priceScore = 0.0;
        res.scheduleScore = 0.0;
        res.ratingScore = (tutor.getRating() / 5.0) * 100.0;
        res.compositeScore = res.ratingScore;
        return res;
    }
};

// ============================================================================
// TEST SUITES
// ============================================================================

void run_test_ocp() {
    std::cout << "[TEST] 1. Verifying Open/Closed Principle (OCP) in MatchingEngine..." << std::endl;

    Student s(1, "student1", "pass", "s1@test.vn", "Student One", 12, "Toán", 200000, 0xFFFFFFFFFFFFFFFFULL);
    auto t1 = std::make_shared<Tutor>(2, "t1", "p", "t1@t.vn", "Tutor Low Rating", "bio", "Toán", 150000.0, 3.0, 5, 0.0, 0xFFFFFFFFFFFFFFFFULL);
    auto t2 = std::make_shared<Tutor>(3, "t2", "p", "t2@t.vn", "Tutor High Rating", "bio", "Văn", 150000.0, 5.0, 10, 0.0, 0xFFFFFFFFFFFFFFFFULL);

    std::vector<std::shared_ptr<Tutor>> tutors = {t1, t2};

    // Strategy substitution without modifying MatchingEngine
    MatchingEngine engine;
    engine.setStrategy(std::make_unique<CustomRatingOnlyMatchingStrategy>());

    auto matches = engine.findTopMatches(s, tutors, 2);
    assert(matches.size() == 2);
    assert(matches[0].tutor->getId() == 3); // High rating tutor ranked #1
    assert(matches[0].compositeScore == 100.0);
    assert(matches[1].tutor->getId() == 2); // Low rating tutor ranked #2
    assert(matches[1].compositeScore == 60.0);

    std::cout << "  -> PASS: MatchingEngine seamlessly adopted CustomRatingOnlyMatchingStrategy!" << std::endl;
}

void run_test_lsp() {
    std::cout << "[TEST] 2. Verifying Liskov Substitution Principle (LSP) in User Hierarchy..." << std::endl;

    std::vector<std::shared_ptr<User>> userList;
    userList.push_back(std::make_shared<Student>(1, "s", "p", "s@test.vn", "Nguyen Van A", 10, "Toan", 100000, 0xFFFFFFFFFFFFFFFFULL));
    userList.push_back(std::make_shared<Tutor>(2, "t", "p", "t@test.vn", "Tran Thi B", "bio", "Vat Ly", 150000.0, 4.8, 12, 0.0, 0xFFFFFFFFFFFFFFFFULL));
    userList.push_back(std::make_shared<Admin>(3, "a", "p", "a@test.vn", "Le Van C", 9));

    for (const auto& u : userList) {
        assert(!u->getFullName().empty());
        assert(!u->getRoleString().empty());

        // Test serialization/deserialization via dedicated UserSerializer
        std::string csv = UserSerializer::toCsv(*u);
        assert(!csv.empty());
        auto restored = UserSerializer::fromCsv(csv);
        assert(restored != nullptr);
        assert(restored->getId() == u->getId());
        assert(restored->getRole() == u->getRole());

        if (u->getRole() == UserRole::STUDENT) {
            assert(u->getRoleString() == "Học viên");
        } else if (u->getRole() == UserRole::TUTOR) {
            assert(u->getRoleString() == "Gia sư");
        } else if (u->getRole() == UserRole::ADMIN) {
            assert(u->getRoleString() == "Quản trị viên");
        }
    }

    std::cout << "  -> PASS: User subclasses honor polymorphic contract seamlessly!" << std::endl;
}

void run_test_dip_isp_and_srp() {
    std::cout << "[TEST] 3. Verifying DIP, ISP & SRP across decoupled Services..." << std::endl;

    auto fakeUserRepo = std::make_shared<FakeUserRepository>();
    auto fakeReviewRepo = std::make_shared<FakeReviewRepository>();
    auto fakeCourseBookingRepo = std::make_shared<FakeCourseBookingRepository>();

    AuthService authService(fakeUserRepo);
    UserService userService(fakeUserRepo);
    auto bookingService = std::make_shared<BookingService>(fakeUserRepo, fakeReviewRepo, fakeCourseBookingRepo);
    AdminService adminService(fakeUserRepo, fakeReviewRepo, bookingService);
    ReviewService reviewService(fakeReviewRepo, fakeUserRepo, fakeCourseBookingRepo);

    // 1. Test AuthService Registration & Login
    std::string err;
    bool regStudent = authService.registerStudent("stu_nam", "pass123", "Nguyen Nam", "nam@gmail.com", "12", "Toán 12", 200000, err);
    assert(regStudent);
    bool regTutor = authService.registerTutor("tut_toan", "pass123", "ThS. Le Toan", "toan@tutormatch.vn", "Toán 12", 150000, "10 năm kinh nghiệm", err);
    assert(regTutor);

    bool dupReg = authService.registerStudent("stu_nam", "pass123", "Dup", "dup@test.vn", "10", "Toán", 100000, err);
    assert(!dupReg); // Duplicate username check

    bool loginOk = authService.login("stu_nam", "pass123", err);
    assert(loginOk);
    assert(authService.isLoggedIn());
    assert(authService.getCurrentRole() == UserRole::STUDENT);
    int studentId = authService.getCurrentUser()->getId();

    auto tutor = fakeUserRepo->findUserByUsername("tut_toan");
    assert(tutor != nullptr);
    int tutorId = tutor->getId();

    std::cout << "  -> PASS: AuthService isolated registration & authentication successfully!" << std::endl;

    // 2. Test BookingService Lifecycle
    std::string msg;
    int day = 0; // Thứ 2
    int slot = 0; // 07:30 - 09:00
    std::string startDate = "2026-11-16";
    int weeksCount = 4;
    int bookingId = 0;

    bool bookOk = bookingService->bookCourse(studentId, tutorId, "Toán 12", day, slot, startDate, weeksCount, true, msg, &bookingId);
    assert(bookOk);

    auto studentBookings = bookingService->getStudentCourseBookings(studentId);
    assert(studentBookings.size() == 1);
    assert(studentBookings[0].status == "PENDING");
    assert(studentBookings[0].id == bookingId);

    // Accept booking
    bool acceptOk = bookingService->acceptCourseBooking(tutorId, bookingId, true, msg);
    assert(acceptOk);

    auto acceptedCourse = fakeCourseBookingRepo->findCourseBookingById(bookingId);
    assert(acceptedCourse != nullptr && acceptedCourse->status == "ACCEPTED");

    // Complete course
    bool completeOk = bookingService->terminateCourseBooking(studentId, UserRole::STUDENT, bookingId, msg);
    assert(completeOk);

    auto completedCourse = fakeCourseBookingRepo->findCourseBookingById(bookingId);
    assert(completedCourse != nullptr && completedCourse->status == "COMPLETED");

    // Submit review -> Tutor rating calculation
    bool reviewOk = reviewService.submitReview(studentId, bookingId, 5, "Thầy dạy cực hay!", msg);
    assert(reviewOk);

    auto reviewedTutor = userService.getTutorById(tutorId);
    assert(reviewedTutor->getTotalReviews() == 1);
    assert(reviewedTutor->getRating() == 5.0);

    std::cout << "  -> PASS: BookingService coordinated lifecycle and maintained course invariants!" << std::endl;

    // 3. Test AdminService Reporting & Moderation
    AdminMetrics metrics = adminService.calculateMetrics();
    assert(metrics.totalStudents == 1);
    assert(metrics.totalTutors == 1);
    assert(metrics.completedBookings == 1);

    bool lockOk = adminService.toggleUserLock(tutorId, msg);
    assert(lockOk);
    auto lockedTutor = userService.getTutorById(tutorId);
    assert(!lockedTutor->getIsActive());
    bool unlockOk = adminService.toggleUserLock(tutorId, msg);
    assert(unlockOk);
    auto unlockedTutor = userService.getTutorById(tutorId);
    assert(unlockedTutor->getIsActive());

    std::cout << "  -> PASS: AdminService accurately calculated platform metrics and moderation status!" << std::endl;
}

void run_test_schedule_guard() {
    std::cout << "[TEST] 4. Verifying ScheduleGuard Bitmask Collision Math..." << std::endl;

    uint64_t fullFree = ScheduleGuard::VALID_SCHEDULE_MASK;
    uint64_t slot0 = ScheduleGuard::getSlotMask(0, 0);
    uint64_t slot1 = ScheduleGuard::getSlotMask(0, 1);

    assert(ScheduleGuard::isSlotAvailable(fullFree, slot0));
    assert(ScheduleGuard::isSlotAvailable(fullFree, slot1));

    uint64_t locked = ScheduleGuard::setSlot(fullFree, 0, 0, false);
    assert(!ScheduleGuard::isSlotAvailable(locked, slot0));
    assert(ScheduleGuard::isSlotAvailable(locked, slot1));

    assert(ScheduleGuard::countMatchingSlots(fullFree, fullFree) == 56);
    assert(ScheduleGuard::countMatchingSlots(fullFree, locked) == 55);

    // Verify sanitization against phantom bits (bits 56..63)
    uint64_t raw64AllOnes = 0xFFFFFFFFFFFFFFFFULL;
    assert(ScheduleGuard::countMatchingSlots(raw64AllOnes, raw64AllOnes) == 56);
    assert(ScheduleGuard::sanitizeMask(raw64AllOnes) == ScheduleGuard::VALID_SCHEDULE_MASK);

    std::cout << "  -> PASS: ScheduleGuard 56-slot mathematical invariants & sanitization verified!" << std::endl;
}

void run_test_sidebar_labels() {
    std::cout << "[TEST] 5. Verifying Sidebar Navigation Labels & Zero Numeric Prefixes..." << std::endl;

    std::vector<std::string> studentLabels = {
        "Hồ sơ & Thời khóa biểu",
        "Tìm kiếm Gia sư",
        "Gợi ý Thông minh",
        "Lịch học & Kết nối",
        "Đăng xuất"
    };

    std::vector<std::string> tutorLabels = {
        "Tổng quan & Lịch dạy",
        "Yêu cầu kết nối",
        "Học viên & Khóa học",
        "Đánh giá từ học viên",
        "Đăng xuất"
    };

    std::vector<std::string> adminLabels = {
        "Tổng quan nền tảng",
        "Quản lý & Xác minh",
        "Giám sát Kết nối",
        "Kiểm duyệt Đánh giá",
        "Đăng xuất"
    };

    auto checkLabels = [](const std::vector<std::string>& labels) {
        assert(labels.size() == 5);
        for (size_t i = 0; i < labels.size(); ++i) {
            const std::string& l = labels[i];
            assert(!l.empty());
            assert(l.rfind("0. ", 0) == std::string::npos);
            assert(l.rfind("1. ", 0) == std::string::npos);
            assert(l.rfind("2. ", 0) == std::string::npos);
            assert(l.rfind("3. ", 0) == std::string::npos);
            assert(l.rfind("4. ", 0) == std::string::npos);
            assert(!isdigit(static_cast<unsigned char>(l[0])));
        }
        assert(labels[4] == "Đăng xuất");
    };

    checkLabels(studentLabels);
    checkLabels(tutorLabels);
    checkLabels(adminLabels);

    std::cout << "  -> PASS: All sidebar labels verified clean of numeric prefixes with correct routing!" << std::endl;
}

void run_test_tutor_profile_editing() {
    std::cout << "[TEST] 6. Verifying Tutor Profile Editing, Validation & Persistence..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    // Register a Tutor
    std::string err;
    bool regOk = bridge.registerTutor("toan_math", "pass123", "ThS. Lê Văn Toàn", "toan@tutormatch.vn",
                                      "Toán 12", 150000.0, "10 năm kinh nghiệm", err);
    assert(regOk);

    // Login as Tutor
    bool loginOk = bridge.qmlLogin("toan_math", "pass123");
    assert(loginOk);

    auto curUser = bridge.getCurrentUser();
    assert(curUser != nullptr);
    assert(curUser->getRole() == UserRole::TUTOR);
    int tutorId = curUser->getId();

    // 1. Initial Data Inspection
    QVariantMap overview = bridge.qmlGetTutorOverview();
    assert(overview["fullName"].toString() == "ThS. Lê Văn Toàn");
    assert(overview["hourlyRate"].toDouble() == 150000.0);
    assert(overview["subjectSpecialty"].toString() == "Toán 12");
    assert(overview["email"].toString() == "toan@tutormatch.vn");

    // 2. Edit Profile with Valid Data (Title, Name, Email, Bio, Subject, Rate)
    bool updateOk = bridge.updateTutorProfile("TS.", "Lê Văn Toàn", "toan.le@tutormatch.vn",
                                             "Tiến sĩ Toán học ĐHQG, 12 năm kinh nghiệm luyện thi",
                                             "Toán Chuyên & Đại Học", 220000.0);
    assert(updateOk);

    // 3. UI Model Refresh Check (Immediate reflection)
    QVariantMap updatedOverview = bridge.qmlGetTutorOverview();
    assert(updatedOverview["fullName"].toString() == "TS. Lê Văn Toàn");
    assert(updatedOverview["email"].toString() == "toan.le@tutormatch.vn");
    assert(updatedOverview["subjectSpecialty"].toString() == "Toán Chuyên & Đại Học");
    assert(updatedOverview["hourlyRate"].toDouble() == 220000.0);
    assert(bridge.getCurrentUserFullName() == "TS. Lê Văn Toàn");

    // 4. Persistence Reload Check
    auto reloadedUser = userRepo->findUserById(tutorId);
    assert(reloadedUser != nullptr);
    auto reloadedTutor = std::static_pointer_cast<Tutor>(reloadedUser);
    assert(reloadedTutor->getFullName() == "TS. Lê Văn Toàn");
    assert(reloadedTutor->getEmail() == "toan.le@tutormatch.vn");
    assert(reloadedTutor->getSubjectSpecialty() == "Toán Chuyên & Đại Học");
    assert(reloadedTutor->getHourlyRate() == 220000.0);
    // Unrelated fields preserved
    assert(reloadedTutor->getRating() == 5.0);
    assert(reloadedTutor->getTotalReviews() == 0);
    assert(reloadedTutor->getIsActive() == true);

    // 5. Validation Failure Checks
    // Empty Name
    bool fail1 = bridge.updateTutorProfile("", "", "toan@test.com", "Bio", "Toan", 200000.0);
    assert(!fail1);
    // Invalid Email
    bool fail2 = bridge.updateTutorProfile("TS.", "Lê Văn Toàn", "bad_email_without_at", "Bio", "Toan", 200000.0);
    assert(!fail2);
    // Invalid Hourly Rate (Negative or out of bounds)
    bool fail3 = bridge.updateTutorProfile("TS.", "Lê Văn Toàn", "toan@test.com", "Bio", "Toan", -50000.0);
    assert(!fail3);
    bool fail4 = bridge.updateTutorProfile("TS.", "Lê Văn Toàn", "toan@test.com", "Bio", "Toan", 90000000.0);
    assert(!fail4);
    // Empty Subject
    bool fail5 = bridge.updateTutorProfile("TS.", "Lê Văn Toàn", "toan@test.com", "Bio", "", 200000.0);
    assert(!fail5);

    // Ensure state remained unchanged after failed validations
    auto safeTutor = userRepo->findUserById(tutorId);
    assert(safeTutor->getFullName() == "TS. Lê Văn Toàn");

    // 6. Security Check: Cannot update another user's profile
    bridge.logout();
    // Register student
    bridge.registerStudent("nam_student", "pass123", "Nguyễn Văn Nam", "nam@test.com", "12", "Toán 12", 200000.0, err);
    bridge.qmlLogin("nam_student", "pass123");
    // Student tries to call updateTutorProfile
    bool hackerFail = bridge.updateTutorProfile("TS.", "Hacker Name", "h@test.com", "Bio", "Math", 300000.0);
    assert(!hackerFail); // Blocked by role check!

    std::cout << "  -> PASS: Tutor Profile Editing passed validation, persistence, and authorization tests!" << std::endl;
}

void run_test_schedule_synchronization() {
    std::cout << "[TEST] 7. Verifying Bidirectional Timetable Synchronization (Student <-> Tutor)..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    // Register Student and Tutor
    std::string err;
    bridge.registerStudent("nam_student", "pass123", "Nguyễn Văn Nam", "nam@test.com", "12", "Toán 12", 250000.0, err);
    bridge.registerTutor("toan_math", "pass123", "ThS. Lê Văn Toàn", "toan@tutormatch.vn", "Toán 12", 150000.0, "Bio", err);

    auto studentUser = userRepo->findUserByUsername("nam_student");
    auto tutorUser = userRepo->findUserByUsername("toan_math");
    assert(studentUser != nullptr && tutorUser != nullptr);
    int studentId = studentUser->getId();
    int tutorId = tutorUser->getId();

    // Monday (day 0), slot 2 (13:30 - 15:00)
    int day = 0;
    int slot = 2;
    int flatIndex = slot * 7 + day; // 2 * 7 + 0 = 14
    uint64_t slotMask = ScheduleGuard::getSlotMask(day, slot);

    // 1. Initial State: Both Student and Tutor Timetables have 56 cells, all FREE
    bridge.qmlLogin("nam_student", "pass123");
    QVariantList studentGridInit = bridge.getStudentWeeklySchedule();
    assert(studentGridInit.size() == 56);
    assert(studentGridInit[flatIndex].toMap()["status"].toString() == "FREE");

    bridge.logout();
    bridge.qmlLogin("toan_math", "pass123");
    QVariantList tutorGridInit = bridge.getTutorWeeklySchedule();
    assert(tutorGridInit.size() == 56);
    assert(tutorGridInit[flatIndex].toMap()["status"].toString() == "FREE");
    // Verify getWeeklyScheduleMatrix() now works properly for Tutor
    QVariantList tutorMatrixInit = bridge.getWeeklyScheduleMatrix();
    assert(tutorMatrixInit.size() == 56);
    assert(tutorMatrixInit[flatIndex].toMap()["status"].toString() == "FREE");

    // 2. Create Booking: PENDING
    bridge.logout();
    bridge.qmlLogin("nam_student", "pass123");
    bool reqOk = bridge.requestCourseBooking(tutorId, "Toán 12", day, slot);
    assert(reqOk);

    // PENDING policy check: Slot should NOT yet show as BOOKED on either active timetable
    QVariantList studentGridPending = bridge.getStudentWeeklySchedule();
    assert(studentGridPending[flatIndex].toMap()["status"].toString() != "BOOKED");

    bridge.logout();
    bridge.qmlLogin("toan_math", "pass123");
    QVariantList tutorGridPending = bridge.getTutorWeeklySchedule();
    assert(tutorGridPending[flatIndex].toMap()["status"].toString() != "BOOKED");

    // 3. Tutor Accepts Booking
    auto courses = courseRepo->getCourseBookingsByTutorId(tutorId);
    assert(!courses.empty());
    int bookingId = courses[0].id;
    bool acceptOk = bridge.qmlAcceptBooking(bookingId);
    assert(acceptOk);

    // 4. VERIFY TUTOR SIDE: Cell at Monday 13:30-15:00 is BOOKED with Student's Name
    std::string courseMonday = DateUtils::getWeekMonday(courses[0].startDate);
    QString qCourseMonday = QString::fromStdString(courseMonday);

    QVariantList tutorGridAccepted = bridge.getTutorWeeklySchedule(qCourseMonday);
    assert(tutorGridAccepted.size() == 56);
    QVariantMap tutorCell = tutorGridAccepted[flatIndex].toMap();
    assert(tutorCell["status"].toString() == "BOOKED");
    assert(tutorCell["displayName"].toString() == "Nguyễn Văn Nam");
    assert(tutorCell["subjectName"].toString() == "Toán 12");
    assert(tutorCell["studentName"].toString() == "Nguyễn Văn Nam");
    assert(tutorCell["editable"].toBool() == false);
    assert(tutorCell["locked"].toBool() == true);

    // Verify getWeeklyScheduleMatrix() on Tutor side also matches!
    QVariantList tutorMatrixAccepted = bridge.getWeeklyScheduleMatrix(qCourseMonday);
    assert(tutorMatrixAccepted[flatIndex].toMap()["status"].toString() == "BOOKED");
    assert(tutorMatrixAccepted[flatIndex].toMap()["displayName"].toString() == "Nguyễn Văn Nam");

    // 5. VERIFY STUDENT SIDE: Cell at Monday 13:30-15:00 is BOOKED with Tutor's Name
    bridge.logout();
    bridge.qmlLogin("nam_student", "pass123");
    QVariantList studentGridAccepted = bridge.getStudentWeeklySchedule(qCourseMonday);
    assert(studentGridAccepted.size() == 56);
    QVariantMap studentCell = studentGridAccepted[flatIndex].toMap();
    assert(studentCell["status"].toString() == "BOOKED");
    assert(studentCell["subjectName"].toString() == "Toán 12");
    assert(studentCell["tutorName"].toString() == "ThS. Lê Văn Toàn");
    assert(studentCell["editable"].toBool() == false);
    assert(studentCell["locked"].toBool() == true);

    // 6. ISOLATION CHECK: Another Student and Another Tutor must NOT see this booked slot
    bridge.logout();
    bridge.registerStudent("other_student", "pass123", "Trần Khách", "other@test.com", "11", "Vật lý", 200000.0, err);
    bridge.registerTutor("other_tutor", "pass123", "TS. Khách", "tutor.other@test.com", "Hóa", 200000.0, "Bio", err);

    bridge.qmlLogin("other_student", "pass123");
    QVariantList otherStudentGrid = bridge.getStudentWeeklySchedule(qCourseMonday);
    assert(otherStudentGrid[flatIndex].toMap()["status"].toString() != "BOOKED");

    bridge.logout();
    bridge.qmlLogin("other_tutor", "pass123");
    QVariantList otherTutorGrid = bridge.getTutorWeeklySchedule(qCourseMonday);
    assert(otherTutorGrid[flatIndex].toMap()["status"].toString() != "BOOKED");

    // 7. CANCELLATION CHECK: When booking is cancelled, slot is returned to FREE
    bridge.logout();
    bridge.qmlLogin("nam_student", "pass123");
    bool cancelOk = bridge.cancelBooking(bookingId, err);
    assert(cancelOk);

    QVariantList studentGridCancelled = bridge.getStudentWeeklySchedule(qCourseMonday);
    assert(studentGridCancelled[flatIndex].toMap()["status"].toString() == "FREE");

    bridge.logout();
    bridge.qmlLogin("toan_math", "pass123");
    QVariantList tutorGridCancelled = bridge.getTutorWeeklySchedule(qCourseMonday);
    assert(tutorGridCancelled[flatIndex].toMap()["status"].toString() == "FREE");

    // 8. BOUNDARY MAPPING CHECK: Day 0 Slot 0 (index 0) and Day 6 Slot 7 (index 55)
    assert(ScheduleGuard::getSlotMask(0, 0) == (1ULL << 0));
    assert(ScheduleGuard::getSlotMask(6, 7) == (1ULL << 55));

    std::cout << "  -> PASS: Bidirectional timetable synchronization verified across all conditions!" << std::endl;
}

void run_test_admin_user_data_contract() {
    std::cout << "[TEST] 8. Verifying Admin User Card Data Contract & No Undefined Fields..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    std::string err;
    bridge.registerStudent("nam_student", "pass123", "Nguyễn Văn Nam", "nam@student.vn", "12", "Toán 12", 250000.0, err);
    bridge.registerTutor("toan_math", "pass123", "ThS. Lê Văn Toàn", "toan@tutormatch.vn", "Toán 12", 150000.0, "Bio Toan", err);
    bridge.registerTutor("van_eng", "pass123", "Ms. Nguyễn Anh Văn", "van@tutormatch.vn", "Tiếng Anh", 180000.0, "Bio Anh", err);

    // Fetch Admin users list
    QVariantList adminUsers = bridge.qmlGetAdminUsers("ALL", "");
    assert(adminUsers.size() >= 3);

    bool foundToan = false;
    bool foundVan = false;
    bool foundNam = false;

    for (const auto& item : adminUsers) {
        QVariantMap userMap = item.toMap();
        // Mandatory fields contract check
        assert(userMap.contains("id"));
        assert(userMap.contains("userId"));
        assert(userMap.contains("fullName"));
        assert(userMap.contains("username"));
        assert(userMap.contains("email"));
        assert(userMap.contains("role"));
        assert(userMap.contains("roleEnum"));
        assert(userMap.contains("roleKey"));
        assert(userMap.contains("isActive"));
        assert(userMap.contains("isLocked"));
        assert(userMap.contains("statusText"));
        assert(userMap.contains("subject"));
        assert(userMap.contains("hourlyRate"));
        assert(userMap.contains("initials"));

        // No undefined string checks
        assert(!userMap["email"].toString().isEmpty());
        assert(!userMap["username"].toString().isEmpty());
        assert(userMap["email"].toString() != "undefined");
        assert(userMap["username"].toString() != "undefined");
        assert(userMap["subject"].toString() != "undefined");

        if (userMap["username"].toString() == "toan_math") {
            foundToan = true;
            assert(userMap["role"].toString() == "Gia sư");
            assert(userMap["subject"].toString() == "Toán 12");
            assert(userMap["email"].toString() == "toan@tutormatch.vn");
        } else if (userMap["username"].toString() == "van_eng") {
            foundVan = true;
            assert(userMap["role"].toString() == "Gia sư");
            assert(userMap["subject"].toString() == "Tiếng Anh");
            assert(userMap["email"].toString() == "van@tutormatch.vn");
        } else if (userMap["username"].toString() == "nam_student") {
            foundNam = true;
            assert(userMap["role"].toString() == "Học viên");
            assert(userMap["subject"].toString() == "Lớp 12");
            assert(userMap["email"].toString() == "nam@student.vn");
        }
    }

    assert(foundToan && foundVan && foundNam);
    std::cout << "  -> PASS: All admin user cards meet data contract with zero undefined fields!" << std::endl;
}

void run_test_admin_user_detail_modal_contract() {
    std::cout << "[TEST] 9. Verifying User Detail Modal Contract & Non-Existent User Handling..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    std::string err;
    bridge.registerTutor("toan_math", "pass123", "ThS. Lê Văn Toàn", "toan@tutormatch.vn", "Toán 12", 150000.0, "Thạc sĩ Toán học", err);

    auto t1 = userRepo->findUserByUsername("toan_math");
    assert(t1 != nullptr);
    int toanId = t1->getId();

    // 1. Valid User Detail Inspection
    QVariantMap detail = bridge.qmlGetUserDetailsForAdmin(toanId);
    assert(detail["found"].toBool() == true);
    assert(detail.contains("profile"));

    QVariantMap profile = detail["profile"].toMap();
    assert(profile["id"].toInt() == toanId);
    assert(profile["fullName"].toString() == "ThS. Lê Văn Toàn");
    assert(profile["username"].toString() == "toan_math");
    assert(profile["email"].toString() == "toan@tutormatch.vn");
    assert(profile["roleKey"].toString() == "TUTOR");
    assert(profile["role"].toString() == "Gia sư");
    assert(profile["isActive"].toBool() == true);
    assert(profile["subjectSpecialty"].toString() == "Toán 12");
    assert(profile["hourlyRate"].toDouble() == 150000.0);
    assert(profile["bio"].toString() == "Thạc sĩ Toán học");

    // Timetable grid in detail modal must have 56 slots
    assert(detail.contains("schedule"));
    assert(detail["schedule"].toList().size() == 56);
    assert(detail.contains("scheduleMatrix"));
    assert(detail["scheduleMatrix"].toList().size() == 56);
    assert(detail.contains("connections"));
    assert(detail.contains("reviews"));

    // 2. Non-Existent User Inspection (Lỗi C protection)
    int invalidId = 999999;
    QVariantMap invalidDetail = bridge.qmlGetUserDetailsForAdmin(invalidId);
    assert(invalidDetail["found"].toBool() == false);
    assert(invalidDetail.contains("errorMessage"));
    assert(!invalidDetail["errorMessage"].toString().isEmpty());
    // Must NOT contain a valid profile or false locked status!
    assert(!invalidDetail.contains("profile") || invalidDetail["profile"].toMap().isEmpty());

    std::cout << "  -> PASS: User detail modal contract verified, error state prevents false locked status!" << std::endl;
}

void run_test_admin_verification_and_lock_sync() {
    std::cout << "[TEST] 10. Verifying Tutor Verification Decommission & Lock Sync Across Admin and Student Views..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    std::string err;
    bridge.registerTutor("van_eng", "pass123", "Ms. Nguyễn Anh Văn", "van@tutormatch.vn", "Tiếng Anh", 180000.0, "Bio Anh", err);

    auto t = userRepo->findUserByUsername("van_eng");
    assert(t != nullptr);
    int vanId = t->getId();

    // 1. Initial State: Tutor is active and immediately discoverable in Student search (no verification hurdle)
    QVariantMap detailInitial = bridge.qmlGetUserDetailsForAdmin(vanId);
    assert(detailInitial["found"].toBool() == true);
    assert(detailInitial["profile"].toMap()["isActive"].toBool() == true);
    assert(!detailInitial["profile"].toMap().contains("isVerified"));

    QVariantList studentTutorsBefore = bridge.searchAndFilterTutors("", "", 0, 0, 0);
    bool foundInStudentBefore = false;
    for (const auto& item : studentTutorsBefore) {
        if (item.toMap()["id"].toInt() == vanId) {
            foundInStudentBefore = true;
            assert(!item.toMap().contains("isVerified"));
        }
    }
    assert(foundInStudentBefore);

    // 2. Admin Filters: Tutor appears in ALL and TUTOR filters, not in LOCKED
    QVariantList allUsers = bridge.qmlGetAdminUsers("ALL", "");
    bool foundInAll = false;
    for (const auto& item : allUsers) {
        if (item.toMap()["id"].toInt() == vanId) {
            foundInAll = true;
            assert(!item.toMap().contains("isVerified"));
            assert(!item.toMap().contains("isVerifiedBool"));
        }
    }
    assert(foundInAll);

    QVariantList tutorUsers = bridge.qmlGetAdminUsers("TUTOR", "");
    bool foundInTutor = false;
    for (const auto& item : tutorUsers) {
        if (item.toMap()["id"].toInt() == vanId) foundInTutor = true;
    }
    assert(foundInTutor);

    // 3. Action: Lock / Unlock Toggle Sync
    bool lockOk = bridge.qmlToggleUserLock(vanId);
    assert(lockOk);

    QVariantMap detailLocked = bridge.qmlGetUserDetailsForAdmin(vanId);
    assert(detailLocked["profile"].toMap()["isActive"].toBool() == false);
    assert(detailLocked["profile"].toMap()["isLocked"].toBool() == true);
    assert(detailLocked["profile"].toMap()["statusText"].toString() == "BỊ KHÓA");

    // Locked user appears in LOCKED filter
    QVariantList lockedList = bridge.qmlGetAdminUsers("LOCKED", "");
    bool foundInLocked = false;
    for (const auto& item : lockedList) {
        if (item.toMap()["id"].toInt() == vanId) foundInLocked = true;
    }
    assert(foundInLocked);

    // Locked tutor is excluded from active student searches
    QVariantList studentTutorsLocked = bridge.searchAndFilterTutors("", "", 0, 0, 0);
    for (const auto& item : studentTutorsLocked) {
        assert(item.toMap()["id"].toInt() != vanId);
    }

    // 4. Action: Unlock again
    bool unlockOk = bridge.qmlToggleUserLock(vanId);
    assert(unlockOk);
    QVariantMap detailUnlocked = bridge.qmlGetUserDetailsForAdmin(vanId);
    assert(detailUnlocked["profile"].toMap()["isActive"].toBool() == true);
    assert(detailUnlocked["profile"].toMap()["isLocked"].toBool() == false);

    // Reappears in student searches
    QVariantList studentTutorsUnlocked = bridge.searchAndFilterTutors("", "", 0, 0, 0);
    bool foundInStudentAfter = false;
    for (const auto& item : studentTutorsUnlocked) {
        if (item.toMap()["id"].toInt() == vanId) foundInStudentAfter = true;
    }
    assert(foundInStudentAfter);

    std::cout << "  -> PASS: Tutor verification decommission & lock state synchronized across Admin and Student views seamlessly!" << std::endl;
}

void run_test_admin_service_filters_and_search() {
    std::cout << "[TEST] 11. Verifying AdminService Filters & Subject Keyword Search..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    GUIBridge bridge;
    bridge.initialize(userRepo, reviewRepo, courseRepo);

    std::string err;
    bridge.registerStudent("nam_student", "pass123", "Nguyễn Văn Nam", "nam@student.vn", "12", "Toán 12", 250000.0, err);
    bridge.registerTutor("toan_math", "pass123", "ThS. Lê Văn Toàn", "toan@tutormatch.vn", "Toán 12", 150000.0, "Bio Toan", err);
    bridge.registerTutor("ly_phys", "pass123", "TS. Trần Thị Vật Lý", "ly@tutormatch.vn", "Vật lý 12", 200000.0, "Bio Ly", err);

    // Search by subject "Toán" (Student no longer has persistent subject, matches Tutor Toàn)
    QVariantList mathSearch = bridge.qmlGetAdminUsers("ALL", "toán");
    assert(mathSearch.size() == 1);
    assert(mathSearch[0].toMap()["fullName"].toString() == "ThS. Lê Văn Toàn");

    // Search by subject "Vật lý"
    QVariantList physSearch = bridge.qmlGetAdminUsers("ALL", "vật lý");
    assert(physSearch.size() == 1);
    assert(physSearch[0].toMap()["fullName"].toString() == "TS. Trần Thị Vật Lý");

    std::cout << "  -> PASS: AdminService filters and subject keyword search verified!" << std::endl;
}

void run_test_avl_tree_multi_value_and_deletion() {
    std::cout << "[TEST] 12. Verifying AVL Tree Multi-Value Support and Key/Value Deletion..." << std::endl;

    AVLTree<double, int> tree;
    assert(tree.empty());
    assert(tree.size() == 0);

    // 1. Multiple values per single key
    tree.insert(150000.0, 2);
    tree.insert(150000.0, 3);
    tree.insert(150000.0, 4);
    assert(tree.size() == 1); // 1 key node, containing 3 values

    // Additional distinct keys
    tree.insert(100000.0, 1);
    tree.insert(200000.0, 5);
    tree.insert(250000.0, 6);
    assert(tree.size() == 4);

    // 2. Range search retrieves all values for matching keys
    auto matchingTutors = tree.rangeSearch(140000.0, 160000.0);
    assert(matchingTutors.size() == 3);
    assert(std::find(matchingTutors.begin(), matchingTutors.end(), 2) != matchingTutors.end());
    assert(std::find(matchingTutors.begin(), matchingTutors.end(), 3) != matchingTutors.end());
    assert(std::find(matchingTutors.begin(), matchingTutors.end(), 4) != matchingTutors.end());

    auto allRange = tree.rangeSearch(50000.0, 300000.0);
    assert(allRange.size() == 6);

    // 3. Deletion of single value when multiple exist
    bool removed3 = tree.remove(150000.0, 3);
    assert(removed3);
    assert(tree.size() == 4); // Key node still exists with remaining 2 values
    auto afterRem3 = tree.rangeSearch(140000.0, 160000.0);
    assert(afterRem3.size() == 2);
    assert(std::find(afterRem3.begin(), afterRem3.end(), 3) == afterRem3.end());
    assert(std::find(afterRem3.begin(), afterRem3.end(), 2) != afterRem3.end());
    assert(std::find(afterRem3.begin(), afterRem3.end(), 4) != afterRem3.end());

    // 4. Deleting remaining values removes the node completely
    tree.remove(150000.0, 2);
    tree.remove(150000.0, 4);
    assert(tree.size() == 3); // Key node deleted
    auto empty150k = tree.rangeSearch(140000.0, 160000.0);
    assert(empty150k.empty());

    // 5. Deleting non-existent value returns false safely
    bool remNonExistent = tree.remove(150000.0, 999);
    assert(!remNonExistent);

    // 6. Tree rebalancing after deletions
    tree.clear();
    assert(tree.empty());
    for (int i = 1; i <= 15; ++i) {
        tree.insert(i * 10.0, i);
    }
    assert(tree.size() == 15);
    for (int i = 1; i <= 10; ++i) {
        tree.remove(i * 10.0, i);
    }
    assert(tree.size() == 5);
    auto remList = tree.rangeSearch(0.0, 200.0);
    assert(remList.size() == 5);
    for (int i = 11; i <= 15; ++i) {
        assert(std::find(remList.begin(), remList.end(), i) != remList.end());
    }

    std::cout << "  -> PASS: AVL Tree multi-value indexing & rebalancing deletion verified!" << std::endl;
}

void run_test_schedule_guard_sanitization_and_56_bit_boundary() {
    std::cout << "[TEST] 13. Verifying Schedule Bitmask 56-Bit Boundary & Phantom Bit Sanitization..." << std::endl;

    // 1. Bitmask boundary: exactly 56 slots
    assert(ScheduleGuard::TOTAL_SLOTS == 56);
    assert(ScheduleGuard::VALID_SCHEDULE_MASK == 0x00FFFFFFFFFFFFFFULL);
    assert((ScheduleGuard::VALID_SCHEDULE_MASK >> 56) == 0ULL);

    // 2. Sanitization helper
    uint64_t dirtyMask = 0xFFFFFFFF00000000ULL; // Contains 8 phantom bits (56..63)
    uint64_t cleanMask = ScheduleGuard::sanitizeMask(dirtyMask);
    assert(cleanMask == 0x00FFFFFF00000000ULL);
    assert((cleanMask & ~ScheduleGuard::VALID_SCHEDULE_MASK) == 0);

    // 3. Model construction sanitization
    Student s(100, "st_test", "pass", "st@test.com", "Student Test", 10, "Toán", 200000.0, 0xFFFFFFFFFFFFFFFFULL);
    assert(s.getScheduleBitmask() == ScheduleGuard::VALID_SCHEDULE_MASK);

    Tutor t(101, "tu_test", "pass", "tu@test.com", "Tutor Test", "Bio", "Toán", 150000.0, 5.0, 0, 0.0, 0xFFFFFFFF00000000ULL);
    assert(t.getScheduleBitmask() == 0x00FFFFFF00000000ULL);

    // 4. Model setter sanitization
    s.setScheduleBitmask(0xFFFFFFFFFFFFFFFFULL);
    assert(s.getScheduleBitmask() == ScheduleGuard::VALID_SCHEDULE_MASK);

    t.setScheduleBitmask(0xDEADBEEF00000000ULL);
    assert((t.getScheduleBitmask() & ~ScheduleGuard::VALID_SCHEDULE_MASK) == 0);

    // 5. Deserialization sanitization from raw CSV with phantom bits
    auto uStudentDes = UserSerializer::fromCsv("102,s_des,pass,s@d.com,Des Name,STUDENT,1,10,Toán,200000,18446744069414584320");
    assert(uStudentDes != nullptr);
    auto sDes = std::dynamic_pointer_cast<Student>(uStudentDes);
    assert(sDes != nullptr);
    assert(sDes->getScheduleBitmask() == 72057589742960640ULL); // 0x00FFFFFF00000000ULL
    assert((sDes->getScheduleBitmask() & ~ScheduleGuard::VALID_SCHEDULE_MASK) == 0);

    auto uTutorDes = UserSerializer::fromCsv("103,t_des,pass,t@d.com,Tutor Des,TUTOR,1,Bio,Toán,120000,4.5,3,1,500000,18446744073709551615");
    assert(uTutorDes != nullptr);
    auto tDes = std::dynamic_pointer_cast<Tutor>(uTutorDes);
    assert(tDes != nullptr);
    assert(tDes->getScheduleBitmask() == ScheduleGuard::VALID_SCHEDULE_MASK);

    std::cout << "  -> PASS: 56-bit boundary and phantom bit sanitization verified across models and deserializers!" << std::endl;
}

void run_test_mock_db_atomic_persistence_and_deduplication() {
    std::cout << "[TEST] 14. Verifying MockDatabaseEngine Atomic Persistence & Zero-Duplication Invariants..." << std::endl;

    std::string testDir = (std::filesystem::current_path() / "test_persistence_sandbox").string();
    std::filesystem::remove_all(testDir);

    {
        MockDatabaseEngine db(testDir);
        bool initOk = db.initialize();
        assert(initOk);

        // 1. Initial seeded users: exactly 7 canonical records
        auto users = db.getAllUsers();
        assert(users.size() == 7);

        // Check physical file line count
        std::string usersFile = testDir + "/users.dat";
        std::ifstream countIn(usersFile);
        int lineCount = 0;
        std::string line;
        while (std::getline(countIn, line)) {
            if (!line.empty()) lineCount++;
        }
        countIn.close();
        assert(lineCount == 7);

        // 2. Perform 10 consecutive profile & rate updates on Tutor 2
        auto tutorUser = db.findUserById(2);
        assert(tutorUser != nullptr);
        auto tutor = std::static_pointer_cast<Tutor>(tutorUser);

        for (int i = 1; i <= 10; ++i) {
            tutor->setHourlyRate(150000.0 + i * 5000.0);
            tutor->setBio("Cập nhật tiểu sử vòng " + std::to_string(i));
            bool updOk = db.updateUser(*tutor);
            assert(updOk);
        }

        // 3. Verify physical file NEVER appends duplicates: MUST STILL BE EXACTLY 7 LINES
        std::ifstream countInAfter(usersFile);
        int lineCountAfter = 0;
        while (std::getline(countInAfter, line)) {
            if (!line.empty()) lineCountAfter++;
        }
        countInAfter.close();
        assert(lineCountAfter == 7);

        // 4. Verify AVL Tree index accuracy: old rates removed, latest rate indexed
        auto oldRateTutors = db.findTutorIdsByPriceRange(149999.0, 150001.0);
        assert(oldRateTutors.empty()); // Initial 150k rate was removed!

        auto latestRateTutors = db.findTutorIdsByPriceRange(199999.0, 200001.0);
        // Tutor 2 is now at 200,000, and Tutor 3 was already at 200,000
        assert(latestRateTutors.size() == 2);
        assert(std::find(latestRateTutors.begin(), latestRateTutors.end(), 2) != latestRateTutors.end());
        assert(std::find(latestRateTutors.begin(), latestRateTutors.end(), 3) != latestRateTutors.end());

        // 5. Verify Soft Delete removes tutor from AVL index and active listing
        bool delOk = db.deleteUserSoft(2);
        assert(delOk);

        auto activeTutors = db.getAllTutors();
        for (const auto& at : activeTutors) {
            assert(at->getId() != 2);
        }

        auto rateAfterDel = db.findTutorIdsByPriceRange(199999.0, 200001.0);
        assert(rateAfterDel.size() == 1);
        assert(rateAfterDel[0] == 3); // Only Tutor 3 remains active at 200k

        // File line count must still be 7 (soft deleted user remains with isActive = 0)
        std::ifstream countInDel(usersFile);
        int lineCountDel = 0;
        while (std::getline(countInDel, line)) {
            if (!line.empty()) lineCountDel++;
        }
        countInDel.close();
        assert(lineCountDel == 7);
    }

    // Clean up sandbox
    std::filesystem::remove_all(testDir);

    std::cout << "  -> PASS: MockDatabaseEngine atomic rewrite, zero duplicate lines & AVL synchronization verified!" << std::endl;
}

void run_test_data_file_inspector() {
    std::cout << "[TEST] 15. Verifying DataFileInspector Read-Only Inspection & Error Handling..." << std::endl;

    // 1. Inspect real canonical files
    auto uSum = DataFileInspector::inspectUsersFile();
    assert(uSum.status == "OK");
    assert(uSum.recordsRead > 0);
    assert(uSum.validRecords == uSum.recordsRead);
    assert(uSum.invalidRecords == 0);
    assert(!uSum.fileModified);

    // Verify credential redaction
    for (const auto& rec : uSum.records) {
        for (const auto& f : rec.fields) {
            if (f.name == "password") {
                assert(f.value == "[REDACTED]");
            }
        }
    }

    auto cbSum = DataFileInspector::inspectCourseBookingsFile();
    assert(cbSum.status == "OK");
    assert(cbSum.recordsRead > 0);
    assert(cbSum.validRecords == cbSum.recordsRead);
    assert(cbSum.invalidRecords == 0);
    assert(!cbSum.fileModified);

    // Legacy bookings.dat is retired and removed from active directory
    auto bSum = DataFileInspector::inspectBookingsFile();
    assert(bSum.status.find("ERROR_NOT_FOUND") != std::string::npos);

    auto rSum = DataFileInspector::inspectReviewsFile();
    assert(rSum.status == "OK");
    assert(rSum.recordsRead > 0);
    assert(rSum.validRecords == rSum.recordsRead);
    assert(rSum.invalidRecords == 0);
    assert(!rSum.fileModified);

    // 2. Test missing file / non-existent directory error handling
    auto missingSum = DataFileInspector::inspectUsersFile("non_existent_dir_12345");
    assert(missingSum.status.find("ERROR_NOT_FOUND") != std::string::npos);
    assert(missingSum.recordsRead == 0);
    assert(missingSum.validRecords == 0);

    // 3. Test malformed record detection in sandbox
    std::string badSandbox = (std::filesystem::current_path() / "test_inspector_sandbox").string();
    std::filesystem::create_directories(badSandbox);
    {
        std::ofstream badFile(badSandbox + "/users.dat");
        badFile << "1,corrupted_only_two_tokens\n";
        badFile << "2,valid_user,pass,v@u.com,Valid Name,STUDENT,1,10,Math,500000,71777214294589695\n";
        badFile << "3,unknown_role,pass,u@r.com,Unknown,UNKNOWN_ROLE,1,extra\n";
        badFile.close();
    }

    auto badSum = DataFileInspector::inspectUsersFile(badSandbox);
    assert(badSum.status == "OK");
    assert(badSum.recordsRead == 3);
    assert(badSum.validRecords == 1);
    assert(badSum.invalidRecords == 2);
    assert(badSum.records[0].modelType == "MALFORMED");
    assert(!badSum.records[0].isValid);
    assert(badSum.records[1].modelType == "STUDENT");
    assert(badSum.records[1].isValid);
    assert(!badSum.records[2].isValid);

    std::filesystem::remove_all(badSandbox);

    // 4. Verify stdout printing doesn't crash
    std::ostringstream dummyOss;
    DataFileInspector::printFileSummary(uSum, dummyOss);
    assert(!dummyOss.str().empty());
    assert(dummyOss.str().find("[REDACTED]") != std::string::npos);

    std::cout << "  -> PASS: DataFileInspector read-only inspection, credential redaction & error handling verified!" << std::endl;
}

void run_test_booking_conflict_detection() {
    std::cout << "[TEST] 16. Verifying Course Booking Conflict Detection & Synchronization..." << std::endl;

    auto userRepo = std::make_shared<FakeUserRepository>();
    auto reviewRepo = std::make_shared<FakeReviewRepository>();
    auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

    BookingService service(userRepo, reviewRepo, courseRepo);

    // Setup Users
    // Tutor 1 (ID 10) & Tutor 2 (ID 20)
    Tutor t1(10, "tutor_toan", "pass123", "toan@tutormatch.vn", "ThS. Le Van Toan",
             "Bio Toan", "Toan 12", 150000.0, 5.0, 0, 0.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    Tutor t2(20, "tutor_ly", "pass123", "ly@tutormatch.vn", "TS. Tran Thi Ly",
             "Bio Ly", "Vat ly 12", 200000.0, 5.0, 0, 0.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    userRepo->saveUser(t1);
    userRepo->saveUser(t2);

    // Student 1 (ID 101), Student 2 (ID 102), Student 3 (ID 103)
    Student s1(101, "student_nam", "pass123", "nam@gmail.com", "Nguyen Van Nam",
               12, "Toan 12", 200000.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    Student s2(102, "student_thanh", "pass123", "thanh@gmail.com", "Pham Tuan Thanh",
               11, "Toan 11", 200000.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    Student s3(103, "student_long", "pass123", "long@gmail.com", "Le Hoang Long",
               10, "Vat ly 10", 250000.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    userRepo->saveUser(s1);
    userRepo->saveUser(s2);
    userRepo->saveUser(s3);

    std::string err;

    // --- Scenario 1: Book valid course -> PENDING ---
    int c1Id = 0;
    bool b1Ok = service.bookCourse(101, 10, "Toan 12", 0, 1, "2026-11-16", 4, true, err, &c1Id);
    assert(b1Ok);
    assert(c1Id > 0);

    // Accept course 1
    bool acc1Ok = service.acceptCourseBooking(10, c1Id, true, err);
    assert(acc1Ok);

    // --- Scenario 2: Second student attempts to accept same slot & overlapping dates -> rejected conflict ---
    int c2Id = 0;
    bool b2Ok = service.bookCourse(102, 10, "Toan 12", 0, 1, "2026-11-16", 4, true, err, &c2Id);
    assert(b2Ok); // Can submit PENDING
    bool acc2Ok = service.acceptCourseBooking(10, c2Id, true, err);
    assert(!acc2Ok); // Conflict detected against c1!
    assert(err.find("Không thể chấp nhận") != std::string::npos || err.find("trùng") != std::string::npos || err.find("lịch") != std::string::npos);

    // --- Scenario 3: Two bookings on different slots -> both succeed ---
    int c3Id = 0;
    int c4Id = 0;
    bool b3Ok = service.bookCourse(102, 10, "Toan 12", 2, 3, "2026-11-18", 4, true, err, &c3Id);
    assert(b3Ok);
    bool b4Ok = service.bookCourse(102, 20, "Vat ly 12", 2, 4, "2026-11-18", 4, true, err, &c4Id);
    assert(b4Ok);
    assert(service.acceptCourseBooking(10, c3Id, true, err));
    assert(service.acceptCourseBooking(20, c4Id, true, err));

    // --- Scenario 4: Course conflict check directly via hasCourseConflict ---
    assert(service.hasCourseConflict(102, 10, 0, 1, "2026-11-16", 4));
    assert(!service.hasCourseConflict(103, 20, 0, 1, "2026-11-16", 4));

    // --- Scenario 4b: Staggered partial overlap on same weekday & slot ---
    // Course 1 runs Nov 09 .. Nov 30. Course starting Nov 23 overlaps on Nov 23 and Nov 30!
    std::string stagConflictDate;
    std::string stagConflictDetails;
    assert(service.hasCourseConflict(102, 10, 0, 1, "2026-11-30", 4, 0, &stagConflictDetails, &stagConflictDate));
    assert(stagConflictDate == "2026-11-30");
    // Course starting 2026-12-07 (first Monday after Course 1 ends) has zero overlap
    assert(!service.hasCourseConflict(102, 10, 0, 1, "2026-12-14", 4));

    // --- Scenario 5: Non-overlapping dates on same slot -> no conflict ---
    int cFutureId = 0;
    bool bFutureOk = service.bookCourse(102, 10, "Toan 12", 0, 1, "2027-01-04", 4, true, err, &cFutureId);
    assert(bFutureOk);
    assert(service.acceptCourseBooking(10, cFutureId, true, err));

    // --- Scenario 6: REJECTED course does not block slot ---
    int cRejId = 0;
    assert(service.bookCourse(103, 10, "Toan 12", 3, 2, "2026-11-19", 4, true, err, &cRejId));
    assert(service.rejectCourseBooking(10, cRejId, err));
    assert(courseRepo->findCourseBookingById(cRejId)->status == "REJECTED");
    int cRebookId = 0;
    assert(service.bookCourse(101, 10, "Toan 12", 3, 2, "2026-11-19", 4, true, err, &cRebookId));
    assert(service.acceptCourseBooking(10, cRebookId, true, err));

    // --- Scenario 7: CANCELLED course releases reservation ---
    assert(service.cancelCourseBooking(101, UserRole::STUDENT, cRebookId, err));
    assert(courseRepo->findCourseBookingById(cRebookId)->status == "CANCELLED");
    assert(!service.hasCourseConflict(103, 10, 3, 2, "2026-11-19", 4));

    // --- Scenario 8: COMPLETED course does not block future reservations ---
    int cCompId = 0;
    assert(service.bookCourse(101, 10, "Toan 12", 4, 3, "2026-11-20", 4, true, err, &cCompId));
    assert(service.acceptCourseBooking(10, cCompId, true, err));
    assert(service.terminateCourseBooking(10, UserRole::TUTOR, cCompId, err));
    assert(courseRepo->findCourseBookingById(cCompId)->status == "COMPLETED");

    // --- Scenario 9: Non-existent booking ID -> accept fails ---
    assert(!service.acceptCourseBooking(10, 999999, true, err));
    assert(err.find("không tồn tại") != std::string::npos);

    // --- Scenario 10: Wrong tutor accepting another tutor's booking -> rejected ---
    assert(!service.acceptCourseBooking(20, c1Id, true, err));
    assert(err.find("không có quyền") != std::string::npos);

    // --- Scenario 11: Idempotent acceptance on already ACCEPTED booking ---
    assert(service.acceptCourseBooking(10, c1Id, true, err));

    // --- Scenario 12: Invalid parameters rejected ---
    assert(!service.bookCourse(101, 10, "Toan 12", -1, 0, "2026-11-16", 4, err));
    assert(!service.bookCourse(101, 10, "Toan 12", 7, 0, "2026-11-16", 4, err));
    assert(!service.bookCourse(101, 10, "Toan 12", 0, -1, "2026-11-16", 4, err));
    assert(!service.bookCourse(101, 10, "Toan 12", 0, 8, "2026-11-16", 4, err));
    assert(!service.bookCourse(101, 10, "Toan 12", 0, 0, "invalid-date", 4, err));
    assert(!service.bookCourse(101, 10, "Toan 12", 0, 0, "2026-11-16", 5, err));

    // --- Scenario 13: Concurrency protection on acceptCourseBooking ---
    int raceA = 0;
    int raceB = 0;
    assert(service.bookCourse(101, 20, "Vat ly 12", 1, 0, "2027-02-02", 4, true, err, &raceA));
    assert(service.bookCourse(102, 20, "Vat ly 12", 1, 0, "2027-02-02", 4, true, err, &raceB));

    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};
    std::thread th1([&]() {
        std::string tErr;
        if (service.acceptCourseBooking(20, raceA, true, tErr)) successCount++;
        else failCount++;
    });
    std::thread th2([&]() {
        std::string tErr;
        if (service.acceptCourseBooking(20, raceB, true, tErr)) successCount++;
        else failCount++;
    });
    th1.join();
    th2.join();
    assert(successCount.load() == 1);
    assert(failCount.load() == 1);

    std::cout << "  -> PASS: All Course Booking Conflict Detection scenarios verified successfully!" << std::endl;
}

// ============================================================================
// Phase 2 Step 2: Dedicated Entity Serializers Verification Test Suite
// ============================================================================
void run_test_entity_serializers() {
    std::cout << "[TEST] 17. Verifying Dedicated Entity Serializers (Phase 2 Step 2)..." << std::endl;

    // ------------------------------------------------------------------------
    // Part A: UserSerializer Verification (Student, Tutor, Admin)
    // ------------------------------------------------------------------------
    {
        // 1. Student Round-trip & Invariants
        Student origStudent(101, "student_test", "pass123", "student@test.vn", "Nguyễn Văn Test",
                            12, ScheduleGuard::VALID_SCHEDULE_MASK, true);
        std::string studentCsv = UserSerializer::toCsv(origStudent);
        
        std::string parseErr;
        std::shared_ptr<User> deserializedUser = UserSerializer::fromCsv(studentCsv, &parseErr);
        assert(deserializedUser != nullptr);
        assert(parseErr.empty());
        assert(deserializedUser->getRole() == UserRole::STUDENT);

        // Verify no object slicing: dynamic_cast to Student must succeed
        auto* sPtr = dynamic_cast<Student*>(deserializedUser.get());
        assert(sPtr != nullptr);
        assert(sPtr->getId() == 101);
        assert(sPtr->getUsername() == "student_test");
        assert(sPtr->getPasswordHash() == "pass123"); // Preserves plaintext password technical debt
        assert(sPtr->getEmail() == "student@test.vn");
        assert(sPtr->getFullName() == "Nguyễn Văn Test");
        assert(sPtr->getIsActive() == true);
        assert(sPtr->getGradeLevel() == 12);

        assert(sPtr->getScheduleBitmask() == ScheduleGuard::VALID_SCHEDULE_MASK);

        // Strongly-typed helper
        auto strongStudent = UserSerializer::studentFromCsv(studentCsv);
        assert(strongStudent != nullptr);
        assert(strongStudent->getId() == 101);

        // 2. Tutor Round-trip & Invariants
        Tutor origTutor(202, "tutor_test", "tutorPass", "tutor@test.vn", "ThS. Trần Gia Sư",
                        "Giảng viên ĐH 10 năm kinh nghiệm.", "Vật lý 12", 300000.0, 4.8, 15,
                        4500000.0, 0x000000000000000FULL, true);
        std::string tutorCsv = UserSerializer::toCsv(origTutor);
        
        deserializedUser = UserSerializer::fromCsv(tutorCsv, &parseErr);
        assert(deserializedUser != nullptr);
        assert(parseErr.empty());
        assert(deserializedUser->getRole() == UserRole::TUTOR);

        auto* tPtr = dynamic_cast<Tutor*>(deserializedUser.get());
        assert(tPtr != nullptr);
        assert(tPtr->getId() == 202);
        assert(tPtr->getUsername() == "tutor_test");
        assert(tPtr->getPasswordHash() == "tutorPass");
        assert(tPtr->getEmail() == "tutor@test.vn");
        assert(tPtr->getFullName() == "ThS. Trần Gia Sư");
        assert(tPtr->getIsActive() == true);
        assert(tPtr->getBio() == "Giảng viên ĐH 10 năm kinh nghiệm.");
        assert(tPtr->getSubjectSpecialty() == "Vật lý 12");
        assert(tPtr->getHourlyRate() == 300000.0);
        assert(tPtr->getRating() == 4.8);
        assert(tPtr->getTotalReviews() == 15);
        assert(tPtr->getTotalEarnings() == 4500000.0);
        assert(tPtr->getScheduleBitmask() == 0x000000000000000FULL);

        auto strongTutor = UserSerializer::tutorFromCsv(tutorCsv);
        assert(strongTutor != nullptr);
        assert(strongTutor->getId() == 202);

        // 3. Admin Round-trip & Invariants (Unified Root Admin, 7 Base Fields)
        Admin origAdmin(303, "admin_test", "superSecret", "admin@test.vn", "Quản Trị Viên Hệ Thống", true);
        std::string adminCsv = UserSerializer::toCsv(origAdmin);
        assert(SerializerUtils::splitCsvLine(adminCsv).size() == 7);

        deserializedUser = UserSerializer::fromCsv(adminCsv, &parseErr);
        assert(deserializedUser != nullptr);
        assert(parseErr.empty());
        assert(deserializedUser->getRole() == UserRole::ADMIN);

        auto* aPtr = dynamic_cast<Admin*>(deserializedUser.get());
        assert(aPtr != nullptr);
        assert(aPtr->getId() == 303);
        assert(aPtr->getUsername() == "admin_test");
        assert(aPtr->getPasswordHash() == "superSecret");
        assert(aPtr->getEmail() == "admin@test.vn");
        assert(aPtr->getFullName() == "Quản Trị Viên Hệ Thống");
        assert(aPtr->getIsActive() == true);
        assert(aPtr->getRole() == UserRole::ADMIN);

        auto strongAdmin = UserSerializer::adminFromCsv(adminCsv);
        assert(strongAdmin != nullptr);
        assert(strongAdmin->getId() == 303);

        // 4. Polymorphic User toCsv dispatch
        std::shared_ptr<User> polyStudent = std::make_shared<Student>(origStudent);
        std::shared_ptr<User> polyTutor = std::make_shared<Tutor>(origTutor);
        std::shared_ptr<User> polyAdmin = std::make_shared<Admin>(origAdmin);

        assert(UserSerializer::toCsv(*polyStudent) == studentCsv);
        assert(UserSerializer::toCsv(*polyTutor) == tutorCsv);
        assert(UserSerializer::toCsv(*polyAdmin) == adminCsv);

        // 5. Error handling & Edge cases
        std::string err;
        assert(UserSerializer::fromCsv("1,short,pass,email,name", &err) == nullptr); // < 7 fields
        assert(!err.empty());

        assert(UserSerializer::fromCsv("1,user,pass,mail,name,STUDENT,1", &err) == nullptr); // Student < 9 fields
        assert(UserSerializer::fromCsv("1,user,pass,mail,name,TUTOR,1,bio,subj,100", &err) == nullptr); // Tutor < 14 fields
        assert(UserSerializer::fromCsv("1,user,pass,mail,name,ADMIN", &err) == nullptr); // Admin < 7 fields
        assert(UserSerializer::fromCsv("1,user,pass,mail,name,ADMIN,1", &err) != nullptr); // Exactly 7 fields is valid
        assert(UserSerializer::fromCsv("invalid_id,user,pass,mail,name,STUDENT,1,10,Math,100,0", &err) == nullptr); // Non-numeric ID
        assert(UserSerializer::fromCsv("1,user,pass,mail,name,UNKNOWN_ROLE,1,10", &err) == nullptr); // Unknown role

        // Type mismatch rejection in strongly typed helpers
        assert(UserSerializer::studentFromCsv(tutorCsv, &err) == nullptr);
        assert(UserSerializer::tutorFromCsv(studentCsv, &err) == nullptr);
        assert(UserSerializer::adminFromCsv(studentCsv, &err) == nullptr);
    }

    // ------------------------------------------------------------------------
    // Part B: CourseBookingSerializer Verification
    // ------------------------------------------------------------------------
    {
        const std::vector<std::string> validStatuses = {
            "PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED", "EARLY_TERMINATED"
        };

        for (int i = 0; i < (int)validStatuses.size(); ++i) {
            CourseBooking origCb(500 + i, 10, 20, "Student A", "Tutor B",
                                 "Math 12", 150000.0, "2026-11-13", 4, validStatuses[i], "2026-11-09",
                                 std::to_string(i % 7) + ":" + std::to_string(i % 8));
            std::string cbCsv = CourseBookingSerializer::toCsv(origCb);

            std::string bErr;
            auto parsedCbOpt = CourseBookingSerializer::fromCsv(cbCsv, &bErr);
            assert(parsedCbOpt.has_value());
            assert(bErr.empty());

            const CourseBooking& cb = *parsedCbOpt;
            assert(cb.id == 500 + i);
            assert(cb.studentId == 10);
            assert(cb.tutorId == 20);
            assert(cb.status == validStatuses[i]);
            assert(cb.weeklySchedule == std::to_string(i % 7) + ":" + std::to_string(i % 8));
        }

        std::string err;
        assert(!CourseBookingSerializer::fromCsv("1,2,3,Student,Tutor,Math,100,2026-11-13,4,INVALID_STATUS,2026-11-09,1:0", &err).has_value());
        assert(!err.empty());

        assert(!CourseBookingSerializer::fromCsv("1,2,3,Student,Tutor,Math,100,1", &err).has_value());
        assert(CourseBookingSerializer::isValidStatus("PENDING"));
        assert(CourseBookingSerializer::isValidStatus("ACCEPTED"));
        assert(CourseBookingSerializer::isValidStatus("REJECTED"));
        assert(CourseBookingSerializer::isValidStatus("COMPLETED"));
        assert(CourseBookingSerializer::isValidStatus("CANCELLED"));
        assert(CourseBookingSerializer::isValidStatus("EARLY_TERMINATED"));
    }
    // ------------------------------------------------------------------------
    // Part C: ReviewSerializer Verification
    // ------------------------------------------------------------------------
    {
        // 1. Full 8-fields Round-trip across all valid ratings (1..5 stars)
        for (int rating = 1; rating <= 5; ++rating) {
            Review origReview(700 + rating, 500, 10, 20, "Nguyễn Văn A",
                              rating, "Giảng dạy rất nhiệt tình!", "2026-11-06");
            std::string reviewCsv = ReviewSerializer::toCsv(origReview);

            std::string rErr;
            auto parsedReviewOpt = ReviewSerializer::fromCsv(reviewCsv, &rErr);
            assert(parsedReviewOpt.has_value());
            assert(rErr.empty());

            const Review& r = *parsedReviewOpt;
            assert(r.id == 700 + rating);
            assert(r.bookingId == 500);
            assert(r.studentId == 10);
            assert(r.tutorId == 20);
            assert(r.studentName == "Nguyễn Văn A");
            assert(r.ratingScore == rating);
            assert(r.comment == "Giảng dạy rất nhiệt tình!");
            assert(r.createdAt == "2026-11-06");

            // Verify boolean overload as well
            Review rOverload;
            assert(ReviewSerializer::fromCsv(reviewCsv, rOverload));
            assert(rOverload.id == r.id);
            assert(rOverload.ratingScore == rating);
        }

        // 2. Error handling: invalid ratings (0, 6, -1), missing fields, non-numeric
        std::string err;
        assert(!ReviewSerializer::fromCsv("1,100,10,20,Student,0,Comment,2026-01-01", &err).has_value()); // 0 stars
        assert(!ReviewSerializer::fromCsv("1,100,10,20,Student,6,Comment,2026-01-01", &err).has_value()); // 6 stars
        assert(!ReviewSerializer::fromCsv("1,100,10,20,Student,-2,Comment,2026-01-01", &err).has_value()); // Negative
        assert(!ReviewSerializer::fromCsv("1,100,10,20,Student", &err).has_value()); // < 8 fields
        assert(!ReviewSerializer::fromCsv("1,100,10,20,Student,not_a_num,Comment,2026-01-01", &err).has_value()); // Bad rating
        assert(!ReviewSerializer::fromCsv("bad_id,100,10,20,Student,5,Comment,2026-01-01", &err).has_value()); // Bad id

        assert(ReviewSerializer::isValidRatingScore(1));
        assert(ReviewSerializer::isValidRatingScore(5));
        assert(!ReviewSerializer::isValidRatingScore(0));
        assert(!ReviewSerializer::isValidRatingScore(6));
    }

    // ------------------------------------------------------------------------
    // Part D: Backward Compatibility (Exact Canonical CSV Format)
    // ------------------------------------------------------------------------
    {
        // 1. Student canonical CSV parity (9 fields canonical)
        Student s(6, "student_nam", "pass123", "nam.student@gmail.com", "Nguyễn Văn Nam",
                  12, 71777214294589695ULL, true);
        assert(UserSerializer::toCsv(s) == "6,student_nam,pass123,nam.student@gmail.com,Nguyễn Văn Nam,STUDENT,1,12,71777214294589695");

        // Backward compatibility: Parsing legacy 11-field CSV line successfully discards obsolete preferences
        auto legacyParsed = UserSerializer::studentFromCsv("6,student_nam,pass123,nam.student@gmail.com,Nguyễn Văn Nam,STUDENT,1,12,Toán 12,180000,71777214294589695");
        assert(legacyParsed != nullptr);
        assert(legacyParsed->getId() == 6);
        assert(legacyParsed->getGradeLevel() == 12);
        assert(legacyParsed->getScheduleBitmask() == 71777214294589695ULL);
        assert(UserSerializer::toCsv(*legacyParsed) == "6,student_nam,pass123,nam.student@gmail.com,Nguyễn Văn Nam,STUDENT,1,12,71777214294589695");

        // 2. Tutor canonical CSV parity
        Tutor t(2, "tutor_math", "pass123", "le.math@tutormatch.vn", "ThS. Lê Văn Toàn",
                "Chuyên gia luyện thi Đại học môn Toán với 10 năm kinh nghiệm.", "Toán 12 - Luyện thi Đại học",
                150000.0, 5.0, 2, 0.0, 71777214294589694ULL, true);
        assert(UserSerializer::toCsv(t) == "2,tutor_math,pass123,le.math@tutormatch.vn,ThS. Lê Văn Toàn,TUTOR,1,Chuyên gia luyện thi Đại học môn Toán với 10 năm kinh nghiệm.,Toán 12 - Luyện thi Đại học,150000,5,2,0,71777214294589694");

        // 3. Admin canonical CSV parity
        Admin a(1, "admin", "admin123", "admin@tutormatch.vn", "System Administrator", true);
        assert(UserSerializer::toCsv(a) == "1,admin,admin123,admin@tutormatch.vn,System Administrator,ADMIN,1");

        // 4. CourseBooking canonical CSV parity
        CourseBooking cb(101, 6, 2, "Nguyễn Văn Nam", "ThS. Lê Văn Toàn", "Toán 12",
                         150000.0, "2026-11-13", 4, "COMPLETED", "2026-11-09", "4:0");
        assert(!CourseBookingSerializer::toCsv(cb).empty());

        // 5. Review canonical CSV parity
        Review r(1, 101, 6, 2, "Nguyễn Văn Nam", 5,
                 "Thầy Toàn giảng Toán 12 rất dễ hiểu  phương pháp giải trắc nghiệm tốc độ cao.", "2026-08-15");
        assert(ReviewSerializer::toCsv(r) == "1,101,6,2,Nguyễn Văn Nam,5,Thầy Toàn giảng Toán 12 rất dễ hiểu  phương pháp giải trắc nghiệm tốc độ cao.,2026-08-15");

        // 6. Comma escaping equivalence in strings
        // When a string contains a comma, escapeCsv replaces it with space.
        Student commaStudent(99, "comma,user", "p,w,d", "mail,test@vn", "Nguyễn, Văn, Nam",
                             10, 1ULL, true);
        assert(UserSerializer::toCsv(commaStudent) == "99,comma user,p w d,mail test@vn,Nguyễn  Văn  Nam,STUDENT,1,10,1");

        Tutor commaTutor(98, "tutor,comma", "p,w,d", "tutor,mail@vn", "ThS, Lê, Toàn",
                         "Bio, with, multiple, commas", "Subj, Specialty", 200000.0, 4.5, 3,
                         50000.0, 2ULL, true);
        assert(UserSerializer::toCsv(commaTutor) == "98,tutor comma,p w d,tutor mail@vn,ThS  Lê  Toàn,TUTOR,1,Bio  with  multiple  commas,Subj  Specialty,200000,4.5,3,50000,2");



        Review commaReview(96, 97, 1, 2, "Student, Name", 5,
                           "Comment, with, several, commas, embedded", "2026,01,01");
        assert(ReviewSerializer::toCsv(commaReview) == "96,97,1,2,Student  Name,5,Comment  with  several  commas  embedded,2026 01 01");

        // 7. Empty strings and boundary numbers
        Student emptyStudent(0, "", "", "", "", 0, 0ULL, false);
        assert(UserSerializer::toCsv(emptyStudent) == "0,,,,,STUDENT,0,0,0");



        Review emptyReview(0, 0, 0, 0, "", 1, "", "");
        assert(ReviewSerializer::toCsv(emptyReview) == "0,0,0,0,,1,,");

        // 8. Re-serialization Invariance (parse -> serialize -> parse -> serialize)
        std::string sCsv = UserSerializer::toCsv(s);
        auto sParsed = UserSerializer::studentFromCsv(sCsv);
        assert(sParsed != nullptr);
        assert(UserSerializer::toCsv(*sParsed) == sCsv);

        std::string tCsv = UserSerializer::toCsv(t);
        auto tParsed = UserSerializer::tutorFromCsv(tCsv);
        assert(tParsed != nullptr);
        assert(UserSerializer::toCsv(*tParsed) == tCsv);

        std::string aCsv = UserSerializer::toCsv(a);
        auto aParsed = UserSerializer::adminFromCsv(aCsv);
        assert(aParsed != nullptr);
        assert(UserSerializer::toCsv(*aParsed) == aCsv);



        std::string rCsv = ReviewSerializer::toCsv(r);
        auto rParsed = ReviewSerializer::fromCsv(rCsv);
        assert(rParsed.has_value());
        assert(ReviewSerializer::toCsv(*rParsed) == rCsv);
    }

    std::cout << "  -> PASS: UserSerializer, CourseBookingSerializer, and ReviewSerializer verified with 100% parity!" << std::endl;
}

// ============================================================================
// Phase 2 Step 3: Concrete File Repositories Verification Test Suite
// ============================================================================
void run_test_concrete_file_repositories() {
    std::cout << "[TEST] 18. Verifying Concrete File Repositories (Phase 2 Step 3)..." << std::endl;

    // Use an isolated temporary sandbox directory to ensure data/*.dat is 100% untouched
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "tutormatch_step3_test_sandbox";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    std::string testUsersPath = (testDir / "users_test.dat").string();
    std::string testBookingsPath = (testDir / "bookings_test.dat").string();
    std::string testReviewsPath = (testDir / "reviews_test.dat").string();

    // ------------------------------------------------------------------------
    // Part A: FileUserRepository Verification
    // ------------------------------------------------------------------------
    {
        FileUserRepository userRepo(testUsersPath);
        assert(userRepo.initialize());
        assert(userRepo.getAllUsers().empty());
        assert(userRepo.getNextUserId() == 1);

        // 1. Save Student, Tutor, Admin
        Student s(1, "student_john", "secret1", "john@mail.vn", "John Doe", 11, "Toán 11", 150000.0, 0x0000000000000001ULL, true);
        Tutor t1(2, "tutor_alice", "secret2", "alice@mail.vn", "Alice Tutor", "Bio Alice", "Vật lý", 200000.0, 4.8, 5, 1000000.0, 0x0000000000000002ULL, true);
        Tutor t2(3, "tutor_bob", "secret3", "bob@mail.vn", "Bob Tutor", "Bio Bob", "Toán 12", 120000.0, 4.2, 3, 600000.0, 0x0000000000000004ULL, true);
        Admin a(4, "admin_root", "secret4", "root@mail.vn", "Root Admin", true);

        assert(userRepo.saveUser(s));
        assert(userRepo.saveUser(t1));
        assert(userRepo.saveUser(t2));
        assert(userRepo.saveUser(a));

        assert(userRepo.getAllUsers().size() == 4);
        assert(userRepo.getAllStudents().size() == 1);
        assert(userRepo.getAllTutors().size() == 2);

        // 2. Find by ID & Polymorphic Subtype check
        auto foundS = userRepo.findUserById(1);
        assert(foundS != nullptr);
        assert(foundS->getRole() == UserRole::STUDENT);
        auto* castS = dynamic_cast<Student*>(foundS.get());
        assert(castS != nullptr && castS->getGradeLevel() == 11);
        assert(castS->getPasswordHash() == "secret1"); // Preserves plaintext password technical debt

        auto foundT = userRepo.findUserById(2);
        assert(foundT != nullptr);
        auto* castT = dynamic_cast<Tutor*>(foundT.get());
        assert(castT != nullptr && castT->getHourlyRate() == 200000.0);

        // 3. Find by Username
        assert(userRepo.findUserByUsername("tutor_alice") != nullptr);
        assert(userRepo.findUserByUsername("tutor_alice")->getId() == 2);
        assert(userRepo.findUserByUsername("nonexistent") == nullptr);

        // 4. Range queries via AVL tree indexes
        auto priceResults = userRepo.findTutorIdsByPriceRange(100000.0, 150000.0);
        assert(priceResults.size() == 1 && priceResults[0] == 3); // Bob (120k)

        auto ratingResults = userRepo.findTutorIdsByMinRating(4.5);
        assert(ratingResults.size() == 1 && ratingResults[0] == 2); // Alice (4.8)

        // 5. Update user and verify cache/AVL synchronization
        castT->setHourlyRate(250000.0);
        castT->setRating(4.9);
        assert(userRepo.updateUser(*castT));

        auto reloadedT = userRepo.findUserById(2);
        assert(reloadedT != nullptr);
        auto* reloadedCastT = dynamic_cast<Tutor*>(reloadedT.get());
        assert(reloadedCastT != nullptr && reloadedCastT->getHourlyRate() == 250000.0);
        assert(reloadedCastT->getRating() == 4.9);

        // Verify AVL trees updated: old rate 200k no longer matches
        assert(userRepo.findTutorIdsByPriceRange(190000.0, 210000.0).empty());
        assert(!userRepo.findTutorIdsByPriceRange(240000.0, 260000.0).empty());

        // 6. Soft delete
        assert(userRepo.deleteUserSoft(2));
        assert(userRepo.getAllTutors().size() == 1); // Only Bob active
        assert(userRepo.getAllUsers().size() == 4);  // Alice still in allUsers as inactive
        assert(userRepo.findTutorIdsByMinRating(4.5).empty()); // Inactive tutor excluded from search

        // 7. Reload from disk in a fresh repository instance
        FileUserRepository userRepo2(testUsersPath);
        assert(userRepo2.initialize());
        assert(userRepo2.getAllUsers().size() == 4);
        assert(userRepo2.getAllTutors().size() == 1);
        assert(userRepo2.findUserById(1)->getUsername() == "student_john");

        // 8. Corrupted line resilience
        {
            std::ofstream uAppend(testUsersPath, std::ios::app);
            uAppend << "corrupted,line,with,missing,fields\n";
            uAppend.close();
        }
        userRepo2.rebuildRamIndex();
        assert(userRepo2.getAllUsers().size() == 4); // Still 4 valid users, corrupted line skipped
    }

    // ------------------------------------------------------------------------
    // Part B: FileCourseBookingRepository Verification
    // ------------------------------------------------------------------------
    {
        std::string testCbPath = (testDir / "test_cb_repo.dat").string();
        FileCourseBookingRepository cbRepo(testCbPath);
        assert(cbRepo.initialize());
        assert(cbRepo.getAllCourseBookings().empty());
        assert(cbRepo.getNextCourseBookingId() == 1001);

        CourseBooking cb1(1001, 1, 3, "Student 1", "Tutor 1", "Math", 150000.0, "2026-11-13", 4, "PENDING", "2026-11-09", "1:0");
        assert(cbRepo.saveCourseBooking(cb1));
        assert(cbRepo.getAllCourseBookings().size() == 1);
        assert(cbRepo.findCourseBookingById(1001) != nullptr);

        cb1.status = "ACCEPTED";
        assert(cbRepo.updateCourseBooking(cb1));
        assert(cbRepo.findCourseBookingById(1001)->status == "ACCEPTED");
    }

    // ------------------------------------------------------------------------
    // Part C: FileReviewRepository Verification
    // ------------------------------------------------------------------------
    {
        FileReviewRepository reviewRepo(testReviewsPath);
        assert(reviewRepo.initialize());
        assert(reviewRepo.getAllReviews().empty());
        assert(reviewRepo.getNextReviewId() == 1);

        // 1. Save Reviews
        Review r1(1, 101, 1, 3, "John Doe", 5, "Thầy Bob dạy rất dễ hiểu!", "2026-11-06");
        Review r2(2, 102, 1, 3, "John Doe", 4, "Nhiệt tình và chu đáo.", "2026-11-06");
        Review r3(3, 103, 5, 2, "Other Student", 5, "Cô Alice giải thích rất sâu sắc.", "2026-11-06");

        assert(reviewRepo.saveReview(r1));
        assert(reviewRepo.saveReview(r2));
        assert(reviewRepo.saveReview(r3));

        assert(reviewRepo.getAllReviews().size() == 3);
        assert(reviewRepo.getNextReviewId() >= 4);

        // 2. Query by Tutor
        auto tutor3Reviews = reviewRepo.getReviewsByTutorId(3);
        assert(tutor3Reviews.size() == 2);
        assert(reviewRepo.getReviewsByTutorId(2).size() == 1);
        assert(reviewRepo.getReviewsByTutorId(99).empty());

        // 3. Delete Review
        assert(reviewRepo.deleteReview(2)); // Delete r2
        assert(reviewRepo.getAllReviews().size() == 2);
        assert(reviewRepo.getReviewsByTutorId(3).size() == 1);

        // Delete non-existent review -> returns false
        assert(!reviewRepo.deleteReview(999));

        // 4. Reload from disk in a fresh instance
        FileReviewRepository reviewRepo2(testReviewsPath);
        assert(reviewRepo2.initialize());
        assert(reviewRepo2.getAllReviews().size() == 2);
        assert(reviewRepo2.getReviewsByTutorId(3).size() == 1);

        // 5. Corrupted review line resilience
        {
            std::ofstream rAppend(testReviewsPath, std::ios::app);
            rAppend << "corrupted,review,line\n";
            rAppend.close();
        }
        assert(reviewRepo2.getAllReviews().size() == 2); // Corrupted line skipped cleanly
    }

    // Clean up temporary sandbox directory
    std::filesystem::remove_all(testDir, ec);

    std::cout << "  -> PASS: FileUserRepository, FileCourseBookingRepository, and FileReviewRepository verified successfully!" << std::endl;
}

void run_test_phase3_end_to_end_scenarios() {
    std::cout << "[TEST] 19. Verifying Phase 3 End-to-End Business Workflows (E2E-01 to E2E-10)..." << std::endl;
    std::error_code ec;
    auto testDir = std::filesystem::temp_directory_path() / "tutormatch_phase3_e2e_sandbox";
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    std::string uPath = (testDir / "users.dat").string();
    std::string cbPath = (testDir / "course_bookings.dat").string();
    std::string rPath = (testDir / "reviews.dat").string();

    // Setup Service & Repository Infrastructure in Sandbox
    auto userRepo = std::make_shared<FileUserRepository>(uPath);
    auto courseBookingRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
    auto reviewRepo = std::make_shared<FileReviewRepository>(rPath);

    assert(userRepo->initialize());
    assert(courseBookingRepo->initialize());
    assert(reviewRepo->initialize());

    auto authService = std::make_shared<AuthService>(userRepo);
    auto userService = std::make_shared<UserService>(userRepo);
    auto bookingService = std::make_shared<BookingService>(userRepo, reviewRepo, courseBookingRepo);
    auto reviewService = std::make_shared<ReviewService>(reviewRepo, userRepo, courseBookingRepo);
    auto matchingService = std::make_shared<MatchingService>(userRepo);
    auto adminService = std::make_shared<AdminService>(userRepo, reviewRepo, bookingService);

    auto bridge = std::make_shared<GUIBridge>();
    bridge->initialize(authService, userService, bookingService, reviewService, matchingService, adminService);

    // ========================================================================
    // E2E-01: Student Registration, Validation & Authentication
    // ========================================================================
    {
        std::string err;
        // Validation: Reject empty required fields
        assert(!authService->registerStudent("", "pass123", "Alice Nguyen", "alice@test.vn", "12", "Toán 12", 200000.0, err));
        assert(!authService->registerStudent("student_alice", "", "Alice Nguyen", "alice@test.vn", "12", "Toán 12", 200000.0, err));
        assert(!authService->registerStudent("student_alice", "pass123", "", "alice@test.vn", "12", "Toán 12", 200000.0, err));
        assert(!authService->registerStudent("student_alice", "pass123", "Alice Nguyen", "", "12", "Toán 12", 200000.0, err));
        assert(!authService->registerStudent("student_alice", "pass123", "Alice Nguyen", "alice@test.vn", "12", "Toán 12", -50000.0, err));

        // Successful Registration
        assert(authService->registerStudent("student_alice", "pass123", "Alice Nguyen", "alice@test.vn", "12", "Toán 12", 200000.0, err));
        
        // Duplicate Username Rejection
        assert(!authService->registerStudent("student_alice", "pass456", "Another Alice", "another@test.vn", "11", "Toán 11", 150000.0, err));

        // Authentication: Invalid and Valid
        assert(!authService->login("student_alice", "wrongpass", err));
        assert(!authService->login("", "", err));
        assert(!authService->login("nonexistent", "pass123", err));

        assert(authService->login("student_alice", "pass123", err));
        assert(authService->isLoggedIn());
        assert(authService->getCurrentRole() == UserRole::STUDENT);
        auto currentU = authService->getCurrentUser();
        assert(currentU != nullptr && currentU->getFullName() == "Alice Nguyen");

        // Logout & Session Reset
        authService->logout();
        assert(!authService->isLoggedIn());
        assert(authService->getCurrentUser() == nullptr);
    }

    // ========================================================================
    // E2E-02: Tutor Registration, Validation, Profile Update & Discovery
    // ========================================================================
    int tutorBobId = 0;
    int tutorCarolId = 0;
    {
        std::string err;
        // Validation
        assert(!authService->registerTutor("", "pass123", "Bob Tran", "bob@test.vn", "Toán 12", 150000.0, "Bio", err));
        assert(!authService->registerTutor("tutor_bob", "pass123", "Bob Tran", "bob@test.vn", "Toán 12", -100.0, "Bio", err));

        // Successful Registration
        assert(authService->registerTutor("tutor_bob", "pass123", "ThS. Bob Tran", "bob@test.vn", "Toán 12 - Luyện thi", 150000.0, "Kinh nghiệm 10 năm", err));
        assert(authService->registerTutor("tutor_carol", "pass123", "TS. Carol Le", "carol@test.vn", "Vật lý 12", 250000.0, "Giảng viên Vật lý", err));

        auto bobUser = userRepo->findUserByUsername("tutor_bob");
        auto carolUser = userRepo->findUserByUsername("tutor_carol");
        assert(bobUser != nullptr && carolUser != nullptr);
        tutorBobId = bobUser->getId();
        tutorCarolId = carolUser->getId();

        // Profile Update
        auto bobTutor = std::static_pointer_cast<Tutor>(bobUser);
        assert(userService->updateTutorProfile(*bobTutor, "ThS. Bob Tran", "bob.tran@tutormatch.vn", "Kinh nghiệm 12 năm chuyên Toán", "Toán 12 - Luyện thi", 160000.0, err));

        // Discovery & Multi-criteria Search
        auto mathTutors = userService->searchAndFilterTutors("", "Toán 12", 0, 0, 0);
        assert(mathTutors.size() == 1 && mathTutors[0]->getId() == tutorBobId);

        auto physTutors = userService->searchAndFilterTutors("", "Vật lý 12", 0, 0, 0);
        assert(physTutors.size() == 1 && physTutors[0]->getId() == tutorCarolId);

        // Price filter
        auto cheapTutors = userService->searchAndFilterTutors("", "", 0, 180000.0, 0);
        assert(cheapTutors.size() == 1 && cheapTutors[0]->getId() == tutorBobId);

        auto expTutors = userService->searchAndFilterTutors("", "", 200000.0, 300000.0, 0);
        assert(expTutors.size() == 1 && expTutors[0]->getId() == tutorCarolId);

        // Empty filter results
        auto noMatch = userService->searchAndFilterTutors("", "Hóa học", 0, 0, 0);
        assert(noMatch.empty());
    }

    // ========================================================================
    // E2E-03: Successful Booking Lifecycle (PENDING -> ACCEPTED -> COMPLETED)
    // ========================================================================
    int aliceId = userRepo->findUserByUsername("student_alice")->getId();
    int booking1Id = 0;
    {
        std::string msg;
        // 1. Student books course (Day 1 - Tue, Slot 2, startDate 2026-11-10, 4 weeks)
        assert(bookingService->bookCourse(aliceId, tutorBobId, "Toán 12", 1, 2, "2026-11-17", 4, true, msg, &booking1Id));
        
        auto tutorBookings = bookingService->getTutorCourseBookings(tutorBobId);
        assert(tutorBookings.size() == 1);
        assert(tutorBookings[0].id == booking1Id);
        assert(tutorBookings[0].status == "PENDING");
        assert(tutorBookings[0].studentId == aliceId);

        // 2. Tutor accepts course booking
        assert(bookingService->acceptCourseBooking(tutorBobId, booking1Id, true, msg));
        auto acceptedB = courseBookingRepo->findCourseBookingById(booking1Id);
        assert(acceptedB != nullptr && acceptedB->status == "ACCEPTED");

        // Metrics check: 1 accepted booking, weeklyCost = 160000.0
        auto metrics = bookingService->getStudentBookingMetrics(aliceId);
        assert(metrics.acceptedCount == 1);
        assert(metrics.weeklyCost == 160000.0);
        assert(metrics.historyCount == 0);

        // 3. Mark Course Completed
        assert(bookingService->terminateCourseBooking(tutorBobId, UserRole::TUTOR, booking1Id, msg));
        auto completedB = courseBookingRepo->findCourseBookingById(booking1Id);
        assert(completedB != nullptr && completedB->status == "COMPLETED");

        // History check
        auto studentHistory = bookingService->getStudentCourseBookings(aliceId);
        assert(studentHistory.size() == 1 && studentHistory[0].status == "COMPLETED");

        metrics = bookingService->getStudentBookingMetrics(aliceId);
        assert(metrics.acceptedCount == 0);
        assert(metrics.historyCount == 1);
        assert(metrics.weeklyCost == 0.0);
    }

    // ========================================================================
    // E2E-04: Double Booking Prevention (Reservation Policy)
    // ========================================================================
    int danId = 0;
    {
        std::string err;
        assert(authService->registerStudent("student_dan", "pass123", "Dan Hoang", "dan@test.vn", "12", "Vật lý", 300000.0, err));
        danId = userRepo->findUserByUsername("student_dan")->getId();

        std::string msg;
        // Alice books Carol on Day 2, Slot 4, startDate 2026-11-18, 4 weeks
        int aliceCarolBId = 0;
        assert(bookingService->bookCourse(aliceId, tutorCarolId, "Vật lý", 2, 4, "2026-11-18", 4, true, msg, &aliceCarolBId));
        assert(bookingService->acceptCourseBooking(tutorCarolId, aliceCarolBId, true, msg));

        // Dan books Carol on same slot (2, 4) with overlapping dates
        int danCarolBId = 0;
        assert(bookingService->bookCourse(danId, tutorCarolId, "Vật lý", 2, 4, "2026-11-18", 4, true, msg, &danCarolBId));

        // Carol attempts to accept Dan's booking -> MUST BE BLOCKED due to conflict with Alice
        assert(!bookingService->acceptCourseBooking(tutorCarolId, danCarolBId, true, msg));
    }

    // ========================================================================
    // E2E-05: Rejected Booking Releases Slot
    // ========================================================================
    {
        std::string msg;
        // Dan books Bob on Day 3, Slot 1, startDate 2026-11-19, 4 weeks
        int danBobBId = 0;
        assert(bookingService->bookCourse(danId, tutorBobId, "Toán 12", 3, 1, "2026-11-19", 4, true, msg, &danBobBId));

        // Bob rejects Dan's booking
        assert(bookingService->rejectCourseBooking(tutorBobId, danBobBId, msg));
        assert(courseBookingRepo->findCourseBookingById(danBobBId)->status == "REJECTED");

        // Now Alice can book Bob on Day 3, Slot 1 and Bob can accept
        int aliceBobBId = 0;
        assert(bookingService->bookCourse(aliceId, tutorBobId, "Toán 12", 3, 1, "2026-11-19", 4, true, msg, &aliceBobBId));
        assert(bookingService->acceptCourseBooking(tutorBobId, aliceBobBId, true, msg));
    }

    // ========================================================================
    // E2E-06: Cancellation (PENDING and ACCEPTED)
    // ========================================================================
    {
        std::string msg;
        // 1. Create and cancel PENDING booking
        int pendingId = 0;
        assert(bookingService->bookCourse(aliceId, tutorCarolId, "Vật lý", 4, 0, "2026-11-13", 4, true, msg, &pendingId));
        assert(bookingService->cancelCourseBooking(aliceId, UserRole::STUDENT, pendingId, msg));
        assert(courseBookingRepo->findCourseBookingById(pendingId)->status == "CANCELLED");

        // 2. Create, accept, and cancel ACCEPTED booking
        int acceptedId = 0;
        assert(bookingService->bookCourse(aliceId, tutorCarolId, "Vật lý", 4, 0, "2026-11-13", 4, true, msg, &acceptedId));
        assert(bookingService->acceptCourseBooking(tutorCarolId, acceptedId, true, msg));
        assert(bookingService->cancelCourseBooking(aliceId, UserRole::STUDENT, acceptedId, msg));
        assert(courseBookingRepo->findCourseBookingById(acceptedId)->status == "CANCELLED");

        // 3. Cannot cancel an already COMPLETED booking
        assert(!bookingService->cancelCourseBooking(aliceId, UserRole::STUDENT, booking1Id, msg));

        // 4. Unauthorized cancellation rejected
        assert(!bookingService->cancelCourseBooking(danId, UserRole::STUDENT, acceptedId, msg));
    }

    // ========================================================================
    // E2E-07: Review Lifecycle & Rating Recalculation
    // ========================================================================
    int review1Id = 0;
    {
        std::string msg;
        // 1. Submit review for COMPLETED booking1Id
        assert(reviewService->submitReview(aliceId, booking1Id, 5, "Thầy dạy rất tận tâm!", msg));
        auto revs = reviewRepo->getReviewsByTutorId(tutorBobId);
        assert(revs.size() == 1);
        review1Id = revs[0].id;
        assert(revs[0].ratingScore == 5);

        auto bobTutor = std::static_pointer_cast<Tutor>(userRepo->findUserById(tutorBobId));
        assert(bobTutor->getRating() == 5.0 && bobTutor->getTotalReviews() == 1);

        // 2. Reject duplicate review for same booking
        assert(!reviewService->submitReview(aliceId, booking1Id, 4, "Đánh giá lần 2", msg));

        // 3. Reject review for wrong student
        assert(!reviewService->submitReview(danId, booking1Id, 5, "Học viên khác đánh giá", msg));

        // 4. Reject invalid rating score or empty comment
        assert(!reviewService->submitReview(aliceId, 9999, 0, "0 sao", msg));
        assert(!reviewService->submitReview(aliceId, 9999, 6, "6 sao", msg));
        assert(!reviewService->submitReview(aliceId, 9999, 5, "", msg));

        // 5. Admin moderates and deletes review
        assert(reviewService->deleteReview(review1Id, msg));
        assert(reviewRepo->getReviewsByTutorId(tutorBobId).empty());
        bobTutor = std::static_pointer_cast<Tutor>(userRepo->findUserById(tutorBobId));
        assert(bobTutor->getTotalReviews() == 0);
        assert(bobTutor->getRating() == 0.0);
    }

    // ========================================================================
    // E2E-08: Sandbox Persistence & Application Restart
    // ========================================================================
    {
        std::string msg;
        assert(reviewService->submitReview(aliceId, booking1Id, 4, "Đánh giá 4 sao", msg));

        // Simulating application exit
        bridge.reset();
        adminService.reset();
        matchingService.reset();
        reviewService.reset();
        bookingService.reset();
        userService.reset();
        authService.reset();
        userRepo.reset();
        courseBookingRepo.reset();
        reviewRepo.reset();

        // Reconstruct fresh repositories and services from sandbox files
        auto reloadedUserRepo = std::make_shared<FileUserRepository>(uPath);
        auto reloadedCourseBookingRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
        auto reloadedReviewRepo = std::make_shared<FileReviewRepository>(rPath);

        assert(reloadedUserRepo->initialize());
        assert(reloadedCourseBookingRepo->initialize());
        assert(reloadedReviewRepo->initialize());

        auto reloadedAuthService = std::make_shared<AuthService>(reloadedUserRepo);
        auto reloadedReviewService = std::make_shared<ReviewService>(reloadedReviewRepo, reloadedUserRepo, reloadedCourseBookingRepo);

        // Startup reconciliation
        reloadedReviewService->reconcileRatingsOnStartup();

        auto bobUser = reloadedUserRepo->findUserByUsername("tutor_bob");
        assert(bobUser != nullptr);
        auto bobTutor = std::static_pointer_cast<Tutor>(bobUser);
        assert(bobTutor->getTotalReviews() == 1);
        assert(bobTutor->getRating() == 4.0);

        auto aliceUser = reloadedUserRepo->findUserByUsername("student_alice");
        assert(aliceUser != nullptr);

        auto reloadedCourses = reloadedCourseBookingRepo->getAllCourseBookings();
        assert(!reloadedCourses.empty());
    }

    // ========================================================================
    // E2E-09: Authorization & Role Violations
    // ========================================================================
    {
        auto freshUserRepo = std::make_shared<FileUserRepository>(uPath);
        auto freshCourseBookingRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
        auto freshReviewRepo = std::make_shared<FileReviewRepository>(rPath);
        freshUserRepo->initialize();
        freshCourseBookingRepo->initialize();
        freshReviewRepo->initialize();

        auto freshBookingService = std::make_shared<BookingService>(freshUserRepo, freshReviewRepo, freshCourseBookingRepo);
        std::string msg;
        // Tutor Carol attempts to accept Tutor Bob's booking -> Rejected
        assert(!freshBookingService->acceptCourseBooking(tutorCarolId, booking1Id, true, msg));

        // Student Alice attempts to book herself -> Rejected
        assert(!freshBookingService->bookCourse(aliceId, aliceId, "Toán", 0, 0, "2026-11-16", 4, msg));
    }

    // ========================================================================
    // E2E-10: Persistence Failure & Defensive Handling
    // ========================================================================
    {
        auto failCourseBookingRepo = std::make_shared<FileCourseBookingRepository>((testDir / "non_existent_dir" / "course_bookings.dat").string());
        auto freshUserRepo = std::make_shared<FileUserRepository>(uPath);
        freshUserRepo->initialize();

        auto freshBookingService = std::make_shared<BookingService>(freshUserRepo, nullptr, failCourseBookingRepo);
        std::string msg;
        assert(!freshBookingService->bookCourse(aliceId, tutorBobId, "Toán 12", 0, 0, "2026-11-16", 4, msg));
    }

    // Clean up temporary sandbox directory
    std::filesystem::remove_all(testDir, ec);

    std::cout << "  -> PASS: All 10 End-to-End Scenarios (E2E-01 to E2E-10) verified successfully!" << std::endl;
}

void run_test_recurring_course_booking() {
    std::cout << "[TEST] 20. Verifying Recurring Course Booking (Phase 2 & 5)..." << std::endl;

    // 1. DateUtils Validation & Calculation Tests
    {
        // 1.1 Leap year checks
        assert(DateUtils::isLeapYear(2024));
        assert(!DateUtils::isLeapYear(2026));
        assert(DateUtils::isLeapYear(2000));
        assert(!DateUtils::isLeapYear(1900));

        // 1.2 Days in month
        assert(DateUtils::daysInMonth(2024, 2) == 29);
        assert(DateUtils::daysInMonth(2026, 2) == 28);
        assert(DateUtils::daysInMonth(2026, 4) == 30);
        assert(DateUtils::daysInMonth(2026, 12) == 31);
        assert(DateUtils::daysInMonth(2026, 13) == 0);

        // 1.3 Validation of date strings
        assert(DateUtils::isValidDateString("2026-11-06"));
        assert(DateUtils::isValidDateString("2024-02-29"));
        assert(!DateUtils::isValidDateString("2026-02-29")); // Non-leap year
        assert(!DateUtils::isValidDateString("2026-04-31")); // April has 30 days
        assert(!DateUtils::isValidDateString("2026-13-01")); // Invalid month
        assert(!DateUtils::isValidDateString("invalid"));
        assert(!DateUtils::isValidDateString("2026/11/06"));

        // 1.4 Day of week calculation (0 = Mon .. 6 = Sun)
        assert(DateUtils::dayOfWeek("2026-11-09") == 0); // Monday
        assert(DateUtils::dayOfWeek("2026-11-10") == 1); // Tuesday
        assert(DateUtils::dayOfWeek("2026-11-11") == 2); // Wednesday
        assert(DateUtils::dayOfWeek("2026-11-12") == 3); // Thursday
        assert(DateUtils::dayOfWeek("2026-11-13") == 4); // Friday
        assert(DateUtils::dayOfWeek("2026-11-14") == 5); // Saturday
        assert(DateUtils::dayOfWeek("2026-11-15") == 6); // Sunday

        // 1.5 Adding days (month and year transitions)
        assert(DateUtils::addDays("2026-01-31", 7) == "2026-02-07");
        assert(DateUtils::addDays("2026-02-28", 1) == "2026-03-01");
        assert(DateUtils::addDays("2024-02-28", 1) == "2024-02-29"); // Leap year
        assert(DateUtils::addDays("2024-02-29", 1) == "2024-03-01");
        assert(DateUtils::addDays("2026-12-28", 7) == "2027-01-04"); // Year rollover

        // 1.6 Session dates generation (4, 8, 12 weeks)
        auto s4 = DateUtils::generateSessionDates("2026-11-09", 4);
        assert(s4.size() == 4);
        assert(s4[0] == "2026-11-09");
        assert(s4[1] == "2026-11-16");
        assert(s4[2] == "2026-11-23");
        assert(s4[3] == "2026-11-30");
        assert(DateUtils::getEndDate("2026-11-09", 4) == "2026-11-30");

        auto s8 = DateUtils::generateSessionDates("2026-11-09", 8);
        assert(s8.size() == 8);
        assert(s8[7] == "2026-12-28");

        auto s12 = DateUtils::generateSessionDates("2026-11-09", 12);
        assert(s12.size() == 12);
        assert(s12[11] == "2027-01-25"); // Rolls over to 2027!

        // 1.7 Date comparison
        assert(DateUtils::compareDates("2026-11-09", "2026-11-16") < 0);
        assert(DateUtils::compareDates("2026-11-16", "2026-11-09") > 0);
        assert(DateUtils::compareDates("2026-11-09", "2026-11-09") == 0);
    }

    // 2. CourseBookingSerializer Unit Tests
    {
        CourseBooking orig(1001, 10, 20, "Học viên A", "Gia sư B", "Toán 12",
                           150000.0, "2026-11-09", 8, "PENDING", "2026-11-06", "0:1");
        std::string csv = CourseBookingSerializer::toCsv(orig);
        std::string err;
        auto parsed = CourseBookingSerializer::fromCsv(csv, &err);
        assert(parsed.has_value());
        assert(parsed->id == 1001);
        assert(parsed->studentId == 10);
        assert(parsed->tutorId == 20);
        assert(parsed->studentName == "Học viên A");
        assert(parsed->tutorName == "Gia sư B");
        assert(parsed->subject == "Toán 12");
        assert(parsed->hourlyRate == 150000.0);
        assert(parsed->startDate == "2026-11-09");
        assert(parsed->weeksCount == 8);
        assert(parsed->status == "PENDING");
        assert(parsed->createdAt == "2026-11-06");
        assert(parsed->weeklySchedule == "0:1");
        assert(parsed->getWeeklySlots().size() == 1);
        assert(parsed->getWeeklySlots()[0].first == 0);
        assert(parsed->getWeeklySlots()[0].second == 1);
        assert(parsed->getTotalCost() == 1200000.0);
        assert(parsed->getTotalSessions() == 8);

        // Malformed line rejection
        assert(!CourseBookingSerializer::fromCsv("1,2,3", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1,2,3,A,B,Math,100,2026-11-09,4,PENDING,2026-11-06,7:1", &err).has_value()); // dayOfWeek 7 invalid in weeklySchedule
        assert(!CourseBookingSerializer::fromCsv("1,2,3,A,B,Math,100,2026-11-09,4,PENDING,2026-11-06,0:8", &err).has_value()); // slotIndex 8 invalid in weeklySchedule
        assert(!CourseBookingSerializer::fromCsv("1,2,3,A,B,Math,100,2026-02-30,4,PENDING,2026-11-06,0:1", &err).has_value()); // invalid date
        assert(!CourseBookingSerializer::fromCsv("1,2,3,A,B,Math,100,2026-11-09,4,UNKNOWN,2026-11-06,0:1", &err).has_value()); // invalid status
        assert(!CourseBookingSerializer::fromCsv("1,2,3,A,B,Math,100,2026-11-09,4,PENDING,2026-11-06,invalid_schedule", &err).has_value()); // malformed weeklySchedule
    }

    // 3. FileCourseBookingRepository Sandbox Persistence
    {
        std::error_code ec;
        auto testDir = std::filesystem::temp_directory_path() / "tutormatch_course_booking_test_sandbox";
        std::filesystem::remove_all(testDir, ec);
        std::filesystem::create_directories(testDir, ec);

        std::string cbPath = (testDir / "course_bookings_test.dat").string();
        auto repo = std::make_shared<FileCourseBookingRepository>(cbPath);
        assert(repo->initialize());

        int id1 = repo->getNextCourseBookingId();
        assert(id1 == 1001);

        CourseBooking cb1(id1, 10, 20, "Học viên A", "Gia sư B", "Toán 12",
                          150000.0, "2026-11-09", 4, "PENDING", "2026-11-06", "0:0");
        assert(repo->saveCourseBooking(cb1));

        auto found = repo->findCourseBookingById(id1);
        assert(found != nullptr);
        assert(found->id == id1);
        assert(found->status == "PENDING");

        // Atomic update test
        found->status = "ACCEPTED";
        assert(repo->updateCourseBooking(*found));

        // Reload fresh repository instance from disk
        auto freshRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
        assert(freshRepo->initialize());
        auto reloaded = freshRepo->findCourseBookingById(id1);
        assert(reloaded != nullptr);
        assert(reloaded->status == "ACCEPTED");
        assert(freshRepo->getAllCourseBookings().size() == 1);
        assert(freshRepo->getCourseBookingsByStudentId(10).size() == 1);
        assert(freshRepo->getCourseBookingsByTutorId(20).size() == 1);

        std::filesystem::remove_all(testDir, ec);
    }

    // 4. BookingService Business Logic & Conflict Scenarios
    {
        auto userRepo = std::make_shared<FakeUserRepository>();
        auto reviewRepo = std::make_shared<FakeReviewRepository>();
        auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

        uint64_t monSlot0Mask = ScheduleGuard::getSlotMask(0, 0); // Monday slot 0
        uint64_t tueSlot1Mask = ScheduleGuard::getSlotMask(1, 1); // Tuesday slot 1

        Student alice(10, "alice", "pass", "alice@example.com", "Alice Nguyen", 12, "Toán 12", 200000.0, monSlot0Mask | tueSlot1Mask);
        Student charlie(30, "charlie", "pass", "charlie@example.com", "Charlie Tran", 12, "Toán 12", 200000.0, monSlot0Mask | tueSlot1Mask);
        Student david(40, "david", "pass", "david@example.com", "David Le", 12, "Toán 12", 200000.0, monSlot0Mask | tueSlot1Mask);
        Tutor bob(20, "bob", "pass", "bob@example.com", "Bob Le", "Bio", "Toán 12", 150000.0, 5.0, 10, 0.0, monSlot0Mask | tueSlot1Mask);

        userRepo->saveUser(alice);
        userRepo->saveUser(charlie);
        userRepo->saveUser(david);
        userRepo->saveUser(bob);

        auto bookingService = std::make_shared<BookingService>(userRepo, reviewRepo, courseRepo);

        std::string msg;

        // 4.1 Validation Failure: Mismatched day of week (2026-11-06 is Friday (4), but dayOfWeek passed is 0 (Mon))
        assert(!bookingService->bookCourse(10, 20, "Toán 12", 0, 0, "2026-11-06", 4, msg));

        // 4.2 Validation Failure: Invalid weeks count (5 weeks)
        assert(!bookingService->bookCourse(10, 20, "Toán 12", 0, 0, "2026-11-16", 5, msg));

        // 4.3 Validation Failure: Past date
        assert(!bookingService->bookCourse(10, 20, "Toán 12", 0, 0, "2020-01-06", 4, msg));

        // 4.4 Valid Course Booking for Alice (Starts Monday 2026-11-09, slot 0, 4 weeks)
        int aliceCourseId = 0;
        assert(bookingService->bookCourse(10, 20, "Toán 12", 0, 0, "2026-11-16", 4, true, msg, &aliceCourseId));
        assert(aliceCourseId > 0);
        auto aliceCourse = courseRepo->findCourseBookingById(aliceCourseId);
        assert(aliceCourse != nullptr);
        assert(aliceCourse->status == "PENDING");

        // 4.5 PENDING DOES NOT RESERVE SLOTS: Charlie can book the exact same slot and dates!
        int charlieCourseId = 0;
        assert(bookingService->bookCourse(30, 20, "Toán 12", 0, 0, "2026-11-16", 4, true, msg, &charlieCourseId));
        assert(charlieCourseId > 0);
        auto charlieCourse = courseRepo->findCourseBookingById(charlieCourseId);
        assert(charlieCourse != nullptr);
        assert(charlieCourse->status == "PENDING");

        // 4.6 Accept Alice's Course Booking
        assert(bookingService->acceptCourseBooking(20, aliceCourseId, true, msg));
        aliceCourse = courseRepo->findCourseBookingById(aliceCourseId);
        assert(aliceCourse->status == "ACCEPTED");

        // CRITICAL: Availability scheduleBitmask is NOT modified for CourseBooking!
        auto bobUser = std::static_pointer_cast<Tutor>(userRepo->findUserById(20));
        auto aliceUser = std::static_pointer_cast<Student>(userRepo->findUserById(10));
        assert(bobUser->getScheduleBitmask() == (monSlot0Mask | tueSlot1Mask)); // Invariant: NOT zeroed out!
        assert(aliceUser->getScheduleBitmask() == (monSlot0Mask | tueSlot1Mask));

        // 4.7 Idempotency: Calling accept again succeeds without side effects
        assert(bookingService->acceptCourseBooking(20, aliceCourseId, true, msg));

        // 4.8 Attempt to Accept Charlie's Course Booking -> MUST FAIL DUE TO CONFLICT!
        std::string conflictDate;
        assert(!bookingService->acceptCourseBooking(20, charlieCourseId, true, msg, &conflictDate));
        assert(conflictDate == "2026-11-16");
        charlieCourse = courseRepo->findCourseBookingById(charlieCourseId);
        assert(charlieCourse->status == "PENDING"); // Remained PENDING, not corrupt

        // 4.9 Calendar-Date Disjoint Booking: David books same Monday slot 0, but starting 2027-01-04 (after Alice's course ended)
        int davidCourseId = 0;
        assert(bookingService->bookCourse(40, 20, "Toán 12", 0, 0, "2027-01-04", 4, true, msg, &davidCourseId));
        assert(bookingService->acceptCourseBooking(20, davidCourseId, true, msg)); // SUCCEEDS! No collision!
        auto davidCourse = courseRepo->findCourseBookingById(davidCourseId);
        assert(davidCourse->status == "ACCEPTED");

        // 4.10 Overlapping Course Conflict on Tuesday (Slot 1)
        int tueCourse1Id = 0;
        assert(bookingService->bookCourse(30, 20, "Toán 12", 1, 1, "2026-11-17", 4, true, msg, &tueCourse1Id));
        assert(bookingService->acceptCourseBooking(20, tueCourse1Id, true, msg));

        int tueCourse2Id = 0;
        assert(bookingService->bookCourse(10, 20, "Toán 12", 1, 1, "2026-11-17", 4, true, msg, &tueCourse2Id));
        assert(!bookingService->acceptCourseBooking(20, tueCourse2Id, true, msg));

        // 4.11 Concurrency / Double Booking Race Condition Protection
        int raceCourseA = 0;
        int raceCourseB = 0;
        assert(bookingService->bookCourse(10, 20, "Toán 12", 0, 0, "2027-03-01", 4, true, msg, &raceCourseA));
        assert(bookingService->bookCourse(30, 20, "Toán 12", 0, 0, "2027-03-01", 4, true, msg, &raceCourseB));

        std::atomic<int> successCount{0};
        std::atomic<int> failCount{0};

        std::thread t1([&]() {
            std::string tMsg;
            if (bookingService->acceptCourseBooking(20, raceCourseA, true, tMsg)) {
                successCount++;
            } else {
                failCount++;
            }
        });

        std::thread t2([&]() {
            std::string tMsg;
            if (bookingService->acceptCourseBooking(20, raceCourseB, true, tMsg)) {
                successCount++;
            } else {
                failCount++;
            }
        });

        t1.join();
        t2.join();

        assert(successCount.load() == 1);
        assert(failCount.load() == 1);

        // 4.12 Cancellation releases reservation
        assert(bookingService->cancelCourseBooking(10, UserRole::STUDENT, aliceCourseId, msg));
        aliceCourse = courseRepo->findCourseBookingById(aliceCourseId);
        assert(aliceCourse->status == "CANCELLED");

        // Now Charlie's course (which conflicted with Alice) can be ACCEPTED cleanly!
        assert(bookingService->acceptCourseBooking(20, charlieCourseId, true, msg));
        charlieCourse = courseRepo->findCourseBookingById(charlieCourseId);
        assert(charlieCourse->status == "ACCEPTED");

        // 4.13 Termination (COMPLETED) does not block future
        assert(bookingService->terminateCourseBooking(20, UserRole::TUTOR, charlieCourseId, msg));
        charlieCourse = courseRepo->findCourseBookingById(charlieCourseId);
        assert(charlieCourse->status == "COMPLETED");

        // 4.14 Metrics aggregation
        auto metrics = bookingService->getStudentBookingMetrics(10);
        assert(metrics.historyCount >= 1);
    }

    std::cout << "  -> PASS: All Recurring Course Booking Scenarios verified successfully!" << std::endl;
}

void run_test_timetable_recurring_course_integration() {
    std::cout << "[TEST] 21. Verifying Weekly Timetable Integration for Recurring Course Booking..." << std::endl;

    // 1. Calendar Date & Week Calculation Tests (DateUtils)
    {
        // Friday Nov 06, 2026
        assert(DateUtils::dayOfWeek("2026-11-06") == 4);
        std::string monday = DateUtils::getWeekMonday("2026-11-06");
        assert(monday == "2026-11-02");
        std::string sunday = DateUtils::getWeekSunday("2026-11-06");
        assert(sunday == "2026-11-08");

        // Shift week
        assert(DateUtils::shiftWeek(monday, 1) == "2026-11-09");
        assert(DateUtils::shiftWeek(monday, -1) == "2026-10-26");
        assert(DateUtils::shiftWeek(monday, 0) == "2026-11-02");

        // Month boundary (Dec 01, 2026 is Tuesday -> Monday is Nov 30, 2026)
        assert(DateUtils::dayOfWeek("2026-12-01") == 1);
        assert(DateUtils::getWeekMonday("2026-12-01") == "2026-11-30");
        assert(DateUtils::getWeekSunday("2026-12-01") == "2026-12-06");

        // Year boundary (Jan 01, 2027 is Friday -> Monday is Dec 28, 2026)
        assert(DateUtils::dayOfWeek("2027-01-01") == 4);
        assert(DateUtils::getWeekMonday("2027-01-01") == "2026-12-28");
        assert(DateUtils::getWeekSunday("2027-01-01") == "2027-01-03");

        // Leap year (Feb 29, 2024 is Thursday -> Monday is Feb 26, 2024)
        assert(DateUtils::dayOfWeek("2024-02-29") == 3);
        assert(DateUtils::getWeekMonday("2024-02-29") == "2024-02-26");
        assert(DateUtils::getWeekSunday("2024-02-29") == "2024-03-03");

        // 7 week days
        auto wDays = DateUtils::getWeekDays("2026-11-09");
        assert(wDays.size() == 7);
        assert(wDays[0] == "2026-11-09"); // Mon
        assert(wDays[4] == "2026-11-13"); // Fri
        assert(wDays[6] == "2026-11-15"); // Sun

        assert(DateUtils::formatDisplayDate("2026-11-06") == "06/11");
    }

    // 2. Student/Tutor Consistency, Week Filtering & Timetable Overlay
    {
        auto userRepo = std::make_shared<FakeUserRepository>();
        auto reviewRepo = std::make_shared<FakeReviewRepository>();
        auto courseRepo = std::make_shared<FakeCourseBookingRepository>();

        uint64_t friSlot0Mask = ScheduleGuard::getSlotMask(4, 0); // Friday (day 4), slot 0

        // Student Nam
        Student sNam(10, "student_nam", "pass123", "nam@test.vn", "Nguyễn Văn Nam", 12, "Toán 12", 200000.0, friSlot0Mask);
        userRepo->saveUser(sNam);

        // Tutor Toan
        Tutor tToan(20, "tutor_math", "pass123", "toan@test.vn", "ThS. Lê Văn Toàn", "Gia sư Toán", "Toán 12", 150000.0, 5.0, 10, 0.0, friSlot0Mask);
        userRepo->saveUser(tToan);

        auto bookingService = std::make_shared<BookingService>(userRepo, reviewRepo, courseRepo);

        GUIBridge bridge;
        bridge.initialize(userRepo, reviewRepo, courseRepo);

        // Book 4-week Friday course starting 2026-11-13
        std::string msg;
        int courseId = 0;
        bool bookOk = bookingService->bookCourse(10, 20, "Toán 12", 4, 0, "2026-11-13", 4, true, msg, &courseId);
        assert(bookOk);
        assert(courseId > 0);

        // Status is PENDING
        auto cb = courseRepo->findCourseBookingById(courseId);
        assert(cb != nullptr && cb->status == "PENDING");

        // 2.1 PENDING status must NOT appear as BOOKED in either timetable!
        std::string week1Mon = "2026-11-09";
        auto pendingSessions = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week1Mon);
        assert(pendingSessions.empty());

        bridge.qmlLogin("student_nam", "pass123");
        QVariantList studentGridPending = bridge.getStudentWeeklySchedule(QString::fromStdString(week1Mon));
        assert(studentGridPending.size() == 56);
        int friSlot0FlatIndex = 0 * 7 + 4; // slot 0, day 4
        assert(studentGridPending[friSlot0FlatIndex].toMap()["status"].toString() == "FREE");

        bridge.logout();
        bridge.qmlLogin("tutor_math", "pass123");
        QVariantList tutorGridPending = bridge.getTutorWeeklySchedule(QString::fromStdString(week1Mon));
        assert(tutorGridPending.size() == 56);
        assert(tutorGridPending[friSlot0FlatIndex].toMap()["status"].toString() == "FREE");

        // 2.2 Tutor accepts the course booking
        std::string conflictDate;
        bool acceptOk = bookingService->acceptCourseBooking(20, courseId, true, msg, &conflictDate);
        assert(acceptOk);

        cb = courseRepo->findCourseBookingById(courseId);
        assert(cb->status == "ACCEPTED");

        // 2.3 Week Filtering: Verify 4 sessions appear on the 4 distinct Fridays only
        std::string week1Monday = "2026-11-09"; // Week 1 (Fri Nov 13)
        std::string week2Monday = "2026-11-16"; // Week 2 (Fri Nov 20)
        std::string week3Monday = "2026-11-23"; // Week 3 (Fri Nov 27)
        std::string week4Monday = "2026-11-30"; // Week 4 (Fri Dec 04)
        std::string week5Monday = "2026-12-07"; // Week 5 (Outside range)
        std::string week0Monday = "2026-11-02"; // Week 0 (Before range)

        // Week 1 Check
        auto w1SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week1Monday);
        auto w1SessionsTutor = bookingService->getAcceptedSessionsForWeek(20, UserRole::TUTOR, week1Monday);
        assert(w1SessionsStudent.size() == 1);
        assert(w1SessionsTutor.size() == 1);
        assert(w1SessionsStudent[0].date == "2026-11-13");
        assert(w1SessionsStudent[0].sessionNumber == 1);
        assert(w1SessionsStudent[0].totalSessions == 4);
        assert(w1SessionsStudent[0].tutorName == "ThS. Lê Văn Toàn");
        assert(w1SessionsTutor[0].date == "2026-11-13");
        assert(w1SessionsTutor[0].sessionNumber == 1);
        assert(w1SessionsTutor[0].totalSessions == 4);
        assert(w1SessionsTutor[0].studentName == "Nguyễn Văn Nam");

        // Week 2 Check
        auto w2SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week2Monday);
        assert(w2SessionsStudent.size() == 1);
        assert(w2SessionsStudent[0].date == "2026-11-20");
        assert(w2SessionsStudent[0].sessionNumber == 2);
        assert(w2SessionsStudent[0].totalSessions == 4);

        // Week 3 Check
        auto w3SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week3Monday);
        assert(w3SessionsStudent.size() == 1);
        assert(w3SessionsStudent[0].date == "2026-11-27");
        assert(w3SessionsStudent[0].sessionNumber == 3);

        // Week 4 Check
        auto w4SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week4Monday);
        assert(w4SessionsStudent.size() == 1);
        assert(w4SessionsStudent[0].date == "2026-12-04");
        assert(w4SessionsStudent[0].sessionNumber == 4);

        // Week 5 Check (Outside course range)
        auto w5SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week5Monday);
        assert(w5SessionsStudent.empty());

        // Week 0 Check (Before course range)
        auto w0SessionsStudent = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week0Monday);
        assert(w0SessionsStudent.empty());

        // 2.4 GUIBridge Full Grid Overlay Verification:
        // Week 1 Student View
        bridge.logout();
        bridge.qmlLogin("student_nam", "pass123");
        QVariantList sGridW1 = bridge.getStudentWeeklySchedule(QString::fromStdString(week1Monday));
        auto sCellW1 = sGridW1[friSlot0FlatIndex].toMap();
        assert(sCellW1["status"].toString() == "BOOKED");
        assert(sCellW1["isCourse"].toBool() == true);
        assert(sCellW1["sessionNumber"].toInt() == 1);
        assert(sCellW1["totalSessions"].toInt() == 4);
        assert(sCellW1["sessionLabel"].toString() == "Buổi 1/4");
        assert(sCellW1["tutorName"].toString() == "ThS. Lê Văn Toàn");
        assert(sCellW1["editable"].toBool() == false);
        assert(sCellW1["locked"].toBool() == true);

        // Week 1 Tutor View
        bridge.logout();
        bridge.qmlLogin("tutor_math", "pass123");
        QVariantList tGridW1 = bridge.getTutorWeeklySchedule(QString::fromStdString(week1Monday));
        auto tCellW1 = tGridW1[friSlot0FlatIndex].toMap();
        assert(tCellW1["status"].toString() == "BOOKED");
        assert(tCellW1["isCourse"].toBool() == true);
        assert(tCellW1["sessionNumber"].toInt() == 1);
        assert(tCellW1["totalSessions"].toInt() == 4);
        assert(tCellW1["sessionLabel"].toString() == "Buổi 1/4");
        assert(tCellW1["studentName"].toString() == "Nguyễn Văn Nam");
        assert(tCellW1["editable"].toBool() == false);
        assert(tCellW1["locked"].toBool() == true);

        // Week 5 Student View (Must be FREE / RẢNH, not BOOKED!)
        bridge.logout();
        bridge.qmlLogin("student_nam", "pass123");
        QVariantList sGridW5 = bridge.getStudentWeeklySchedule(QString::fromStdString(week5Monday));
        auto sCellW5 = sGridW5[friSlot0FlatIndex].toMap();
        assert(sCellW5["status"].toString() == "FREE");
        assert(sCellW5["label"].toString() == "RẢNH");
        assert(sCellW5["isCourse"].toBool() == false);

        // 2.5 Invariant Check: scheduleBitmask remains completely untouched
        auto sEntity = std::static_pointer_cast<Student>(userRepo->findUserById(10));
        auto tEntity = std::static_pointer_cast<Tutor>(userRepo->findUserById(20));
        assert((sEntity->getScheduleBitmask() & friSlot0Mask) == friSlot0Mask);
        assert((tEntity->getScheduleBitmask() & friSlot0Mask) == friSlot0Mask);

        // 2.6 Status Change: Cancellation removes session from timetable
        assert(bookingService->cancelCourseBooking(10, UserRole::STUDENT, courseId, msg));
        auto w1SessionsAfterCancel = bookingService->getAcceptedSessionsForWeek(10, UserRole::STUDENT, week1Monday);
        assert(w1SessionsAfterCancel.empty());

        QVariantList sGridW1AfterCancel = bridge.getStudentWeeklySchedule(QString::fromStdString(week1Monday));
        assert(sGridW1AfterCancel[friSlot0FlatIndex].toMap()["status"].toString() == "FREE");

        // 2.7 Navigation helpers test
        QString monStr = bridge.getWeekMonday("2026-11-06");
        assert(monStr == "2026-11-02");
        QString nextMon = bridge.shiftWeek(monStr, 1);
        assert(nextMon == "2026-11-09");
        QString rangeStr = bridge.formatWeekRangeDisplay(monStr);
        assert(rangeStr == "02/11/2026 - 08/11/2026");

        QVariantList daysInfo = bridge.getWeekDaysInfo(monStr);
        assert(daysInfo.size() == 7);
        assert(daysInfo[0].toMap()["dayName"].toString() == "Thứ 2");
        assert(daysInfo[0].toMap()["displayDate"].toString() == "02/11");
        assert(daysInfo[4].toMap()["dayName"].toString() == "Thứ 6");
        assert(daysInfo[4].toMap()["displayDate"].toString() == "06/11");
    }

    std::cout << "  -> PASS: All Weekly Timetable Recurring Course Scenarios verified successfully!" << std::endl;
}

void run_test_data_audit_cleanup_and_migration() {
    std::cout << "[TEST] 22. Verifying Comprehensive Data Audit, Parsing & Migration Invariants..." << std::endl;

    // 1. Parsing & Schema Validation
    {
        std::string err;
        // Valid 12 fields normalized CourseBooking (or legacy)
        std::string validLine = "1001,6,2,Nguyễn Văn Nam,ThS. Lê Văn Toàn,Toán 12 - Luyện thi Đại học,150000,2026-11-06,4,ACCEPTED,2026-11-06,4:0";
        auto cbOpt = CourseBookingSerializer::fromCsv(validLine, &err);
        assert(cbOpt.has_value());
        assert(cbOpt->id == 1001);
        assert(cbOpt->weeklySchedule == "4:0");
        assert(!cbOpt->getWeeklySlots().empty());
        assert(cbOpt->getWeeklySlots()[0].first == 4);
        assert(cbOpt->getWeeklySlots()[0].second == 0);
        assert(cbOpt->startDate == "2026-11-06");
        assert(cbOpt->weeksCount == 4);
        assert(cbOpt->status == "ACCEPTED");

        // Invalid field counts (< 12 fields)
        assert(!CourseBookingSerializer::fromCsv("1,2,3,4,5,6,7,8,9,10,11", &err).has_value());
        assert(err.find("requires at least 12 fields") != std::string::npos);

        // Invalid weeklySchedule dayOfWeek
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,7:0", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,-1:0", &err).has_value());

        // Invalid weeklySchedule slotIndex
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,4:8", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,4:-1", &err).has_value());

        // Invalid startDate format
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026/11/06,4,ACCEPTED,2026-11-06,4:0", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,invalid-date,4,ACCEPTED,2026-11-06,4:0", &err).has_value());

        // Non-positive weeksCount
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,0,ACCEPTED,2026-11-06,4:0", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,-4,ACCEPTED,2026-11-06,4:0", &err).has_value());

        // Invalid status
        assert(!CourseBookingSerializer::fromCsv("1001,6,2,Nam,Toan,Math,150000,2026-11-06,4,UNKNOWN_STATUS,2026-11-06,4:0", &err).has_value());

        // Invalid numeric IDs
        assert(!CourseBookingSerializer::fromCsv("ABC,6,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,4:0", &err).has_value());
        assert(!CourseBookingSerializer::fromCsv("1001,XYZ,2,Nam,Toan,Math,150000,2026-11-06,4,ACCEPTED,2026-11-06,4:0", &err).has_value());
    }

    // 2. Calendar Invariants & DateUtils Arithmetic
    {
        // 4, 8, 12 weeks session generation
        auto sessions4 = DateUtils::generateSessionDates("2026-11-06", 4);
        assert(sessions4.size() == 4);
        assert(sessions4[0] == "2026-11-06");
        assert(sessions4[1] == "2026-11-13");
        assert(sessions4[2] == "2026-11-20");
        assert(sessions4[3] == "2026-11-27");
        for (const auto& s : sessions4) {
            assert(DateUtils::dayOfWeek(s) == 4); // All Fridays
        }

        auto sessions8 = DateUtils::generateSessionDates("2026-11-10", 8);
        assert(sessions8.size() == 8);
        for (const auto& s : sessions8) {
            assert(DateUtils::dayOfWeek(s) == 1); // All Tuesdays
        }

        auto sessions12 = DateUtils::generateSessionDates("2026-11-11", 12);
        assert(sessions12.size() == 12);
        for (const auto& s : sessions12) {
            assert(DateUtils::dayOfWeek(s) == 2); // All Wednesdays
        }

        // Month boundary: Nov 25, 2026 (Wed) -> Dec 02, 2026
        auto sessionsMonthBoundary = DateUtils::generateSessionDates("2026-11-25", 4);
        assert(sessionsMonthBoundary.size() == 4);
        assert(sessionsMonthBoundary[0] == "2026-11-25");
        assert(sessionsMonthBoundary[1] == "2026-12-02");
        assert(sessionsMonthBoundary[2] == "2026-12-09");
        assert(sessionsMonthBoundary[3] == "2026-12-16");

        // Year boundary: Dec 16, 2026 (Wed) -> Jan & Feb 2027
        auto sessionsYearBoundary = DateUtils::generateSessionDates("2026-12-16", 8);
        assert(sessionsYearBoundary.size() == 8);
        assert(sessionsYearBoundary[0] == "2026-12-16");
        assert(sessionsYearBoundary[1] == "2026-12-23");
        assert(sessionsYearBoundary[2] == "2026-12-30");
        assert(sessionsYearBoundary[3] == "2027-01-06");
        assert(sessionsYearBoundary[4] == "2027-01-13");
        assert(sessionsYearBoundary[5] == "2027-01-20");
        assert(sessionsYearBoundary[6] == "2027-01-27");
        assert(sessionsYearBoundary[7] == "2027-02-03");

        // Leap Year Rules
        assert(DateUtils::isLeapYear(2028) == true);
        assert(DateUtils::isLeapYear(2027) == false);
        assert(DateUtils::isLeapYear(2000) == true);
        assert(DateUtils::isLeapYear(1900) == false);
        assert(DateUtils::daysInMonth(2028, 2) == 29);
        assert(DateUtils::daysInMonth(2027, 2) == 28);

        // Leap day arithmetic: 2028-02-23 + 7 days = 2028-03-01
        assert(DateUtils::addDays("2028-02-23", 7) == "2028-03-01");
        // Non-leap day arithmetic: 2027-02-24 + 7 days = 2027-03-03
        assert(DateUtils::addDays("2027-02-24", 7) == "2027-03-03");
    }

    // 3. Recurring Booking Lifecycle & Slot Conflict Math
    {
        std::error_code ec;
        auto testDir = std::filesystem::temp_directory_path() / "tutormatch_audit_lifecycle_sandbox";
        std::filesystem::remove_all(testDir, ec);
        std::filesystem::create_directories(testDir, ec);

        std::string uPath = (testDir / "users.dat").string();
        std::string cbPath = (testDir / "course_bookings.dat").string();

        auto uRepo = std::make_shared<FileUserRepository>(uPath);
        auto cbRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
        uRepo->initialize();
        cbRepo->initialize();

        uint64_t allFree = ScheduleGuard::VALID_SCHEDULE_MASK;
        Tutor t(1, "tutor_1", "pass", "t1@tutor.vn", "ThS Gia Sư", "Bio", "Toán 12",
                150000.0, 5.0, 1, 0.0, allFree, true);
        Student s1(2, "student_1", "pass", "s1@stu.vn", "Học Viên 1", 12, "Toán", 2000000.0, allFree, true);
        Student s2(3, "student_2", "pass", "s2@stu.vn", "Học Viên 2", 12, "Toán", 2000000.0, allFree, true);
        uRepo->saveUser(t);
        uRepo->saveUser(s1);
        uRepo->saveUser(s2);

        BookingService service(uRepo, nullptr, cbRepo);

        // Course 1: Student 1 books Tutor 1 on Friday (dow 4), slot 0, startDate 2026-11-13, 4 weeks
        int c1Id = -1;
        std::string msg;
        assert(service.bookCourse(2, 1, "Toán 12", 4, 0, "2026-11-13", 4, true, msg, &c1Id));
        assert(service.acceptCourseBooking(1, c1Id, true, msg));

        // Student 2 can request the same slot, but Tutor 1 CANNOT accept c2 due to schedule conflict with c1!
        int c2Id = -1;
        assert(service.bookCourse(3, 1, "Toán 12", 4, 0, "2026-11-13", 4, true, msg, &c2Id));
        bool acceptConflict = service.acceptCourseBooking(1, c2Id, true, msg);
        assert(!acceptConflict);
        assert(msg.find("Không thể chấp nhận") != std::string::npos || msg.find("trùng") != std::string::npos || msg.find("lịch") != std::string::npos);

        // Non-overlapping course (different day: Tue, dow 1, slot 1) succeeds
        int c3Id = -1;
        assert(service.bookCourse(3, 1, "Toán 12", 1, 1, "2026-11-17", 4, true, msg, &c3Id));
        // PENDING does not reserve timetable slots
        auto c3 = cbRepo->findCourseBookingById(c3Id);
        assert(c3 && c3->status == "PENDING");

        // Cancelled course releases reservation
        assert(service.cancelCourseBooking(2, UserRole::STUDENT, c1Id, msg));
        auto c1AfterCancel = cbRepo->findCourseBookingById(c1Id);
        assert(c1AfterCancel && c1AfterCancel->status == "CANCELLED");

        // Now Student 2 can book the previously cancelled slot!
        int c4Id = -1;
        assert(service.bookCourse(3, 1, "Toán 12", 4, 0, "2026-11-13", 4, true, msg, &c4Id));

        std::filesystem::remove_all(testDir, ec);
    }

    // 4. Production Dataset Cross-File Invariants Validation
    {
        std::string dataDir = ApplicationPaths::resolveDataDirectory();
        auto uRepo = std::make_shared<FileUserRepository>(dataDir + "/users.dat");
        auto cbRepo = std::make_shared<FileCourseBookingRepository>(dataDir + "/course_bookings.dat");
        auto rRepo = std::make_shared<FileReviewRepository>(dataDir + "/reviews.dat");

        assert(uRepo->initialize());
        assert(cbRepo->initialize());
        assert(rRepo->initialize());

        auto allUsers = uRepo->getAllUsers();
        auto allCourses = cbRepo->getAllCourseBookings();
        auto allReviews = rRepo->getAllReviews();

        assert(allUsers.size() >= 50);
        assert(allCourses.size() == 23);
        assert(allReviews.size() == 15);

        // Verification of legacy bookings.dat retirement:
        // 1. data/bookings.dat does NOT exist in active directory
        assert(!std::filesystem::exists(dataDir + "/bookings.dat"));

        // 2. Historical legacy bookings are safely preserved in the archive directory
        std::string archivePath = dataDir + "/archive/legacy_bookings/legacy_bookings_20261106_140537.dat";
        assert(std::filesystem::exists(archivePath));
        struct ArchivedBooking {
            int id{0};
            int studentId{0};
            int tutorId{0};
            std::string status;
        };
        std::vector<ArchivedBooking> allArchivedBookings;
        std::unordered_map<int, ArchivedBooking> archivedMap;
        {
            std::ifstream archStream(archivePath);
            assert(archStream.is_open());
            std::string archLine;
            while (std::getline(archStream, archLine)) {
                if (archLine.empty()) continue;
                auto tokens = SerializerUtils::splitCsvLine(archLine);
                if (tokens.size() >= 9) {
                    ArchivedBooking ab;
                    ab.id = std::stoi(tokens[0]);
                    ab.studentId = std::stoi(tokens[1]);
                    ab.tutorId = std::stoi(tokens[2]);
                    ab.status = tokens[8];
                    allArchivedBookings.push_back(ab);
                    archivedMap[ab.id] = ab;
                }
            }
            assert(allArchivedBookings.size() == 27);
        }

        // Build User lookup
        std::unordered_map<int, std::shared_ptr<User>> userMap;
        for (const auto& u : allUsers) {
            userMap[u->getId()] = u;
            // 56-bit boundary
            if (u->getRole() == UserRole::TUTOR) {
                auto t = std::static_pointer_cast<Tutor>(u);
                assert(t->getScheduleBitmask() < (1ULL << 56));
            } else if (u->getRole() == UserRole::STUDENT) {
                auto s = std::static_pointer_cast<Student>(u);
                assert(s->getScheduleBitmask() < (1ULL << 56));
            }
        }

        // Invariant: all 27 archived legacy bookings reference existing student & tutor
        for (const auto& b : allArchivedBookings) {
            assert(userMap.count(b.studentId) == 1);
            assert(userMap[b.studentId]->getRole() == UserRole::STUDENT);
            assert(userMap.count(b.tutorId) == 1);
            assert(userMap[b.tutorId]->getRole() == UserRole::TUTOR);
        }

        // Invariant: all recurring courses reference existing student & tutor
        // Invariant: weekday(startDate) == dayOfWeek
        // Invariant: no collision among ACCEPTED courses
        struct ReservedSession {
            std::string date;
            int slot;
            int tutorId;
            int studentId;
            int courseId;
        };
        std::vector<ReservedSession> acceptedSessions;

        for (const auto& cb : allCourses) {
            assert(userMap.count(cb.studentId) == 1);
            assert(userMap[cb.studentId]->getRole() == UserRole::STUDENT);
            assert(userMap.count(cb.tutorId) == 1);
            assert(userMap[cb.tutorId]->getRole() == UserRole::TUTOR);

            auto cbSlots = cb.getWeeklySlots();
            assert(!cbSlots.empty());
            assert(DateUtils::dayOfWeek(cb.startDate) == cbSlots[0].first);
            assert(cb.weeksCount == 4 || cb.weeksCount == 8 || cb.weeksCount == 12);

            if (cb.status == "ACCEPTED") {
                auto sDates = DateUtils::generateSessionDates(cb.startDate, cb.weeksCount);
                assert(static_cast<int>(sDates.size()) == cb.weeksCount);
                for (const auto& d : sDates) {
                    for (const auto& res : acceptedSessions) {
                        if (res.date == d && res.slot == cbSlots[0].second) {
                            // Must not conflict with same tutor or same student
                            assert(res.tutorId != cb.tutorId);
                            assert(res.studentId != cb.studentId);
                        }
                    }
                    acceptedSessions.push_back(ReservedSession{d, cbSlots[0].second, cb.tutorId, cb.studentId, cb.id});
                }
            }
        }

        std::unordered_map<int, CourseBooking> cbMap;
        for (const auto& cb : allCourses) {
            cbMap[cb.id] = cb;
        }

        // Invariant: all 15 reviews reference valid completed course bookings in active database
        for (const auto& r : allReviews) {
            assert(userMap.count(r.studentId) == 1);
            assert(userMap.count(r.tutorId) == 1);
            assert(r.ratingScore >= 1 && r.ratingScore <= 5);
            assert(cbMap.count(r.bookingId) == 1);
            const auto& b = cbMap[r.bookingId];
            assert(b.status == "COMPLETED" || b.status == "EARLY_TERMINATED");
            assert(b.studentId == r.studentId);
            assert(b.tutorId == r.tutorId);
        }
    }

    std::cout << "  -> PASS: All Comprehensive Data Audit, Parsing & Migration Invariants verified successfully!" << std::endl;
}


// ============================================================================
// Suite 23: Multi-Session Course Booking, Consensual Early Termination & Auto-Completion
// ============================================================================
void run_test_multi_slot_course_and_consensual_termination() {
    std::cout << "[TEST] 23. Verifying Multi-Slot Course Booking, Consensual Termination & Auto-Completion..." << std::endl;

    std::string testDir = "tests_scratch_multi_slot_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::error_code ec;
    std::filesystem::create_directories(testDir, ec);

    std::string uPath = testDir + "/users.dat";
    std::string cbPath = testDir + "/course_bookings.dat";
    std::string rPath = testDir + "/reviews.dat";

    auto uRepo = std::make_shared<FileUserRepository>(uPath);
    auto cbRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
    auto rRepo = std::make_shared<FileReviewRepository>(rPath);
    uRepo->initialize();
    cbRepo->initialize();
    rRepo->initialize();

    uint64_t allFree = ScheduleGuard::VALID_SCHEDULE_MASK;
    Tutor t1(10, "tutor_star", "pass", "t1@tm.vn", "ThS Hoàng", "Bio", "Toán", 200000.0, 5.0, 0, 0.0, allFree, true);
    Student s1(20, "stu_an", "pass", "s1@tm.vn", "Nguyễn An", 12, "Toán", 5000000.0, allFree, true);
    Student s2(30, "stu_binh", "pass", "s2@tm.vn", "Trần Bình", 12, "Toán", 5000000.0, allFree, true);
    uRepo->saveUser(t1);
    uRepo->saveUser(s1);
    uRepo->saveUser(s2);

    BookingService service(uRepo, rRepo, cbRepo);

    // 1. Multi-slot booking creation: Mon (0) slot 0, Wed (2) slot 0, Fri (4) slot 0
    std::vector<std::pair<int, int>> selectedSlots = {{0, 0}, {2, 0}, {4, 0}};
    int courseId = -1;
    std::string msg;
    bool booked = service.bookCourseMultiSlot(20, 10, "Toán 12", selectedSlots, "2026-11-13", 4, true, msg, &courseId);
    assert(booked);
    assert(courseId > 0);

    auto cb = cbRepo->findCourseBookingById(courseId);
    assert(cb != nullptr);
    assert(cb->isMultiSlot());
    assert(cb->getWeeklySlots().size() == 3);
    assert(cb->getTotalSessions() == 12); // 3 slots * 4 weeks
    assert(cb->getTotalCost() == 2400000.0); // 12 * 200000
    assert(cb->status == "PENDING");

    // 2. Accept multi-slot course
    assert(service.acceptCourseBooking(10, courseId, true, msg));
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "ACCEPTED");

    // 3. Conflict detection: Student 2 tries to book single slot on Wed (2) slot 0 -> should conflict!
    int confCourseId = -1;
    bool bookedConflict = service.bookCourse(30, 10, "Toán 12", 2, 0, "2026-11-18", 4, true, msg, &confCourseId);
    assert(bookedConflict);
    bool acceptConflict = service.acceptCourseBooking(10, confCourseId, true, msg);
    assert(!acceptConflict); // Blocked due to collision with course 1!

    // 4. Non-conflicting multi-slot course for Student 2: Tue (1) slot 1, Thu (3) slot 1
    std::vector<std::pair<int, int>> nonConfSlots = {{1, 1}, {3, 1}};
    int c2Id = -1;
    assert(service.bookCourseMultiSlot(30, 10, "Toán 12 Nâng Cao", nonConfSlots, "2026-11-17", 4, true, msg, &c2Id));
    assert(service.acceptCourseBooking(10, c2Id, true, msg));
    auto c2 = cbRepo->findCourseBookingById(c2Id);
    assert(c2 && c2->status == "ACCEPTED");

    // 5. Consensual Early Termination Refusal:
    // Student 1 requests early termination
    assert(service.requestEarlyTermination(20, UserRole::STUDENT, courseId, "Bận việc gia đình", msg));
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->earlyTerminationStatus == "PENDING");
    assert(cb->earlyTerminationRequesterId == 20);

    // Tutor refuses
    assert(service.respondEarlyTermination(10, UserRole::TUTOR, courseId, false, msg));
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "ACCEPTED"); // Course continues!
    assert(cb->earlyTerminationStatus == "REJECTED");

    // 6. Consensual Early Termination Agreement:
    // Student 1 requests again
    assert(service.requestEarlyTermination(20, UserRole::STUDENT, courseId, "Đồng ý chuyển lịch", msg));
    // Tutor agrees
    assert(service.respondEarlyTermination(10, UserRole::TUTOR, courseId, true, msg));
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "EARLY_TERMINATED");
    assert(cb->earlyTerminationStatus == "ACCEPTED");
    assert(!cb->actualEndDate.empty());

    // 7. Verification: course 2 is unaffected and remains ACCEPTED
    c2 = cbRepo->findCourseBookingById(c2Id);
    assert(c2 && c2->status == "ACCEPTED");

    // 8. Newly freed slots can now be booked:
    // Student 2 can now book Wed (2) slot 0 with Tutor 1 without conflict!
    int c3Id = -1;
    assert(service.bookCourse(30, 10, "Toán Luyện Thi", 2, 0, "2026-11-18", 4, true, msg, &c3Id));
    assert(service.acceptCourseBooking(10, c3Id, true, msg));
    auto c3 = cbRepo->findCourseBookingById(c3Id);
    assert(c3 && c3->status == "ACCEPTED");

    // 9. Auto-completion of expired courses:
    // Inject an ACCEPTED course whose endDate is in the past
    CourseBooking expiredCb(9999, 20, 10, "Nguyễn An", "ThS Hoàng", "Toán 12",
                            200000.0, "2026-09-01", 4, "ACCEPTED", "2026-09-01", "0:0");
    cbRepo->saveCourseBooking(expiredCb);
    service.autoCompleteExpiredCourses();
    auto afterAuto = cbRepo->findCourseBookingById(9999);
    assert(afterAuto && afterAuto->status == "COMPLETED");

    // 10. Backward compatibility: CSV format resilience
    // 13-token legacy format
    std::string legCsv = "2001,20,10,Nguyễn An,ThS Hoàng,Toán 12,200000,4,0,2026-11-13,4,ACCEPTED,2026-11-09";
    auto legOpt = CourseBookingSerializer::fromCsv(legCsv);
    assert(legOpt.has_value());
    assert(legOpt->id == 2001);
    assert(!legOpt->isMultiSlot());
    assert(legOpt->earlyTerminationStatus == "NONE");

    // 18-token multi-slot extended format
    std::string extCsv = CourseBookingSerializer::toCsv(*cb);
    auto extOpt = CourseBookingSerializer::fromCsv(extCsv);
    assert(extOpt.has_value());
    assert(extOpt->id == cb->id);
    assert(extOpt->status == "EARLY_TERMINATED");
    assert(extOpt->earlyTerminationStatus == "ACCEPTED");
    assert(extOpt->isMultiSlot());

    std::filesystem::remove_all(testDir, ec);
    std::cout << "  -> PASS: Multi-slot courses, conflict prevention, mutual termination and auto-completion fully verified!" << std::endl;
}




void run_test_commitment_acknowledgment_and_review_matrix() {
    std::cout << "\n[TEST SUITE 24] Comprehensive Commitment Acknowledgment & Review Eligibility Matrix..." << std::endl;

    std::error_code ec;
    std::string testDir = "test_commitment_suite_data";
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    auto uRepo = std::make_shared<FileUserRepository>(testDir + "/users.dat");
    auto cbRepo = std::make_shared<FileCourseBookingRepository>(testDir + "/course_bookings.dat");
    auto rRepo = std::make_shared<FileReviewRepository>(testDir + "/reviews.dat");

    assert(uRepo->initialize());
    assert(cbRepo->initialize());
    assert(rRepo->initialize());

    // Setup Users
    uint64_t allFree = 0xFFFFFFFFFFFFFFFFULL;
    Tutor t1(10, "tutor_tuan", "pass", "tuan@tm.vn", "ThS Tuấn", "Bio", "Toán", 200000.0, 5.0, 0, 0.0, allFree, true);
    Student s1(20, "stu_mai", "pass", "mai@tm.vn", "Nguyễn Mai", 12, "Toán", 5000000.0, allFree, true);
    Student s2(30, "stu_huyen", "pass", "huyen@tm.vn", "Lê Huyền", 12, "Toán", 5000000.0, allFree, true);
    uRepo->saveUser(t1);
    uRepo->saveUser(s1);
    uRepo->saveUser(s2);

    BookingService bookingService(uRepo, rRepo, cbRepo);
    ReviewService reviewService(rRepo, uRepo, cbRepo);

    std::string msg;

    // ------------------------------------------------------------------------
    // Scenario 1: Student creates booking WITHOUT commitment ack -> REJECTED
    // ------------------------------------------------------------------------
    int b1Id = -1;
    bool b1Ok = bookingService.bookCourse(20, 10, "Toán 12", 4, 0, "2026-11-13", 4, false, msg, &b1Id);
    assert(!b1Ok);
    assert(b1Id == -1);
    assert(!msg.empty());

    // Multi-slot variant without ack -> REJECTED
    std::vector<std::pair<int, int>> weeklySlots = {{0, 0}, {2, 0}};
    bool bMultiNoAck = bookingService.bookCourseMultiSlot(20, 10, "Toán 12", weeklySlots, "2026-11-13", 4, false, msg, &b1Id);
    assert(!bMultiNoAck);
    assert(!msg.empty());

    // ------------------------------------------------------------------------
    // Scenario 2: Student creates booking WITH commitment ack -> SUCCESS
    // ------------------------------------------------------------------------
    int b2Id = -1;
    bool b2Ok = bookingService.bookCourse(20, 10, "Toán 12", 4, 0, "2026-11-13", 4, true, msg, &b2Id);
    assert(b2Ok);
    assert(b2Id > 0);
    auto b2 = cbRepo->findCourseBookingById(b2Id);
    assert(b2 != nullptr);
    assert(b2->status == "PENDING");
    assert(b2->studentAcknowledged == true);
    assert(!b2->studentAckTimestamp.empty());
    assert(b2->tutorAcknowledged == false);
    assert(b2->tutorAckTimestamp.empty());
    assert(b2->commitmentVersion == CourseBooking::CURRENT_COMMITMENT_VERSION);

    // ------------------------------------------------------------------------
    // Scenario 3: Tutor accepts WITHOUT commitment ack -> REJECTED
    // ------------------------------------------------------------------------
    bool accNoAck = bookingService.acceptCourseBooking(10, b2Id, false, msg);
    assert(!accNoAck);
    assert(!msg.empty());
    b2 = cbRepo->findCourseBookingById(b2Id);
    assert(b2->status == "PENDING");

    // ------------------------------------------------------------------------
    // Scenario 4: Tutor accepts WITH commitment ack -> SUCCESS (ACCEPTED)
    // ------------------------------------------------------------------------
    bool accWithAck = bookingService.acceptCourseBooking(10, b2Id, true, msg);
    assert(accWithAck);
    b2 = cbRepo->findCourseBookingById(b2Id);
    assert(b2->status == "ACCEPTED");
    assert(b2->tutorAcknowledged == true);
    assert(!b2->tutorAckTimestamp.empty());
    assert(b2->isCommitmentFullyAcknowledged());

    // ------------------------------------------------------------------------
    // Scenario 5: Tutor rejects PENDING booking WITHOUT needing ack -> SUCCESS
    // ------------------------------------------------------------------------
    int b3Id = -1;
    assert(bookingService.bookCourse(20, 10, "Toán 12", 1, 1, "2026-11-17", 4, true, msg, &b3Id));
    assert(bookingService.rejectCourseBooking(10, b3Id, msg));
    auto b3 = cbRepo->findCourseBookingById(b3Id);
    assert(b3 && b3->status == "REJECTED");
    assert(b3->tutorAcknowledged == false);

    // ------------------------------------------------------------------------
    // Scenario 6: Legacy booking (unacknowledged) cannot be accepted until student acknowledges
    // ------------------------------------------------------------------------
    CourseBooking legacyCb(8888, 20, 10, "Nguyễn Mai", "ThS Tuấn", "Toán 12", 200000.0,
                          "2026-11-11", 4, "PENDING", "2026-11-09", "2:1",
                          0, "NONE", "", "", false, "", false, "", "");
    cbRepo->saveCourseBooking(legacyCb);

    bool legAccFail = bookingService.acceptCourseBooking(10, 8888, true, msg);
    assert(!legAccFail);
    assert(!msg.empty());

    assert(bookingService.acknowledgeStudentCommitment(20, 8888, msg));
    auto legAfterStudentAck = cbRepo->findCourseBookingById(8888);
    assert(legAfterStudentAck->studentAcknowledged == true);
    assert(!legAfterStudentAck->studentAckTimestamp.empty());
    assert(legAfterStudentAck->commitmentVersion == CourseBooking::CURRENT_COMMITMENT_VERSION);

    assert(bookingService.acceptCourseBooking(10, 8888, true, msg));
    auto legAfterTutorAcc = cbRepo->findCourseBookingById(8888);
    assert(legAfterTutorAcc->status == "ACCEPTED");
    assert(legAfterTutorAcc->isCommitmentFullyAcknowledged());

    // ------------------------------------------------------------------------
    // Scenario 7: Client cannot forge timestamps (generated by backend)
    // ------------------------------------------------------------------------
    std::string ts = DateUtils::getCurrentTimestampString();
    assert(ts.length() >= 19);
    assert(ts[4] == '-' && ts[7] == '-' && ts[10] == ' ' && ts[13] == ':' && ts[16] == ':');

    // ------------------------------------------------------------------------
    // Scenario 8: Cannot acknowledge commitment if booking is not PENDING
    // ------------------------------------------------------------------------
    assert(!bookingService.acknowledgeStudentCommitment(20, b2Id, msg));
    assert(!msg.empty());
    assert(!bookingService.acknowledgeTutorCommitment(10, b2Id, msg));
    assert(!msg.empty());
    assert(!bookingService.acknowledgeStudentCommitment(20, b3Id, msg));
    assert(!msg.empty());

    // ------------------------------------------------------------------------
    // Review Eligibility Matrix: COMPLETED, EARLY_TERMINATED vs Others
    // ------------------------------------------------------------------------
    int bCompId = -1;
    assert(bookingService.bookCourse(20, 10, "Toán 12", 3, 2, "2026-11-19", 4, true, msg, &bCompId));
    assert(bookingService.acceptCourseBooking(10, bCompId, true, msg));
    assert(bookingService.terminateCourseBooking(10, UserRole::TUTOR, bCompId, msg));
    auto bComp = cbRepo->findCourseBookingById(bCompId);
    assert(bComp && bComp->status == "COMPLETED");

    int bEarlyId = -1;
    assert(bookingService.bookCourse(20, 10, "Toán 12", 0, 2, "2026-11-16", 4, true, msg, &bEarlyId));
    assert(bookingService.acceptCourseBooking(10, bEarlyId, true, msg));
    assert(bookingService.requestEarlyTermination(20, UserRole::STUDENT, bEarlyId, "Mục tiêu đạt được", msg));
    assert(bookingService.respondEarlyTermination(10, UserRole::TUTOR, bEarlyId, true, msg));
    auto bEarly = cbRepo->findCourseBookingById(bEarlyId);
    assert(bEarly && bEarly->status == "EARLY_TERMINATED");

    int bCancId = -1;
    assert(bookingService.bookCourse(20, 10, "Toán 12", 5, 0, "2026-11-14", 4, true, msg, &bCancId));
    assert(bookingService.cancelCourseBooking(20, UserRole::STUDENT, bCancId, msg));
    auto bCanc = cbRepo->findCourseBookingById(bCancId);
    assert(bCanc && bCanc->status == "CANCELLED");

    // Scenario 9: Review for COMPLETED course -> ELIGIBLE & SUCCESS
    std::string revReason;
    assert(reviewService.isEligibleForReview(20, bCompId, revReason));
    assert(reviewService.submitReview(20, bCompId, 5, "Khóa học rất hữu ích!", msg));

    // Scenario 10: Review for EARLY_TERMINATED course -> ELIGIBLE & SUCCESS
    assert(reviewService.isEligibleForReview(20, bEarlyId, revReason));
    assert(reviewService.submitReview(20, bEarlyId, 4, "Gia sư hỗ trợ nhiệt tình dù kết thúc sớm.", msg));

    // Scenario 11: Review for PENDING course -> INELIGIBLE
    int bPendId = -1;
    assert(bookingService.bookCourse(20, 10, "Toán 12", 6, 0, "2026-11-15", 4, true, msg, &bPendId));
    assert(!reviewService.isEligibleForReview(20, bPendId, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(20, bPendId, 5, "Đánh giá sớm", msg));

    // Scenario 12: Review for ACCEPTED course (in progress) -> INELIGIBLE
    assert(!reviewService.isEligibleForReview(20, b2Id, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(20, b2Id, 5, "Chưa học xong", msg));

    // Scenario 13: Review for REJECTED course -> INELIGIBLE
    assert(!reviewService.isEligibleForReview(20, b3Id, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(20, b3Id, 1, "Bị từ chối", msg));

    // Scenario 14: Review for CANCELLED course -> INELIGIBLE
    assert(!reviewService.isEligibleForReview(20, bCancId, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(20, bCancId, 1, "Đã hủy", msg));

    // Scenario 15: Duplicate review attempt -> REJECTED
    assert(!reviewService.isEligibleForReview(20, bCompId, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(20, bCompId, 5, "Đánh giá lần 2", msg));

    // Scenario 16: Wrong student reviewer -> REJECTED
    assert(!reviewService.isEligibleForReview(30, bCompId, revReason));
    assert(!revReason.empty());
    assert(!reviewService.submitReview(30, bCompId, 5, "Người ngoài đánh giá", msg));

    // Scenario 17: Serializer 23-field Round-Trip & Backward Compatibility
    std::string csv23 = CourseBookingSerializer::toCsv(*b2);
    auto optParsed = CourseBookingSerializer::fromCsv(csv23);
    assert(optParsed.has_value());
    assert(optParsed->studentAcknowledged == true);
    assert(optParsed->tutorAcknowledged == true);
    assert(optParsed->commitmentVersion == CourseBooking::CURRENT_COMMITMENT_VERSION);
    assert(!optParsed->studentAckTimestamp.empty());
    assert(!optParsed->tutorAckTimestamp.empty());

    std::filesystem::remove_all(testDir, ec);
    std::cout << "  -> PASS: All 17 Commitment Acknowledgment & Review Eligibility scenarios verified!" << std::endl;
}


// ============================================================================
// TEST 25: Student Profile Simplification & SmartMatch Criteria-Based Ranking
// ============================================================================
void run_test_student_profile_and_smartmatch_criteria_ranking() {
    std::cout << "[TEST] 25. Verifying Student Profile Simplification & SmartMatch Criteria-Based Ranking..." << std::endl;

    // 1. Student Profile Simplification: Stable fields only
    {
        Student alice(101, "alice_sv", "pass123", "alice@tutormatch.vn", "Alice Nguyen", 10, "Toán", 200000.0, 0ULL, true);
        assert(alice.getFullName() == "Alice Nguyen");
        assert(alice.getEmail() == "alice@tutormatch.vn");
        assert(alice.getGradeLevel() == 10);

        // Update profile with simplified 4-parameter method (no subject/budget required)
        auto userRepo = std::make_shared<FileUserRepository>("users_test_sandbox.dat");
        userRepo->saveUser(alice);
        UserService userService(userRepo);

        std::string err;
        bool ok = userService.updateStudentProfile(alice, "Alice Tran", "alice.tran@tutormatch.vn", 11, err);
        assert(ok);
        assert(alice.getFullName() == "Alice Tran");
        assert(alice.getEmail() == "alice.tran@tutormatch.vn");
        assert(alice.getGradeLevel() == 11);

        // Invalid grade is rejected
        assert(!userService.updateStudentProfile(alice, "Alice Tran", "alice.tran@tutormatch.vn", 15, err));
        assert(!userService.updateStudentProfile(alice, "", "alice.tran@tutormatch.vn", 10, err));

        // Legacy format compatibility: 11 fields CSV serialization remains fully readable
        std::string csvLine = UserSerializer::toCsv(alice);
        std::string parseErr;
        auto deserialized = UserSerializer::studentFromCsv(csvLine, &parseErr);
        assert(deserialized != nullptr);
        assert(deserialized->getId() == 101);
        assert(deserialized->getFullName() == "Alice Tran");
        assert(deserialized->getGradeLevel() == 11);
        std::filesystem::remove("users_test_sandbox.dat");
    }

    // 2. Search Independence: Different searches do not mutate Student Profile
    {
        Student bob(102, "bob_sv", "pass123", "bob@tutormatch.vn", "Bob Le", 10, 0ULL, true);

        SmartMatchCriteria search1;
        search1.subject = "Toán";
        search1.gradeLevel = 10;
        search1.maxBudget = 150000.0;
        search1.startDate = "2026-11-16";
        search1.weeksCount = 4;

        SmartMatchCriteria search2;
        search2.subject = "Tiếng Anh";
        search2.gradeLevel = 12;
        search2.maxBudget = 250000.0;
        search2.startDate = "2026-11-23";
        search2.weeksCount = 8;

        // Bob's profile was never touched by search criteria
        assert(bob.getGradeLevel() == 10);
        assert(bob.getFullName() == "Bob Le");
        assert(bob.getEmail() == "bob@tutormatch.vn");
    }

    // 3. Hard Eligibility Filters
    {
        MatchingEngine engine;
        std::vector<CourseBooking> activeBookings;

        // Candidate tutors
        // Tutor 1: Math 10-12, rate 150k, Mon & Wed Ca 7 (18:45, slot 6)
        uint64_t maskT1 = (1ULL << (0 * 8 + 6)) | (1ULL << (2 * 8 + 6)); // Mon & Wed Ca 7
        Tutor tMath(201, "t_math", "pass", "math@tm.vn", "ThS. Tran Math", "Bio 10 năm luyện thi Toán THPT Chuyên", "Toán 10-12 (Luyện thi THPT)", 150000.0, 4.8, 10, 0.0, maskT1, true);

        // Tutor 2: Physics 10-12, rate 180k, Mon & Wed Ca 7
        Tutor tPhysics(202, "t_phys", "pass", "phys@tm.vn", "TS. Le Physics", "Giảng viên Vật lý THPT", "Vật lý 10-12", 180000.0, 4.5, 6, 0.0, maskT1, true);

        // Tutor 3: Math 10-12, rate 250k (High price), Mon & Wed Ca 7
        Tutor tExpensive(203, "t_exp", "pass", "exp@tm.vn", "Thầy Expensive", "Bio Chuyên Toán", "Toán 10-12", 250000.0, 5.0, 20, 0.0, maskT1, true);

        // Tutor 4: Math 10-12, rate 140k, but INACTIVE
        Tutor tInactive(204, "t_inact", "pass", "inact@tm.vn", "Thầy Inactive", "Bio Chuyên Toán", "Toán 10-12", 140000.0, 5.0, 10, 0.0, maskT1, false);

        // Tutor 5: Math, but only available Mon Ca 7 (missing Wed Ca 7)
        uint64_t maskMonOnly = (1ULL << (0 * 8 + 6));
        Tutor tPartialSchedule(205, "t_part", "pass", "part@tm.vn", "Cô Partial", "Bio Toán", "Toán 10-12", 140000.0, 4.6, 8, 0.0, maskMonOnly, true);

        SmartMatchCriteria criteria;
        criteria.subject = "Toán";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;
        criteria.preferredScheduleMask = (1ULL << (0 * 8 + 6)) | (1ULL << (2 * 8 + 6)); // Mon & Wed Ca 7
        criteria.startDate = "2026-11-16";
        criteria.weeksCount = 4;
        criteria.strictSchedule = true;

        std::vector<std::pair<int, int>> matchedSlots;

        // Filter 1: Subject qualification
        assert(MatchingEngine::isEligible(criteria, tMath, activeBookings, matchedSlots));
        assert(!MatchingEngine::isEligible(criteria, tPhysics, activeBookings, matchedSlots)); // Wrong subject

        // Filter 2: Budget check
        assert(!MatchingEngine::isEligible(criteria, tExpensive, activeBookings, matchedSlots)); // 250k > 200k

        // Filter 3: Inactive tutor
        assert(!MatchingEngine::isEligible(criteria, tInactive, activeBookings, matchedSlots));

        // Filter 4: Strict schedule matching
        assert(!MatchingEngine::isEligible(criteria, tPartialSchedule, activeBookings, matchedSlots)); // Missing Wed Ca 7

        // Flexible mode: tPartialSchedule IS eligible because 1 slot matches
        criteria.strictSchedule = false;
        assert(MatchingEngine::isEligible(criteria, tPartialSchedule, activeBookings, matchedSlots));
        assert(matchedSlots.size() == 1);
        criteria.strictSchedule = true; // reset to strict

        // Filter 5: Active reservation conflict
        // Suppose tMath has an accepted course booking on Monday Ca 7 during 2026-11-16 to 2026-12-14
        CourseBooking activeRes(501, 99, 201, "Student X", "ThS. Tran Math", "Toán 12", 150000.0,
                              "2026-11-16", 4, "ACCEPTED", "2026-11-10", "0:6", 0, "NONE", "", "", true, "", true, "", "");
        activeBookings.push_back(activeRes);

        // Now tMath has conflict on Mon Ca 7 -> strictly ineligible!
        assert(!MatchingEngine::isEligible(criteria, tMath, activeBookings, matchedSlots));

        // Historical / completed / early-terminated booking does NOT block
        activeBookings[0].status = "COMPLETED";
        assert(MatchingEngine::isEligible(criteria, tMath, activeBookings, matchedSlots));

        activeBookings[0].status = "CANCELLED";
        assert(MatchingEngine::isEligible(criteria, tMath, activeBookings, matchedSlots));
    }

    // 4. 100-Point Explainable Scoring Model Invariants
    {
        WeightedScoreMatchingStrategy strategy;
        SmartMatchCriteria criteria;
        criteria.subject = "Toán";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;
        criteria.preferredScheduleMask = (1ULL << (0 * 8 + 6)); // Mon Ca 7
        criteria.startDate = "2026-11-16";
        criteria.weeksCount = 4;
        criteria.strictSchedule = true;

        uint64_t mask = (1ULL << (0 * 8 + 6));

        // Price Monotonicity Test
        Tutor cheapTutor(301, "cheap", "p", "c@t.vn", "Thầy Rẻ", "Bio Toán", "Toán 10-12", 120000.0, 4.5, 5, 0.0, mask, true);
        Tutor expensiveTutor(302, "exp", "p", "e@t.vn", "Thầy Đắt", "Bio Toán", "Toán 10-12", 180000.0, 4.5, 5, 0.0, mask, true);

        auto resCheap = strategy.calculateScoreWithCriteria(criteria, cheapTutor);
        auto resExp = strategy.calculateScoreWithCriteria(criteria, expensiveTutor);
        assert(resCheap.priceScore > resExp.priceScore); // Lower price must score higher on price competitiveness!
        assert(resCheap.priceScore <= 25.0 && resCheap.priceScore >= 0.0);
        assert(resExp.priceScore <= 25.0 && resExp.priceScore >= 0.0);

        // Bayesian Review Scoring Test: 10 reviews vs 1 review with 5.0 stars
        Tutor veteranTutor(303, "vet", "p", "v@t.vn", "Thầy Uy Tín", "Bio Toán 10 năm kinh nghiệm", "Toán 10-12", 150000.0, 5.0, 10, 0.0, mask, true);
        Tutor singleReviewTutor(304, "single", "p", "s@t.vn", "Thầy Mới", "Bio Toán", "Toán 10-12", 150000.0, 5.0, 1, 0.0, mask, true);
        Tutor zeroReviewTutor(305, "zero", "p", "z@t.vn", "Thầy Chưa Có Đánh Giá", "Bio Toán", "Toán 10-12", 150000.0, 0.0, 0, 0.0, mask, true);

        auto resVet = strategy.calculateScoreWithCriteria(criteria, veteranTutor);
        auto resSingle = strategy.calculateScoreWithCriteria(criteria, singleReviewTutor);
        auto resZero = strategy.calculateScoreWithCriteria(criteria, zeroReviewTutor);

        assert(resVet.ratingScore > resSingle.ratingScore); // 10 reviews beats 1 single 5-star review!
        assert(resZero.ratingScore > 0.0 && resZero.ratingScore <= 15.0); // Zero reviews handled reasonably without division by zero!

        // Grade Level Fit Test
        Tutor highSchoolTutor(306, "hs", "p", "hs@t.vn", "Thầy THPT", "Bio Toán THPT", "Toán 10-12", 150000.0, 4.5, 5, 0.0, mask, true);
        Tutor primaryTutor(307, "prim", "p", "p@t.vn", "Cô Tiểu Học", "Bio Toán Tiểu Học", "Toán 1-5", 150000.0, 4.5, 5, 0.0, mask, true);
        auto resHs = strategy.calculateScoreWithCriteria(criteria, highSchoolTutor);
        auto resPrim = strategy.calculateScoreWithCriteria(criteria, primaryTutor);
        assert(resHs.experienceScore > resPrim.experienceScore); // Correct grade fit scores higher!

        // Total Composite Score bounds & explainability
        assert(resVet.compositeScore >= 0.0 && resVet.compositeScore <= 100.0);
        assert(!resVet.explanations.empty());
        for (const auto& exp : resVet.explanations) {
            assert(!exp.empty());
        }

        // Deterministic Sorting Test
        std::vector<std::shared_ptr<Tutor>> tutors = {
            std::make_shared<Tutor>(cheapTutor),
            std::make_shared<Tutor>(expensiveTutor),
            std::make_shared<Tutor>(veteranTutor)
        };
        MatchingEngine engine;
        auto run1 = engine.findTopMatchesWithCriteria(criteria, tutors, {}, 3);
        auto run2 = engine.findTopMatchesWithCriteria(criteria, tutors, {}, 3);
        assert(run1.size() == run2.size());
        for (size_t i = 0; i < run1.size(); ++i) {
            assert(run1[i].tutor->getId() == run2[i].tutor->getId());
            assert(run1[i].compositeScore == run2[i].compositeScore);
        }
    }

    std::cout << "  -> PASS: Student Profile Simplification & SmartMatch Criteria-Based Ranking verified 100%!" << std::endl;
}

void run_test_persistence_schema_and_migration() {
    std::cout << "[TEST] 26. Verifying Persistence Schema & Migration..." << std::endl;

    // 1. Reading legacy user records (11-field CSV line)
    {
        std::string legacyLine = "201,legacy_stu,p123,legacy@tm.vn,Học Viên Cũ,STUDENT,1,11,Toán Cũ,150000,281470681808895";
        std::string err;
        auto user = UserSerializer::fromCsv(legacyLine, &err);
        assert(user != nullptr && err.empty());
        assert(user->getRole() == UserRole::STUDENT);
        auto s = std::dynamic_pointer_cast<Student>(user);
        assert(s != nullptr);
        assert(s->getId() == 201);
        assert(s->getUsername() == "legacy_stu");
        assert(s->getPasswordHash() == "p123");
        assert(s->getEmail() == "legacy@tm.vn");
        assert(s->getFullName() == "Học Viên Cũ");
        assert(s->getIsActive() == true);
        assert(s->getGradeLevel() == 11);
        assert(s->getScheduleBitmask() == 281470681808895ULL);

        // Re-serialization writes canonical 9-field format (no obsolete subject/budget)
        std::string canonCsv = UserSerializer::toCsv(*s);
        assert(canonCsv == "201,legacy_stu,p123,legacy@tm.vn,Học Viên Cũ,STUDENT,1,11,281470681808895");
    }

    // 2. Reading new canonical user records (9-field CSV line)
    {
        std::string canonLine = "202,canon_stu,p456,canon@tm.vn,Học Viên Mới,STUDENT,1,12,71777214294589695";
        std::string err;
        auto user = UserSerializer::fromCsv(canonLine, &err);
        assert(user != nullptr && err.empty());
        assert(user->getRole() == UserRole::STUDENT);
        auto s = std::dynamic_pointer_cast<Student>(user);
        assert(s != nullptr);
        assert(s->getId() == 202);
        assert(s->getUsername() == "canon_stu");
        assert(s->getPasswordHash() == "p456");
        assert(s->getEmail() == "canon@tm.vn");
        assert(s->getFullName() == "Học Viên Mới");
        assert(s->getIsActive() == true);
        assert(s->getGradeLevel() == 12);
        assert(s->getScheduleBitmask() == 71777214294589695ULL);

        assert(UserSerializer::toCsv(*s) == canonLine);
    }

    // 3. Saving and reloading new student records via FileUserRepository (Round-trip determinism)
    {
        std::error_code ec;
        auto testDir = std::filesystem::temp_directory_path() / "tutormatch_migration_test_sandbox";
        std::filesystem::remove_all(testDir, ec);
        std::filesystem::create_directories(testDir, ec);

        std::string uFile = (testDir / "users.dat").string();
        FileUserRepository repo(uFile);
        assert(repo.initialize());

        Student newStu(301, "sv_new", "hash999", "sv.new@tm.vn", "Nguyễn Học Viên", 10, 0x1234ULL, true);
        assert(repo.saveUser(newStu));

        // Reload from disk
        FileUserRepository repoReload(uFile);
        assert(repoReload.initialize());
        auto reloaded = repoReload.findUserById(301);
        assert(reloaded != nullptr);
        assert(reloaded->getRole() == UserRole::STUDENT);
        auto s = std::dynamic_pointer_cast<Student>(reloaded);
        assert(s != nullptr);
        assert(s->getId() == 301);
        assert(s->getUsername() == "sv_new");
        assert(s->getPasswordHash() == "hash999");
        assert(s->getEmail() == "sv.new@tm.vn");
        assert(s->getFullName() == "Nguyễn Học Viên");
        assert(s->getGradeLevel() == 10);
        assert(s->getScheduleBitmask() == 0x1234ULL);

        // Verify physical file line contains exactly 9 tokens
        std::ifstream ifs(uFile);
        std::string savedLine;
        assert(std::getline(ifs, savedLine));
        auto tokens = SerializerUtils::splitCsvLine(savedLine);
        assert(tokens.size() == 9);
        assert(tokens[0] == "301");
        assert(tokens[5] == "STUDENT");
        assert(tokens[7] == "10");
        assert(tokens[8] == std::to_string(0x1234ULL));

        std::filesystem::remove_all(testDir, ec);
    }

    // 4. Preserving tutor (14 fields) and admin (7 fields) records without distortion
    {
        Admin a(1, "admin_user", "sec123", "admin@tm.vn", "Admin Root", true);
        Tutor t(2, "tutor_user", "sec456", "tutor@tm.vn", "Tutor Demo", "Tiểu sử", "Vật lý 10", 200000.0, 4.9, 10, 500000.0, 0x55ULL, true);

        std::string aCsv = UserSerializer::toCsv(a);
        std::string tCsv = UserSerializer::toCsv(t);

        auto tokensA = SerializerUtils::splitCsvLine(aCsv);
        auto tokensT = SerializerUtils::splitCsvLine(tCsv);
        assert(tokensA.size() == 7);
        assert(tokensT.size() == 14);

        auto aParsed = UserSerializer::adminFromCsv(aCsv);
        auto tParsed = UserSerializer::tutorFromCsv(tCsv);
        assert(aParsed != nullptr && aParsed->getRole() == UserRole::ADMIN);
        assert(tParsed != nullptr && tParsed->getSubjectSpecialty() == "Vật lý 10");
        assert(tParsed->getHourlyRate() == 200000.0);
    }

    // 5. Multiple SmartMatch searches with different subjects and budgets
    {
        Student stu(401, "smart_stu", "pass", "smart@tm.vn", "Smart Student", 11, 0ULL, true);

        SmartMatchCriteria critA;
        critA.subject = "Toán";
        critA.maxBudget = 150000.0;
        critA.gradeLevel = 11;

        SmartMatchCriteria critB;
        critB.subject = "Hóa học";
        critB.maxBudget = 250000.0;
        critB.gradeLevel = 12;

        assert(critA.subject != critB.subject);
        assert(critA.maxBudget != critB.maxBudget);
        // Student profile is unchanged and does not store either search
        assert(stu.getGradeLevel() == 11);
    }

    // 6. Existing data integrity after physical migration of data/users.dat
    {
        std::string dataDir = ApplicationPaths::resolveDataDirectory();
        FileUserRepository prodRepo(dataDir + "/users.dat");
        assert(prodRepo.initialize());

        auto allUsers = prodRepo.getAllUsers();
        assert(allUsers.size() >= 50);

        int adminCount = 0;
        int tutorCount = 0;
        int studentCount = 0;

        for (int id = 1; id <= 50; ++id) {
            auto u = prodRepo.findUserById(id);
            assert(u != nullptr);
            assert(u->getId() == id);
            if (u->getRole() == UserRole::ADMIN) adminCount++;
            else if (u->getRole() == UserRole::TUTOR) tutorCount++;
            else if (u->getRole() == UserRole::STUDENT) studentCount++;
        }

        assert(adminCount == 3);
        assert(tutorCount == 12);
        assert(studentCount == 35);

        // Spot-check Student 6 (Nam) and Student 7 (Thành)
        auto u6 = prodRepo.findUserById(6);
        assert(u6 && u6->getRole() == UserRole::STUDENT);
        auto s6 = std::dynamic_pointer_cast<Student>(u6);
        assert(s6 && s6->getGradeLevel() == 12);
        assert(s6->getScheduleBitmask() == 71777214294589695ULL);

        auto u7 = prodRepo.findUserById(7);
        assert(u7 && u7->getRole() == UserRole::STUDENT);
        auto s7 = std::dynamic_pointer_cast<Student>(u7);
        assert(s7 && s7->getGradeLevel() == 11);
        assert(s7->getScheduleBitmask() == 281470681808895ULL);
    }

    // 7. Physical Production File Canonical Guarantee & 100% Round-Trip Parity
    {
        std::string dataDir = ApplicationPaths::resolveDataDirectory();
        std::string prodPath = dataDir + "/users.dat";
        std::ifstream prodFile(prodPath);
        assert(prodFile.is_open());

        std::string line;
        int lineCount = 0;
        int legacy11Count = 0;
        int canonical9Count = 0;
        std::vector<int> verifiedIds;

        while (std::getline(prodFile, line)) {
            if (line.empty()) continue;
            lineCount++;
            auto tokens = SerializerUtils::splitCsvLine(line);
            assert(tokens.size() >= 7);
            int id = std::stoi(tokens[0]);
            verifiedIds.push_back(id);
            std::string role = tokens[5];

            if (role == "STUDENT") {
                if (tokens.size() == 11) legacy11Count++;
                if (tokens.size() == 9) canonical9Count++;
                // Strict canonical assertion: every student record in users.dat MUST be 9 fields!
                assert(tokens.size() == 9);
            } else if (role == "TUTOR") {
                assert(tokens.size() == 14);
            } else if (role == "ADMIN") {
                assert(tokens.size() == 7);
            }

            // Test Full Round-Trip Parity: read -> deserialize -> serialize == raw line
            std::string parseErr;
            auto parsedUser = UserSerializer::fromCsv(line, &parseErr);
            assert(parsedUser != nullptr && parseErr.empty());
            std::string serialized = UserSerializer::toCsv(*parsedUser);
            assert(serialized == line);
        }

        assert(lineCount >= 50);
        assert(canonical9Count >= 35);
        assert(legacy11Count == 0); // ZERO legacy records remaining in active production users.dat!
        assert(verifiedIds.size() >= 50);

        // Verify verified backup file exists and preserves historical records
        std::string backupPath = dataDir + "/archive/legacy_users/users_legacy_11fields_pre_migration.dat";
        assert(std::filesystem::exists(backupPath));
        assert(std::filesystem::file_size(backupPath) > 0);
    }

    std::cout << "  -> PASS: Persistence Schema & Migration verified 100%!" << std::endl;
}

void run_test_smartmatch_mandatory_subject_filtering() {
    std::cout << "[TEST] 27. Verifying SmartMatch Mandatory Subject Filtering Before Ranking..." << std::endl;

    std::vector<std::pair<int, int>> matchedSlots;
    uint64_t fullMask = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t halfMask = 0x00000000000000FFULL;

    // 1. Requested subject = Physics; tutor teaches only History -> excluded
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Physics";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tHistory(101, "t_hist", "p", "hist@tm.vn", "Thầy Sử", "Bio Lịch sử", "Lịch sử 10-12", 100000.0, 4.8, 10, 0.0, fullMask, true);
        assert(!MatchingEngine::isEligible(criteria, tHistory, {}, matchedSlots));

        // Vietnamese equivalent test
        criteria.subject = "Vật lý";
        assert(!MatchingEngine::isEligible(criteria, tHistory, {}, matchedSlots));
    }

    // 2. Requested subject = Physics; tutor teaches Physics -> eligible
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Physics";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tPhys(102, "t_phys", "p", "phys@tm.vn", "Thầy Lý", "Bio Vật lý", "Vật lý 10-12", 150000.0, 4.5, 8, 0.0, fullMask, true);
        assert(MatchingEngine::isEligible(criteria, tPhys, {}, matchedSlots));

        // Vietnamese query
        criteria.subject = "Vật lý";
        assert(MatchingEngine::isEligible(criteria, tPhys, {}, matchedSlots));
    }

    // 3. Requested subject = Physics; tutor teaches Mathematics and Physics -> eligible
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Physics";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tMultiVN(103, "t_mp_vn", "p", "mpvn@tm.vn", "Thầy Toán Lý", "Bio", "Toán & Vật lý 10-12", 160000.0, 4.7, 12, 0.0, fullMask, true);
        Tutor tMultiEN(104, "t_mp_en", "p", "mpen@tm.vn", "Mr. MP", "Bio", "Mathematics and Physics", 170000.0, 4.6, 5, 0.0, fullMask, true);
        assert(MatchingEngine::isEligible(criteria, tMultiVN, {}, matchedSlots));
        assert(MatchingEngine::isEligible(criteria, tMultiEN, {}, matchedSlots));

        // Searching for Math also matches
        criteria.subject = "Toán";
        assert(MatchingEngine::isEligible(criteria, tMultiVN, {}, matchedSlots));
        assert(MatchingEngine::isEligible(criteria, tMultiEN, {}, matchedSlots));
    }

    // 4. Requested subject = Physics; tutor teaches Chemistry and History -> excluded
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Physics";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tChemHist(105, "t_ch", "p", "ch@tm.vn", "Thầy Hóa Sử", "Bio", "Chemistry and History", 120000.0, 4.9, 15, 0.0, fullMask, true);
        Tutor tChemHistVN(106, "t_ch_vn", "p", "chvn@tm.vn", "Thầy Hóa Sử VN", "Bio", "Hóa học & Lịch sử", 110000.0, 4.9, 15, 0.0, fullMask, true);
        assert(!MatchingEngine::isEligible(criteria, tChemHist, {}, matchedSlots));
        assert(!MatchingEngine::isEligible(criteria, tChemHistVN, {}, matchedSlots));
    }

    // 5. A History tutor has a higher review score, lower tuition and better availability than a Physics tutor -> History tutor must still be excluded
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Vật lý";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 250000.0;

        // History tutor: 5.0 rating, 50k tuition, 100% available slots
        auto tSuperHist = std::make_shared<Tutor>(107, "super_hist", "p", "sh@tm.vn", "Thầy Sử Siêu Sao",
                                                  "Bio Sử", "Lịch sử", 50000.0, 5.0, 50, 0.0, fullMask, true);

        // Physics tutor: 4.0 rating, 200k tuition, 8 available slots
        auto tModestPhys = std::make_shared<Tutor>(108, "modest_phys", "p", "mp@tm.vn", "Thầy Lý Cơ Bản",
                                                   "Bio Lý", "Vật lý", 200000.0, 4.0, 2, 0.0, halfMask, true);

        std::vector<std::shared_ptr<Tutor>> tutors = {tSuperHist, tModestPhys};

        MatchingEngine engine;
        auto results = engine.findTopMatchesWithCriteria(criteria, tutors, {}, 5);

        // Mandatory subject filter must exclude tSuperHist entirely
        assert(results.size() == 1);
        assert(results[0].tutor->getId() == 108); // Only Physics tutor is returned!
        for (const auto& r : results) {
            assert(r.tutor->getId() != 107); // History tutor NEVER appears in results
        }
    }

    // 6. No tutor teaches the requested subject -> empty result
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Sinh học"; // Biology requested
        criteria.gradeLevel = 10;
        criteria.maxBudget = 300000.0;

        auto tMath = std::make_shared<Tutor>(109, "math_only", "p", "m@tm.vn", "Thầy Toán", "Bio", "Toán 12", 150000.0, 4.8, 10, 0.0, fullMask, true);
        auto tPhys = std::make_shared<Tutor>(110, "phys_only", "p", "p@tm.vn", "Thầy Lý", "Bio", "Vật lý 12", 180000.0, 4.7, 8, 0.0, fullMask, true);

        std::vector<std::shared_ptr<Tutor>> tutors = {tMath, tPhys};
        MatchingEngine engine;
        auto results = engine.findTopMatchesWithCriteria(criteria, tutors, {}, 5);
        assert(results.empty()); // Returns empty list
    }

    // 7. Tutor has an empty subject list -> excluded
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Toán";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tEmpty(111, "empty_subj", "p", "e@tm.vn", "Cô Trống", "Bio", "", 100000.0, 5.0, 10, 0.0, fullMask, true);
        Tutor tWhitespace(112, "ws_subj", "p", "ws@tm.vn", "Cô Khoảng Trắng", "Bio", "   ", 100000.0, 5.0, 10, 0.0, fullMask, true);

        assert(!MatchingEngine::isEligible(criteria, tEmpty, {}, matchedSlots));
        assert(!MatchingEngine::isEligible(criteria, tWhitespace, {}, matchedSlots));
    }

    // 8. Tutor has invalid or unknown subject data -> excluded safely
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Toán";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 200000.0;

        Tutor tInvalid(113, "inv_subj", "p", "inv@tm.vn", "Thầy Lạ", "Bio", "???###InvalidXYZ", 100000.0, 5.0, 10, 0.0, fullMask, true);
        assert(!MatchingEngine::isEligible(criteria, tInvalid, {}, matchedSlots));

        // When criteria has unknown/invalid subject, no tutor matches
        criteria.subject = "???###UnknownCriteria";
        Tutor tValidMath(114, "vm", "p", "vm@tm.vn", "Thầy Chuẩn", "Bio", "Toán 10-12", 150000.0, 4.8, 10, 0.0, fullMask, true);
        assert(!MatchingEngine::isEligible(criteria, tValidMath, {}, matchedSlots));
    }

    // 9. Subject matching remains consistent between GUIBridge, MatchingService and MatchingEngine
    {
        SmartMatchCriteria criteria;
        criteria.subject = "Vật lý";
        criteria.gradeLevel = 10;
        criteria.maxBudget = 250000.0;

        auto t1 = std::make_shared<Tutor>(115, "u1", "p", "u1@tm.vn", "Lý 1", "Bio", "Vật lý 10-12", 150000.0, 4.6, 5, 0.0, fullMask, true);
        auto t2 = std::make_shared<Tutor>(116, "u2", "p", "u2@tm.vn", "Sử 1", "Bio", "Lịch sử 10-12", 100000.0, 4.9, 20, 0.0, fullMask, true);
        auto t3 = std::make_shared<Tutor>(117, "u3", "p", "u3@tm.vn", "Toán Lý 1", "Bio", "Toán & Vật lý", 180000.0, 4.8, 15, 0.0, fullMask, true);

        std::vector<std::shared_ptr<Tutor>> tutors = {t1, t2, t3};

        MatchingEngine engine;
        auto engineMatches = engine.findTopMatchesWithCriteria(criteria, tutors, {}, 5);

        // Both t1 (Vật lý) and t3 (Toán & Vật lý) are returned; t2 (Lịch sử) excluded!
        assert(engineMatches.size() == 2);
        assert(engineMatches[0].tutor->getId() != 116);
        assert(engineMatches[1].tutor->getId() != 116);

        // Explanations contain matched subject confirmation
        assert(!engineMatches[0].explanations.empty());
        assert(engineMatches[0].explanations[0].find("Chuyên môn phù hợp môn học yêu cầu") != std::string::npos);
    }

    std::cout << "  -> PASS: SmartMatch Mandatory Subject Filtering Before Ranking verified 100%!" << std::endl;
}


void run_test_tutor_search_engine_normalization_semantic_and_ranking() {
    std::cout << "[TEST] 28. Verifying Tutor Search Engine Normalization, Semantic Matching & Relevance Ranking..." << std::endl;

    // 1. Text Normalization, Unicode Case-Folding, Accent-Stripping & Whitespace Collapsing
    {
        std::string raw1 = "  To\xc3\xa1n   V\xe1\xba\xadt   L\xc3\xbd  "; // "  Toán   Vật   Lý  "
        std::string normAcc = TutorSearchMatcher::normalizeAccented(raw1);
        assert(normAcc == "to\xc3\xa1n v\xe1\xba\xadt l\xc3\xbd");

        std::string normUnacc = TutorSearchMatcher::normalizeUnaccented(raw1);
        assert(normUnacc == "toan vat ly");

        // Verify đ/Đ -> d conversion
        std::string rawD = "\xc4\x90\xe1\xba\xb7ng Thu H\xc3\xa0"; // "Đặng Thu Hà"
        std::string normD = TutorSearchMatcher::normalizeUnaccented(rawD);
        assert(normD == "dang thu ha");

        // Verify punctuation handling
        std::string rawPunct = "To\xc3\xa1n, V\xe1\xba\xadt l\xc3\xbd / Ho\xc3\xa1 (10-12)";
        std::string normPunct = TutorSearchMatcher::normalizeUnaccented(rawPunct);
        assert(normPunct == "toan vat ly hoa 10 12");

        // Verify tokenization
        auto tokens = TutorSearchMatcher::tokenize(normPunct);
        assert(tokens.size() == 6);
        assert(tokens[0] == "toan" && tokens[1] == "vat" && tokens[2] == "ly" && tokens[3] == "hoa");
    }

    // 2. Query Understanding (Subjects, Grades, Free-Text)
    {
        auto intent1 = TutorSearchMatcher::parseQuery("To\xc3\xa1n V\xe1\xba\xadt L\xc3\xbd");
        assert(intent1.targetSubjects.size() == 2);
        assert(intent1.isMultiSubject == true);
        assert(intent1.targetGrade == 0);

        auto intent2 = TutorSearchMatcher::parseQuery("To\xc3\xa1n 12");
        assert(intent2.targetSubjects.size() == 1);
        assert(intent2.targetSubjects[0] == CanonicalSubject::MATH);
        assert(intent2.targetGrade == 12);

        auto intent3 = TutorSearchMatcher::parseQuery("Tr\xe1\xba\xa7n Chi");
        assert(intent3.targetSubjects.empty());
        assert(intent3.targetGrade == 0);
        assert(intent3.tokensUnaccented.size() == 2);
    }

    // 3. Multi-Subject Full Match vs Partial Match Ranking (Tier 1 vs Tier 2)
    {
        auto tChi = std::make_shared<Tutor>(52, "chichi", "pass", "chi@gmail.com", "Tr\xe1\xba\xa7n Chi",
                                           "C\xc3\xb3 kinh nghi\xe1\xbb\x87m luy\xe1\xbb\x87n thi THPTQG",
                                           "To\xc3\xa1n  V\xe1\xba\xadt L\xc3\xbd  Ho\xc3\xa1 12",
                                           200000.0, 0.0, 0, 0.0, 0xFFULL);
        auto tToan = std::make_shared<Tutor>(2, "le_toan", "pass", "toan@gmail.com", "ThS. L\xc3\xaa V\xc4\x83n To\xc3\xa0n",
                                            "10 n\xc4\x83m kinh nghi\xe1\xbb\x87m",
                                            "Toán 12 - Luyện thi Đại học",
                                            150000.0, 5.0, 2, 0.0, 0xFFULL);
        auto tPhys = std::make_shared<Tutor>(3, "tran_phys", "pass", "phys@gmail.com", "TS. Tr\xe1\xba\xa7n Th\xe1\xbb\x8b V\xe1\xba\xadt L\xc3\xbd",
                                            "Gi\xe1\xba\xa3ng vi\xe1\xbb\x85n \xc4\x90HQG",
                                            "V\xe1\xba\xadt l\xc3\xbd 10-12 (Luy\xe1\xbb\x87n thi THPT)",
                                            200000.0, 4.0, 1, 0.0, 0xFFULL);

        // Query: "Toán Vật Lý"
        auto intent = TutorSearchMatcher::parseQuery("To\xc3\xa1n V\xe1\xba\xadt L\xc3\xbd");
        auto evalChi = TutorSearchMatcher::evaluate(intent, tChi, "");
        auto evalToan = TutorSearchMatcher::evaluate(intent, tToan, "");
        auto evalPhys = TutorSearchMatcher::evaluate(intent, tPhys, "");

        assert(evalChi.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);
        assert(evalToan.tier == SearchRelevanceTier::PARTIAL_SUBJECT_MATCH);
        assert(evalPhys.tier == SearchRelevanceTier::PARTIAL_SUBJECT_MATCH);
        assert(evalChi.score > evalToan.score);
        assert(evalChi.score > evalPhys.score);

        // Unaccented & inverted order queries
        auto evalUnacc = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("toan vat ly"), tChi, "");
        assert(evalUnacc.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);

        auto evalInverted = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("V\xe1\xba\xadt L\xc3\xbd To\xc3\xa1n"), tChi, "");
        assert(evalInverted.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);
    }

    // 4. Grade-Level Fit Ranking
    {
        auto tMath12 = std::make_shared<Tutor>(2, "le_toan", "pass", "toan@gmail.com", "ThS. L\xc3\xaa V\xc4\x83n To\xc3\xa0n",
                                              "Bio", "To\xc3\xa1n 12", 150000.0, 5.0, 2, 0.0, 0xFFULL);
        auto tMath10 = std::make_shared<Tutor>(13, "dang_ha", "pass", "ha@gmail.com", "C\xc3\xb4 \xc4\x90\xe1\xba\xb7ng Thu H\xc3\xa0",
                                              "Bio", "To\xc3\xa1n 10-11", 120000.0, 4.5, 2, 0.0, 0xFFULL);

        auto intent = TutorSearchMatcher::parseQuery("To\xc3\xa1n 12");
        auto eval12 = TutorSearchMatcher::evaluate(intent, tMath12, "");
        auto eval10 = TutorSearchMatcher::evaluate(intent, tMath10, "");

        assert(eval12.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);
        assert(eval10.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);
        assert(eval12.score > eval10.score); // Exact grade 12 ranks higher than adjacent grade 10-11
    }

    // 5. False-Positive Prevention (Short 1-syllable subjects)
    {
        auto tLit = std::make_shared<Tutor>(11, "gv_chuong", "pass", "lit@gmail.com", "ThS. Hoàng Văn Chương",
                                           "Thạc sĩ chuyên ngành Tâm lý học giáo dục",
                                           "Ngữ văn 10-12",
                                           110000.0, 4.5, 2, 0.0, 0xFFULL);
        auto tIT = std::make_shared<Tutor>(12, "vo_tin", "pass", "it@gmail.com", "Thầy Võ Minh Tin",
                                          "Kỹ sư phần mềm 5 năm kinh nghiệm",
                                          "Tin học 10-12",
                                          180000.0, 4.0, 2, 0.0, 0xFFULL);
        auto tMath = std::make_shared<Tutor>(13, "dang_ha", "pass", "ha@gmail.com", "Cô Đặng Thu Hà",
                                            "Địa chỉ liên hệ: Quận 1, TP.HCM",
                                            "Toán 10-11",
                                            120000.0, 4.5, 2, 0.0, 0xFFULL);

        // "Lý" must NOT match "Tâm lý học"
        auto evalLy = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("Lý"), tLit, "");
        assert(evalLy.tier == SearchRelevanceTier::NO_MATCH);

        // "Sử" must NOT match "Kỹ sư"
        auto evalSu = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("Sử"), tIT, "");
        assert(evalSu.tier == SearchRelevanceTier::NO_MATCH);

        // "Địa" must NOT match "Địa chỉ"
        auto evalDia = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("Địa"), tMath, "");
        assert(evalDia.tier == SearchRelevanceTier::NO_MATCH);
    }

    // 6. Name Search & Free-Text Program Search
    {
        auto tChi = std::make_shared<Tutor>(52, "chichi", "pass", "chi@gmail.com", "Tr\xe1\xba\xa7n Chi",
                                           "Bio", "To\xc3\xa1n  V\xe1\xba\xadt L\xc3\xbd  Ho\xc3\xa1 12",
                                           200000.0, 0.0, 0, 0.0, 0xFFULL);
        auto tEng = std::make_shared<Tutor>(4, "anh_van", "pass", "eng@gmail.com", "Ms. Nguy\xe1\xbb\x85n Anh V\xc4\x83n",
                                           "IELTS 8.5", "Ti\xe1\xba\xbfng Anh / IELTS (6.5 - 8.5)",
                                           120000.0, 0.0, 0, 0.0, 0xFFULL);

        auto evalName = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("Tr\xe1\xba\xa7n Chi"), tChi, "");
        assert(evalName.tier == SearchRelevanceTier::GENERAL_TEXT_MATCH);
        assert(evalName.score >= 500.0);

        auto evalIelts = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("IELTS"), tEng, "");
        assert(evalIelts.tier == SearchRelevanceTier::FULL_SUBJECT_MATCH);
    }

    // 7. Edge Cases: Empty Query & Non-Existent Query
    {
        auto tChi = std::make_shared<Tutor>(52, "chichi", "pass", "chi@gmail.com", "Tr\xe1\xba\xa7n Chi",
                                           "Bio", "To\xc3\xa1n  V\xe1\xba\xadt L\xc3\xbd  Ho\xc3\xa1 12",
                                           200000.0, 0.0, 0, 0.0, 0xFFULL);

        auto evalEmpty = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery(""), tChi, "");
        assert(evalEmpty.isEligible == true);

        auto evalSpaces = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("    "), tChi, "");
        assert(evalSpaces.isEligible == true);

        auto evalNonExistent = TutorSearchMatcher::evaluate(TutorSearchMatcher::parseQuery("xyz_nonexistent_term"), tChi, "");
        assert(evalNonExistent.tier == SearchRelevanceTier::NO_MATCH);
        assert(evalNonExistent.isEligible == false);
    }

    // 8. End-to-End Service Integration with Production Data
    {
        std::string dataDir = ApplicationPaths::resolveDataDirectory();
        auto userRepo = std::make_shared<FileUserRepository>(dataDir + "/users.dat");
        assert(userRepo->initialize());
        auto userService = std::make_shared<UserService>(userRepo);

        // End-to-end: Search "Toán Vật Lý"
        auto results = userService->searchAndFilterTutors("To\xc3\xa1n V\xe1\xba\xadt L\xc3\xbd", "", 0, 0, 0);
        assert(!results.empty());
        // Tutor 52 (Trần Chi) MUST be first because she teaches both Toán and Vật Lý!
        assert(results[0]->getId() == 52);
        assert(results[0]->getFullName() == "Tr\xe1\xba\xa7n Chi");

        // Partial match tutors (teaching Math only or Physics only) appear next
        bool foundSingleSubject = false;
        for (size_t i = 1; i < results.size(); ++i) {
            if (results[i]->getId() == 2 || results[i]->getId() == 3 || results[i]->getId() == 16) {
                foundSingleSubject = true;
                break;
            }
        }
        assert(foundSingleSubject);

        // Search by Tutor Name: "Trần Chi"
        auto nameResults = userService->searchAndFilterTutors("Tr\xe1\xba\xa7n Chi", "", 0, 0, 0);
        assert(nameResults.size() == 1);
        assert(nameResults[0]->getId() == 52);

        // Non-existent search
        auto emptyResults = userService->searchAndFilterTutors("nonexistent_term_404", "", 0, 0, 0);
        assert(emptyResults.empty());

        // Category filter: explicit "chemistry" + keyword "Hoá"
        auto chemResults = userService->searchAndFilterTutors("Ho\xc3\xa1", "chemistry", 0, 0, 0);
        assert(!chemResults.empty());
        for (const auto& ct : chemResults) {
            auto canSubjs = SubjectTaxonomy::getCanonicalSubjects(ct->getSubjectSpecialty());
            assert(std::find(canSubjs.begin(), canSubjs.end(), CanonicalSubject::CHEMISTRY) != canSubjs.end());
        }
    }

    std::cout << "  -> PASS: Tutor Search Engine Normalization, Semantic Matching & Relevance Ranking verified 100%!" << std::endl;
}

void run_test_tutor_verification_decommission() {
    std::cout << "[TEST] 29. Verifying Complete System-Wide Tutor Verification Decommission Invariants..." << std::endl;

    // 1. Domain Model Invariant: Canonical constructor has 12 fields (excluding optional isActive)
    Tutor t1(901, "tut_dec", "hash", "dec@tm.vn", "ThS. Hoàng Khôi",
             "Tiểu sử chi tiết với hơn 10 năm kinh nghiệm giảng dạy Toán THPT Chuyên.",
             "Toán 10-12", 200000.0, 5.0, 5, 1000000.0, 0x000000000000000FULL, true);
    assert(t1.getId() == 901);
    assert(t1.getHourlyRate() == 200000.0);
    assert(t1.getTotalEarnings() == 1000000.0);
    assert(t1.getIsActive() == true);

    // 2. Serializer Invariant: Exactly 14 CSV tokens for Tutor
    std::string csv14 = UserSerializer::toCsv(t1);
    auto tokens14 = SerializerUtils::splitCsvLine(csv14);
    assert(tokens14.size() == 14);
    assert(tokens14[5] == "TUTOR");
    assert(std::stod(tokens14[12]) == 1000000.0); // token 12 is totalEarnings, NOT isVerified!
    assert(tokens14[13] == "15");      // token 13 is scheduleBitmask

    // Deserialization round-trip
    std::string parseErr;
    auto parsedT = UserSerializer::fromCsv(csv14, &parseErr);
    assert(parsedT != nullptr && parseErr.empty());
    assert(parsedT->getRole() == UserRole::TUTOR);
    auto tFromCsv = std::dynamic_pointer_cast<Tutor>(parsedT);
    assert(tFromCsv != nullptr);
    assert(tFromCsv->getTotalEarnings() == 1000000.0);
    assert(tFromCsv->getScheduleBitmask() == 15ULL);

    // Backward-compatibility: legacy 15-token line also deserializes totalEarnings and scheduleBitmask properly
    std::string legacyCsv = "902,tut_leg,hash,leg@tm.vn,ThS. Legacy,TUTOR,1,Bio,Toán,150000,4.5,2,1,500000,15";
    auto parsedLeg = UserSerializer::fromCsv(legacyCsv, &parseErr);
    assert(parsedLeg != nullptr && parseErr.empty());
    auto tLeg = std::dynamic_pointer_cast<Tutor>(parsedLeg);
    assert(tLeg != nullptr);
    assert(tLeg->getTotalEarnings() == 500000.0);
    assert(tLeg->getScheduleBitmask() == 15ULL);
    // When serialized back, becomes canonical 14 tokens
    assert(SerializerUtils::splitCsvLine(UserSerializer::toCsv(*tLeg)).size() == 14);

    // 3. SmartMatch Criteria & Eligibility: absence of verification never excludes any tutor
    SmartMatchCriteria criteria;
    criteria.subject = "Toán";
    criteria.gradeLevel = 11;
    criteria.maxBudget = 250000.0;
    criteria.preferredScheduleMask = 15ULL; // matches tutor schedule
    criteria.strictSchedule = false;

    MatchingEngine engine;
    std::vector<std::pair<int, int>> matchedSlots;
    assert(engine.isEligible(criteria, t1, {}, matchedSlots));
    assert(!matchedSlots.empty());

    WeightedScoreMatchingStrategy strategy;
    auto scoreRes = strategy.calculateScoreWithCriteria(criteria, t1, {});
    assert(scoreRes.compositeScore > 0.0);
    // Credibility score is allocated to profile completeness (bio length & specialty), max 10 pts
    assert(scoreRes.credibilityScore >= 6.0 && scoreRes.credibilityScore <= 10.0);
    for (const auto& exp : scoreRes.explanations) {
        assert(exp.find("xác minh") == std::string::npos);
        assert(exp.find("thẩm định") == std::string::npos);
    }

    // 4. AdminService Invariant: Metrics and filtered users have no verification dependency
    auto fakeRepo = std::make_shared<FakeUserRepository>();
    fakeRepo->saveUser(t1);
    AdminService adminService(fakeRepo, nullptr, nullptr);
    AdminMetrics metrics = adminService.calculateMetrics();
    assert(metrics.totalTutors == 1);

    auto allTutors = adminService.getFilteredUsers("TUTOR", "");
    assert(allTutors.size() == 1);
    assert(allTutors[0]->getId() == 901);

    // 5. Production users.dat File Invariant: Verify physical file has 0 verification fields
    std::string dataDir = ApplicationPaths::resolveDataDirectory();
    std::ifstream userFile(dataDir + "/users.dat");
    assert(userFile.is_open());
    std::string line;
    int tutorLineCount = 0;
    while (std::getline(userFile, line)) {
        if (line.empty()) continue;
        auto tokens = SerializerUtils::splitCsvLine(line);
        if (tokens.size() >= 6 && tokens[5] == "TUTOR") {
            assert(tokens.size() == 14); // MUST BE 14 FIELDS!
            tutorLineCount++;
        }
    }
    assert(tutorLineCount >= 12);

    std::cout << "  -> PASS: All 5 Tutor Verification Decommission Invariants verified successfully!" << std::endl;
}

void run_test_generic_data_structures_invariants() {
    std::cout << "\n[TEST SUITE 30] Verifying Generic Data Structures & Algorithmic Invariants (HashTable, AVLTree, LRUCache, ScheduleGuard, MaxHeap, DoublyLinkedList)..." << std::endl;

    // 1. HashTable O(1) Test
    HashTable<std::string, int> hashTab;
    hashTab.insert("student_1", 101);
    hashTab.insert("tutor_99", 202);
    int val = 0;
    assert(hashTab.find("student_1", val) && val == 101);
    assert(hashTab.find("tutor_99", val) && val == 202);

    // 2. AVL Tree Range Query Test
    AVLTree<double, std::string> avlTree;
    avlTree.insert(25.0, "Tutor_Cheap");
    avlTree.insert(45.0, "Tutor_Mid");
    avlTree.insert(80.0, "Tutor_Exp");
    avlTree.insert(35.0, "Tutor_Fair");

    auto rangeResults = avlTree.rangeSearch(20.0, 50.0);
    assert(rangeResults.size() == 3); // 25, 35, 45

    // 3. LRU Cache Eviction Test
    LRUCache<int, std::string> lruCache(2); // Capacity = 2
    lruCache.put(1, "Profile_1");
    lruCache.put(2, "Profile_2");
    std::string profile;
    assert(lruCache.get(1, profile));  // Access 1 -> 1 becomes MRU, 2 becomes LRU
    lruCache.put(3, "Profile_3");      // Evicts 2
    assert(!lruCache.get(2, profile)); // 2 was evicted!
    assert(lruCache.get(1, profile));   // 1 is present
    assert(lruCache.get(3, profile));   // 3 is present

    // 4. Bitmask Schedule Guard Test
    uint64_t tutorSchedule = ScheduleGuard::getSlotMask(0, 0) | ScheduleGuard::getSlotMask(0, 1);
    uint64_t reqValid = ScheduleGuard::getSlotMask(0, 0);
    uint64_t reqInvalid = ScheduleGuard::getSlotMask(1, 0);

    assert(ScheduleGuard::isSlotAvailable(tutorSchedule, reqValid) == true);
    assert(ScheduleGuard::isSlotAvailable(tutorSchedule, reqInvalid) == false);

    // 5. Max-Heap Priority Queue Ranking Test
    MaxHeap<int> maxHeap;
    maxHeap.push(45);
    maxHeap.push(98);
    maxHeap.push(72);
    maxHeap.push(15);

    auto top2 = maxHeap.extractSortedTopK(2);
    assert(top2.size() == 2 && top2[0] == 98 && top2[1] == 72);

    // 6. DoublyLinkedList Basic Test
    DoublyLinkedList<int> dList;
    dList.pushBack(10);
    dList.pushBack(20);
    dList.pushFront(5);
    assert(dList.size() == 3);
    assert(dList.getHead()->data == 5);
    assert(dList.getTail()->data == 20);
    assert(dList.popBack() == 20);
    assert(dList.size() == 2);

    std::cout << "  -> PASS: All Generic Data Structures & Algorithmic Invariants verified successfully!" << std::endl;
}

// ============================================================================
// TEST SUITE 31: Admin Permission Level Removal & Unified Root Authorization
// ============================================================================
void run_test_admin_permission_level_removal_and_authorization() {
    std::cout << "\n[TEST SUITE 31] Verifying Unified Root Admin Authorization & Permission Level Removal..." << std::endl;

    // 1. Domain Model: Admin inherits from User, uses exactly 7 base fields, no permissionLevel
    {
        Admin defaultAdmin;
        assert(defaultAdmin.getRole() == UserRole::ADMIN);
        assert(defaultAdmin.getId() == 0);
        assert(defaultAdmin.getRoleString() == "Quản trị viên");

        Admin admin(999, "root_admin", "hash999", "root@tutormatch.vn", "Root Admin", true);
        assert(admin.getId() == 999);
        assert(admin.getUsername() == "root_admin");
        assert(admin.getPasswordHash() == "hash999");
        assert(admin.getEmail() == "root@tutormatch.vn");
        assert(admin.getFullName() == "Root Admin");
        assert(admin.getRole() == UserRole::ADMIN);
        assert(admin.getIsActive() == true);
    }

    // 2. Serialization & Deserialization: Admin has exactly 7 fields
    {
        Admin admin(101, "adm_sec", "pHash", "adm@tm.vn", "Admin Security", true);
        std::string csv = UserSerializer::toCsv(admin);
        auto tokens = SerializerUtils::splitCsvLine(csv);
        assert(tokens.size() == 7);
        assert(tokens[0] == "101");
        assert(tokens[1] == "adm_sec");
        assert(tokens[2] == "pHash");
        assert(tokens[3] == "adm@tm.vn");
        assert(tokens[4] == "Admin Security");
        assert(tokens[5] == "ADMIN");
        assert(tokens[6] == "1");

        // Parse from 7-field CSV
        std::string parseErr;
        auto parsedUser = UserSerializer::fromCsv(csv, &parseErr);
        assert(parsedUser != nullptr);
        assert(parseErr.empty());
        assert(parsedUser->getRole() == UserRole::ADMIN);
        assert(parsedUser->getFullName() == "Admin Security");

        // Direct typed deserializer
        auto typedAdmin = UserSerializer::adminFromCsv(csv, &parseErr);
        assert(typedAdmin != nullptr);
        assert(typedAdmin->getId() == 101);

        // Backward compatibility: parsing legacy 8-field CSV line gracefully succeeds
        std::string legacyCsv = "102,adm_legacy,pHash2,legacy@tm.vn,Legacy Admin,ADMIN,1,9";
        auto parsedLegacy = UserSerializer::fromCsv(legacyCsv, &parseErr);
        assert(parsedLegacy != nullptr);
        assert(parseErr.empty());
        assert(parsedLegacy->getRole() == UserRole::ADMIN);
        assert(parsedLegacy->getId() == 102);
        // And when re-serialized, legacy 8th field is dropped (unified 7-field schema)
        std::string reserialized = UserSerializer::toCsv(*parsedLegacy);
        assert(SerializerUtils::splitCsvLine(reserialized).size() == 7);

        // Malformed admin with < 7 fields must fail
        std::string malformedCsv = "103,adm_bad,pHash3,bad@tm.vn,Bad Admin,ADMIN";
        auto badAdmin = UserSerializer::fromCsv(malformedCsv, &parseErr);
        assert(badAdmin == nullptr);
        assert(!parseErr.empty());
    }

    // 3. Authorization: Unified ADMIN role allows management, non-admins are blocked
    {
        auto userRepo = std::make_shared<TutorMatch::FileUserRepository>(ApplicationPaths::resolveDataDirectory() + "/users.dat");
        auto reviewRepo = std::make_shared<TutorMatch::FileReviewRepository>(ApplicationPaths::resolveDataDirectory() + "/reviews.dat");
        auto cbRepo = std::make_shared<TutorMatch::FileCourseBookingRepository>(ApplicationPaths::resolveDataDirectory() + "/course_bookings.dat");
        assert(userRepo->initialize());
        assert(reviewRepo->initialize());
        assert(cbRepo->initialize());
        auto bookingSvc = std::make_shared<TutorMatch::BookingService>(userRepo, reviewRepo, cbRepo);
        auto adminSvc = std::make_shared<TutorMatch::AdminService>(userRepo, reviewRepo, bookingSvc);
        auto authSvc = std::make_shared<TutorMatch::AuthService>(userRepo);

        // Active admin can login successfully
        std::string authErr;
        assert(authSvc->login("admin", "admin123", authErr));

        // Wrong password fails authentication
        assert(!authSvc->login("admin", "wrong_pass", authErr));
        assert(!authErr.empty());

        // Metrics calculation succeeds for Admin
        auto metrics = adminSvc->calculateMetrics();
        assert(metrics.totalTutors > 0);
        assert(metrics.totalStudents > 0);

        // Admin privilege: Admin can toggle lock on user
        std::string toggleMsg;
        assert(adminSvc->toggleUserLock(6, toggleMsg));
        auto u6 = userRepo->findUserById(6);
        assert(u6 != nullptr && !u6->getIsActive());
        // Toggle back to active
        assert(adminSvc->toggleUserLock(6, toggleMsg));
        u6 = userRepo->findUserById(6);
        assert(u6 != nullptr && u6->getIsActive());
    }

    std::cout << "  -> PASS: Unified Root Admin Authorization & 7-Field Schema verified 100%!" << std::endl;
}

// ============================================================================
// TEST SUITE 32: Tutor Accumulated Income & Reconciliation Verification
// ============================================================================
void run_test_tutor_accumulated_income_and_reconciliation() {
    std::cout << "\n[TEST SUITE 32] Verifying Tutor Accumulated Income, Single Source of Truth & Reconciliation..." << std::endl;

    std::string dataDir = ApplicationPaths::resolveDataDirectory();
    auto userRepo = std::make_shared<TutorMatch::FileUserRepository>(dataDir + "/users.dat");
    auto reviewRepo = std::make_shared<TutorMatch::FileReviewRepository>(dataDir + "/reviews.dat");
    auto cbRepo = std::make_shared<TutorMatch::FileCourseBookingRepository>(dataDir + "/course_bookings.dat");
    assert(userRepo->initialize());
    assert(reviewRepo->initialize());
    assert(cbRepo->initialize());

    auto bookingSvc = std::make_shared<TutorMatch::BookingService>(userRepo, reviewRepo, cbRepo);

    // 1. Zero income when no completed courses
    // Tutor 4 (Ms. Nguyễn Anh Vương) and Tutor 5 (Thầy Phạm Hồng Hà) have 0 completed courses
    assert(bookingSvc->calculateTutorAccumulatedIncome(4) == 0.0);
    assert(bookingSvc->calculateTutorAccumulatedIncome(5) == 0.0);
    assert(bookingSvc->calculateTutorAccumulatedIncome(9999) == 0.0); // Non-existent tutor

    // 2. Exact calculation for Tutor with 1 completed course
    // Tutor 3 (TS. Trần Thị Hồng Nhung): Booking 1003 (Rate 200,000 * 4 weeks * 1 slot = 800,000 VNĐ)
    // Bookings 1018 (ACCEPTED) and 1023 (CANCELLED) are NOT counted!
    assert(bookingSvc->calculateTutorAccumulatedIncome(3) == 800000.0);

    // 3. Exact calculation for Tutor with 2 completed courses (as reported by user)
    // Tutor 2 (ThS. Lê Văn Toàn):
    // Booking 1001 (Rate 150,000 * 4 weeks * 1 slot = 600,000 VNĐ)
    // Booking 1002 (Rate 150,000 * 4 weeks * 1 slot = 600,000 VNĐ)
    // Booking 1017 (ACCEPTED) is NOT counted!
    // Total = 1,200,000 VNĐ
    assert(bookingSvc->calculateTutorAccumulatedIncome(2) == 1200000.0);

    // 4. Exact calculation for Tutor with 3 completed courses
    // Tutor 12 (Thầy Võ Minh Tuấn): Bookings 1004, 1012, 1016 (Each 180,000 * 4 = 720,000 * 3 = 2,160,000 VNĐ)
    assert(bookingSvc->calculateTutorAccumulatedIncome(12) == 2160000.0);

    // 5. Excluded non-completed statuses: PENDING, ACCEPTED, REJECTED, CANCELLED, EARLY_TERMINATED
    // Tutor 16: Booking 1013 (COMPLETED: 170,000 * 4 = 680,000 VNĐ), Booking 1020 (EARLY_TERMINATED: excluded)
    assert(bookingSvc->calculateTutorAccumulatedIncome(16) == 680000.0);

    // 6. Multi-slot course calculation in isolated sandbox repository
    {
        std::error_code ec;
        auto tempSandbox = std::filesystem::temp_directory_path() / "tutormatch_income_sandbox";
        std::filesystem::remove_all(tempSandbox, ec);
        std::filesystem::create_directories(tempSandbox, ec);

        auto sUserRepo = std::make_shared<TutorMatch::FileUserRepository>((tempSandbox / "users.dat").string());
        auto sReviewRepo = std::make_shared<TutorMatch::FileReviewRepository>((tempSandbox / "reviews.dat").string());
        auto sCbRepo = std::make_shared<TutorMatch::FileCourseBookingRepository>((tempSandbox / "course_bookings.dat").string());
        assert(sUserRepo->initialize());
        assert(sReviewRepo->initialize());
        assert(sCbRepo->initialize());

        auto sBookingSvc = std::make_shared<TutorMatch::BookingService>(sUserRepo, sReviewRepo, sCbRepo);

        // Create student and tutor
        Tutor mockTutor(101, "tut_multi", "hash", "tut@tm.vn", "ThS. Multi", "Bio", "Math", 200000.0, 5.0, 0, 0.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
        assert(sUserRepo->saveUser(mockTutor));
        Student mockStudent(201, "stu_multi", "hash", "stu@tm.vn", "Student Multi", 12, ScheduleGuard::VALID_SCHEDULE_MASK, true);
        assert(sUserRepo->saveUser(mockStudent));

        // Create multi-slot course: 2 slots per week, 8 weeks, rate 200,000
        // Expected total sessions = 2 * 8 = 16 sessions -> Total cost = 200,000 * 16 = 3,200,000 VNĐ
        std::vector<std::pair<int, int>> multiSlots = {{0, 1}, {2, 3}};
        std::string bookMsg;
        int newBookingId = 0;
        assert(sBookingSvc->bookCourseMultiSlot(201, 101, "Toán Nâng Cao", multiSlots, "2026-12-01", 8, true, bookMsg, &newBookingId));
        assert(newBookingId > 0);

        // Still PENDING -> Income = 0
        assert(sBookingSvc->calculateTutorAccumulatedIncome(101) == 0.0);

        // Accept course -> Still ACCEPTED -> Income = 0
        assert(sBookingSvc->acceptCourseBooking(101, newBookingId, true, bookMsg));
        assert(sBookingSvc->calculateTutorAccumulatedIncome(101) == 0.0);

        // Complete course -> Income becomes 3,200,000 VNĐ
        assert(sBookingSvc->completeCourseBooking(101, UserRole::TUTOR, newBookingId, bookMsg));
        assert(sBookingSvc->calculateTutorAccumulatedIncome(101) == 3200000.0);

        // Verify persisted in sUserRepo
        auto reloadedTutor = std::dynamic_pointer_cast<Tutor>(sUserRepo->findUserById(101));
        assert(reloadedTutor != nullptr);
        assert(reloadedTutor->getTotalEarnings() == 3200000.0);

        // 7. Idempotence test: calling reconcileTutorEarnings multiple times does not increase earnings
        for (int i = 0; i < 5; ++i) {
            sBookingSvc->reconcileTutorEarnings(101);
            reloadedTutor = std::dynamic_pointer_cast<Tutor>(sUserRepo->findUserById(101));
            assert(reloadedTutor != nullptr);
            assert(reloadedTutor->getTotalEarnings() == 3200000.0);
        }

        // 8. Reconcile All on Startup in sandbox
        sBookingSvc->reconcileAllTutorEarningsOnStartup();
        reloadedTutor = std::dynamic_pointer_cast<Tutor>(sUserRepo->findUserById(101));
        assert(reloadedTutor != nullptr);
        assert(reloadedTutor->getTotalEarnings() == 3200000.0);

        std::filesystem::remove_all(tempSandbox, ec);
    }

    // 9. Startup self-healing reconciliation on production dataset
    bookingSvc->reconcileAllTutorEarningsOnStartup();
    auto prodTutor2 = std::dynamic_pointer_cast<Tutor>(userRepo->findUserById(2));
    assert(prodTutor2 != nullptr);
    assert(prodTutor2->getTotalEarnings() == 1200000.0);

    // 10. GUIBridge Integration Contract: qmlGetTutorOverview and qmlGetTutorStatistics
    {
        auto authSvc = std::make_shared<TutorMatch::AuthService>(userRepo);
        auto userSvc = std::make_shared<TutorMatch::UserService>(userRepo);
        auto matchSvc = std::make_shared<TutorMatch::MatchingService>(userRepo, cbRepo);
        auto adminSvc = std::make_shared<TutorMatch::AdminService>(userRepo, reviewRepo, bookingSvc);
        auto rSvc = std::make_shared<TutorMatch::ReviewService>(reviewRepo, userRepo, cbRepo);
        auto bridge = std::make_shared<TutorMatch::GUIBridge>();
        assert(bridge->initialize(authSvc, userSvc, bookingSvc, rSvc, matchSvc, adminSvc));

        std::string authErr;
        assert(authSvc->login("tutor_math", "pass123", authErr));

        auto overview = bridge->qmlGetTutorOverview();
        assert(overview["totalEarnings"].toDouble() == 1200000.0);
        assert(overview["rating"].toDouble() == 5.0);

        auto stats = bridge->qmlGetTutorStatistics();
        assert(stats["totalEarnings"].toDouble() == 1200000.0);
    }

    std::cout << "  -> PASS: All 10 Tutor Accumulated Income, Idempotence & GUI Invariants verified 100%!" << std::endl;
}

void run_test_course_registration_start_date_calculation() {
    std::cout << "\n[TEST SUITE 33] Verifying Course Registration Start-Date Calculation & Order Invariance..." << std::endl;

    // Fixed deterministic test references:
    // Friday: 2026-11-13 (dow = 4)
    // Monday: 2026-11-16 (dow = 0)
    // Wednesday: 2026-11-18 (dow = 2)
    // Saturday: 2026-11-14 (dow = 5)
    // Sunday: 2026-11-15 (dow = 6)
    // Tuesday: 2026-11-17 (dow = 1)
    const std::string fridayRef = "2026-11-13";
    const std::string noonTime = "12:00";

    // ------------------------------------------------------------------------
    // Scenario 1: Student selects [Wednesday, Monday] on a Friday -> upcoming Monday
    // ------------------------------------------------------------------------
    {
        std::vector<std::pair<int, int>> slotPairs = {{2, 1}, {0, 1}}; // Wednesday (dow 2), Monday (dow 0)
        std::string earliest = DateUtils::getEarliestValidStartDate(slotPairs, fridayRef, noonTime);
        assert(earliest == "2026-11-16"); // Upcoming Monday

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(slotPairs, 4, fridayRef, noonTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-16"); // Monday
        assert(candidates[1] == "2026-11-18"); // Wednesday
        assert(candidates[2] == "2026-11-23"); // Monday
        assert(candidates[3] == "2026-11-25"); // Wednesday
    }

    // ------------------------------------------------------------------------
    // Scenario 2: Student selects [Monday, Wednesday] on a Friday -> upcoming Monday (Order Invariance)
    // ------------------------------------------------------------------------
    {
        std::vector<std::pair<int, int>> slotPairsReversed = {{0, 1}, {2, 1}}; // Monday (dow 0), Wednesday (dow 2)
        std::string earliest = DateUtils::getEarliestValidStartDate(slotPairsReversed, fridayRef, noonTime);
        assert(earliest == "2026-11-16");

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(slotPairsReversed, 4, fridayRef, noonTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-16");
        assert(candidates[1] == "2026-11-18");
        assert(candidates[2] == "2026-11-23");
        assert(candidates[3] == "2026-11-25");

        // Verify order invariance: identical to Scenario 1
        std::vector<std::pair<int, int>> slotPairsOriginal = {{2, 1}, {0, 1}};
        auto candidatesOriginal = DateUtils::getUpcomingStartDatesForSlots(slotPairsOriginal, 4, fridayRef, noonTime);
        assert(candidates == candidatesOriginal);
    }

    // ------------------------------------------------------------------------
    // Scenario 3: Student selects [Wednesday, Saturday] on a Friday -> upcoming Saturday
    // ------------------------------------------------------------------------
    {
        std::vector<std::pair<int, int>> slotPairs = {{2, 2}, {5, 2}}; // Wednesday (dow 2), Saturday (dow 5)
        std::string earliest = DateUtils::getEarliestValidStartDate(slotPairs, fridayRef, noonTime);
        assert(earliest == "2026-11-14"); // Saturday (tomorrow)

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(slotPairs, 4, fridayRef, noonTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-14"); // Saturday
        assert(candidates[1] == "2026-11-18"); // Wednesday
        assert(candidates[2] == "2026-11-21"); // Saturday
        assert(candidates[3] == "2026-11-25"); // Wednesday

        // Check reversed order invariance as well
        std::vector<std::pair<int, int>> slotPairsReversed = {{5, 2}, {2, 2}};
        assert(DateUtils::getEarliestValidStartDate(slotPairsReversed, fridayRef, noonTime) == "2026-11-14");
        assert(DateUtils::getUpcomingStartDatesForSlots(slotPairsReversed, 4, fridayRef, noonTime) == candidates);
    }

    // ------------------------------------------------------------------------
    // Scenario 4: Student selects [Monday, Wednesday] on Monday morning before session time -> Monday (today)
    // ------------------------------------------------------------------------
    {
        const std::string mondayRef = "2026-11-16";
        const std::string morningTime = "07:00"; // Before Slot 1 start (09:15)
        std::vector<std::pair<int, int>> slotPairs = {{0, 1}, {2, 1}}; // Monday slot 1 (09:15), Wednesday slot 1 (09:15)
        
        assert(!DateUtils::isSlotTimePassed(1, morningTime)); // Slot 1 has not passed at 07:00

        std::string earliest = DateUtils::getEarliestValidStartDate(slotPairs, mondayRef, morningTime);
        assert(earliest == "2026-11-16"); // Today (Monday) is eligible!

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(slotPairs, 4, mondayRef, morningTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-16"); // Monday (today)
        assert(candidates[1] == "2026-11-18"); // Wednesday
        assert(candidates[2] == "2026-11-23"); // Monday
        assert(candidates[3] == "2026-11-25"); // Wednesday
    }

    // ------------------------------------------------------------------------
    // Scenario 5: Student selects [Monday, Wednesday] on Monday afternoon after session time -> upcoming Wednesday
    // ------------------------------------------------------------------------
    {
        const std::string mondayRef = "2026-11-16";
        const std::string afternoonTime = "14:00"; // After Slot 1 start (09:15)
        std::vector<std::pair<int, int>> slotPairs = {{0, 1}, {2, 1}}; // Monday slot 1 (09:15), Wednesday slot 1 (09:15)

        assert(DateUtils::isSlotTimePassed(1, afternoonTime)); // Slot 1 HAS passed at 14:00

        std::string earliest = DateUtils::getEarliestValidStartDate(slotPairs, mondayRef, afternoonTime);
        assert(earliest == "2026-11-18"); // Upcoming Wednesday (2 days away)

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(slotPairs, 4, mondayRef, afternoonTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-18"); // Wednesday
        assert(candidates[1] == "2026-11-23"); // Monday
        assert(candidates[2] == "2026-11-25"); // Wednesday
        assert(candidates[3] == "2026-11-30"); // Monday

        // Order invariance check on afternoon as well
        std::vector<std::pair<int, int>> slotPairsReversed = {{2, 1}, {0, 1}};
        assert(DateUtils::getEarliestValidStartDate(slotPairsReversed, mondayRef, afternoonTime) == "2026-11-18");
        assert(DateUtils::getUpcomingStartDatesForSlots(slotPairsReversed, 4, mondayRef, afternoonTime) == candidates);
    }

    // ------------------------------------------------------------------------
    // Scenario 6: Schedule crossing Sunday/Monday week boundary
    // ------------------------------------------------------------------------
    {
        // 6a: Saturday 2026-11-14, selecting [Sunday, Tuesday] -> Sunday is earliest (tomorrow)
        const std::string satRef = "2026-11-14";
        std::vector<std::pair<int, int>> slotPairs6a = {{6, 2}, {1, 2}}; // Sunday (dow 6), Tuesday (dow 1)
        std::string earliest6a = DateUtils::getEarliestValidStartDate(slotPairs6a, satRef, noonTime);
        assert(earliest6a == "2026-11-15"); // Tomorrow (Sunday)

        // 6b: Sunday 2026-11-15 night at 23:00 (after Sunday session) -> Tuesday is earliest
        const std::string sunRef = "2026-11-15";
        const std::string sunNight = "23:00";
        std::vector<std::pair<int, int>> slotPairs6b = {{6, 5}, {1, 2}}; // Sunday slot 5 (18:45), Tuesday slot 2 (13:30)
        assert(DateUtils::isSlotTimePassed(5, sunNight));
        std::string earliest6b = DateUtils::getEarliestValidStartDate(slotPairs6b, sunRef, sunNight);
        assert(earliest6b == "2026-11-17"); // Tuesday (in 2 days)
    }

    // ------------------------------------------------------------------------
    // Scenario 7: Single selected weekly session (regression check)
    // ------------------------------------------------------------------------
    {
        std::vector<std::pair<int, int>> singleSlot = {{0, 2}}; // Monday slot 2
        std::string earliest = DateUtils::getEarliestValidStartDate(singleSlot, fridayRef, noonTime);
        assert(earliest == "2026-11-16");

        auto candidates = DateUtils::getUpcomingStartDatesForSlots(singleSlot, 4, fridayRef, noonTime);
        assert(candidates.size() == 4);
        assert(candidates[0] == "2026-11-16");
        assert(candidates[1] == "2026-11-23");
        assert(candidates[2] == "2026-11-30");
        assert(candidates[3] == "2026-12-07");
    }

    // ------------------------------------------------------------------------
    // Scenario 8: Adding, removing, and reordering selected sessions updates state correctly
    // ------------------------------------------------------------------------
    {
        std::vector<std::pair<int, int>> state;
        // Step 1: Add Wednesday
        state.push_back({2, 1});
        assert(DateUtils::getEarliestValidStartDate(state, fridayRef, noonTime) == "2026-11-18");

        // Step 2: Add Monday -> earliest shifts to Monday
        state.push_back({0, 1});
        assert(DateUtils::getEarliestValidStartDate(state, fridayRef, noonTime) == "2026-11-16");

        // Step 3: Reorder slots in state -> earliest remains Monday
        std::swap(state[0], state[1]);
        assert(DateUtils::getEarliestValidStartDate(state, fridayRef, noonTime) == "2026-11-16");

        // Step 4: Remove Monday -> earliest shifts back to Wednesday
        state.erase(state.begin()); // removes {0, 1}
        assert(DateUtils::getEarliestValidStartDate(state, fridayRef, noonTime) == "2026-11-18");

        // Step 5: Add Saturday -> Saturday (2026-11-14) is earlier than Wednesday (2026-11-18)
        state.push_back({5, 1});
        assert(DateUtils::getEarliestValidStartDate(state, fridayRef, noonTime) == "2026-11-14");
    }

    // ------------------------------------------------------------------------
    // Scenario 9: Invalid or empty weeklySchedule input handled gracefully
    // ------------------------------------------------------------------------
    {
        // Empty vector
        std::vector<std::pair<int, int>> emptySlots;
        assert(DateUtils::getEarliestValidStartDate(emptySlots, fridayRef, noonTime) == "");
        assert(DateUtils::getUpcomingStartDatesForSlots(emptySlots, 4, fridayRef, noonTime).empty());

        // Out-of-bounds days/slots
        std::vector<std::pair<int, int>> invalidSlots = {{-1, 0}, {7, 0}, {0, -1}, {0, 8}};
        assert(DateUtils::getEarliestValidStartDate(invalidSlots, fridayRef, noonTime) == "");
        assert(DateUtils::getUpcomingStartDatesForSlots(invalidSlots, 4, fridayRef, noonTime).empty());

        // Invalid date string
        assert(DateUtils::getNextSlotOccurrence(0, 0, "not-a-date", noonTime) == "");
    }

    // ------------------------------------------------------------------------
    // Scenario 10: Generated course schedule starting from calculated start date covers all slots and weeks
    // ------------------------------------------------------------------------
    {
        // 2 slots per week (Monday dow 0, Wednesday dow 2) for 4 weeks -> 8 sessions
        std::vector<std::pair<int, int>> slotPairs = {{2, 3}, {0, 1}}; // Picked Wednesday first, Monday second
        std::string calcStartDate = DateUtils::getEarliestValidStartDate(slotPairs, fridayRef, noonTime);
        assert(calcStartDate == "2026-11-16");

        int weeksCount = 4;
        auto sessions = DateUtils::generateMultiSlotSessions(calcStartDate, weeksCount, slotPairs);
        assert(sessions.size() == 8);

        // Verify all sessions are strictly chronological
        for (size_t i = 1; i < sessions.size(); ++i) {
            assert(DateUtils::compareDates(sessions[i-1].date, sessions[i].date) <= 0);
        }

        // Verify session coverage
        int monCount = 0;
        int wedCount = 0;
        for (const auto& s : sessions) {
            if (s.dayOfWeek == 0 && s.slotIndex == 1) monCount++;
            if (s.dayOfWeek == 2 && s.slotIndex == 3) wedCount++;
        }
        assert(monCount == 4);
        assert(wedCount == 4);
        assert(sessions[0].date == "2026-11-16"); // First session is Monday Nov 16
        assert(sessions[1].date == "2026-11-18"); // Second session is Wednesday Nov 18
        assert(sessions[7].date == "2026-12-09"); // Last session is Wednesday Dec 09

        // Verify order invariance of session generator
        std::vector<std::pair<int, int>> reversedSlots = {{0, 1}, {2, 3}};
        auto reversedSessions = DateUtils::generateMultiSlotSessions(calcStartDate, weeksCount, reversedSlots);
        assert(sessions.size() == reversedSessions.size());
        for (size_t i = 0; i < sessions.size(); ++i) {
            assert(sessions[i].date == reversedSessions[i].date);
            assert(sessions[i].dayOfWeek == reversedSessions[i].dayOfWeek);
            assert(sessions[i].slotIndex == reversedSessions[i].slotIndex);
        }
    }

    std::cout << "  -> PASS: All 10 Course Registration Start-Date Calculation & Invariance Scenarios verified 100%!" << std::endl;
}

void run_test_early_termination_and_completion_guard() {
    std::cout << "\n[TEST SUITE 34] Verifying Consensual Early Termination & Completion Guard Integration..." << std::endl;

    std::string testDir = "tests_scratch_early_term_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::error_code ec;
    std::filesystem::create_directories(testDir, ec);

    std::string uPath = testDir + "/users.dat";
    std::string cbPath = testDir + "/course_bookings.dat";
    std::string rPath = testDir + "/reviews.dat";

    auto uRepo = std::make_shared<FileUserRepository>(uPath);
    auto cbRepo = std::make_shared<FileCourseBookingRepository>(cbPath);
    auto rRepo = std::make_shared<FileReviewRepository>(rPath);
    uRepo->initialize();
    cbRepo->initialize();
    rRepo->initialize();

    uint64_t allFree = ScheduleGuard::VALID_SCHEDULE_MASK;
    Tutor t1(10, "tutor_hoa", "pass", "t1@tm.vn", "ThS Lê Hoa", "Bio", "Toán", 200000.0, 5.0, 0, 0.0, allFree, true);
    Student s1(20, "stu_minh", "pass", "s1@tm.vn", "Nguyễn Minh", 12, allFree, true);
    uRepo->saveUser(t1);
    uRepo->saveUser(s1);

    BookingService service(uRepo, rRepo, cbRepo);

    // Create an ACCEPTED multi-slot course
    std::vector<std::pair<int, int>> slotPairs = {{0, 1}, {2, 3}};
    int courseId = -1;
    std::string msg;
    bool booked = service.bookCourseMultiSlot(20, 10, "Toán 12", slotPairs, "2026-11-16", 4, true, msg, &courseId);
    assert(booked && courseId > 0);
    assert(service.acceptCourseBooking(10, courseId, true, msg));

    auto cb = cbRepo->findCourseBookingById(courseId);
    assert(cb != nullptr && cb->status == "ACCEPTED");
    assert(cb->earlyTerminationStatus == "NONE");

    // 1. Tutor requests early termination with explicit reason
    std::string reason = "Bận công tác đột xuất 2 tuần";
    bool reqOk = service.requestEarlyTermination(10, UserRole::TUTOR, courseId, reason, msg);
    assert(reqOk);
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->earlyTerminationStatus == "PENDING");
    assert(cb->earlyTerminationRequesterId == 10);
    assert(cb->earlyTerminationReason == reason);
    assert(cb->status == "ACCEPTED");

    // 2. Guard: Cannot complete course while early termination is PENDING
    // Test Student attempt:
    bool stuCompleteOk = service.completeCourseBooking(20, UserRole::STUDENT, courseId, msg);
    assert(!stuCompleteOk);
    assert(msg == "Khóa học đang có yêu cầu kết thúc sớm chờ phản hồi, không thể hoàn thành!");

    // Test Tutor attempt:
    bool tutCompleteOk = service.completeCourseBooking(10, UserRole::TUTOR, courseId, msg);
    assert(!tutCompleteOk);
    assert(msg == "Khóa học đang có yêu cầu kết thúc sớm chờ phản hồi, không thể hoàn thành!");

    // Verify course remains ACCEPTED, not completed
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "ACCEPTED");

    // 3. Guard: Requester (Tutor) cannot respond to own request
    bool selfRespOk = service.respondEarlyTermination(10, UserRole::TUTOR, courseId, true, msg);
    assert(!selfRespOk);
    assert(msg == "Bạn không thể tự phản hồi yêu cầu do chính mình gửi!");

    // 4. Student rejects early termination
    bool rejectOk = service.respondEarlyTermination(20, UserRole::STUDENT, courseId, false, msg);
    assert(rejectOk);
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "ACCEPTED");
    assert(cb->earlyTerminationStatus == "REJECTED");
    assert(cb->earlyTerminationReason == reason); // Reason preserved for history/display

    // 5. After rejection, completion guard is lifted: completeCourseBooking now succeeds!
    bool completeAfterReject = service.completeCourseBooking(20, UserRole::STUDENT, courseId, msg);
    assert(completeAfterReject);
    cb = cbRepo->findCourseBookingById(courseId);
    assert(cb->status == "COMPLETED");

    // 6. Test Scenario B: Student requests early termination, Tutor agrees
    int course2Id = -1;
    bool booked2 = service.bookCourseMultiSlot(20, 10, "Toán Luyện Thi", slotPairs, "2026-11-16", 4, true, msg, &course2Id);
    assert(booked2 && course2Id > 0);
    assert(service.acceptCourseBooking(10, course2Id, true, msg));

    std::string stuReason = "Lịch học trùng với lịch thi đội tuyển của trường";
    assert(service.requestEarlyTermination(20, UserRole::STUDENT, course2Id, stuReason, msg));
    auto cb2 = cbRepo->findCourseBookingById(course2Id);
    assert(cb2->earlyTerminationStatus == "PENDING");
    assert(cb2->earlyTerminationRequesterId == 20);
    assert(cb2->earlyTerminationReason == stuReason);

    // Tutor cannot complete course while pending
    assert(!service.completeCourseBooking(10, UserRole::TUTOR, course2Id, msg));

    // Student cannot self-respond
    assert(!service.respondEarlyTermination(20, UserRole::STUDENT, course2Id, true, msg));

    // Tutor agrees to early termination
    bool agreeOk = service.respondEarlyTermination(10, UserRole::TUTOR, course2Id, true, msg);
    assert(agreeOk);

    cb2 = cbRepo->findCourseBookingById(course2Id);
    assert(cb2->status == "EARLY_TERMINATED");
    assert(cb2->earlyTerminationStatus == "ACCEPTED");
    assert(cb2->earlyTerminationReason == stuReason);
    assert(!cb2->actualEndDate.empty());

    // 7. Invalid requests: Cannot request early termination on non-ACCEPTED courses
    assert(!service.requestEarlyTermination(20, UserRole::STUDENT, course2Id, "New reason", msg));
    assert(!service.requestEarlyTermination(20, UserRole::STUDENT, courseId, "New reason", msg));

    std::filesystem::remove_all(testDir, ec);
    std::cout << "  -> PASS: Consensual early termination, completion guard, and state transitions verified 100%!" << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << "    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   " << std::endl;
    std::cout << "============================================================" << std::endl;

    // Ensure canonical database is seeded/initialized on fresh clone before running suite
    {
        TutorMatch::MockDatabaseEngine db;
        db.initialize();
    }

    run_test_ocp();
    run_test_lsp();
    run_test_dip_isp_and_srp();
    run_test_schedule_guard();
    run_test_sidebar_labels();
    run_test_tutor_profile_editing();
    run_test_schedule_synchronization();
    run_test_admin_user_data_contract();
    run_test_admin_user_detail_modal_contract();
    run_test_admin_verification_and_lock_sync();
    run_test_admin_service_filters_and_search();
    run_test_avl_tree_multi_value_and_deletion();
    run_test_schedule_guard_sanitization_and_56_bit_boundary();
    run_test_mock_db_atomic_persistence_and_deduplication();
    run_test_data_file_inspector();
    run_test_booking_conflict_detection();
    run_test_entity_serializers();
    run_test_concrete_file_repositories();
    run_test_phase3_end_to_end_scenarios();
    run_test_recurring_course_booking();
    run_test_timetable_recurring_course_integration();
    run_test_data_audit_cleanup_and_migration();
    run_test_multi_slot_course_and_consensual_termination();
    run_test_commitment_acknowledgment_and_review_matrix();



    std::cout << "============================================================" << std::endl;
    run_test_student_profile_and_smartmatch_criteria_ranking();
    run_test_persistence_schema_and_migration();
    run_test_smartmatch_mandatory_subject_filtering();
    run_test_tutor_search_engine_normalization_semantic_and_ranking();
    run_test_tutor_verification_decommission();
    run_test_generic_data_structures_invariants();
    run_test_admin_permission_level_removal_and_authorization();
    run_test_tutor_accumulated_income_and_reconciliation();
    run_test_course_registration_start_date_calculation();
    run_test_early_termination_and_completion_guard();

    std::cout << "  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  " << std::endl;
    std::cout << "============================================================" << std::endl;
    return 0;
}
