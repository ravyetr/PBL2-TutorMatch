#include "data_access/serializers/CourseBookingSerializer.h"
#include "data_access/serializers/SerializerUtils.h"
#include "business/domain/DateUtils.h"

#include <sstream>

namespace TutorMatch {

bool CourseBookingSerializer::isValidStatus(const std::string& status) {
    return status == "PENDING"   ||
           status == "ACCEPTED"  ||
           status == "REJECTED"  ||
           status == "COMPLETED" ||
           status == "CANCELLED" ||
           status == "EARLY_TERMINATED";
}

bool CourseBookingSerializer::isValidWeeklySchedule(const std::string& schedule) {
    if (schedule.empty()) return false;
    auto slots = CourseBooking::parseWeeklySchedule(schedule);
    return !slots.empty();
}

std::string CourseBookingSerializer::toCsv(const CourseBooking& b) {
    std::ostringstream oss;
    oss << b.id << ","
        << b.studentId << ","
        << b.tutorId << ","
        << SerializerUtils::escapeCsv(b.studentName) << ","
        << SerializerUtils::escapeCsv(b.tutorName) << ","
        << SerializerUtils::escapeCsv(b.subject) << ","
        << b.hourlyRate << ","
        << SerializerUtils::escapeCsv(b.startDate) << ","
        << b.weeksCount << ","
        << SerializerUtils::escapeCsv(b.status) << ","
        << SerializerUtils::escapeCsv(b.createdAt) << ","
        << SerializerUtils::escapeCsv(b.weeklySchedule);

    // Append extended fields if present or non-default
    if (b.earlyTerminationRequesterId > 0 ||
        b.earlyTerminationStatus != "NONE" || !b.actualEndDate.empty() ||
        b.studentAcknowledged || b.tutorAcknowledged || !b.commitmentVersion.empty()) {
        oss << ","
            << b.earlyTerminationRequesterId << ","
            << SerializerUtils::escapeCsv(b.earlyTerminationStatus) << ","
            << SerializerUtils::escapeCsv(b.earlyTerminationReason) << ","
            << SerializerUtils::escapeCsv(b.actualEndDate) << ","
            << (b.studentAcknowledged ? 1 : 0) << ","
            << SerializerUtils::escapeCsv(b.studentAckTimestamp) << ","
            << (b.tutorAcknowledged ? 1 : 0) << ","
            << SerializerUtils::escapeCsv(b.tutorAckTimestamp) << ","
            << SerializerUtils::escapeCsv(b.commitmentVersion);
    }

    return oss.str();
}

std::optional<CourseBooking> CourseBookingSerializer::fromCsv(const std::string& csvLine, std::string* outError) {
    CourseBooking b;
    if (fromCsv(csvLine, b, outError)) {
        return b;
    }
    return std::nullopt;
}

bool CourseBookingSerializer::fromCsv(const std::string& csvLine, CourseBooking& outBooking, std::string* outError) {
    std::vector<std::string> tokens = SerializerUtils::splitCsvLine(csvLine);
    if (tokens.size() < 12) {
        if (outError) {
            *outError = "CourseBooking CSV requires at least 12 fields, got " + std::to_string(tokens.size());
        }
        return false;
    }

    int id;
    if (!SerializerUtils::safeStringToInt(tokens[0], id)) {
        if (outError) *outError = "Invalid CourseBooking id: " + tokens[0];
        return false;
    }

    int studentId;
    if (!SerializerUtils::safeStringToInt(tokens[1], studentId)) {
        if (outError) *outError = "Invalid CourseBooking studentId: " + tokens[1];
        return false;
    }

    int tutorId;
    if (!SerializerUtils::safeStringToInt(tokens[2], tutorId)) {
        if (outError) *outError = "Invalid CourseBooking tutorId: " + tokens[2];
        return false;
    }

    std::string studentName = tokens[3];
    std::string tutorName = tokens[4];
    std::string subject = tokens[5];

    double hourlyRate;
    if (!SerializerUtils::safeStringToDouble(tokens[6], hourlyRate)) {
        if (outError) *outError = "Invalid CourseBooking hourlyRate: " + tokens[6];
        return false;
    }

    // Check if this is a legacy format:
    // Legacy format has tokens.size() >= 13, AND tokens[7] is an integer dow (0..6),
    // AND tokens[8] is an integer slot (0..7), AND tokens[9] is a valid date (startDate).
    bool isLegacy = false;
    int legacyDow = -1, legacySlot = -1;
    if (tokens.size() >= 13 &&
        SerializerUtils::safeStringToInt(tokens[7], legacyDow) && legacyDow >= 0 && legacyDow <= 6 &&
        SerializerUtils::safeStringToInt(tokens[8], legacySlot) && legacySlot >= 0 && legacySlot <= 7 &&
        DateUtils::isValidDateString(tokens[9])) {
        isLegacy = true;
    }

    std::string startDate;
    int weeksCount = 0;
    std::string status;
    std::string createdAt;
    std::string weeklySchedule;
    int reqId = 0;
    std::string termStatus = "NONE";
    std::string termReason = "";
    std::string actualEnd = "";
    bool studentAck = false;
    std::string studentAckTs = "";
    bool tutorAck = false;
    std::string tutorAckTs = "";
    std::string commitVer = "";

    if (isLegacy) {
        startDate = tokens[9];
        if (!SerializerUtils::safeStringToInt(tokens[10], weeksCount) || weeksCount <= 0) {
            if (outError) *outError = "Invalid CourseBooking weeksCount: " + tokens[10];
            return false;
        }
        status = tokens[11];
        if (!isValidStatus(status)) {
            if (outError) *outError = "Invalid CourseBooking status value: " + status;
            return false;
        }
        createdAt = tokens[12];
        weeklySchedule = (tokens.size() >= 14 && !tokens[13].empty())
            ? tokens[13] : (std::to_string(legacyDow) + ":" + std::to_string(legacySlot));

        if (tokens.size() >= 15 && !tokens[14].empty()) SerializerUtils::safeStringToInt(tokens[14], reqId);
        if (tokens.size() >= 16 && !tokens[15].empty()) termStatus = tokens[15];
        if (tokens.size() >= 17) termReason = tokens[16];
        if (tokens.size() >= 18) actualEnd = tokens[17];
        if (tokens.size() >= 19 && (tokens[18] == "1" || tokens[18] == "true")) studentAck = true;
        if (tokens.size() >= 20) studentAckTs = tokens[19];
        if (tokens.size() >= 21 && (tokens[20] == "1" || tokens[20] == "true")) tutorAck = true;
        if (tokens.size() >= 22) tutorAckTs = tokens[21];
        if (tokens.size() >= 23) commitVer = tokens[22];
    } else {
        startDate = tokens[7];
        if (!DateUtils::isValidDateString(startDate)) {
            if (outError) *outError = "Invalid CourseBooking startDate (expected YYYY-MM-DD): " + startDate;
            return false;
        }

        if (!SerializerUtils::safeStringToInt(tokens[8], weeksCount) || weeksCount <= 0) {
            if (outError) *outError = "Invalid CourseBooking weeksCount: " + tokens[8];
            return false;
        }

        status = tokens[9];
        if (!isValidStatus(status)) {
            if (outError) *outError = "Invalid CourseBooking status value: " + status;
            return false;
        }

        createdAt = tokens[10];
        weeklySchedule = tokens[11];
        if (!isValidWeeklySchedule(weeklySchedule)) {
            if (outError) *outError = "Invalid CourseBooking weeklySchedule: " + weeklySchedule;
            return false;
        }

        if (tokens.size() >= 13 && !tokens[12].empty()) SerializerUtils::safeStringToInt(tokens[12], reqId);
        if (tokens.size() >= 14 && !tokens[13].empty()) termStatus = tokens[13];
        if (tokens.size() >= 15) termReason = tokens[14];
        if (tokens.size() >= 16) actualEnd = tokens[15];
        if (tokens.size() >= 17 && (tokens[16] == "1" || tokens[16] == "true")) studentAck = true;
        if (tokens.size() >= 18) studentAckTs = tokens[17];
        if (tokens.size() >= 19 && (tokens[18] == "1" || tokens[18] == "true")) tutorAck = true;
        if (tokens.size() >= 20) tutorAckTs = tokens[19];
        if (tokens.size() >= 21) commitVer = tokens[20];
    }

    outBooking = CourseBooking(id, studentId, tutorId, studentName, tutorName,
                               subject, hourlyRate,
                               startDate, weeksCount, status, createdAt,
                               weeklySchedule, reqId, termStatus, termReason, actualEnd,
                               studentAck, studentAckTs, tutorAck, tutorAckTs, commitVer);
    return true;
}

} // namespace TutorMatch
