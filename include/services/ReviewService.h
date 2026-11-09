#ifndef TUTOR_MATCH_REVIEW_SERVICE_H
#define TUTOR_MATCH_REVIEW_SERVICE_H

#include "../models/User.h"
#include "../models/CourseBooking.h"
#include "../models/Review.h"
#include "../storage/IRepository.h"

#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace TutorMatch {

/**
 * @brief ReviewService encapsulates the entire review and rating lifecycle.
 *
 * Implements ADR-003 (Service Extraction: Review & Matching):
 * - Single Responsibility Principle (SRP): Decoupled from BookingService.
 * - Manages review eligibility (only COMPLETED course bookings, one review per booking).
 * - Handles review submission and violation deletion.
 * - Recalculates tutor rating and review counts atomically and updates IUserRepository.
 * - Thread-safe operations protected by recursive mutex.
 */
class ReviewService {
private:
    std::shared_ptr<IReviewRepository> reviewRepo;
    std::shared_ptr<IUserRepository> userRepo;
    std::shared_ptr<ICourseBookingRepository> courseBookingRepo;
    mutable std::recursive_mutex reviewMutex;

public:
    ReviewService(std::shared_ptr<IReviewRepository> rRepo,
                  std::shared_ptr<IUserRepository> uRepo,
                  std::shared_ptr<ICourseBookingRepository> cbRepo = nullptr);
    virtual ~ReviewService() = default;

    // Review Lifecycle Operations
    bool submitReview(int studentId, int bookingId, int ratingScore,
                      const std::string& comment, std::string& outMsg);

    bool deleteReview(int reviewId, std::string& outMsg);

    // Queries
    std::vector<Review> getReviewsByTutorId(int tutorId) const;
    std::vector<Review> getReviewsByStudentId(int studentId) const;
    std::vector<Review> getAllReviews() const;

    // Rating Calculation Helper
    void recalculateTutorRating(int tutorId);
    bool calculateTutorRating(int tutorId, double& outRating, int& outCount) const;

    // ADR-007: Startup Self-Healing Reconciliation
    void reconcileRatingsOnStartup();

    // Review Eligibility
    bool isEligibleForReview(int studentId, int bookingId, std::string& outReason) const;
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_REVIEW_SERVICE_H
