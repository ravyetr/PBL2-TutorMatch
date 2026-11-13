#ifndef TUTOR_MATCH_COURSE_BOOKING_H
#define TUTOR_MATCH_COURSE_BOOKING_H

#include <string>
#include <vector>
#include <set>
#include <cstdint>
#include <utility>

namespace TutorMatch {

/// Central commitment terms version constant
inline constexpr const char* CURRENT_COMMITMENT_VERSION = "COMMITMENT_V1";

/// Standard text describing the consensual early termination & matching policy
inline constexpr const char* COMMITMENT_POLICY_TEXT =
    "CHÍNH SÁCH CAM KẾT KHÓA HỌC & KẾT THÚC SỚM (PHIÊN BẢN V1):\n"
    "1. Khóa học được lên lịch cố định theo các ca học đã thỏa thuận trong tuần.\n"
    "2. Một bên có thể đề nghị kết thúc sớm khóa học bất kỳ lúc nào.\n"
    "3. Khóa học chỉ kết thúc sớm khi bên còn lại đồng ý (đồng thuận hai bên).\n"
    "4. Nếu bên còn lại từ chối, khóa học tiếp tục diễn ra theo lịch đã thỏa thuận.\n"
    "5. Nếu cả hai bên đồng ý, khóa học chuyển sang trạng thái EARLY_TERMINATED "
    "và toàn bộ lịch học tương lai của chính khóa học đó được giải phóng.\n"
    "6. Việc kết thúc sớm không làm ảnh hưởng đến các khóa học khác của hai bên.\n"
    "7. TutorMatch chỉ hỗ trợ kết nối và quản lý lịch học đã thỏa thuận, "
    "không xác nhận buổi học thực tế đã diễn ra.";

/**
 * @brief Represents a Recurring Course Booking over multiple consecutive weeks.
 *
 * Domain Rules:
 * - Recurring schedule: Supports one or multiple time slots per week (weeklySchedule).
 * - Multi-week duration: weeksCount is 4, 8, or 12 weeks.
 * - Start date: Effective start date (YYYY-MM-DD).
 * - Status lifecycle: PENDING -> ACCEPTED -> COMPLETED / CANCELLED / EARLY_TERMINATED / REJECTED.
 * - Consensual Early Termination: Requires mutual agreement between student and tutor.
 * - Commitment Acknowledgment: Both parties must acknowledge COMMITMENT_V1 before ACCEPTED.
 */
struct CourseBooking {
    static inline constexpr const char* CURRENT_COMMITMENT_VERSION = TutorMatch::CURRENT_COMMITMENT_VERSION;
    static inline constexpr const char* COMMITMENT_POLICY_TEXT = TutorMatch::COMMITMENT_POLICY_TEXT;

    int id;
    int studentId;
    int tutorId;
    std::string studentName;
    std::string tutorName;
    std::string subject;
    double hourlyRate;
    std::string startDate;  ///< YYYY-MM-DD (effective start date)
    int weeksCount;         ///< 4, 8, 12 weeks
    std::string status;     ///< "PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED", "EARLY_TERMINATED"
    std::string createdAt;  ///< YYYY-MM-DD (request submission timestamp)

    // Recurring weekly schedule (Single source of truth: format "dow:slot;dow:slot", e.g. "0:1;4:0")
    std::string weeklySchedule;

    // Consensual Early Termination Lifecycle
    int earlyTerminationRequesterId = 0;         ///< User ID of initiator (0 if none)
    std::string earlyTerminationStatus = "NONE"; ///< "NONE", "PENDING", "ACCEPTED", "REJECTED"
    std::string earlyTerminationReason;          ///< Reason provided by requester
    std::string actualEndDate;                   ///< YYYY-MM-DD (date early termination took effect)

    // Commitment Acknowledgment Lifecycle
    bool studentAcknowledged = false;            ///< True if student acknowledged commitment policy
    std::string studentAckTimestamp;             ///< Backend-generated ISO timestamp YYYY-MM-DD HH:MM:SS
    bool tutorAcknowledged = false;              ///< True if tutor acknowledged commitment policy
    std::string tutorAckTimestamp;               ///< Backend-generated ISO timestamp YYYY-MM-DD HH:MM:SS
    std::string commitmentVersion;               ///< Must equal CURRENT_COMMITMENT_VERSION

    CourseBooking()
        : id(0), studentId(0), tutorId(0), hourlyRate(0.0),
          weeksCount(4), status("PENDING"), earlyTerminationRequesterId(0),
          earlyTerminationStatus("NONE"), studentAcknowledged(false), tutorAcknowledged(false) {}

    CourseBooking(int id, int studentId, int tutorId, std::string studentName, std::string tutorName,
                  std::string subject, double hourlyRate,
                  std::string startDate, int weeksCount, std::string status = "PENDING",
                  std::string createdAt = "", std::string weeklySchedule = "",
                  int earlyTerminationRequesterId = 0, std::string earlyTerminationStatus = "NONE",
                  std::string earlyTerminationReason = "", std::string actualEndDate = "",
                  bool studentAcknowledged = false, std::string studentAckTimestamp = "",
                  bool tutorAcknowledged = false, std::string tutorAckTimestamp = "",
                  std::string commitmentVersion = "")
        : id(id), studentId(studentId), tutorId(tutorId), studentName(std::move(studentName)),
          tutorName(std::move(tutorName)), subject(std::move(subject)), hourlyRate(hourlyRate),
          startDate(std::move(startDate)),
          weeksCount(weeksCount), status(std::move(status)), createdAt(std::move(createdAt)),
          weeklySchedule(std::move(weeklySchedule)),
          earlyTerminationRequesterId(earlyTerminationRequesterId),
          earlyTerminationStatus(std::move(earlyTerminationStatus)),
          earlyTerminationReason(std::move(earlyTerminationReason)),
          actualEndDate(std::move(actualEndDate)),
          studentAcknowledged(studentAcknowledged),
          studentAckTimestamp(std::move(studentAckTimestamp)),
          tutorAcknowledged(tutorAcknowledged),
          tutorAckTimestamp(std::move(tutorAckTimestamp)),
          commitmentVersion(std::move(commitmentVersion)) {}

    bool isCommitmentFullyAcknowledged() const {
        return studentAcknowledged && tutorAcknowledged &&
               !studentAckTimestamp.empty() && !tutorAckTimestamp.empty() &&
               commitmentVersion == CURRENT_COMMITMENT_VERSION;
    }

    /**
     * @brief Parses a weekly schedule string ("dow:slot;dow:slot") into validated slot pairs.
     * Sanitizes extra/missing delimiters, invalid ranges, and duplicates.
     */
    static std::vector<std::pair<int, int>> parseWeeklySchedule(const std::string& scheduleStr) {
        std::vector<std::pair<int, int>> resultSlots;
        if (scheduleStr.empty()) return resultSlots;

        std::set<std::pair<int, int>> seen;
        size_t start = 0;
        while (start < scheduleStr.size()) {
            size_t semi = scheduleStr.find(';', start);
            std::string part = (semi == std::string::npos) ? scheduleStr.substr(start) : scheduleStr.substr(start, semi - start);

            // Trim whitespace
            size_t first = part.find_first_not_of(" \t\r\n");
            size_t last = part.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos) {
                part = part.substr(first, last - first + 1);
            } else {
                part.clear();
            }

            if (!part.empty()) {
                size_t colon = part.find(':');
                if (colon != std::string::npos && colon > 0 && colon + 1 < part.size()) {
                    try {
                        int w = std::stoi(part.substr(0, colon));
                        int s = std::stoi(part.substr(colon + 1));
                        if (w >= 0 && w <= 6 && s >= 0 && s <= 7) {
                            std::pair<int, int> pairVal{w, s};
                            if (seen.find(pairVal) == seen.end()) {
                                seen.insert(pairVal);
                                resultSlots.push_back(pairVal);
                            }
                        }
                    } catch (...) {}
                }
            }

            if (semi == std::string::npos) break;
            start = semi + 1;
        }
        return resultSlots;
    }

    std::vector<std::pair<int, int>> getWeeklySlots() const {
        return parseWeeklySchedule(weeklySchedule);
    }

    bool isMultiSlot() const {
        return getWeeklySlots().size() > 1;
    }

    std::string getWeeklyScheduleSummary() const {
        auto ws = getWeeklySlots();
        if (ws.empty()) return "Chưa có lịch";
        std::string s;
        for (size_t i = 0; i < ws.size(); ++i) {
            if (i > 0) s += ", ";
            s += "T" + std::to_string(ws[i].first + 2) + " · Ca " + std::to_string(ws[i].second + 1);
        }
        return s;
    }

    int getTotalSessions() const {
        return static_cast<int>(getWeeklySlots().size()) * weeksCount;
    }

    double getTotalCost() const {
        return hourlyRate * getTotalSessions();
    }

    bool hasEarlyTerminationRequest() const {
        return earlyTerminationStatus == "PENDING";
    }

    bool isEarlyTerminated() const {
        return status == "EARLY_TERMINATED";
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_COURSE_BOOKING_H
