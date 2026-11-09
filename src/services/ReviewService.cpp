#include "services/ReviewService.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <unordered_map>

namespace TutorMatch {

static std::string getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm buf;
#if defined(_WIN32)
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf);
#endif
    ss << std::put_time(&buf, "%Y-%m-%d");
    return ss.str();
}

ReviewService::ReviewService(std::shared_ptr<IReviewRepository> rRepo,
                             std::shared_ptr<IUserRepository> uRepo,
                             std::shared_ptr<ICourseBookingRepository> cbRepo)
    : reviewRepo(std::move(rRepo)),
      userRepo(std::move(uRepo)),
      courseBookingRepo(std::move(cbRepo)) {}

bool ReviewService::isEligibleForReview(int studentId, int bookingId, std::string& outReason) const {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);

    if (!courseBookingRepo) {
        outReason = "Dịch vụ đơn đặt lịch không khả dụng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(bookingId);
    if (!bPtr) {
        outReason = "Không tìm thấy thông tin đơn đặt lịch!";
        return false;
    }

    if (bPtr->studentId != studentId) {
        outReason = "Đơn đặt lịch không thuộc về bạn!";
        return false;
    }

    if (bPtr->status != "COMPLETED" && bPtr->status != "EARLY_TERMINATED") {
        if (bPtr->status == "PENDING") {
            outReason = "Khóa học đang chờ duyệt, chưa thể đánh giá!";
        } else if (bPtr->status == "ACCEPTED") {
            outReason = "Khóa học đang diễn ra, chỉ có thể đánh giá sau khi hoàn thành hoặc kết thúc sớm!";
        } else if (bPtr->status == "REJECTED") {
            outReason = "Khóa học đã bị từ chối, không thể đánh giá!";
        } else if (bPtr->status == "CANCELLED") {
            outReason = "Khóa học đã bị hủy, không thể đánh giá!";
        } else {
            outReason = "Chỉ có thể đánh giá khóa học sau khi đã hoàn thành hoặc kết thúc sớm!";
        }
        return false;
    }

    if (reviewRepo) {
        auto existingRevs = reviewRepo->getAllReviews();
        for (const auto& r : existingRevs) {
            if (r.bookingId == bookingId && r.studentId == studentId) {
                outReason = "Bạn đã gửi đánh giá cho khóa học này rồi!";
                return false;
            }
        }
    }

    return true;
}

bool ReviewService::submitReview(int studentId, int bookingId, int ratingScore,
                                 const std::string& comment, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);

    if (!reviewRepo || !userRepo) {
        outMsg = "Hệ thống dữ liệu đánh giá chưa sẵn sàng!";
        return false;
    }

    if (ratingScore < 1 || ratingScore > 5) {
        outMsg = "Điểm đánh giá phải từ 1 đến 5 sao!";
        return false;
    }

    if (comment.empty()) {
        outMsg = "Nội dung nhận xét không được để trống!";
        return false;
    }

    int tutorId = 0;
    if (courseBookingRepo) {
        std::string reason;
        if (!isEligibleForReview(studentId, bookingId, reason)) {
            outMsg = reason;
            return false;
        }
        auto bPtr = courseBookingRepo->findCourseBookingById(bookingId);
        if (!bPtr) {
            outMsg = "Không tìm thấy thông tin đơn đặt lịch!";
            return false;
        }
        tutorId = bPtr->tutorId;
    }

    auto sUser = userRepo->findUserById(studentId);
    if (!sUser) {
        outMsg = "Không tìm thấy thông tin học viên!";
        return false;
    }

    int nextRId = reviewRepo->getNextReviewId();
    Review rev(nextRId, bookingId, studentId, tutorId, sUser->getFullName(),
               ratingScore, comment, getCurrentDateString());

    if (!reviewRepo->saveReview(rev)) {
        outMsg = "Lỗi khi lưu bản ghi đánh giá!";
        return false;
    }

    if (tutorId > 0) {
        auto tutorU = userRepo->findUserById(tutorId);
        if (tutorU && tutorU->getRole() == UserRole::TUTOR) {
            auto t = std::static_pointer_cast<Tutor>(tutorU);
            auto allRevs = reviewRepo->getReviewsByTutorId(tutorId);
            double sum = 0.0;
            for (const auto& r : allRevs) sum += r.ratingScore;
            double newAvg = allRevs.empty() ? 0.0 : (sum / allRevs.size());
            t->setRating(newAvg);
            t->setTotalReviews(static_cast<int>(allRevs.size()));
            if (!userRepo->updateUser(*t)) {
                // ADR-007 Compensating rollback: delete review to prevent cross-file inconsistency
                reviewRepo->deleteReview(nextRId);
                outMsg = "Lỗi khi cập nhật điểm đánh giá gia sư! Thao tác đã được hoàn tác an toàn.";
                return false;
            }
        }
    }

    outMsg = "Gửi đánh giá và nhận xét thành công!";
    return true;
}

bool ReviewService::deleteReview(int reviewId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);

    if (!reviewRepo || !userRepo) {
        outMsg = "Hệ thống dữ liệu đánh giá chưa sẵn sàng!";
        return false;
    }

    int tutorId = 0;
    auto all = reviewRepo->getAllReviews();
    bool found = false;
    for (const auto& r : all) {
        if (r.id == reviewId) {
            tutorId = r.tutorId;
            found = true;
            break;
        }
    }

    if (!found) {
        outMsg = "Không tìm thấy bản ghi đánh giá!";
        return false;
    }

    if (!reviewRepo->deleteReview(reviewId)) {
        outMsg = "Lỗi khi xóa bản ghi đánh giá!";
        return false;
    }

    if (tutorId > 0) {
        recalculateTutorRating(tutorId);
    }

    outMsg = "Xóa đánh giá thành công!";
    return true;
}

std::vector<Review> ReviewService::getReviewsByTutorId(int tutorId) const {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);
    if (!reviewRepo) return {};
    return reviewRepo->getReviewsByTutorId(tutorId);
}

std::vector<Review> ReviewService::getReviewsByStudentId(int studentId) const {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);
    std::vector<Review> result;
    if (!reviewRepo) return result;
    auto all = reviewRepo->getAllReviews();
    for (const auto& r : all) {
        if (r.studentId == studentId) {
            result.push_back(r);
        }
    }
    return result;
}

std::vector<Review> ReviewService::getAllReviews() const {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);
    if (!reviewRepo) return {};
    return reviewRepo->getAllReviews();
}

void ReviewService::recalculateTutorRating(int tutorId) {
    if (!userRepo || !reviewRepo) return;

    auto tutorU = userRepo->findUserById(tutorId);
    if (tutorU && tutorU->getRole() == UserRole::TUTOR) {
        auto t = std::static_pointer_cast<Tutor>(tutorU);
        auto allRevs = reviewRepo->getReviewsByTutorId(tutorId);

        double sum = 0.0;
        for (const auto& r : allRevs) {
            sum += r.ratingScore;
        }

        double newAvg = allRevs.empty() ? 0.0 : (sum / allRevs.size());
        t->setRating(newAvg);
        t->setTotalReviews(static_cast<int>(allRevs.size()));
        userRepo->updateUser(*t);
    }
}

bool ReviewService::calculateTutorRating(int tutorId, double& outRating, int& outCount) const {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);
    if (!reviewRepo) return false;

    auto allRevs = reviewRepo->getReviewsByTutorId(tutorId);
    outCount = static_cast<int>(allRevs.size());

    double sum = 0.0;
    for (const auto& r : allRevs) {
        sum += r.ratingScore;
    }

    outRating = allRevs.empty() ? 0.0 : (sum / allRevs.size());
    return true;
}

void ReviewService::reconcileRatingsOnStartup() {
    std::lock_guard<std::recursive_mutex> lock(reviewMutex);
    if (!userRepo || !reviewRepo) return;

    try {
        auto allReviews = reviewRepo->getAllReviews();
        std::unordered_map<int, std::pair<double, int>> tutorStats; // tutorId -> (sumScores, count)

        for (const auto& r : allReviews) {
            if (r.tutorId > 0) {
                tutorStats[r.tutorId].first += r.ratingScore;
                tutorStats[r.tutorId].second += 1;
            }
        }

        auto tutors = userRepo->getAllTutors();
        for (const auto& t : tutors) {
            if (!t) continue;
            int tId = t->getId();
            double expectedRating = 0.0;
            int expectedCount = 0;

            if (tutorStats.find(tId) != tutorStats.end()) {
                expectedCount = tutorStats[tId].second;
                expectedRating = (expectedCount > 0) ? (tutorStats[tId].first / expectedCount) : 0.0;
            }

            bool countDiff = (t->getTotalReviews() != expectedCount);
            bool ratingDiff = (std::abs(t->getRating() - expectedRating) > 0.01);

            if (countDiff || ratingDiff) {
                std::cout << "[ReviewService Self-Healing] Reconciling rating for Tutor ID " << tId
                          << " (Rating: " << t->getRating() << " -> " << expectedRating
                          << ", Reviews: " << t->getTotalReviews() << " -> " << expectedCount << ")" << std::endl;
                t->setRating(expectedRating);
                t->setTotalReviews(expectedCount);
                userRepo->updateUser(*t);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "[ReviewService Warning] Self-healing reconciliation error: " << ex.what() << std::endl;
    }
}

} // namespace TutorMatch
