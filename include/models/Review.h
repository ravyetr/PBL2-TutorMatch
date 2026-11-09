#ifndef TUTOR_MATCH_REVIEW_H
#define TUTOR_MATCH_REVIEW_H

#include "User.h"
#include <string>

namespace TutorMatch {

struct Review {
    int id;
    int bookingId;
    int studentId;
    int tutorId;
    std::string studentName;
    int ratingScore; // 1 to 5 stars
    std::string comment;
    std::string createdAt;

    Review()
        : id(0), bookingId(0), studentId(0), tutorId(0), ratingScore(5) {}

    Review(int id, int bookingId, int studentId, int tutorId, std::string studentName,
           int ratingScore, std::string comment, std::string createdAt = "2026-09-06")
        : id(id), bookingId(bookingId), studentId(studentId), tutorId(tutorId),
          studentName(std::move(studentName)), ratingScore(ratingScore),
          comment(std::move(comment)), createdAt(std::move(createdAt)) {}
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_REVIEW_H
