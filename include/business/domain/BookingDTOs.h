#ifndef TUTOR_MATCH_BOOKING_DTOS_H
#define TUTOR_MATCH_BOOKING_DTOS_H

#include <string>

namespace TutorMatch {

/**
 * @brief Data Transfer Object (DTO) conveying aggregated timetable and financial
 * metrics for a Student's active and historical bookings.
 *
 * Implements Phase 1 ADR-006 & Gate Review Review H:
 * Replaces client-side JavaScript business logic in QML (StudentBookingsView.qml)
 * with an authoritative, precomputed backend contract provided by Business Logic Layer.
 *
 * Architectural Boundary Rules:
 * - Pure value type / POCO (Plain Old C++ Object).
 * - Zero dependencies on QML, Qt GUI, or File I/O.
 * - Does not perform queries or contain business logic rules.
 * - Safe for value-copying, thread-safe return from services, and serialization
 *   to QVariantMap at presentation boundary (GUIBridge).
 */
struct StudentBookingMetrics {
    int acceptedCount = 0;   ///< Number of actively connected bookings (status == "ACCEPTED")
    int pendingCount = 0;    ///< Number of bookings awaiting tutor response (status == "PENDING")
    int historyCount = 0;    ///< Number of past/terminated bookings (status in {"COMPLETED", "CANCELLED", "REJECTED"})
    double weeklyCost = 0.0; ///< Total weekly recurring tuition (sum of hourlyRate for all ACCEPTED bookings)

    constexpr StudentBookingMetrics() = default;

    constexpr StudentBookingMetrics(int accepted, int pending, int history, double cost)
        : acceptedCount(accepted), pendingCount(pending), historyCount(history), weeklyCost(cost) {}

    bool operator==(const StudentBookingMetrics& other) const {
        return acceptedCount == other.acceptedCount &&
               pendingCount == other.pendingCount &&
               historyCount == other.historyCount &&
               weeklyCost == other.weeklyCost;
    }

    bool operator!=(const StudentBookingMetrics& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Data Transfer Object for an actual accepted session occurring on a specific calendar date.
 */
struct TimetableSessionDTO {
    int courseBookingId = 0;
    int sessionNumber = 0;       ///< 1-based index of this session within the course (e.g. 1, 2, 3, 4)
    int totalSessions = 0;       ///< Total number of sessions in the course (weeksCount)
    std::string date;            ///< Actual session date in YYYY-MM-DD
    int dayOfWeek = 0;           ///< 0 = Monday .. 6 = Sunday
    int slotIndex = 0;           ///< 0 .. 7
    std::string subject;
    int studentId = 0;
    std::string studentName;
    int tutorId = 0;
    std::string tutorName;
    double hourlyRate = 0.0;
    std::string status = "ACCEPTED";
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_BOOKING_DTOS_H
