#ifndef TUTOR_MATCH_COURSE_BOOKING_SERIALIZER_H
#define TUTOR_MATCH_COURSE_BOOKING_SERIALIZER_H

#include "business/domain/CourseBooking.h"
#include <string>
#include <optional>

namespace TutorMatch {

/**
 * @brief Dedicated Serializer for CourseBooking entities.
 *
 * Implements Single Responsibility Principle (SRP):
 * - Serializes CourseBooking to CSV string (12 core fields, up to 21 fields with commitment V1).
 * - Deserializes CSV string to CourseBooking with strict field validation.
 * - Validates status, weeklySchedule, startDate, and weeksCount.
 * - Supports weeklySchedule as single source of truth for recurring slots.
 */
class CourseBookingSerializer {
public:
    static bool isValidStatus(const std::string& status);
    static bool isValidWeeklySchedule(const std::string& schedule);
    static std::string toCsv(const CourseBooking& cb);
    static std::optional<CourseBooking> fromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static bool fromCsv(const std::string& csvLine, CourseBooking& outBooking, std::string* outError = nullptr);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_COURSE_BOOKING_SERIALIZER_H
