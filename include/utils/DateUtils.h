#ifndef TUTOR_MATCH_DATE_UTILS_H
#define TUTOR_MATCH_DATE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace TutorMatch {

/**
 * @brief Pure C++17 Gregorian Calendar & Date Calculation Utilities.
 *
 * Implements strict ISO-8601 YYYY-MM-DD parsing, validation, leap year arithmetic,
 * day-of-week computation, and recurring session date generation.
 *
 * Zero dependencies on Qt or external date libraries in core business logic.
 */
class DateUtils {
public:
    static bool isLeapYear(int year);
    static int daysInMonth(int year, int month);
    static bool isValidDate(int year, int month, int day);
    static bool parseDate(const std::string& dateStr, int& outY, int& outM, int& outD);
    static bool isValidDateString(const std::string& dateStr);
    static std::string formatDate(int year, int month, int day);

    /**
     * @brief Computes day of week.
     * @return 0 = Monday (Thứ 2), 1 = Tuesday (Thứ 3), ..., 6 = Sunday (Chủ Nhật).
     */
    static int dayOfWeek(int year, int month, int day);
    static int dayOfWeek(const std::string& dateStr);

    /**
     * @brief Adds or subtracts days to an ISO date string (YYYY-MM-DD).
     */
    static std::string addDays(const std::string& dateStr, int daysToAdd);

    struct ScheduledSession {
        std::string date;
        int dayOfWeek = 0;
        int slotIndex = 0;
    };

    /**
     * @brief Generates list of all recurring session dates:
     * sessionDate(i) = startDate + 7 * i days, for i in [0 .. weeksCount - 1].
     */
    static std::vector<std::string> generateSessionDates(const std::string& startDate, int weeksCount);

    /**
     * @brief Generates list of all recurring session dates across multiple weekly slots.
     * For each slot (dow, slotIndex), finds first occurrence on or after startDate,
     * generates weeksCount occurrences, and sorts all sessions chronologically.
     */
    static std::vector<ScheduledSession> generateMultiSlotSessions(
        const std::string& startDate,
        int weeksCount,
        const std::vector<std::pair<int, int>>& weeklySlots);

    /**
     * @brief Calculates end date of course: startDate + 7 * (weeksCount - 1) days.
     */
    static std::string getEndDate(const std::string& startDate, int weeksCount);

    /**
     * @brief Formats system clock current date to YYYY-MM-DD.
     */
    static std::string getCurrentDateString();

    /**
     * @brief Formats system clock current timestamp to YYYY-MM-DD HH:MM:SS.
     */
    static std::string getCurrentTimestampString();

    /**
     * @brief Checks if dateStr is today or in the future compared to referenceDate (or today).
     */
    static bool isFutureOrToday(const std::string& dateStr, const std::string& referenceDate = "");

    /**
     * @brief Compares two YYYY-MM-DD strings.
     * @return < 0 if d1 < d2, 0 if d1 == d2, > 0 if d1 > d2.
     */
    static int compareDates(const std::string& d1, const std::string& d2);

    /**
     * @brief Computes ISO date YYYY-MM-DD of Monday for the given reference date (or today if empty).
     */
    static std::string getWeekMonday(const std::string& dateStr = "");

    /**
     * @brief Computes ISO date YYYY-MM-DD of Sunday for the given reference date (or today if empty).
     */
    static std::string getWeekSunday(const std::string& dateStr = "");

    /**
     * @brief Returns the 7 calendar dates (YYYY-MM-DD) for Monday through Sunday.
     */
    static std::vector<std::string> getWeekDays(const std::string& mondayStr);

    /**
     * @brief Shifts a week Monday by weekOffset (+1 for next week, -1 for previous week).
     */
    static std::string shiftWeek(const std::string& mondayStr, int weekOffset);

    /**
     * @brief Formats YYYY-MM-DD into display format DD/MM.
     */
    static std::string formatDisplayDate(const std::string& dateStr);

private:
    static int64_t toCivilDays(int y, int m, int d);
    static void fromCivilDays(int64_t z, int& y, int& m, int& d);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_DATE_UTILS_H
