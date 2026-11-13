#include "business/domain/DateUtils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace TutorMatch {

bool DateUtils::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DateUtils::daysInMonth(int year, int month) {
    if (month < 1 || month > 12) return 0;
    if (month == 2) {
        return isLeapYear(year) ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

bool DateUtils::isValidDate(int year, int month, int day) {
    if (year < 1900 || year > 2200) return false;
    if (month < 1 || month > 12) return false;
    int maxDays = daysInMonth(year, month);
    return (day >= 1 && day <= maxDays);
}

bool DateUtils::parseDate(const std::string& dateStr, int& outY, int& outM, int& outD) {
    if (dateStr.length() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;

    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(dateStr[i]))) {
            return false;
        }
    }

    try {
        int y = std::stoi(dateStr.substr(0, 4));
        int m = std::stoi(dateStr.substr(5, 2));
        int d = std::stoi(dateStr.substr(8, 2));

        if (!isValidDate(y, m, d)) return false;

        outY = y;
        outM = m;
        outD = d;
        return true;
    } catch (...) {
        return false;
    }
}

bool DateUtils::isValidDateString(const std::string& dateStr) {
    int y = 0, m = 0, d = 0;
    return parseDate(dateStr, y, m, d);
}

std::string DateUtils::formatDate(int year, int month, int day) {
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year << "-"
        << std::setw(2) << month << "-"
        << std::setw(2) << day;
    return oss.str();
}

int DateUtils::dayOfWeek(int year, int month, int day) {
    // Tomohiko Sakamoto's algorithm (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;
    int m = month;
    if (m < 3) y -= 1;
    int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + day) % 7;
    // Normalize to: 0 = Monday (Thứ 2) .. 6 = Sunday (Chủ Nhật)
    return (dow + 6) % 7;
}

int DateUtils::dayOfWeek(const std::string& dateStr) {
    int y = 0, m = 0, d = 0;
    if (!parseDate(dateStr, y, m, d)) return -1;
    return dayOfWeek(y, m, d);
}

int64_t DateUtils::toCivilDays(int y, int m, int d) {
    y -= (m <= 2);
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

void DateUtils::fromCivilDays(int64_t z, int& y, int& m, int& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era * 400);
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);
}

std::string DateUtils::addDays(const std::string& dateStr, int daysToAdd) {
    int y = 0, m = 0, d = 0;
    if (!parseDate(dateStr, y, m, d)) return "";

    int64_t days = toCivilDays(y, m, d) + daysToAdd;
    int outY = 0, outM = 0, outD = 0;
    fromCivilDays(days, outY, outM, outD);
    return formatDate(outY, outM, outD);
}

std::vector<std::string> DateUtils::generateSessionDates(const std::string& startDate, int weeksCount) {
    std::vector<std::string> dates;
    if (weeksCount <= 0 || !isValidDateString(startDate)) return dates;

    dates.reserve(weeksCount);
    for (int i = 0; i < weeksCount; ++i) {
        std::string sDate = addDays(startDate, 7 * i);
        if (!sDate.empty()) {
            dates.push_back(sDate);
        }
    }
    return dates;
}

std::vector<DateUtils::ScheduledSession> DateUtils::generateMultiSlotSessions(
    const std::string& startDate,
    int weeksCount,
    const std::vector<std::pair<int, int>>& weeklySlots) {
    std::vector<ScheduledSession> result;
    if (weeksCount <= 0 || !isValidDateString(startDate) || weeklySlots.empty()) {
        return result;
    }

    int startDow = dayOfWeek(startDate);
    if (startDow < 0) return result;

    for (const auto& slotPair : weeklySlots) {
        int w = slotPair.first;
        int s = slotPair.second;
        if (w < 0 || w > 6 || s < 0 || s > 7) continue;

        int delta = (w - startDow + 7) % 7;
        std::string firstDate = addDays(startDate, delta);

        for (int i = 0; i < weeksCount; ++i) {
            std::string sDate = addDays(firstDate, i * 7);
            if (!sDate.empty()) {
                result.push_back({sDate, w, s});
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const ScheduledSession& a, const ScheduledSession& b) {
        int c = compareDates(a.date, b.date);
        if (c != 0) return c < 0;
        return a.slotIndex < b.slotIndex;
    });

    return result;
}

std::string DateUtils::getEndDate(const std::string& startDate, int weeksCount) {
    if (weeksCount <= 0 || !isValidDateString(startDate)) return "";
    return addDays(startDate, 7 * (weeksCount - 1));
}

std::string DateUtils::getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d");
    return oss.str();
}

std::string DateUtils::getCurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string DateUtils::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%H:%M:%S");
    return oss.str();
}

bool DateUtils::parseTime(const std::string& timeStr, int& outH, int& outM) {
    if (timeStr.size() < 4) return false;
    size_t colonPos = timeStr.find(':');
    if (colonPos == std::string::npos || colonPos == 0) return false;
    try {
        int h = std::stoi(timeStr.substr(0, colonPos));
        int m = std::stoi(timeStr.substr(colonPos + 1, 2));
        if (h < 0 || h > 23 || m < 0 || m > 59) return false;
        outH = h;
        outM = m;
        return true;
    } catch (...) {
        return false;
    }
}

bool DateUtils::getSlotStartTime(int slotIndex, int& outHour, int& outMinute) {
    static const int START_TIMES[8][2] = {
        {7, 30},  // Slot 0: 07:30 - 09:00
        {9, 15},  // Slot 1: 09:15 - 10:45
        {13, 30}, // Slot 2: 13:30 - 15:00
        {15, 15}, // Slot 3: 15:15 - 16:45
        {17, 0},  // Slot 4: 17:00 - 18:30
        {18, 45}, // Slot 5: 18:45 - 20:15
        {20, 30}, // Slot 6: 20:30 - 22:00
        {22, 0}   // Slot 7: 22:00 - 23:30
    };
    if (slotIndex < 0 || slotIndex >= 8) return false;
    outHour = START_TIMES[slotIndex][0];
    outMinute = START_TIMES[slotIndex][1];
    return true;
}

bool DateUtils::isSlotTimePassed(int slotIndex, const std::string& refTime) {
    if (slotIndex < 0 || slotIndex >= 8) return false;
    int curH = 0, curM = 0;
    std::string tStr = refTime.empty() ? getCurrentTimeString() : refTime;
    if (!parseTime(tStr, curH, curM)) {
        return false;
    }
    int slotH = 0, slotM = 0;
    if (!getSlotStartTime(slotIndex, slotH, slotM)) {
        return false;
    }
    if (curH > slotH) return true;
    if (curH == slotH && curM >= slotM) return true;
    return false;
}

std::string DateUtils::getNextSlotOccurrence(int dow, int slotIndex,
                                             const std::string& refDate,
                                             const std::string& refTime) {
    if (dow < 0 || dow > 6 || slotIndex < 0 || slotIndex > 7) {
        return "";
    }
    std::string baseDate = refDate.empty() ? getCurrentDateString() : refDate;
    if (!isValidDateString(baseDate)) {
        return "";
    }
    int currentDow = dayOfWeek(baseDate);
    if (currentDow < 0) return "";

    int daysUntil = (dow - currentDow + 7) % 7;
    if (daysUntil == 0) {
        if (isSlotTimePassed(slotIndex, refTime)) {
            daysUntil = 7;
        }
    }
    return addDays(baseDate, daysUntil);
}

std::string DateUtils::getEarliestValidStartDate(
    const std::vector<std::pair<int, int>>& weeklySlots,
    const std::string& refDate,
    const std::string& refTime)
{
    if (weeklySlots.empty()) return "";
    std::string earliest = "";

    for (const auto& slot : weeklySlots) {
        if (slot.first < 0 || slot.first > 6 || slot.second < 0 || slot.second > 7) {
            continue;
        }
        std::string occ = getNextSlotOccurrence(slot.first, slot.second, refDate, refTime);
        if (occ.empty()) continue;
        if (earliest.empty() || compareDates(occ, earliest) < 0) {
            earliest = occ;
        }
    }
    return earliest;
}

std::vector<std::string> DateUtils::getUpcomingStartDatesForSlots(
    const std::vector<std::pair<int, int>>& weeklySlots,
    int count,
    const std::string& refDate,
    const std::string& refTime)
{
    if (weeklySlots.empty() || count <= 0) return {};

    std::vector<std::string> allCandidateDates;
    for (const auto& slot : weeklySlots) {
        if (slot.first < 0 || slot.first > 6 || slot.second < 0 || slot.second > 7) {
            continue;
        }
        std::string firstOcc = getNextSlotOccurrence(slot.first, slot.second, refDate, refTime);
        if (firstOcc.empty()) continue;
        for (int i = 0; i < count; ++i) {
            std::string d = addDays(firstOcc, i * 7);
            if (!d.empty()) {
                allCandidateDates.push_back(d);
            }
        }
    }

    if (allCandidateDates.empty()) return {};

    std::sort(allCandidateDates.begin(), allCandidateDates.end(), [](const std::string& a, const std::string& b) {
        return compareDates(a, b) < 0;
    });

    std::vector<std::string> uniqueDates;
    uniqueDates.reserve(count);
    for (const auto& d : allCandidateDates) {
        if (uniqueDates.empty() || uniqueDates.back() != d) {
            uniqueDates.push_back(d);
            if (static_cast<int>(uniqueDates.size()) == count) {
                break;
            }
        }
    }

    return uniqueDates;
}

bool DateUtils::isFutureOrToday(const std::string& dateStr, const std::string& referenceDate) {
    std::string ref = referenceDate.empty() ? getCurrentDateString() : referenceDate;
    if (!isValidDateString(dateStr) || !isValidDateString(ref)) return false;
    return compareDates(dateStr, ref) >= 0;
}

int DateUtils::compareDates(const std::string& d1, const std::string& d2) {
    int y1 = 0, m1 = 0, day1 = 0;
    int y2 = 0, m2 = 0, day2 = 0;
    if (!parseDate(d1, y1, m1, day1) || !parseDate(d2, y2, m2, day2)) {
        return d1.compare(d2);
    }
    int64_t days1 = toCivilDays(y1, m1, day1);
    int64_t days2 = toCivilDays(y2, m2, day2);
    if (days1 < days2) return -1;
    if (days1 > days2) return 1;
    return 0;
}

std::string DateUtils::getWeekMonday(const std::string& dateStr) {
    std::string ref = dateStr.empty() ? getCurrentDateString() : dateStr;
    if (!isValidDateString(ref)) {
        ref = getCurrentDateString();
    }
    int dow = dayOfWeek(ref);
    if (dow < 0) dow = 0;
    return addDays(ref, -dow);
}

std::string DateUtils::getWeekSunday(const std::string& dateStr) {
    std::string monday = getWeekMonday(dateStr);
    return addDays(monday, 6);
}

std::vector<std::string> DateUtils::getWeekDays(const std::string& mondayStr) {
    std::vector<std::string> days;
    std::string monday = isValidDateString(mondayStr) ? getWeekMonday(mondayStr) : getWeekMonday();
    days.reserve(7);
    for (int i = 0; i < 7; ++i) {
        days.push_back(addDays(monday, i));
    }
    return days;
}

std::string DateUtils::shiftWeek(const std::string& mondayStr, int weekOffset) {
    std::string monday = isValidDateString(mondayStr) ? getWeekMonday(mondayStr) : getWeekMonday();
    return addDays(monday, weekOffset * 7);
}

std::string DateUtils::formatDisplayDate(const std::string& dateStr) {
    int y = 0, m = 0, d = 0;
    if (!parseDate(dateStr, y, m, d)) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d/%02d", d, m);
    return std::string(buf);
}

} // namespace TutorMatch
