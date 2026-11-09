#ifndef TUTOR_MATCH_MATCHING_SERVICE_H
#define TUTOR_MATCH_MATCHING_SERVICE_H

#include "../algorithms/MatchingEngine.h"
#include "../storage/IRepository.h"
#include "../models/User.h"

#include <memory>
#include <vector>
#include <mutex>

namespace TutorMatch {

/**
 * @brief MatchingService coordinates the intelligent tutor matching application use case.
 *
 * Implements ADR-003 (Service Extraction: Review & Matching):
 * - Application Service layer (BLL) encapsulating matching workflow.
 * - Decouples Presentation Layer (GUIBridge, ConsoleApp) from direct Algorithmic Engine & Repository coordination.
 * - Retrieves active, verified candidate tutors from IUserRepository.
 * - Delegates ranking and top-K selection to MatchingEngine (Priority Queue / MaxHeap).
 * - Allows runtime strategy swapping (Strategy Pattern / Open-Closed Principle).
 */
class MatchingService {
private:
    std::shared_ptr<IUserRepository> userRepo;
    std::unique_ptr<MatchingEngine> matchingEngine;
    mutable std::mutex serviceMutex;

public:
    MatchingService(std::shared_ptr<IUserRepository> uRepo,
                    std::unique_ptr<MatchingEngine> engine = nullptr);
    virtual ~MatchingService() = default;

    // Use cases
    std::vector<TutorMatchResult> findTopMatchesForStudent(int studentId, size_t topK = 3);
    std::vector<TutorMatchResult> findMatches(const Student& student, size_t topK = 3);

    // Strategy configuration (OCP)
    void setMatchingStrategy(std::unique_ptr<IMatchingStrategy> strategy);

    MatchingEngine* getEngine() const { return matchingEngine.get(); }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MATCHING_SERVICE_H
