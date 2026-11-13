#ifndef TUTOR_MATCH_REVIEW_SERIALIZER_H
#define TUTOR_MATCH_REVIEW_SERIALIZER_H

#include "business/domain/Review.h"
#include <string>
#include <optional>

namespace TutorMatch {

/**
 * @brief Dedicated Serializer for Review domain entity.
 *
 * Implements Phase 2 Step 2:
 * - Decouples serialization/deserialization from Review value struct.
 * - Preserves 100% backward compatibility with legacy reviews.dat CSV format.
 * - Validates 8 canonical fields and 1..5 star rating invariant.
 * - Zero File I/O, zero Qt dependencies.
 */
class ReviewSerializer {
public:
    static std::string toCsv(const Review& review);
    static std::optional<Review> fromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static bool fromCsv(const std::string& csvLine, Review& outReview, std::string* outError = nullptr);

    static bool isValidRatingScore(int ratingScore);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_REVIEW_SERIALIZER_H
