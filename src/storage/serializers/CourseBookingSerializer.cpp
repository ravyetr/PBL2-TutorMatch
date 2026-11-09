#include "storage/serializers/CourseBookingSerializer.h"
#include "storage/serializers/SerializerUtils.h"
#include "utils/DateUtils.h"

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

std::string CourseBookingSerializer::toCsv(const CourseBooking& b) {
    std::ostringstream oss;
    oss << b.id << ","
        << b.studentId << ","
        << b.tutorId << ","
        << SerializerUtils::escapeCsv(b.studentName) << ","
        << SerializerUtils::escapeCsv(b.tutorName) << ","
        << SerializerUtils::escapeCsv(b.subject) << ","
        << b.hourlyRate << ","
        << b.dayOfWeek << ","
        << b.slotIndex << ","
        << SerializerUtils::escapeCsv(b.startDate) << ","
        << b.weeksCount << ","
        << SerializerUtils::escapeCsv(b.status) << ","
        << SerializerUtils::escapeCsv(b.createdAt);

    // Append extended fields if present or non-default
    if (!b.weeklySchedule.empty() || b.earlyTerminationRequesterId > 0 ||
        b.earlyTerminationStatus != "NONE" || !b.actualEndDate.empty() ||
        b.studentAcknowledged || b.tutorAcknowledged || !b.commitmentVersion.empty()) {
        oss << ","
            << SerializerUtils::escapeCsv(b.weeklySchedule) << ","
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
    if (tokens.size() < 13) {
        if (outError) {
            *outError = "CourseBooking CSV requires at least 13 fields, got " + std::to_string(tokens.size());
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

    int dayOfWeek;
    if (!SerializerUtils::safeStringToInt(tokens[7], dayOfWeek) || dayOfWeek < 0 || dayOfWeek > 6) {
        if (outError) *outError = "Invalid CourseBooking dayOfWeek (expected 0..6): " + tokens[7];
        return false;
    }

    int slotIndex;
    if (!SerializerUtils::safeStringToInt(tokens[8], slotIndex) || slotIndex < 0 || slotIndex > 7) {
        if (outError) *outError = "Invalid CourseBooking slotIndex (expected 0..7): " + tokens[8];
        return false;
    }

    std::string startDate = tokens[9];
    if (!DateUtils::isValidDateString(startDate)) {
        if (outError) *outError = "Invalid CourseBooking startDate (expected YYYY-MM-DD): " + startDate;
        return false;
    }

    int weeksCount;
    if (!SerializerUtils::safeStringToInt(tokens[10], weeksCount) || weeksCount <= 0) {
        if (outError) *outError = "Invalid CourseBooking weeksCount: " + tokens[10];
        return false;
    }

    std::string status = tokens[11];
    if (!isValidStatus(status)) {
        if (outError) *outError = "Invalid CourseBooking status value: " + status;
        return false;
    }

    std::string createdAt = tokens[12];

    std::string weeklySchedule = (tokens.size() >= 14 && !tokens[13].empty())
        ? tokens[13] : (std::to_string(dayOfWeek) + ":" + std::to_string(slotIndex));

    int reqId = 0;
    if (tokens.size() >= 15 && !tokens[14].empty()) {
        SerializerUtils::safeStringToInt(tokens[14], reqId);
    }

    std::string termStatus = (tokens.size() >= 16 && !tokens[15].empty())
        ? tokens[15] : "NONE";

    std::string termReason = (tokens.size() >= 17) ? tokens[16] : "";
    std::string actualEnd = (tokens.size() >= 18) ? tokens[17] : "";

    bool studentAck = (tokens.size() >= 19 && (tokens[18] == "1" || tokens[18] == "true"));
    std::string studentAckTs = (tokens.size() >= 20) ? tokens[19] : "";
    bool tutorAck = (tokens.size() >= 21 && (tokens[20] == "1" || tokens[20] == "true"));
    std::string tutorAckTs = (tokens.size() >= 22) ? tokens[21] : "";
    std::string commitVer = (tokens.size() >= 23) ? tokens[22] : "";

    outBooking = CourseBooking(id, studentId, tutorId, studentName, tutorName,
                               subject, hourlyRate, dayOfWeek, slotIndex,
                               startDate, weeksCount, status, createdAt,
                               weeklySchedule, reqId, termStatus, termReason, actualEnd,
                               studentAck, studentAckTs, tutorAck, tutorAckTs, commitVer);
    return true;
}

} // namespace TutorMatch
