#include "services/MatchingService.h"
#include <algorithm>

namespace TutorMatch {

MatchingService::MatchingService(std::shared_ptr<IUserRepository> uRepo,
                                 std::shared_ptr<ICourseBookingRepository> cbRepo,
                                 std::unique_ptr<MatchingEngine> engine)
    : userRepo(std::move(uRepo)),
      courseBookingRepo(std::move(cbRepo)),
      matchingEngine(std::move(engine)) {
    if (!matchingEngine) {
        matchingEngine = std::make_unique<MatchingEngine>();
    }
}

std::vector<TutorMatchResult> MatchingService::findMatchesWithCriteria(const SmartMatchCriteria& criteria, size_t topK) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (!userRepo || !matchingEngine) return {};

    auto allTutors = userRepo->getAllTutors();
    std::vector<std::shared_ptr<Tutor>> activeTutors;
    activeTutors.reserve(allTutors.size());
    for (const auto& t : allTutors) {
        if (t && t->getIsActive()) {
            activeTutors.push_back(t);
        }
    }

    std::vector<CourseBooking> activeBookings;
    if (courseBookingRepo) {
        activeBookings = courseBookingRepo->getAllCourseBookings();
    }

    return matchingEngine->findTopMatchesWithCriteria(criteria, activeTutors, activeBookings, topK);
}

std::vector<TutorMatchResult> MatchingService::findTopMatchesForStudent(int studentId, size_t topK) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (!userRepo || !matchingEngine) return {};

    auto u = userRepo->findUserById(studentId);
    if (!u || u->getRole() != UserRole::STUDENT) return {};

    auto student = std::static_pointer_cast<Student>(u);
    return findMatches(*student, topK);
}

std::vector<TutorMatchResult> MatchingService::findMatches(const Student& student, size_t topK) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (!userRepo || !matchingEngine) return {};

    auto allTutors = userRepo->getAllTutors();
    std::vector<std::shared_ptr<Tutor>> activeTutors;
    activeTutors.reserve(allTutors.size());
    for (const auto& t : allTutors) {
        if (t && t->getIsActive()) {
            activeTutors.push_back(t);
        }
    }

    std::vector<CourseBooking> activeBookings;
    if (courseBookingRepo) {
        activeBookings = courseBookingRepo->getAllCourseBookings();
    }

    SmartMatchCriteria criteria;
    criteria.gradeLevel = student.getGradeLevel();
    criteria.preferredScheduleMask = student.getScheduleBitmask();
    criteria.strictSchedule = false; // Legacy fallback

    return matchingEngine->findTopMatchesWithCriteria(criteria, activeTutors, activeBookings, topK);
}

void MatchingService::setMatchingStrategy(std::unique_ptr<IMatchingStrategy> strategy) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    if (matchingEngine) {
        matchingEngine->setStrategy(std::move(strategy));
    }
}

} // namespace TutorMatch
