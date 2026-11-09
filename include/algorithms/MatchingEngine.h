#ifndef TUTOR_MATCH_MATCHING_ENGINE_H
#define TUTOR_MATCH_MATCHING_ENGINE_H

#include "IMatchingStrategy.h"
#include "../ds/MaxHeap.h"
#include "../models/User.h"
#include <vector>
#include <memory>

namespace TutorMatch {

// Concrete Weighted Multi-Criteria Score Matching Strategy
class WeightedScoreMatchingStrategy : public IMatchingStrategy {
private:
    double weightSubject;
    double weightPrice;
    double weightSchedule;
    double weightRating;
    double weightVerification;

public:
    WeightedScoreMatchingStrategy(double wSubj = 0.30, double wPrice = 0.20, double wSched = 0.25, double wRating = 0.15, double wVer = 0.10)
        : weightSubject(wSubj), weightPrice(wPrice), weightSchedule(wSched), weightRating(wRating), weightVerification(wVer) {}

    std::string getStrategyName() const override { return "Weighted Multi-Criteria Score Strategy (30/20/25/15/10)"; }

    TutorMatchResult calculateScore(const Student& student, const Tutor& tutor) override;
};

// Smart Matching Engine utilizing Priority Queue (Max-Heap)
class MatchingEngine {
private:
    std::unique_ptr<IMatchingStrategy> activeStrategy;

public:
    MatchingEngine();
    explicit MatchingEngine(std::unique_ptr<IMatchingStrategy> strategy);

    void setStrategy(std::unique_ptr<IMatchingStrategy> newStrategy);

    // Ranks all tutors for a student and extracts Top K using MaxHeap
    std::vector<TutorMatchResult> findTopMatches(const Student& student,
                                                 const std::vector<std::shared_ptr<Tutor>>& tutors,
                                                 size_t topK = 3);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MATCHING_ENGINE_H
