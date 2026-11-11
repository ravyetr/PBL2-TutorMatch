#include "../../include/algorithms/MatchingEngine.h"
#include "../../include/ds/ScheduleGuard.h"
#include "../../include/models/SubjectTaxonomy.h"
#include "../../include/utils/DateUtils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace TutorMatch {

namespace {

std::string formatCurrencyHelper(double amount) {
    long long rounded = static_cast<long long>(std::round(amount));
    std::string s = std::to_string(rounded);
    std::string result;
    int count = 0;
    for (int i = static_cast<int>(s.length()) - 1; i >= 0; --i) {
        result.push_back(s[i]);
        count++;
        if (count % 3 == 0 && i > 0) {
            result.push_back('.');
        }
    }
    std::reverse(result.begin(), result.end());
    return result + " VNĐ/buổi";
}

std::string formatDoubleHelper(double val, int decimals = 1) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << val;
    return oss.str();
}

// Checks whether an active ACCEPTED course booking conflicts with slot (day, slotIndex)
// during the course timeframe [startDate, startDate + weeksCount].
bool isSlotConflictedWithActiveBookings(int tutorId, int day, int slotIndex,
                                       const std::string& startDate, int weeksCount,
                                       const std::vector<CourseBooking>& activeBookings) {
    if (activeBookings.empty() || startDate.empty() || weeksCount <= 0) return false;

    std::string targetEndDate = DateUtils::getEndDate(startDate, weeksCount);

    for (const auto& cb : activeBookings) {
        if (cb.tutorId != tutorId) continue;
        if (cb.status != "ACCEPTED") continue; // Only ACCEPTED course bookings reserve slots

        // Check if cb includes (day, slotIndex)
        bool hasSlot = false;
        auto cbSlots = cb.getWeeklySlots();
        if (cbSlots.empty()) {
            hasSlot = (cb.dayOfWeek == day && cb.slotIndex == slotIndex);
        } else {
            for (const auto& p : cbSlots) {
                if (p.first == day && p.second == slotIndex) {
                    hasSlot = true;
                    break;
                }
            }
        }
        if (!hasSlot) continue;

        // Check date range overlap between [startDate, targetEndDate] and [cb.startDate, cbEndDate]
        std::string cbEndDate = DateUtils::getEndDate(cb.startDate, cb.weeksCount);
        std::string overlapStart = (DateUtils::compareDates(cb.startDate, startDate) > 0) ? cb.startDate : startDate;
        std::string overlapEnd = (DateUtils::compareDates(cbEndDate, targetEndDate) < 0) ? cbEndDate : targetEndDate;

        if (DateUtils::compareDates(overlapStart, overlapEnd) <= 0) {
            // If cb was early terminated, sessions after actualEndDate were cancelled
            if (cb.isEarlyTerminated() && !cb.actualEndDate.empty()) {
                if (DateUtils::compareDates(overlapStart, cb.actualEndDate) > 0) {
                    continue;
                }
            }
            return true; // Conflict found!
        }
    }
    return false;
}

} // anonymous namespace

TutorMatchResult WeightedScoreMatchingStrategy::calculateScore(const Student& student, const Tutor& tutor) {
    SmartMatchCriteria criteria;
    criteria.gradeLevel = student.getGradeLevel();
    criteria.preferredScheduleMask = student.getScheduleBitmask();
    criteria.strictSchedule = false; // Legacy fallback
    return calculateScoreWithCriteria(criteria, tutor, {});
}

TutorMatchResult WeightedScoreMatchingStrategy::calculateScoreWithCriteria(
    const SmartMatchCriteria& criteria,
    const Tutor& tutor,
    const std::vector<CourseBooking>& activeBookings) {

    TutorMatchResult res;
    res.tutor = std::make_shared<Tutor>(tutor);

    auto requestedSlots = criteria.getRequestedWeeklySlots();
    res.requestedSlotCount = static_cast<int>(requestedSlots.size());

    // 1. Evaluate Matching Available Schedule Slots
    std::vector<std::pair<int, int>> matched;
    uint64_t tutorMask = tutor.getScheduleBitmask();

    if (!requestedSlots.empty()) {
        for (const auto& p : requestedSlots) {
            int d = p.first;
            int s = p.second;
            uint64_t sMask = (1ULL << (d * 8 + s));
            if ((tutorMask & sMask) != 0) {
                if (!isSlotConflictedWithActiveBookings(tutor.getId(), d, s, criteria.startDate, criteria.weeksCount, activeBookings)) {
                    matched.push_back(p);
                }
            }
        }
        res.matchedSlotCount = static_cast<int>(matched.size());
        res.matchedSlots = matched;

        if (criteria.strictSchedule) {
            res.scheduleScore = (res.matchedSlotCount == res.requestedSlotCount) ? weightSchedule : 0.0;
        } else {
            double ratio = (res.requestedSlotCount > 0) ? (static_cast<double>(res.matchedSlotCount) / res.requestedSlotCount) : 1.0;
            res.scheduleScore = weightSchedule * std::max(0.0, std::min(1.0, ratio));
        }
    } else {
        // No specific slots requested: evaluate general availability
        int totalTutorSlots = ScheduleGuard::countMatchingSlots(tutorMask, 0xFFFFFFFFFFFFFFFFULL);
        res.matchedSlotCount = totalTutorSlots;
        double ratio = std::min(1.0, static_cast<double>(totalTutorSlots) / 8.0);
        res.scheduleScore = weightSchedule * ratio;
    }

    // 2. Price Competitiveness Score (25%) - Monotonic: lower valid price yields higher score
    double budget = std::max(criteria.maxBudget, 1.0);
    double rate = std::max(0.0, tutor.getHourlyRate());
    if (rate <= budget) {
        // When rate == budget -> 50% of price points (12.5 pts)
        // When rate == 0.5 * budget -> 75% of price points (18.75 pts)
        // When rate -> 0 -> 100% of price points (25.0 pts)
        double priceRatio = 1.0 - (rate / (2.0 * budget));
        res.priceScore = weightPrice * std::max(0.0, std::min(1.0, priceRatio));
    } else {
        res.priceScore = 0.0;
    }

    // 3. Student Review Rating Score (15%) - Bayesian Confidence Weighted
    // Prevents single review domination and gracefully scores tutors with 0 reviews
    double rawRating = tutor.getRating();
    if (rawRating < 1.0) rawRating = 4.0;
    int reviewCount = tutor.getTotalReviews();
    double bayesianRating = (3.0 * 4.0 + reviewCount * rawRating) / (3.0 + reviewCount);
    res.ratingScore = weightRating * std::max(0.0, std::min(1.0, bayesianRating / 5.0));

    // 4. Relevant Teaching Experience & Level Fit Score (15%)
    ParsedSubject tutorSubj = SubjectTaxonomy::parseSubject(tutor.getSubjectSpecialty());
    int targetGrade = criteria.gradeLevel;
    double gradePoints = 4.0; // Baseline compatibility
    if (targetGrade > 0 && tutorSubj.code != CanonicalSubject::OTHER) {
        if (targetGrade >= tutorSubj.minGrade && targetGrade <= tutorSubj.maxGrade) {
            gradePoints = 10.0; // Exact grade coverage
        } else if (std::abs(targetGrade - tutorSubj.minGrade) <= 1 || std::abs(targetGrade - tutorSubj.maxGrade) <= 1) {
            gradePoints = 7.0;  // Adjacent grade
        }
    } else if (targetGrade == 0) {
        gradePoints = 8.0;
    }

    // Depth & Program Specialty (e.g. THPT, Luyện thi, HSG, Chuyên, IELTS)
    double depthPoints = 2.0;
    std::string normSpecialty = SubjectTaxonomy::normalizeText(tutor.getSubjectSpecialty());
    std::string normBio = SubjectTaxonomy::normalizeText(tutor.getBio());
    if (normSpecialty.find("luyen thi") != std::string::npos ||
        normSpecialty.find("thpt") != std::string::npos ||
        normSpecialty.find("chuyen") != std::string::npos ||
        normSpecialty.find("hsg") != std::string::npos ||
        normSpecialty.find("ielts") != std::string::npos ||
        normSpecialty.find("dai hoc") != std::string::npos ||
        normBio.length() >= 50) {
        depthPoints = 5.0;
    }
    res.experienceScore = std::min(weightExperience, gradePoints + depthPoints);

    // 5. Profile Completeness Score (10%)
    double credPoints = 0.0;
    if (tutor.getBio().length() >= 50) credPoints += 6.0;
    else if (tutor.getBio().length() >= 20) credPoints += 4.0;
    else if (!tutor.getBio().empty()) credPoints += 2.0;

    if (!tutor.getSubjectSpecialty().empty()) credPoints += 4.0;
    res.credibilityScore = std::min(weightCredibility, credPoints);

    // Composite Total Score (0.0 to 100.0)
    res.compositeScore = res.scheduleScore + res.priceScore + res.ratingScore +
                         res.experienceScore + res.credibilityScore;
    res.compositeScore = std::max(0.0, std::min(100.0, res.compositeScore));

    // Generate Explainability Bullets (Phase E)
    if (!criteria.subject.empty()) {
        res.explanations.push_back("Chuyên môn phù hợp môn học yêu cầu (" + tutor.getSubjectSpecialty() + ").");
    }

    if (res.requestedSlotCount > 0) {
        if (res.matchedSlotCount == res.requestedSlotCount) {
            res.explanations.push_back("Khớp toàn bộ " + std::to_string(res.requestedSlotCount) + " ca học mong muốn.");
        } else if (res.matchedSlotCount > 0) {
            res.explanations.push_back("Khớp " + std::to_string(res.matchedSlotCount) + "/" +
                                       std::to_string(res.requestedSlotCount) + " ca học mong muốn.");
        }
    } else {
        res.explanations.push_back("Gia sư có " + std::to_string(res.matchedSlotCount) + " ca rảnh cố định trong tuần.");
    }

    if (rate <= 0.85 * budget) {
        res.explanations.push_back("Học phí (" + formatCurrencyHelper(rate) + ") thấp hơn ngân sách tối đa.");
    } else {
        res.explanations.push_back("Học phí (" + formatCurrencyHelper(rate) + ") phù hợp với ngân sách cho phép.");
    }

    if (targetGrade > 0 && targetGrade >= tutorSubj.minGrade && targetGrade <= tutorSubj.maxGrade) {
        res.explanations.push_back("Có chuyên môn giảng dạy đúng khối lớp " + std::to_string(targetGrade) + ".");
    }

    if (tutor.getRating() >= 4.5 && tutor.getTotalReviews() >= 1) {
        res.explanations.push_back("Được đánh giá cao bởi học viên trước (" +
                                   formatDoubleHelper(tutor.getRating()) + "⭐, " +
                                   std::to_string(tutor.getTotalReviews()) + " lượt đánh giá).");
    }

    if (tutor.getBio().length() >= 50) {
        res.explanations.push_back("Hồ sơ giới thiệu chi tiết về phương pháp và kinh nghiệm giảng dạy.");
    }

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

bool MatchingEngine::isEligible(const SmartMatchCriteria& criteria,
                                const Tutor& tutor,
                                const std::vector<CourseBooking>& activeBookings,
                                std::vector<std::pair<int, int>>& outMatchedSlots) {
    outMatchedSlots.clear();

    // Stage A1: Requested subject compatibility — mandatory first condition
    // A tutor who does not teach the requested subject must be excluded entirely.
    if (criteria.subject.empty() || tutor.getSubjectSpecialty().empty()) {
        return false;
    }
    if (!SubjectTaxonomy::matchesFilter(tutor.getSubjectSpecialty(), criteria.subject)) {
        return false;
    }

    // Stage A2: Tutor account eligibility/status
    if (!tutor.getIsActive()) {
        return false;
    }

    // 3. Educational Level Fit
    if (criteria.gradeLevel > 0) {
        ParsedSubject tutorSubj = SubjectTaxonomy::parseSubject(tutor.getSubjectSpecialty());
        if (tutorSubj.code != CanonicalSubject::OTHER) {
            // Tutor must teach grade level within reasonable tolerance
            if (criteria.gradeLevel < tutorSubj.minGrade - 1 || criteria.gradeLevel > tutorSubj.maxGrade + 1) {
                return false;
            }
        }
    }

    // 4. Maximum Tuition per Session
    if (criteria.maxBudget > 0.0 && tutor.getHourlyRate() > criteria.maxBudget) {
        return false;
    }

    // 5. Schedule & Active Reservation Compatibility
    auto requestedSlots = criteria.getRequestedWeeklySlots();
    uint64_t tutorMask = tutor.getScheduleBitmask();

    if (!requestedSlots.empty()) {
        for (const auto& p : requestedSlots) {
            int d = p.first;
            int s = p.second;
            uint64_t sMask = (1ULL << (d * 8 + s));
            if ((tutorMask & sMask) != 0) {
                if (!isSlotConflictedWithActiveBookings(tutor.getId(), d, s, criteria.startDate, criteria.weeksCount, activeBookings)) {
                    outMatchedSlots.push_back(p);
                }
            }
        }

        if (criteria.strictSchedule) {
            // In strict mode: ALL requested slots must be available and conflict-free!
            if (outMatchedSlots.size() < requestedSlots.size()) {
                return false;
            }
        } else {
            // In flexible mode: At least ONE slot must be matched and conflict-free
            if (outMatchedSlots.empty()) {
                return false;
            }
        }
    }

    // 6. Optional Preferences (only enforced when explicitly selected by student)
    if (criteria.minRating > 0.0 && tutor.getRating() < criteria.minRating) {
        return false;
    }

    return true;
}

std::vector<TutorMatchResult> MatchingEngine::findTopMatchesWithCriteria(
    const SmartMatchCriteria& criteria,
    const std::vector<std::shared_ptr<Tutor>>& tutors,
    const std::vector<CourseBooking>& activeBookings,
    size_t topK) {

    MaxHeap<TutorMatchResult> maxHeap;

    for (const auto& tutorPtr : tutors) {
        if (!tutorPtr) continue;

        std::vector<std::pair<int, int>> matchedSlots;
        if (!isEligible(criteria, *tutorPtr, activeBookings, matchedSlots)) {
            continue; // Exclude tutors who fail any hard filter
        }

        TutorMatchResult res = activeStrategy->calculateScoreWithCriteria(criteria, *tutorPtr, activeBookings);
        maxHeap.push(res);
    }

    return maxHeap.extractSortedTopK(topK);
}

std::vector<TutorMatchResult> MatchingEngine::findTopMatches(
    const Student& student,
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
