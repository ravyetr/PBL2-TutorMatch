#ifndef TUTOR_MATCH_MATCHING_ENGINE_H
#define TUTOR_MATCH_MATCHING_ENGINE_H

#include "business/matching/IMatchingStrategy.h"
#include "business/algorithms/MaxHeap.h"
#include "business/domain/User.h"
#include "business/domain/CourseBooking.h"
#include <vector>
#include <memory>

namespace TutorMatch {

// 100-Point Explainable Criteria-Based Matching Strategy (Phase D2)
// Weights: Schedule (35), Price (25), Reviews (15), Experience (15), Credibility (10)
class WeightedScoreMatchingStrategy : public IMatchingStrategy {
private:
    double weightSchedule;
    double weightPrice;
    double weightRating;
    double weightExperience;
    double weightCredibility;

public:
    WeightedScoreMatchingStrategy(double wSched = 35.0, double wPrice = 25.0,
                                  double wRating = 15.0, double wExp = 15.0,
                                  double wCred = 10.0)
        : weightSchedule(wSched), weightPrice(wPrice), weightRating(wRating),
          weightExperience(wExp), weightCredibility(wCred) {}

    std::string getStrategyName() const override {
        return "Explainable 100-Point Multi-Criteria Strategy (35/25/15/15/10)";
    }

    // Legacy interface adapter
    TutorMatchResult calculateScore(const Student& student, const Tutor& tutor) override;

    // Phase D2: Full 100-point explainable scoring with criteria and active bookings
    TutorMatchResult calculateScoreWithCriteria(const SmartMatchCriteria& criteria,
                                                const Tutor& tutor,
                                                const std::vector<CourseBooking>& activeBookings = {}) override;
};

// Smart Matching Engine utilizing Priority Queue (Max-Heap)
class MatchingEngine {
private:
    std::unique_ptr<IMatchingStrategy> activeStrategy;

public:
    MatchingEngine();
    explicit MatchingEngine(std::unique_ptr<IMatchingStrategy> strategy);

    void setStrategy(std::unique_ptr<IMatchingStrategy> newStrategy);
    IMatchingStrategy* getStrategy() const { return activeStrategy.get(); }

    // Hard eligibility evaluation (Phase D1)
    static bool isEligible(const SmartMatchCriteria& criteria,
                           const Tutor& tutor,
                           const std::vector<CourseBooking>& activeBookings,
                           std::vector<std::pair<int, int>>& outMatchedSlots);

    // Criteria-based matching and ranking (Phase D)
    std::vector<TutorMatchResult> findTopMatchesWithCriteria(
        const SmartMatchCriteria& criteria,
        const std::vector<std::shared_ptr<Tutor>>& tutors,
        const std::vector<CourseBooking>& activeBookings = {},
        size_t topK = 5);

    // Legacy overload for backward compatibility with existing tests and callers
    std::vector<TutorMatchResult> findTopMatches(
        const Student& student,
        const std::vector<std::shared_ptr<Tutor>>& tutors,
        size_t topK = 3);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MATCHING_ENGINE_H
