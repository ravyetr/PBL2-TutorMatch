#ifndef TUTOR_MATCH_FILE_REVIEW_REPOSITORY_H
#define TUTOR_MATCH_FILE_REVIEW_REPOSITORY_H

#include "storage/IRepository.h"
#include "storage/serializers/ReviewSerializer.h"
#include "models/Review.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>
#include <filesystem>

namespace TutorMatch {

/**
 * @brief Concrete File-based Repository for Review entities.
 *
 * Implements Phase 2 Step 3 (Data Access Layer):
 * - Implements IReviewRepository contract (ISP & DIP).
 * - Manages persistent storage in reviews.dat (CSV format).
 * - Delegates CSV formatting/parsing to ReviewSerializer (SRP).
 * - Provides thread-safe append, deletion of violating reviews, and queries by tutor.
 * - Atomic persistence on review deletion with temporary file renaming.
 */
class FileReviewRepository : public IReviewRepository {
private:
    std::string reviewsFilePath;
    mutable std::recursive_mutex reviewRepoMutex;
    int nextReviewId;

public:
    explicit FileReviewRepository(const std::string& customFilePathOrDir = "");
    ~FileReviewRepository() override = default;

    bool initialize();

    // IReviewRepository Implementation
    int getNextReviewId() override;
    bool saveReview(const Review& review) override;
    bool deleteReview(int reviewId) override;
    std::vector<Review> getReviewsByTutorId(int tutorId) override;
    std::vector<Review> getAllReviews() override;

    // Diagnostics & Inspection Helpers
    const std::string& getFilePath() const { return reviewsFilePath; }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_FILE_REVIEW_REPOSITORY_H
