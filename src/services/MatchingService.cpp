#include "services/MatchingService.h"
#include <algorithm>

namespace TutorMatch {

MatchingService::MatchingService(std::shared_ptr<IUserRepository> uRepo,
                                 std::unique_ptr<MatchingEngine> engine)
    : userRepo(std::move(uRepo)),
      matchingEngine(std::move(engine)) {
    if (!matchingEngine) {
        matchingEngine = std::make_unique<MatchingEngine>();
    }
}

std::vector<TutorMatchResult> MatchingService::findTopMatchesForStudent(int studentId, size_t topK) {
    std::lock_guard<std::mutex> lock(serviceMutex);

    if (!userRepo || !matchingEngine) return {};

    auto u = userRepo->findUserById(studentId);
    if (!u || u->getRole() != UserRole::STUDENT) return {};

    auto student = std::static_pointer_cast<Student>(u);
    auto allTutors = userRepo->getAllTutors();

    std::vector<std::shared_ptr<Tutor>> activeTutors;
    activeTutors.reserve(allTutors.size());
    for (const auto& t : allTutors) {
        if (t && t->getIsActive()) {
            activeTutors.push_back(t);
        }
    }

    return matchingEngine->findTopMatches(*student, activeTutors, topK);
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

    return matchingEngine->findTopMatches(student, activeTutors, topK);
}

void MatchingService::setMatchingStrategy(std::unique_ptr<IMatchingStrategy> strategy) {
    std::lock_guard<std::mutex> lock(serviceMutex);
    if (matchingEngine) {
        matchingEngine->setStrategy(std::move(strategy));
    }
}

} // namespace TutorMatch
