#ifndef TUTOR_MATCH_MOCK_DATABASE_ENGINE_H
#define TUTOR_MATCH_MOCK_DATABASE_ENGINE_H

#include "IRepository.h"
#include "repositories/FileUserRepository.h"
#include "repositories/FileReviewRepository.h"
#include "repositories/FileCourseBookingRepository.h"
#include "../models/User.h"
#include "../models/CourseBooking.h"
#include "../models/Review.h"

#include <string>
#include <memory>
#include <vector>
#include <mutex>

namespace TutorMatch {

/**
 * Mock Database Engine:
 * - Pure Delegating Facade (ADR-002) implementing IUserRepository, IReviewRepository, ICourseBookingRepository.
 * - Delegates all operations to concrete File Repositories:
 *     * FileUserRepository
 *     * FileReviewRepository
 *     * FileCourseBookingRepository
 * - Maintains backwards compatibility with existing services, tests, and GUIBridge.
 */
class MockDatabaseEngine : public IUserRepository, public IReviewRepository, public ICourseBookingRepository {
private:
    std::string dataDirPath;
    std::string usersFilePath;
    std::string reviewsFilePath;
    std::string courseBookingsFilePath;

    std::shared_ptr<FileUserRepository> userRepo;
    std::shared_ptr<FileReviewRepository> reviewRepo;
    std::shared_ptr<FileCourseBookingRepository> courseBookingRepo;

    mutable std::recursive_mutex dbMutex;

    void seedDefaultDataIfEmpty();

public:
    explicit MockDatabaseEngine(std::string dataDir = "data");
    ~MockDatabaseEngine() override;

    bool initialize();
    void rebuildRamIndex();

    // Repository accessors
    std::shared_ptr<FileUserRepository> getUserRepository() const { return userRepo; }
    std::shared_ptr<FileReviewRepository> getReviewRepository() const { return reviewRepo; }
    std::shared_ptr<FileCourseBookingRepository> getCourseBookingRepository() const { return courseBookingRepo; }

    // Auto Increment IDs
    int getNextUserId() override;
    int getNextCourseBookingId() override;
    int getNextReviewId() override;

    // User Operations
    bool saveUser(const User& user) override;
    std::shared_ptr<User> findUserById(int userId) override;
    std::shared_ptr<User> findUserByUsername(const std::string& username) override;
    bool updateUser(const User& user) override;
    bool deleteUserSoft(int userId) override;

    std::vector<std::shared_ptr<Tutor>> getAllTutors() override;
    std::vector<std::shared_ptr<Student>> getAllStudents() override;
    std::vector<std::shared_ptr<User>> getAllUsers() override;

    // Range Queries via AVL Tree
    std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) override;
    std::vector<int> findTutorIdsByMinRating(double minRating) override;

    // Course Booking Operations
    bool saveCourseBooking(const CourseBooking& booking) override;
    bool updateCourseBooking(const CourseBooking& booking) override;
    std::shared_ptr<CourseBooking> findCourseBookingById(int id) override;
    std::vector<CourseBooking> getAllCourseBookings() override;
    std::vector<CourseBooking> getCourseBookingsByStudentId(int studentId) override;
    std::vector<CourseBooking> getCourseBookingsByTutorId(int tutorId) override;

    // Review Operations
    bool saveReview(const Review& review) override;
    bool deleteReview(int reviewId) override;
    std::vector<Review> getReviewsByTutorId(int tutorId) override;
    std::vector<Review> getAllReviews() override;

    // Cache & Index Stats
    size_t getCacheSize() const { return userRepo ? userRepo->getCacheSize() : 0; }
    size_t getIndexCount() const { return userRepo ? userRepo->getIndexCount() : 0; }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MOCK_DATABASE_ENGINE_H
