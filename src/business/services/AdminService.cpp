#include "business/services/AdminService.h"
#include <algorithm>
#include <cctype>

namespace TutorMatch {

AdminService::AdminService(std::shared_ptr<IUserRepository> uRepo,
                           std::shared_ptr<IReviewRepository> rRepo,
                           std::shared_ptr<BookingService> bService)
    : userRepo(std::move(uRepo)),
      reviewRepo(std::move(rRepo)),
      bookingService(std::move(bService)) {}

AdminMetrics AdminService::calculateMetrics() {
    std::lock_guard<std::mutex> lock(adminMutex);
    AdminMetrics m;
    if (!userRepo) return m;

    m.totalTutors = static_cast<int>(userRepo->getAllTutors().size());
    m.totalStudents = static_cast<int>(userRepo->getAllStudents().size());

    if (bookingService) {
        auto courseBookings = bookingService->getAllCourseBookings();
        m.totalBookings = static_cast<int>(courseBookings.size());
        for (const auto& cb : courseBookings) {
            if (cb.status == "COMPLETED") {
                m.completedBookings++;
            } else if (cb.status == "ACCEPTED") {
                m.activeMatches++;
            }
        }
    }

    return m;
}

std::vector<std::shared_ptr<User>> AdminService::getAllUsers() {
    if (!userRepo) return {};
    return userRepo->getAllUsers();
}

std::vector<std::shared_ptr<User>> AdminService::getFilteredUsers(const std::string& filter,
                                                                 const std::string& searchKeyword) {
    std::vector<std::shared_ptr<User>> result;
    if (!userRepo) return result;

    auto users = userRepo->getAllUsers();

    std::string upperFilter = filter;
    std::transform(upperFilter.begin(), upperFilter.end(), upperFilter.begin(), [](unsigned char c) { return std::toupper(c); });

    std::string kw = searchKeyword;
    std::transform(kw.begin(), kw.end(), kw.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const auto& u : users) {
        if (!u) continue;

        bool roleMatches = false;
        if (upperFilter == "ALL" || upperFilter.empty()) {
            roleMatches = true;
        } else if (upperFilter == "STUDENT" && u->getRole() == UserRole::STUDENT) {
            roleMatches = true;
        } else if ((upperFilter == "TUTOR" || upperFilter == "ALL_TUTORS") && u->getRole() == UserRole::TUTOR) {
            roleMatches = true;
        } else if (upperFilter == "LOCKED" && !u->getIsActive()) {
            roleMatches = true;
        }

        if (!roleMatches) continue;

        // Keyword filter
        if (!kw.empty()) {
            std::string nameLower = u->getFullName();
            std::string emailLower = u->getEmail();
            std::string userLower = u->getUsername();
            std::string subjLower = "";
            if (u->getRole() == UserRole::TUTOR) {
                subjLower = std::static_pointer_cast<Tutor>(u)->getSubjectSpecialty();
            } else if (u->getRole() == UserRole::STUDENT) {
                subjLower = "lớp " + std::to_string(std::static_pointer_cast<Student>(u)->getGradeLevel());
            }
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(userLower.begin(), userLower.end(), userLower.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(subjLower.begin(), subjLower.end(), subjLower.begin(), [](unsigned char c) { return std::tolower(c); });

            if (nameLower.find(kw) == std::string::npos &&
                emailLower.find(kw) == std::string::npos &&
                userLower.find(kw) == std::string::npos &&
                subjLower.find(kw) == std::string::npos) {
                continue;
            }
        }

        result.push_back(u);
    }

    return result;
}

bool AdminService::toggleUserLock(int userId, std::string& outMsg) {
    if (!userRepo) return false;
    auto u = userRepo->findUserById(userId);
    if (u) {
        u->setIsActive(!u->getIsActive());
        userRepo->updateUser(*u);
        outMsg = "Tài khoản " + u->getFullName() + " hiện đã chuyển sang trạng thái: " + (u->getIsActive() ? "HOẠT ĐỘNG" : "BỊ KHÓA");
        return true;
    }
    outMsg = "Không tìm thấy người dùng!";
    return false;
}

bool AdminService::deleteViolatingReview(int reviewId, std::string& outMsg) {
    if (!reviewRepo || !userRepo) return false;

    auto tutors = userRepo->getAllTutors();
    int tutorId = 0;
    bool found = false;

    for (const auto& t : tutors) {
        auto revs = reviewRepo->getReviewsByTutorId(t->getId());
        for (const auto& r : revs) {
            if (r.id == reviewId) {
                tutorId = t->getId();
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        outMsg = "Không tìm thấy đánh giá!";
        return false;
    }

    if (reviewRepo->deleteReview(reviewId)) {
        // Recalculate average rating for the tutor
        auto tutorU = userRepo->findUserById(tutorId);
        if (tutorU && tutorU->getRole() == UserRole::TUTOR) {
            auto t = std::static_pointer_cast<Tutor>(tutorU);
            auto allRevs = reviewRepo->getReviewsByTutorId(t->getId());
            double sum = 0.0;
            for (const auto& r : allRevs) {
                sum += r.ratingScore;
            }
            double newAvg = allRevs.empty() ? 0.0 : (sum / allRevs.size());
            t->setRating(newAvg);
            t->setTotalReviews(static_cast<int>(allRevs.size()));
            userRepo->updateUser(*t);
        }
        outMsg = "Xóa đánh giá vi phạm tiêu chuẩn thành công!";
        return true;
    }

    outMsg = "Lỗi khi xóa bản ghi đánh giá!";
    return false;
}

bool AdminService::terminateMatch(int bookingId, std::string& outMsg) {
    if (!bookingService) {
        outMsg = "Booking service chưa sẵn sàng!";
        return false;
    }
    return bookingService->cancelCourseBooking(0, UserRole::ADMIN, bookingId, outMsg);
}

} // namespace TutorMatch
