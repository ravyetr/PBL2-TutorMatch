#ifndef TUTOR_MATCH_ADMIN_SERVICE_H
#define TUTOR_MATCH_ADMIN_SERVICE_H

#include "business/domain/User.h"
#include "business/domain/CourseBooking.h"
#include "business/domain/Review.h"
#include "data_access/repositories/IRepository.h"
#include "business/services/BookingService.h"
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace TutorMatch {

struct AdminMetrics {
    int totalTutors = 0;
    int totalStudents = 0;
    int totalBookings = 0;
    int completedBookings = 0;
    int activeMatches = 0;
};

/**
 * AdminService:
 * Implements Single Responsibility Principle (SRP) for platform administration,
 * audit metrics, moderation, and reporting.
 *
 * Implements DIP and ISP by depending on IUserRepository, IReviewRepository,
 * and BookingService.
 */
class AdminService {
private:
    std::shared_ptr<IUserRepository> userRepo;
    std::shared_ptr<IReviewRepository> reviewRepo;
    std::shared_ptr<BookingService> bookingService;

    mutable std::mutex adminMutex;

public:
    AdminService(std::shared_ptr<IUserRepository> uRepo,
                 std::shared_ptr<IReviewRepository> rRepo,
                 std::shared_ptr<BookingService> bService);
    virtual ~AdminService() = default;

    AdminMetrics calculateMetrics();

    std::vector<std::shared_ptr<User>> getAllUsers();
    std::vector<std::shared_ptr<User>> getFilteredUsers(const std::string& filter,
                                                        const std::string& searchKeyword);

    bool toggleUserLock(int userId, std::string& outMsg);
    bool deleteViolatingReview(int reviewId, std::string& outMsg);
    bool terminateMatch(int bookingId, std::string& outMsg);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_ADMIN_SERVICE_H
