#ifndef TUTOR_MATCH_MATCHING_SERVICE_H
#define TUTOR_MATCH_MATCHING_SERVICE_H

#include "business/matching/MatchingEngine.h"
#include "data_access/repositories/IRepository.h"
#include "business/domain/User.h"
#include "business/domain/CourseBooking.h"

#include <memory>
#include <vector>
#include <mutex>

namespace TutorMatch {

/**
 * @brief MatchingService coordinates intelligent tutor matching and criteria-based search.
 *
 * Implements ADR-003 and SmartMatch criteria-based ranking:
 * - Decouples Presentation Layer (GUIBridge) from direct Algorithmic Engine & Repository coordination.
 * - Retrieves active candidate tutors from IUserRepository.
 * - Retrieves active course reservations from ICourseBookingRepository to check schedule conflicts.
 * - Delegates eligibility filtering and 100-point ranking to MatchingEngine (MaxHeap).
 */
class MatchingService {
private:
    std::shared_ptr<IUserRepository> userRepo;
    std::shared_ptr<ICourseBookingRepository> courseBookingRepo;
    std::unique_ptr<MatchingEngine> matchingEngine;
    mutable std::mutex serviceMutex;

public:
    MatchingService(std::shared_ptr<IUserRepository> uRepo,
                    std::shared_ptr<ICourseBookingRepository> cbRepo = nullptr,
                    std::unique_ptr<MatchingEngine> engine = nullptr);
    virtual ~MatchingService() = default;

    void setCourseBookingRepository(std::shared_ptr<ICourseBookingRepository> cbRepo) {
        std::lock_guard<std::mutex> lock(serviceMutex);
        courseBookingRepo = std::move(cbRepo);
    }

    // Phase D: Criteria-based search
    std::vector<TutorMatchResult> findMatchesWithCriteria(const SmartMatchCriteria& criteria, size_t topK = 5);

    // Legacy use cases maintained for backward compatibility
    std::vector<TutorMatchResult> findTopMatchesForStudent(int studentId, size_t topK = 3);
    std::vector<TutorMatchResult> findMatches(const Student& student, size_t topK = 3);

    // Strategy configuration (OCP)
    void setMatchingStrategy(std::unique_ptr<IMatchingStrategy> strategy);

    MatchingEngine* getEngine() const { return matchingEngine.get(); }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MATCHING_SERVICE_H
