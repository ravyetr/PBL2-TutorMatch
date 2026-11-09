#ifndef TUTOR_MATCH_COURSE_BOOKING_H
#define TUTOR_MATCH_COURSE_BOOKING_H

#include <string>
#include <vector>
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
    int dayOfWeek;          ///< Primary slot weekday: 0 = Thứ 2 .. 6 = Chủ Nhật
    int slotIndex;          ///< Primary slot: 0..7
    std::string startDate;  ///< YYYY-MM-DD (effective start date)
    int weeksCount;         ///< 4, 8, 12 weeks
    std::string status;     ///< "PENDING", "ACCEPTED", "REJECTED", "COMPLETED", "CANCELLED", "EARLY_TERMINATED"
    std::string createdAt;  ///< YYYY-MM-DD (request submission timestamp)

    // Multi-session schedule support (format: "dow:slot;dow:slot", e.g. "0:1;4:0")
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
        : id(0), studentId(0), tutorId(0), hourlyRate(0.0), dayOfWeek(0), slotIndex(0),
          weeksCount(4), status("PENDING"), earlyTerminationRequesterId(0),
          earlyTerminationStatus("NONE"), studentAcknowledged(false), tutorAcknowledged(false) {}

    CourseBooking(int id, int studentId, int tutorId, std::string studentName, std::string tutorName,
                  std::string subject, double hourlyRate, int dayOfWeek, int slotIndex,
                  std::string startDate, int weeksCount, std::string status = "PENDING",
                  std::string createdAt = "", std::string weeklySchedule = "",
                  int earlyTerminationRequesterId = 0, std::string earlyTerminationStatus = "NONE",
                  std::string earlyTerminationReason = "", std::string actualEndDate = "",
                  bool studentAcknowledged = false, std::string studentAckTimestamp = "",
                  bool tutorAcknowledged = false, std::string tutorAckTimestamp = "",
                  std::string commitmentVersion = "")
        : id(id), studentId(studentId), tutorId(tutorId), studentName(std::move(studentName)),
          tutorName(std::move(tutorName)), subject(std::move(subject)), hourlyRate(hourlyRate),
          dayOfWeek(dayOfWeek), slotIndex(slotIndex), startDate(std::move(startDate)),
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
          commitmentVersion(std::move(commitmentVersion)) {
        if (this->weeklySchedule.empty() && dayOfWeek >= 0 && slotIndex >= 0) {
            this->weeklySchedule = std::to_string(dayOfWeek) + ":" + std::to_string(slotIndex);
        }
    }

    bool isCommitmentFullyAcknowledged() const {
        return studentAcknowledged && tutorAcknowledged &&
               !studentAckTimestamp.empty() && !tutorAckTimestamp.empty() &&
               commitmentVersion == CURRENT_COMMITMENT_VERSION;
    }

    std::vector<std::pair<int, int>> getWeeklySlots() const {
        if (weeklySchedule.empty()) {
            return { {dayOfWeek, slotIndex} };
        }
        std::vector<std::pair<int, int>> resultSlots;
        size_t start = 0;
        while (start < weeklySchedule.size()) {
            size_t semi = weeklySchedule.find(';', start);
            std::string part = (semi == std::string::npos) ? weeklySchedule.substr(start) : weeklySchedule.substr(start, semi - start);
            size_t colon = part.find(':');
            if (colon != std::string::npos) {
                try {
                    int w = std::stoi(part.substr(0, colon));
                    int s = std::stoi(part.substr(colon + 1));
                    resultSlots.push_back({w, s});
                } catch (...) {}
            }
            if (semi == std::string::npos) break;
            start = semi + 1;
        }
        if (resultSlots.empty()) {
            resultSlots.push_back({dayOfWeek, slotIndex});
        }
        return resultSlots;
    }

    bool isMultiSlot() const {
        return getWeeklySlots().size() > 1;
    }

    std::string getWeeklyScheduleSummary() const {
        auto ws = getWeeklySlots();
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
