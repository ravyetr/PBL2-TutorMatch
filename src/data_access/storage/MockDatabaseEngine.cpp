#include "data_access/storage/MockDatabaseEngine.h"
#include "data_access/storage/ApplicationPaths.h"
#include "business/algorithms/ScheduleGuard.h"
#include <iostream>
#include <filesystem>
#include <fstream>

namespace TutorMatch {

MockDatabaseEngine::MockDatabaseEngine(std::string dataDir)
    : dataDirPath(ApplicationPaths::resolveDataDirectory(dataDir)) {
    usersFilePath = dataDirPath + "/users.dat";
    reviewsFilePath = dataDirPath + "/reviews.dat";
    courseBookingsFilePath = dataDirPath + "/course_bookings.dat";

    userRepo = std::make_shared<FileUserRepository>(usersFilePath);
    reviewRepo = std::make_shared<FileReviewRepository>(reviewsFilePath);
    courseBookingRepo = std::make_shared<FileCourseBookingRepository>(courseBookingsFilePath);
}

MockDatabaseEngine::~MockDatabaseEngine() = default;

bool MockDatabaseEngine::initialize() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    try {
        if (!std::filesystem::exists(dataDirPath)) {
            std::filesystem::create_directories(dataDirPath);
        }

        userRepo->initialize();
        reviewRepo->initialize();
        courseBookingRepo->initialize();

        seedDefaultDataIfEmpty();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[MockDatabaseEngine Error] Initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

void MockDatabaseEngine::rebuildRamIndex() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (userRepo) {
        userRepo->rebuildRamIndex();
    }
}

void MockDatabaseEngine::seedDefaultDataIfEmpty() {
    if (userRepo && userRepo->getIndexCount() == 0) {
        std::cout << "[DB Seed Engine] Database is uninitialized or empty. Resolving seed dataset..." << std::endl;

        std::string seedDir = ApplicationPaths::resolveSeedDirectory(dataDirPath);
        bool seededFromFiles = false;

        if (!seedDir.empty() && std::filesystem::exists(seedDir)) {
            std::string seedUsers = seedDir + "/users.dat";
            std::string seedBookings = seedDir + "/course_bookings.dat";
            std::string seedReviews = seedDir + "/reviews.dat";

            if (std::filesystem::exists(seedUsers) && std::filesystem::file_size(seedUsers) > 0) {
                try {
                    if (!std::filesystem::exists(dataDirPath)) {
                        std::filesystem::create_directories(dataDirPath);
                    }

                    // Copy seed files to runtime data files using binary streams for maximum portability
                    {
                        std::ifstream in(seedUsers, std::ios::binary);
                        std::ofstream out(usersFilePath, std::ios::binary | std::ios::trunc);
                        out << in.rdbuf();
                    }
                    if (std::filesystem::exists(seedBookings) && std::filesystem::file_size(seedBookings) > 0) {
                        std::ifstream in(seedBookings, std::ios::binary);
                        std::ofstream out(courseBookingsFilePath, std::ios::binary | std::ios::trunc);
                        out << in.rdbuf();
                    }
                    if (std::filesystem::exists(seedReviews) && std::filesystem::file_size(seedReviews) > 0) {
                        std::ifstream in(seedReviews, std::ios::binary);
                        std::ofstream out(reviewsFilePath, std::ios::binary | std::ios::trunc);
                        out << in.rdbuf();
                    }

                    // Re-initialize repositories with newly copied seed data
                    userRepo->initialize();
                    reviewRepo->initialize();
                    courseBookingRepo->initialize();

                    if (userRepo->getIndexCount() > 0) {
                        std::cout << "[DB Seed Engine] Successfully seeded database from version-controlled seed data: " 
                                  << seedDir << " (" << userRepo->getIndexCount() << " users loaded)" << std::endl;
                        seededFromFiles = true;
                    }
                } catch (const std::exception& ex) {
                    std::cerr << "[DB Seed Engine Warning] File seeding failed: " << ex.what() 
                              << ", falling back to programmatic default dataset..." << std::endl;
                }
            }
        }

        if (!seededFromFiles) {
            std::cout << "[DB Seed Engine] Seed dataset unavailable or unreadable. Auto-recovering programmatic default dataset..." << std::endl;

            // Truncate existing empty/corrupted files to ensure clean recovery (only active files, never bookings.dat)
            { std::ofstream uTrunc(usersFilePath, std::ios::out | std::ios::trunc); }
            { std::ofstream rTrunc(reviewsFilePath, std::ios::out | std::ios::trunc); }
            { std::ofstream cbTrunc(courseBookingsFilePath, std::ios::out | std::ios::trunc); }

            userRepo->initialize();
            reviewRepo->initialize();
            courseBookingRepo->initialize();

        // 1. Admin Account (ID 1)
        Admin admin(userRepo->getNextUserId(), "admin", "admin123", "admin@tutormatch.vn", "System Administrator", true);
        userRepo->saveUser(admin);

        // 2. Sample Tutors (IDs 2, 3, 4, 5)
        Tutor t1(userRepo->getNextUserId(), "tutor_math", "pass123", "le.math@tutormatch.vn", "ThS. Lê Văn Toàn",
                 "Chuyên gia luyện thi Đại học môn Toán với 10 năm kinh nghiệm.", "Toán 12 - Luyện thi Đại học",
                 150000.0, 5.0, 2, 0.0, 0x00FF00FF00FF00FFULL, true);

        Tutor t2(userRepo->getNextUserId(), "tutor_phys", "pass123", "tran.phys@tutormatch.vn", "TS. Trần Thị Vật Lý",
                 "Giảng viên ĐHQG, chuyên Toán & Vật lý Nâng cao THPT.", "Vật lý 10-12 (Luyện thi THPT)",
                 200000.0, 4.0, 1, 0.0, 0x0000FFFF0000FFFFULL, true);

        Tutor t3(userRepo->getNextUserId(), "tutor_eng", "pass123", "nguyen.eng@tutormatch.vn", "Ms. Nguyễn Anh Văn",
                 "IELTS 8.5, luyện thi Chuyên & Đại học cấp tốc.", "Tiếng Anh / IELTS (6.5 - 8.5)",
                 120000.0, 0.0, 0, 0.0, 0x00FFFFFF00000000ULL, true);

        Tutor t4(userRepo->getNextUserId(), "tutor_chem", "pass123", "pham.chem@tutormatch.vn", "Thầy Phạm Hóa Học",
                 "Dạy kèm Hóa 10-12, Luyện thi HSG Tỉnh.", "Hóa học 10-12 (Luyện thi HSG)",
                 100000.0, 0.0, 0, 0.0, 0x00000000FFFFFFFFULL, true);

        userRepo->saveUser(t1);
        userRepo->saveUser(t2);
        userRepo->saveUser(t3);
        userRepo->saveUser(t4);

        // 3. Sample Students (IDs 6, 7)
        Student s1(userRepo->getNextUserId(), "student_nam", "pass123", "nam.student@gmail.com", "Nguyễn Văn Nam",
                   12, "Toán 12", 180000.0, 0x00FF00FF00FF00FFULL, true);
        Student s2(userRepo->getNextUserId(), "student_thanh", "pass123", "thanh.student@gmail.com", "Phạm Tuấn Thành",
                   11, "Toán 11", 200000.0, 0x0000FFFF0000FFFFULL, true);
        userRepo->saveUser(s1);
        userRepo->saveUser(s2);

        // 4. Sample CourseBookings (IDs 1001, 1002, 1003, 1004, 1005)
        if (courseBookingRepo) {
            // cb1: Completed Course for Nam with Toan -> Backs Review 1
            CourseBooking cb1(courseBookingRepo->getNextCourseBookingId(), s1.getId(), t1.getId(),
                              s1.getFullName(), t1.getFullName(), "Toán 12 - Luyện thi Đại học",
                              150000.0, "2026-07-20", 4, "COMPLETED", "2026-07-15",
                              "0:1", 0, "NONE", "", "", true, "2026-07-15 08:00:00", true, "2026-07-15 08:30:00",
                              CURRENT_COMMITMENT_VERSION);
            courseBookingRepo->saveCourseBooking(cb1);

            // cb2: Completed Course for Thanh with Toan -> Backs Review 2
            CourseBooking cb2(courseBookingRepo->getNextCourseBookingId(), s2.getId(), t1.getId(),
                              s2.getFullName(), t1.getFullName(), "Toán 12",
                              150000.0, "2026-07-22", 4, "COMPLETED", "2026-07-16",
                              "2:1", 0, "NONE", "", "", true, "2026-07-16 08:00:00", true, "2026-07-16 08:30:00",
                              CURRENT_COMMITMENT_VERSION);
            courseBookingRepo->saveCourseBooking(cb2);

            // cb3: Completed Course for Nam with Ly -> Backs Review 3
            CourseBooking cb3(courseBookingRepo->getNextCourseBookingId(), s1.getId(), t2.getId(),
                              s1.getFullName(), t2.getFullName(), "Vật lý 10-12 (Luyện thi THPT)",
                              200000.0, "2026-07-21", 4, "COMPLETED", "2026-07-18",
                              "1:2", 0, "NONE", "", "", true, "2026-07-18 08:00:00", true, "2026-07-18 08:30:00",
                              CURRENT_COMMITMENT_VERSION);
            courseBookingRepo->saveCourseBooking(cb3);

            // cb4: Active Ongoing Course for Nam with Toan
            CourseBooking cb4(courseBookingRepo->getNextCourseBookingId(), s1.getId(), t1.getId(),
                              s1.getFullName(), t1.getFullName(), "Toán 12 - Luyện thi Đại học",
                              150000.0, "2026-11-06", 4, "ACCEPTED", "2026-11-06",
                              "4:0", 0, "NONE", "", "", true, "2026-11-06 08:00:00", true, "2026-11-06 08:30:00",
                              CURRENT_COMMITMENT_VERSION);
            courseBookingRepo->saveCourseBooking(cb4);

            // cb5: Active Ongoing Course for Thanh with Ly
            CourseBooking cb5(courseBookingRepo->getNextCourseBookingId(), s2.getId(), t2.getId(),
                              s2.getFullName(), t2.getFullName(), "Vật lý 10-12 (Luyện thi THPT)",
                              200000.0, "2026-11-10", 8, "ACCEPTED", "2026-11-06",
                              "1:1", 0, "NONE", "", "", true, "2026-11-06 08:00:00", true, "2026-11-06 08:30:00",
                              CURRENT_COMMITMENT_VERSION);
            courseBookingRepo->saveCourseBooking(cb5);

            // 5. Sample Reviews matching COMPLETED CourseBookings strictly 1:1
            Review r1(reviewRepo->getNextReviewId(), cb1.id, s1.getId(), t1.getId(), s1.getFullName(),
                      5, "Thầy Toàn giảng Toán 12 rất dễ hiểu, phương pháp giải trắc nghiệm tốc độ cao.", "2026-08-15");
            Review r2(reviewRepo->getNextReviewId(), cb2.id, s2.getId(), t1.getId(), s2.getFullName(),
                      5, "Thầy hỗ trợ giải bộ đề Toán 12 Đại học cực kỳ chi tiết!", "2026-08-20");
            Review r3(reviewRepo->getNextReviewId(), cb3.id, s1.getId(), t2.getId(), s1.getFullName(),
                      4, "Cô dạy Vật lý 12 rất hay, bài tập nâng cao bám sát cấu trúc đề thi THPT.", "2026-08-22");
            reviewRepo->saveReview(r1);
            reviewRepo->saveReview(r2);
            reviewRepo->saveReview(r3);
        }

        std::cout << "[DB Seed Engine] Seed completed cleanly!" << std::endl;
    }
}
}

// Auto Increment IDs
int MockDatabaseEngine::getNextUserId() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->getNextUserId() : 1;
}

int MockDatabaseEngine::getNextCourseBookingId() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->getNextCourseBookingId() : 1001;
}

int MockDatabaseEngine::getNextReviewId() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return reviewRepo ? reviewRepo->getNextReviewId() : 1;
}

// User Operations
bool MockDatabaseEngine::saveUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->saveUser(user) : false;
}

std::shared_ptr<User> MockDatabaseEngine::findUserById(int userId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->findUserById(userId) : nullptr;
}

std::shared_ptr<User> MockDatabaseEngine::findUserByUsername(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->findUserByUsername(username) : nullptr;
}

bool MockDatabaseEngine::updateUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->updateUser(user) : false;
}

bool MockDatabaseEngine::deleteUserSoft(int userId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->deleteUserSoft(userId) : false;
}

std::vector<std::shared_ptr<Tutor>> MockDatabaseEngine::getAllTutors() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->getAllTutors() : std::vector<std::shared_ptr<Tutor>>{};
}

std::vector<std::shared_ptr<Student>> MockDatabaseEngine::getAllStudents() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->getAllStudents() : std::vector<std::shared_ptr<Student>>{};
}

std::vector<std::shared_ptr<User>> MockDatabaseEngine::getAllUsers() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->getAllUsers() : std::vector<std::shared_ptr<User>>{};
}

std::vector<int> MockDatabaseEngine::findTutorIdsByPriceRange(double minRate, double maxRate) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->findTutorIdsByPriceRange(minRate, maxRate) : std::vector<int>{};
}

std::vector<int> MockDatabaseEngine::findTutorIdsByMinRating(double minRating) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return userRepo ? userRepo->findTutorIdsByMinRating(minRating) : std::vector<int>{};
}

// Course Booking Operations
bool MockDatabaseEngine::saveCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->saveCourseBooking(booking) : false;
}

bool MockDatabaseEngine::updateCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->updateCourseBooking(booking) : false;
}

std::shared_ptr<CourseBooking> MockDatabaseEngine::findCourseBookingById(int id) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->findCourseBookingById(id) : nullptr;
}

std::vector<CourseBooking> MockDatabaseEngine::getAllCourseBookings() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->getAllCourseBookings() : std::vector<CourseBooking>{};
}

std::vector<CourseBooking> MockDatabaseEngine::getCourseBookingsByStudentId(int studentId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->getCourseBookingsByStudentId(studentId) : std::vector<CourseBooking>{};
}

std::vector<CourseBooking> MockDatabaseEngine::getCourseBookingsByTutorId(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return courseBookingRepo ? courseBookingRepo->getCourseBookingsByTutorId(tutorId) : std::vector<CourseBooking>{};
}

// Review Operations
bool MockDatabaseEngine::saveReview(const Review& review) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return reviewRepo ? reviewRepo->saveReview(review) : false;
}

bool MockDatabaseEngine::deleteReview(int reviewId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return reviewRepo ? reviewRepo->deleteReview(reviewId) : false;
}

std::vector<Review> MockDatabaseEngine::getReviewsByTutorId(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return reviewRepo ? reviewRepo->getReviewsByTutorId(tutorId) : std::vector<Review>{};
}

std::vector<Review> MockDatabaseEngine::getAllReviews() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    return reviewRepo ? reviewRepo->getAllReviews() : std::vector<Review>{};
}

} // namespace TutorMatch
