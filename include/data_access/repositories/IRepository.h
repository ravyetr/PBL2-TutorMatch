#ifndef TUTOR_MATCH_I_REPOSITORY_H
#define TUTOR_MATCH_I_REPOSITORY_H

#include <vector>
#include <memory>
#include <string>

namespace TutorMatch {

class User;
class Student;
class Tutor;
struct CourseBooking;
struct Review;

// ============================================================================
// Interface Segregation Principle (ISP) - Role-Based Repository Contracts
// ============================================================================

/**
 * @brief Abstract persistence contract for User entities (Student, Tutor, Admin).
 *
 * Implements Dependency Inversion Principle (DIP):
 * - Business Logic Layer depends strictly on this abstraction.
 * - Data Access Layer (FileUserRepository, MockDatabaseEngine) implements it.
 *
 * Ownership & Lifetime Semantics:
 * - findUserById / findUserByUsername return std::shared_ptr<User> representing
 *   polymorphic instances (Student, Tutor, Admin).
 * - getAllTutors / getAllStudents / getAllUsers return vectors of shared_ptr to
 *   avoid object slicing and preserve polymorphic behaviors.
 * - saveUser / updateUser receive const reference; callers retain ownership of original.
 *
 * Error Handling:
 * - Return boolean status (true = success, false = failure/not found/io error).
 * - Queries return nullptr when target entity does not exist.
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual int getNextUserId() = 0;
    virtual bool saveUser(const User& user) = 0;
    virtual std::shared_ptr<User> findUserById(int userId) = 0;
    virtual std::shared_ptr<User> findUserByUsername(const std::string& username) = 0;
    virtual bool updateUser(const User& user) = 0;
    virtual bool deleteUserSoft(int userId) = 0;

    virtual std::vector<std::shared_ptr<Tutor>> getAllTutors() = 0;
    virtual std::vector<std::shared_ptr<Student>> getAllStudents() = 0;
    virtual std::vector<std::shared_ptr<User>> getAllUsers() = 0;

    virtual std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) = 0;
    virtual std::vector<int> findTutorIdsByMinRating(double minRating) = 0;
};

/**
 * @brief Abstract persistence contract for Review & Feedback management.
 *
 * Implements Dependency Inversion Principle (DIP):
 * - Encapsulates storage operations for Review value entities.
 * - Authoritative persistent Source of Truth for reviews.dat.
 *
 * Ownership & Lifetime Semantics:
 * - Review is a copiable domain value struct.
 * - Queries return value vectors.
 *
 * Error Handling:
 * - Return boolean status for mutations (saveReview, deleteReview).
 * - Queries return empty vector when no matching reviews exist.
 */
class IReviewRepository {
public:
    virtual ~IReviewRepository() = default;

    virtual int getNextReviewId() = 0;
    virtual bool saveReview(const Review& review) = 0;
    virtual bool deleteReview(int reviewId) = 0;
    virtual std::vector<Review> getReviewsByTutorId(int tutorId) = 0;
    virtual std::vector<Review> getAllReviews() = 0;
};

/**
 * @brief Abstract persistence contract for Recurring Course Booking operations.
 *
 * Implements Dependency Inversion Principle (DIP):
 * - Encapsulates storage operations for CourseBooking value entities.
 * - Manages multi-week calendar date reservations.
 */
class ICourseBookingRepository {
public:
    virtual ~ICourseBookingRepository() = default;

    virtual int getNextCourseBookingId() = 0;
    virtual bool saveCourseBooking(const CourseBooking& booking) = 0;
    virtual bool updateCourseBooking(const CourseBooking& booking) = 0;
    virtual std::shared_ptr<CourseBooking> findCourseBookingById(int id) = 0;
    virtual std::vector<CourseBooking> getAllCourseBookings() = 0;
    virtual std::vector<CourseBooking> getCourseBookingsByStudentId(int studentId) = 0;
    virtual std::vector<CourseBooking> getCourseBookingsByTutorId(int tutorId) = 0;
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_I_REPOSITORY_H
