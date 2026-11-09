#ifndef TUTOR_MATCH_I_MATCHING_STRATEGY_H
#define TUTOR_MATCH_I_MATCHING_STRATEGY_H

#include "../models/User.h"
#include <memory>

namespace TutorMatch {

struct TutorMatchResult {
    std::shared_ptr<Tutor> tutor;
    double compositeScore; // 0.0 -> 100.0
    double subjectScore;
    double priceScore;
    double scheduleScore;
    double ratingScore;

    // Overload > operator for MaxHeap ordering with deterministic tie-breaking
    bool operator>(const TutorMatchResult& other) const {
        if (compositeScore != other.compositeScore) {
            return compositeScore > other.compositeScore;
        }
        if (ratingScore != other.ratingScore) {
            return ratingScore > other.ratingScore;
        }
        if (tutor && other.tutor) {
            if (tutor->getTotalReviews() != other.tutor->getTotalReviews()) {
                return tutor->getTotalReviews() > other.tutor->getTotalReviews();
            }
            return tutor->getId() < other.tutor->getId();
        }
        return false;
    }
    bool operator<(const TutorMatchResult& other) const {
        if (compositeScore != other.compositeScore) {
            return compositeScore < other.compositeScore;
        }
        if (ratingScore != other.ratingScore) {
            return ratingScore < other.ratingScore;
        }
        if (tutor && other.tutor) {
            if (tutor->getTotalReviews() != other.tutor->getTotalReviews()) {
                return tutor->getTotalReviews() < other.tutor->getTotalReviews();
            }
            return tutor->getId() > other.tutor->getId();
        }
        return false;
    }
};

// Polymorphic Matching Strategy Interface (OOP - Polymorphism)
class IMatchingStrategy {
public:
    virtual ~IMatchingStrategy() = default;
    virtual std::string getStrategyName() const = 0;
    virtual TutorMatchResult calculateScore(const Student& student, const Tutor& tutor) = 0;
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_I_MATCHING_STRATEGY_H
