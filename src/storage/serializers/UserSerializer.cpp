#include "storage/serializers/UserSerializer.h"
#include "storage/serializers/SerializerUtils.h"
#include "ds/ScheduleGuard.h"

#include <sstream>

namespace TutorMatch {

std::string UserSerializer::toCsv(const Student& s) {
    std::ostringstream oss;
    oss << s.getId() << ","
        << SerializerUtils::escapeCsv(s.getUsername()) << ","
        << SerializerUtils::escapeCsv(s.getPasswordHash()) << ","
        << SerializerUtils::escapeCsv(s.getEmail()) << ","
        << SerializerUtils::escapeCsv(s.getFullName()) << ","
        << userRoleToString(s.getRole()) << ","
        << (s.getIsActive() ? 1 : 0) << ","
        << s.getGradeLevel() << ","
        << s.getScheduleBitmask();
    return oss.str();
}

std::string UserSerializer::toCsv(const Tutor& t) {
    std::ostringstream oss;
    oss << t.getId() << ","
        << SerializerUtils::escapeCsv(t.getUsername()) << ","
        << SerializerUtils::escapeCsv(t.getPasswordHash()) << ","
        << SerializerUtils::escapeCsv(t.getEmail()) << ","
        << SerializerUtils::escapeCsv(t.getFullName()) << ","
        << userRoleToString(t.getRole()) << ","
        << (t.getIsActive() ? 1 : 0) << ","
        << SerializerUtils::escapeCsv(t.getBio()) << ","
        << SerializerUtils::escapeCsv(t.getSubjectSpecialty()) << ","
        << t.getHourlyRate() << ","
        << t.getRating() << ","
        << t.getTotalReviews() << ","
        << t.getTotalEarnings() << ","
        << t.getScheduleBitmask();
    return oss.str();
}

std::string UserSerializer::toCsv(const Admin& a) {
    std::ostringstream oss;
    oss << a.getId() << ","
        << SerializerUtils::escapeCsv(a.getUsername()) << ","
        << SerializerUtils::escapeCsv(a.getPasswordHash()) << ","
        << SerializerUtils::escapeCsv(a.getEmail()) << ","
        << SerializerUtils::escapeCsv(a.getFullName()) << ","
        << userRoleToString(a.getRole()) << ","
        << (a.getIsActive() ? 1 : 0) << ","
        << a.getPermissionLevel();
    return oss.str();
}

std::string UserSerializer::toCsv(const User& user) {
    switch (user.getRole()) {
        case UserRole::STUDENT: {
            const auto* s = dynamic_cast<const Student*>(&user);
            if (s) return toCsv(*s);
            break;
        }
        case UserRole::TUTOR: {
            const auto* t = dynamic_cast<const Tutor*>(&user);
            if (t) return toCsv(*t);
            break;
        }
        case UserRole::ADMIN: {
            const auto* a = dynamic_cast<const Admin*>(&user);
            if (a) return toCsv(*a);
            break;
        }
        default:
            break;
    }
    // Fallback: 7 fields base representation
    std::ostringstream oss;
    oss << user.getId() << ","
        << SerializerUtils::escapeCsv(user.getUsername()) << ","
        << SerializerUtils::escapeCsv(user.getPasswordHash()) << ","
        << SerializerUtils::escapeCsv(user.getEmail()) << ","
        << SerializerUtils::escapeCsv(user.getFullName()) << ","
        << userRoleToString(user.getRole()) << ","
        << (user.getIsActive() ? 1 : 0);
    return oss.str();
}

std::shared_ptr<User> UserSerializer::fromCsv(const std::string& csvLine, std::string* outError) {
    std::vector<std::string> tokens = SerializerUtils::splitCsvLine(csvLine);
    if (tokens.size() < 7) {
        if (outError) {
            *outError = "User CSV requires at least 7 fields, got " + std::to_string(tokens.size());
        }
        return nullptr;
    }

    int id;
    if (!SerializerUtils::safeStringToInt(tokens[0], id)) {
        if (outError) *outError = "Invalid User id: " + tokens[0];
        return nullptr;
    }

    std::string username = tokens[1];
    std::string passwordHash = tokens[2];
    std::string email = tokens[3];
    std::string fullName = tokens[4];
    std::string roleStr = tokens[5];

    int activeInt;
    if (!SerializerUtils::safeStringToInt(tokens[6], activeInt)) {
        if (outError) *outError = "Invalid User isActive flag: " + tokens[6];
        return nullptr;
    }
    bool isActive = (activeInt == 1);

    if (roleStr == "STUDENT") {
        if (tokens.size() < 9) {
            if (outError) {
                *outError = "Student CSV requires at least 9 fields, got " + std::to_string(tokens.size());
            }
            return nullptr;
        }

        int gradeLevel;
        if (!SerializerUtils::safeStringToInt(tokens[7], gradeLevel)) {
            if (outError) *outError = "Invalid Student gradeLevel: " + tokens[7];
            return nullptr;
        }

        uint64_t mask = 0;
        if (tokens.size() == 9 || tokens.size() == 10) {
            // Canonical 9-field schema: token 8 is scheduleBitmask
            if (!SerializerUtils::safeStringToUll(tokens[8], mask)) {
                if (outError) *outError = "Invalid Student scheduleBitmask: " + tokens[8];
                return nullptr;
            }
        } else if (tokens.size() >= 11) {
            // Legacy 11-field schema:
            // token 7: gradeLevel
            // token 8: preferredSubject (obsolete search preference, safely discarded)
            // token 9: maxBudget (obsolete search preference, safely discarded)
            // token 10: scheduleBitmask
            if (!SerializerUtils::safeStringToUll(tokens[10], mask)) {
                if (outError) *outError = "Invalid Student scheduleBitmask: " + tokens[10];
                return nullptr;
            }
        }

        uint64_t sanitizedMask = ScheduleGuard::sanitizeMask(mask);
        return std::make_shared<Student>(id, username, passwordHash, email, fullName,
                                         gradeLevel, sanitizedMask, isActive);
    } else if (roleStr == "TUTOR") {
        if (tokens.size() < 14) {
            if (outError) {
                *outError = "Tutor CSV requires at least 14 fields, got " + std::to_string(tokens.size());
            }
            return nullptr;
        }

        std::string bio = tokens[7];
        std::string subjectSpecialty = tokens[8];

        double hourlyRate;
        if (!SerializerUtils::safeStringToDouble(tokens[9], hourlyRate)) {
            if (outError) *outError = "Invalid Tutor hourlyRate: " + tokens[9];
            return nullptr;
        }

        double rating;
        if (!SerializerUtils::safeStringToDouble(tokens[10], rating)) {
            if (outError) *outError = "Invalid Tutor rating: " + tokens[10];
            return nullptr;
        }

        int totalReviews;
        if (!SerializerUtils::safeStringToInt(tokens[11], totalReviews)) {
            if (outError) *outError = "Invalid Tutor totalReviews: " + tokens[11];
            return nullptr;
        }

        double totalEarnings = 0.0;
        uint64_t mask = 0;

        if (tokens.size() == 14) {
            // Canonical schema (14 tokens): token 12 is totalEarnings, token 13 is scheduleBitmask
            if (!SerializerUtils::safeStringToDouble(tokens[12], totalEarnings)) {
                if (outError) *outError = "Invalid Tutor totalEarnings: " + tokens[12];
                return nullptr;
            }
            if (!SerializerUtils::safeStringToUll(tokens[13], mask)) {
                if (outError) *outError = "Invalid Tutor scheduleBitmask: " + tokens[13];
                return nullptr;
            }
        } else {
            // Legacy schema (15 tokens): token 12 was isVerified (safely discarded),
            // token 13 is totalEarnings, token 14 is scheduleBitmask
            if (!SerializerUtils::safeStringToDouble(tokens[13], totalEarnings)) {
                if (outError) *outError = "Invalid Tutor totalEarnings: " + tokens[13];
                return nullptr;
            }
            if (!SerializerUtils::safeStringToUll(tokens[14], mask)) {
                if (outError) *outError = "Invalid Tutor scheduleBitmask: " + tokens[14];
                return nullptr;
            }
        }

        uint64_t sanitizedMask = ScheduleGuard::sanitizeMask(mask);
        return std::make_shared<Tutor>(id, username, passwordHash, email, fullName,
                                       bio, subjectSpecialty, hourlyRate, rating,
                                       totalReviews, totalEarnings,
                                       sanitizedMask, isActive);
    } else if (roleStr == "ADMIN") {
        if (tokens.size() < 8) {
            if (outError) {
                *outError = "Admin CSV requires at least 8 fields, got " + std::to_string(tokens.size());
            }
            return nullptr;
        }

        int permissionLevel;
        if (!SerializerUtils::safeStringToInt(tokens[7], permissionLevel)) {
            if (outError) *outError = "Invalid Admin permissionLevel: " + tokens[7];
            return nullptr;
        }

        return std::make_shared<Admin>(id, username, passwordHash, email, fullName,
                                       permissionLevel, isActive);
    }

    if (outError) {
        *outError = "Unrecognized User role: " + roleStr;
    }
    return nullptr;
}

std::shared_ptr<Student> UserSerializer::studentFromCsv(const std::string& csvLine, std::string* outError) {
    auto u = fromCsv(csvLine, outError);
    if (!u) return nullptr;
    auto s = std::dynamic_pointer_cast<Student>(u);
    if (!s && outError) {
        *outError = "Deserialized entity is not a Student (role mismatch)";
    }
    return s;
}

std::shared_ptr<Tutor> UserSerializer::tutorFromCsv(const std::string& csvLine, std::string* outError) {
    auto u = fromCsv(csvLine, outError);
    if (!u) return nullptr;
    auto t = std::dynamic_pointer_cast<Tutor>(u);
    if (!t && outError) {
        *outError = "Deserialized entity is not a Tutor (role mismatch)";
    }
    return t;
}

std::shared_ptr<Admin> UserSerializer::adminFromCsv(const std::string& csvLine, std::string* outError) {
    auto u = fromCsv(csvLine, outError);
    if (!u) return nullptr;
    auto a = std::dynamic_pointer_cast<Admin>(u);
    if (!a && outError) {
        *outError = "Deserialized entity is not an Admin (role mismatch)";
    }
    return a;
}

} // namespace TutorMatch
