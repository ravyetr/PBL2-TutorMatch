#ifndef TUTOR_MATCH_I_MATCHING_STRATEGY_H
#define TUTOR_MATCH_I_MATCHING_STRATEGY_H

#include "../models/User.h"
#include "../models/CourseBooking.h"
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace TutorMatch {

struct SmartMatchCriteria {
    std::string subject;                // Selected subject name/code (e.g., "Toán", "Vật lý")
    int gradeLevel = 10;                // Target grade/level (1-12, or 0 for any/other)
    double maxBudget = 200000.0;        // Maximum tuition per session (VND)
    uint64_t preferredScheduleMask = 0; // 7x8 weekly slot bitmask (0 = no specific preference)
    std::string startDate;              // Planned start date (YYYY-MM-DD)
    int weeksCount = 4;                 // Duration: 4, 8, or 12 weeks
    bool strictSchedule = true;         // true = strict (all requested slots required), false = flexible
    double minRating = 0.0;             // Optional filter: minimum rating threshold (e.g. 4.0)

    std::vector<std::pair<int, int>> getRequestedWeeklySlots() const {
        std::vector<std::pair<int, int>> reqSlots;
        if (preferredScheduleMask == 0) return reqSlots;
        for (int day = 0; day < 7; ++day) {
            for (int slot = 0; slot < 8; ++slot) {
                if ((preferredScheduleMask & (1ULL << (day * 8 + slot))) != 0) {
                    reqSlots.emplace_back(day, slot);
                }
            }
        }
        return reqSlots;
    }
};

struct TutorMatchResult {
    std::shared_ptr<Tutor> tutor;
    double compositeScore = 0.0; // 0.0 -> 100.0 (Total explainable SmartMatch score)
    double scheduleScore = 0.0;  // 0.0 -> 35.0
    double priceScore = 0.0;     // 0.0 -> 25.0
    double ratingScore = 0.0;    // 0.0 -> 15.0
    double experienceScore = 0.0;// 0.0 -> 15.0
    double subjectScore = 0.0;   // Backward-compatible alias for strategy adapters
    double credibilityScore = 0.0;// 0.0 -> 10.0

    // Match details & explainability
    int matchedSlotCount = 0;
    int requestedSlotCount = 0;
    std::vector<std::pair<int, int>> matchedSlots;
    std::vector<std::string> explanations; // Concise Vietnamese explanation bullets

    // Deterministic tie-breaking for MaxHeap / Priority Queue:
    // 1. compositeScore descending
    // 2. scheduleScore descending
    // 3. priceScore descending
    // 4. tutor ID ascending (deterministic tie-breaker)
    bool operator>(const TutorMatchResult& other) const {
        if (compositeScore != other.compositeScore) {
            return compositeScore > other.compositeScore;
        }
        if (scheduleScore != other.scheduleScore) {
            return scheduleScore > other.scheduleScore;
        }
        if (priceScore != other.priceScore) {
            return priceScore > other.priceScore;
        }
        if (tutor && other.tutor) {
            return tutor->getId() < other.tutor->getId();
        }
        return false;
    }

    bool operator<(const TutorMatchResult& other) const {
        if (compositeScore != other.compositeScore) {
            return compositeScore < other.compositeScore;
        }
        if (scheduleScore != other.scheduleScore) {
            return scheduleScore < other.scheduleScore;
        }
        if (priceScore != other.priceScore) {
            return priceScore < other.priceScore;
        }
        if (tutor && other.tutor) {
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

    // Legacy interface for backward compatibility
    virtual TutorMatchResult calculateScore(const Student& student, const Tutor& tutor) = 0;

    // Rich criteria-based matching interface (Phase D)
    virtual TutorMatchResult calculateScoreWithCriteria(const SmartMatchCriteria& criteria,
                                                        const Tutor& tutor,
                                                        const std::vector<CourseBooking>& activeBookings = {}) {
        (void)activeBookings;
        Student dummyStudent;
        dummyStudent.setGradeLevel(criteria.gradeLevel);
        dummyStudent.setScheduleBitmask(criteria.preferredScheduleMask);
        return calculateScore(dummyStudent, tutor);
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_I_MATCHING_STRATEGY_H
