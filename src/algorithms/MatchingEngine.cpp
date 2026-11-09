#include "../../include/algorithms/MatchingEngine.h"
#include "../../include/ds/ScheduleGuard.h"
#include "../../include/models/SubjectTaxonomy.h"
#include <algorithm>
#include <cctype>

namespace TutorMatch {

TutorMatchResult WeightedScoreMatchingStrategy::calculateScore(const Student& student, const Tutor& tutor) {
    TutorMatchResult res;
    res.tutor = std::make_shared<Tutor>(tutor);

    // 1. Subject & Grade Affinity Score (30%)
    res.subjectScore = SubjectTaxonomy::calculateAffinityScore(student, tutor);

    // 2. Price Affordability Score (20%)
    double maxB = student.getMaxBudget();
    double hRate = tutor.getHourlyRate();
    if (maxB <= 0 || hRate <= maxB) {
        res.priceScore = 100.0;
    } else {
        double diff = hRate - maxB;
        res.priceScore = std::max(0.0, 100.0 - (diff / maxB) * 100.0);
    }

    // 3. Schedule Overlap Bitmask Score (25%)
    int matchingSlots = ScheduleGuard::countMatchingSlots(tutor.getScheduleBitmask(), student.getScheduleBitmask());
    res.scheduleScore = std::min(100.0, matchingSlots * 25.0); // 4 matching slots = 100%

    // 4. Rating Score (15%) - 1.0 -> 5.0 converted to 0 -> 100
    res.ratingScore = (tutor.getRating() / 5.0) * 100.0;

    // 5. Verification Bonus (10%)
    double verificationScore = tutor.getIsVerified() ? 100.0 : 0.0;

    // Weighted Composite Score Formula
    res.compositeScore = (res.subjectScore * weightSubject) +
                         (res.priceScore * weightPrice) +
                         (res.scheduleScore * weightSchedule) +
                         (res.ratingScore * weightRating) +
                         (verificationScore * weightVerification);

    res.compositeScore = std::min(100.0, res.compositeScore);
    return res;
}

MatchingEngine::MatchingEngine() {
    activeStrategy = std::make_unique<WeightedScoreMatchingStrategy>();
}

MatchingEngine::MatchingEngine(std::unique_ptr<IMatchingStrategy> strategy)
    : activeStrategy(std::move(strategy)) {}

void MatchingEngine::setStrategy(std::unique_ptr<IMatchingStrategy> newStrategy) {
    activeStrategy = std::move(newStrategy);
}

std::vector<TutorMatchResult> MatchingEngine::findTopMatches(const Student& student,
                                                             const std::vector<std::shared_ptr<Tutor>>& tutors,
                                                             size_t topK) {
    MaxHeap<TutorMatchResult> maxHeap;

    for (const auto& tutorPtr : tutors) {
        if (!tutorPtr || !tutorPtr->getIsActive()) continue;
        TutorMatchResult res = activeStrategy->calculateScore(student, *tutorPtr);
        maxHeap.push(res);
    }

    return maxHeap.extractSortedTopK(topK);
}

} // namespace TutorMatch
