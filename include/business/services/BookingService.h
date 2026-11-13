#ifndef TUTOR_MATCH_BOOKING_SERVICE_H
#define TUTOR_MATCH_BOOKING_SERVICE_H

#include "business/domain/User.h"
#include "business/domain/CourseBooking.h"
#include "business/domain/BookingDTOs.h"
#include "business/domain/Review.h"
#include "data_access/repositories/IRepository.h"
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <utility>

namespace TutorMatch {

/**
 * @brief Manages the lifecycle of Recurring Course Bookings.
 *
 * Implements Single Responsibility Principle (SRP) by encapsulating the entire
 * recurring course booking lifecycle.
 *
 * Guarantees domain invariants:
 * - Recurring schedule: Supports single or multiple time slots per week across multiple weeks.
 * - Symmetrical and strict conflict detection for both Students and Tutors.
 * - Proper state machine transitions: PENDING -> ACCEPTED -> COMPLETED / CANCELLED / EARLY_TERMINATED / REJECTED.
 * - Consensual Early Termination: Requires mutual agreement between Student and Tutor.
 * - Accurate recalculation of Tutor aggregate rating upon review creation or deletion.
 *
 * Implements Dependency Inversion Principle (DIP) and Interface Segregation Principle (ISP)
 * by depending on IUserRepository, IReviewRepository, and ICourseBookingRepository.
 */
class BookingService {
private:
    std::shared_ptr<IUserRepository> userRepo;
    std::shared_ptr<IReviewRepository> reviewRepo;
    std::shared_ptr<ICourseBookingRepository> courseBookingRepo;
    mutable std::recursive_mutex serviceMutex;

public:
    BookingService(std::shared_ptr<IUserRepository> uRepo,
                   std::shared_ptr<IReviewRepository> rRepo,
                   std::shared_ptr<ICourseBookingRepository> cbRepo);
    virtual ~BookingService() = default;

    void setCourseBookingRepository(std::shared_ptr<ICourseBookingRepository> cbRepo) {
        std::lock_guard<std::recursive_mutex> lock(serviceMutex);
        courseBookingRepo = std::move(cbRepo);
    }
    std::shared_ptr<ICourseBookingRepository> getCourseBookingRepository() const {
        std::lock_guard<std::recursive_mutex> lock(serviceMutex);
        return courseBookingRepo;
    }

    // Recurring Course Booking Conflict Detection
    bool hasCourseConflict(int studentId, int tutorId, int dayOfWeek, int slotIndex,
                           const std::string& startDate, int weeksCount,
                           int excludeBookingId = 0,
                           std::string* outConflictDetails = nullptr,
                           std::string* outConflictDate = nullptr);

    bool hasCourseConflictMulti(int studentId, int tutorId,
                                const std::vector<std::pair<int, int>>& weeklySlots,
                                const std::string& startDate, int weeksCount,
                                int excludeBookingId = 0,
                                std::string* outConflictDetails = nullptr,
                                std::string* outConflictDate = nullptr);

    // Recurring Course Booking Operations
    bool bookCourse(int studentId, int tutorId, const std::string& subject,
                    int dayOfWeek, int slotIndex, const std::string& startDate,
                    int weeksCount, bool studentAcknowledgeCommitment,
                    std::string& outMsg, int* outBookingId = nullptr);

    bool bookCourse(int studentId, int tutorId, const std::string& subject,
                    int dayOfWeek, int slotIndex, const std::string& startDate,
                    int weeksCount, std::string& outMsg, int* outBookingId = nullptr);

    bool bookCourseMultiSlot(int studentId, int tutorId, const std::string& subject,
                             const std::vector<std::pair<int, int>>& weeklySlots,
                             const std::string& startDate, int weeksCount,
                             bool studentAcknowledgeCommitment,
                             std::string& outMsg, int* outBookingId = nullptr);

    bool bookCourseMultiSlot(int studentId, int tutorId, const std::string& subject,
                             const std::vector<std::pair<int, int>>& weeklySlots,
                             const std::string& startDate, int weeksCount,
                             std::string& outMsg, int* outBookingId = nullptr);

    // Commitment Acknowledgment Operations
    bool acknowledgeStudentCommitment(int studentId, int courseBookingId, std::string& outMsg);
    bool acknowledgeTutorCommitment(int tutorId, int courseBookingId, std::string& outMsg);

    bool acceptCourseBooking(int tutorId, int courseBookingId, bool tutorAcknowledgeCommitment,
                             std::string& outMsg, std::string* outConflictDate = nullptr);
    bool acceptCourseBooking(int tutorId, int courseBookingId, std::string& outMsg,
                             std::string* outConflictDate = nullptr);
    bool rejectCourseBooking(int tutorId, int courseBookingId, std::string& outMsg);
    bool cancelCourseBooking(int operatorUserId, UserRole operatorRole, int courseBookingId, std::string& outMsg);
    bool completeCourseBooking(int operatorUserId, UserRole operatorRole, int courseBookingId, std::string& outMsg);
    bool terminateCourseBooking(int operatorUserId, UserRole operatorRole, int courseBookingId, std::string& outMsg);

    // Consensual Early Termination (Mutual Consent)
    bool requestEarlyTermination(int operatorUserId, UserRole operatorRole, int courseBookingId,
                                 const std::string& reason, std::string& outMsg);
    bool respondEarlyTermination(int operatorUserId, UserRole operatorRole, int courseBookingId,
                                 bool agree, std::string& outMsg);

    // Auto-complete expired courses
    void autoCompleteExpiredCourses();

    // Tutor Accumulated Income & Reconciliation Operations
    double calculateTutorAccumulatedIncome(int tutorId) const;
    void reconcileTutorEarnings(int tutorId);
    void reconcileAllTutorEarningsOnStartup();

    bool submitReview(int studentId, int bookingId, int ratingScore,
                      const std::string& comment, std::string& outMsg);

    std::vector<CourseBooking> getStudentCourseBookings(int studentId);
    std::vector<CourseBooking> getTutorCourseBookings(int tutorId);

    /**
     * @brief Retrieves all accepted course sessions that fall within the given calendar week.
     * @param userId The student ID or tutor ID.
     * @param role UserRole::STUDENT, UserRole::TUTOR, or UserRole::ADMIN.
     * @param mondayDateStr ISO date YYYY-MM-DD of Monday of the target week (or empty for current week).
     * @return Vector of TimetableSessionDTO for sessions occurring in this week.
     */
    std::vector<TimetableSessionDTO> getAcceptedSessionsForWeek(int userId, UserRole role, const std::string& mondayDateStr = "");

    StudentBookingMetrics getStudentBookingMetrics(int studentId);
    std::vector<Review> getTutorReviews(int tutorId);
    std::vector<CourseBooking> getAllCourseBookings();
    std::vector<Review> getAllReviews();
    std::shared_ptr<CourseBooking> findCourseBookingById(int courseBookingId);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_BOOKING_SERVICE_H
