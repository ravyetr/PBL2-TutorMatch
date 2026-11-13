#include "data_access/serializers/ReviewSerializer.h"
#include "data_access/serializers/SerializerUtils.h"

#include <sstream>

namespace TutorMatch {

bool ReviewSerializer::isValidRatingScore(int ratingScore) {
    return ratingScore >= 1 && ratingScore <= 5;
}

std::string ReviewSerializer::toCsv(const Review& r) {
    std::ostringstream oss;
    oss << r.id << ","
        << r.bookingId << ","
        << r.studentId << ","
        << r.tutorId << ","
        << SerializerUtils::escapeCsv(r.studentName) << ","
        << r.ratingScore << ","
        << SerializerUtils::escapeCsv(r.comment) << ","
        << SerializerUtils::escapeCsv(r.createdAt);
    return oss.str();
}

std::optional<Review> ReviewSerializer::fromCsv(const std::string& csvLine, std::string* outError) {
    Review r;
    if (fromCsv(csvLine, r, outError)) {
        return r;
    }
    return std::nullopt;
}

bool ReviewSerializer::fromCsv(const std::string& csvLine, Review& outReview, std::string* outError) {
    std::vector<std::string> tokens = SerializerUtils::splitCsvLine(csvLine);
    if (tokens.size() < 8) {
        if (outError) {
            *outError = "Review CSV requires at least 8 fields, got " + std::to_string(tokens.size());
        }
        return false;
    }

    int id;
    if (!SerializerUtils::safeStringToInt(tokens[0], id)) {
        if (outError) *outError = "Invalid Review id: " + tokens[0];
        return false;
    }

    int bookingId;
    if (!SerializerUtils::safeStringToInt(tokens[1], bookingId)) {
        if (outError) *outError = "Invalid Review bookingId: " + tokens[1];
        return false;
    }

    int studentId;
    if (!SerializerUtils::safeStringToInt(tokens[2], studentId)) {
        if (outError) *outError = "Invalid Review studentId: " + tokens[2];
        return false;
    }

    int tutorId;
    if (!SerializerUtils::safeStringToInt(tokens[3], tutorId)) {
        if (outError) *outError = "Invalid Review tutorId: " + tokens[3];
        return false;
    }

    std::string studentName = tokens[4];

    int ratingScore;
    if (!SerializerUtils::safeStringToInt(tokens[5], ratingScore)) {
        if (outError) *outError = "Invalid Review ratingScore: " + tokens[5];
        return false;
    }

    if (!isValidRatingScore(ratingScore)) {
        if (outError) {
            *outError = "Review ratingScore must be between 1 and 5 stars, got " + std::to_string(ratingScore);
        }
        return false;
    }

    std::string comment = tokens[6];
    std::string createdAt = tokens[7];

    outReview = Review(id, bookingId, studentId, tutorId, studentName,
                       ratingScore, comment, createdAt);
    return true;
}

} // namespace TutorMatch
