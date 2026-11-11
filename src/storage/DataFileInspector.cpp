#include "../../include/storage/DataFileInspector.h"
#include "../../include/storage/ApplicationPaths.h"
#include "../../include/ds/ScheduleGuard.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace TutorMatch {

static std::string formatField(const std::string& name, const std::string& val, const std::string& type, const std::string& desc) {
    std::ostringstream oss;
    oss << std::left << std::setw(18) << name << " : "
        << std::left << std::setw(28) << val << " ["
        << type << "] - " << desc;
    return oss.str();
}

static std::string decodeBookingSlot(uint64_t mask) {
    for (int d = 0; d < ScheduleGuard::DAYS_PER_WEEK; ++d) {
        for (int s = 0; s < ScheduleGuard::SLOTS_PER_DAY; ++s) {
            if ((mask & ScheduleGuard::getSlotMask(d, s)) != 0) {
                return ScheduleGuard::formatSlotSingle(d, s);
            }
        }
    }
    return "Không xác định (Mask = 0)";
}

FileInspectionSummary DataFileInspector::inspectUsersFile(const std::string& dataDirHint) {
    FileInspectionSummary summary;
    summary.fileName = "users.dat";
    summary.formatDescription = "CSV (Plain-text comma-delimited, polymorphic User hierarchy)";
    summary.recordsRead = 0;
    summary.validRecords = 0;
    summary.invalidRecords = 0;
    summary.fileModified = false;

    std::string baseDir = ApplicationPaths::resolveDataDirectory(dataDirHint);
    summary.resolvedPath = baseDir + "/users.dat";

    if (!std::filesystem::exists(summary.resolvedPath)) {
        summary.status = "ERROR_NOT_FOUND (Tệp không tồn tại)";
        return summary;
    }

    std::ifstream in(summary.resolvedPath, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        summary.status = "ERROR_CANNOT_OPEN (Không thể mở tệp)";
        return summary;
    }

    summary.status = "OK";
    std::string line;
    int lineIndex = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        lineIndex++;
        summary.recordsRead++;

        RecordInspection rec;
        rec.recordIndex = lineIndex;
        rec.rawLine = line;
        rec.isValid = true;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 7) {
            rec.isValid = false;
            rec.modelType = "MALFORMED";
            rec.errorMessage = "Không đủ trường tối thiểu của lớp cơ sở User (yêu cầu >= 7 trường, thực tế: " + std::to_string(tokens.size()) + ")";
            summary.invalidRecords++;
            summary.records.push_back(rec);
            continue;
        }

        std::string roleStr = tokens[5];
        rec.modelType = roleStr;

        // Base User Fields
        rec.fields.push_back({"id", tokens[0], "int", "Mã định danh duy nhất (Primary Key)"});
        rec.fields.push_back({"username", tokens[1], "string", "Tên đăng nhập hệ thống"});
        rec.fields.push_back({"password", "[REDACTED]", "credential", "Mật khẩu xác thực (bảo mật)"});
        rec.fields.push_back({"email", tokens[3], "string", "Địa chỉ thư điện tử"});
        rec.fields.push_back({"fullName", tokens[4], "string", "Họ và tên hiển thị"});
        rec.fields.push_back({"role", roleStr, "enum UserRole", "Vai trò người dùng (ADMIN / TUTOR / STUDENT)"});
        rec.fields.push_back({"isActive", (tokens[6] == "1" ? "1 (Hoạt động)" : "0 (Khóa/Xóa mềm)"), "bool/int", "Trạng thái kích hoạt tài khoản"});

        if (roleStr == "ADMIN") {
            if (tokens.size() < 8) {
                rec.isValid = false;
                rec.errorMessage = "Bản ghi ADMIN thiếu trường permissionLevel (cần 8 trường, thực tế: " + std::to_string(tokens.size()) + ")";
                summary.invalidRecords++;
            } else {
                rec.fields.push_back({"permissionLevel", tokens[7], "int", "Cấp độ quyền hạn quản trị (9 = Quản trị viên tối cao)"});
                summary.validRecords++;
            }
        } else if (roleStr == "STUDENT") {
            if (tokens.size() < 9) {
                rec.isValid = false;
                rec.errorMessage = "Bản ghi STUDENT không đủ trường (cần ít nhất 9 trường, thực tế: " + std::to_string(tokens.size()) + ")";
                summary.invalidRecords++;
            } else {
                rec.fields.push_back({"gradeLevel", "Lớp " + tokens[7], "int", "Khối lớp học tập của học viên"});

                uint64_t mask = 0;
                if (tokens.size() == 9 || tokens.size() == 10) {
                    try { mask = std::stoull(tokens[8]); } catch (...) {}
                } else if (tokens.size() >= 11) {
                    rec.fields.push_back({"preferredSubject [Legacy]", tokens[8], "string", "Môn học mục tiêu (định dạng cũ - đã loại bỏ khỏi hồ sơ)"});
                    rec.fields.push_back({"maxBudget [Legacy]", tokens[9] + " VNĐ/buổi", "double", "Ngân sách tối đa (định dạng cũ - đã loại bỏ khỏi hồ sơ)"});
                    try { mask = std::stoull(tokens[10]); } catch (...) {}
                }

                int freeSlots = ScheduleGuard::countMatchingSlots(mask, ScheduleGuard::VALID_SCHEDULE_MASK);
                std::string maskDesc = std::to_string(mask) + " (" + std::to_string(freeSlots) + "/56 ca rảnh)";
                rec.fields.push_back({"scheduleBitmask", maskDesc, "uint64_t", "Mặt nạ nhị phân thời khóa biểu 56 ca"});
                summary.validRecords++;
            }
        } else if (roleStr == "TUTOR") {
            if (tokens.size() < 14) {
                rec.isValid = false;
                rec.errorMessage = "Bản ghi TUTOR không đủ trường (cần 14 trường, thực tế: " + std::to_string(tokens.size()) + ")";
                summary.invalidRecords++;
            } else {
                rec.fields.push_back({"bio", tokens[7], "string", "Tiểu sử & kinh nghiệm giảng dạy"});
                rec.fields.push_back({"subjectSpecialty", tokens[8], "string", "Môn chuyên môn giảng dạy"});
                rec.fields.push_back({"hourlyRate", tokens[9] + " VNĐ/buổi", "double", "Học phí đề xuất mỗi buổi học"});
                rec.fields.push_back({"rating", tokens[10] + " ★", "double", "Điểm đánh giá trung bình tích lũy (1.0-5.0)"});
                rec.fields.push_back({"totalReviews", tokens[11] + " lượt", "int", "Tổng số lượt đánh giá đã nhận"});

                uint64_t mask = 0;
                if (tokens.size() == 14) {
                    rec.fields.push_back({"totalEarnings", tokens[12] + " VNĐ", "double", "Tổng thu nhập giảng dạy tích lũy"});
                    try { mask = std::stoull(tokens[13]); } catch (...) {}
                } else {
                    rec.fields.push_back({"totalEarnings", tokens[13] + " VNĐ", "double", "Tổng thu nhập giảng dạy tích lũy"});
                    try { mask = std::stoull(tokens[14]); } catch (...) {}
                }

                int freeSlots = ScheduleGuard::countMatchingSlots(mask, ScheduleGuard::VALID_SCHEDULE_MASK);
                std::string maskDesc = std::to_string(mask) + " (" + std::to_string(freeSlots) + "/56 ca rảnh)";
                rec.fields.push_back({"scheduleBitmask", maskDesc, "uint64_t", "Mặt nạ nhị phân lịch rảnh giảng dạy 56 ca"});
                summary.validRecords++;
            }
        } else {
            rec.isValid = false;
            rec.errorMessage = "Vai trò người dùng không xác định: '" + roleStr + "'";
            summary.invalidRecords++;
        }

        summary.records.push_back(rec);
    }
    in.close();

    return summary;
}

FileInspectionSummary DataFileInspector::inspectBookingsFile(const std::string& dataDirHint) {
    FileInspectionSummary summary;
    summary.fileName = "bookings.dat";
    summary.formatDescription = "CSV (Plain-text comma-delimited, Booking entity)";
    summary.recordsRead = 0;
    summary.validRecords = 0;
    summary.invalidRecords = 0;
    summary.fileModified = false;

    std::string baseDir = ApplicationPaths::resolveDataDirectory(dataDirHint);
    summary.resolvedPath = baseDir + "/bookings.dat";

    if (!std::filesystem::exists(summary.resolvedPath)) {
        summary.status = "ERROR_NOT_FOUND (Tệp không tồn tại)";
        return summary;
    }

    std::ifstream in(summary.resolvedPath, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        summary.status = "ERROR_CANNOT_OPEN (Không thể mở tệp)";
        return summary;
    }

    summary.status = "OK";
    std::string line;
    int lineIndex = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        lineIndex++;
        summary.recordsRead++;

        RecordInspection rec;
        rec.recordIndex = lineIndex;
        rec.rawLine = line;
        rec.isValid = true;
        rec.modelType = "Booking";

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 10) {
            rec.isValid = false;
            rec.modelType = "MALFORMED";
            rec.errorMessage = "Không đủ trường Booking (yêu cầu 10 trường, thực tế: " + std::to_string(tokens.size()) + ")";
            summary.invalidRecords++;
            summary.records.push_back(rec);
            continue;
        }

        rec.fields.push_back({"id", tokens[0], "int", "Mã đơn đặt lịch hẹn (Primary Key)"});
        rec.fields.push_back({"studentId", tokens[1], "int", "Mã ID học viên đặt (Foreign Key -> User.id)"});
        rec.fields.push_back({"tutorId", tokens[2], "int", "Mã ID gia sư được đặt (Foreign Key -> User.id)"});
        rec.fields.push_back({"studentName", tokens[3], "string", "Họ và tên học viên tại thời điểm đặt"});
        rec.fields.push_back({"tutorName", tokens[4], "string", "Họ và tên gia sư tại thời điểm đặt"});
        rec.fields.push_back({"subject", tokens[5], "string", "Môn học đăng ký kèm"});
        rec.fields.push_back({"hourlyRate", tokens[6] + " VNĐ/buổi", "double", "Học phí thỏa thuận cho buổi học"});

        uint64_t mask = 0;
        try { mask = std::stoull(tokens[7]); } catch (...) {}
        std::string slotDesc = decodeBookingSlot(mask);
        rec.fields.push_back({"slotMask", std::to_string(mask) + " [" + slotDesc + "]", "uint64_t", "Mặt nạ 1 bit ca học trong tuần"});

        rec.fields.push_back({"status", tokens[8], "string", "Trạng thái đơn (PENDING/ACCEPTED/REJECTED/COMPLETED/CANCELLED)"});
        rec.fields.push_back({"createdAt", tokens[9], "string", "Ngày tạo yêu cầu đặt lịch (YYYY-MM-DD)"});

        summary.validRecords++;
        summary.records.push_back(rec);
    }
    in.close();

    return summary;
}

FileInspectionSummary DataFileInspector::inspectReviewsFile(const std::string& dataDirHint) {
    FileInspectionSummary summary;
    summary.fileName = "reviews.dat";
    summary.formatDescription = "CSV (Plain-text comma-delimited, Review entity)";
    summary.recordsRead = 0;
    summary.validRecords = 0;
    summary.invalidRecords = 0;
    summary.fileModified = false;

    std::string baseDir = ApplicationPaths::resolveDataDirectory(dataDirHint);
    summary.resolvedPath = baseDir + "/reviews.dat";

    if (!std::filesystem::exists(summary.resolvedPath)) {
        summary.status = "ERROR_NOT_FOUND (Tệp không tồn tại)";
        return summary;
    }

    std::ifstream in(summary.resolvedPath, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        summary.status = "ERROR_CANNOT_OPEN (Không thể mở tệp)";
        return summary;
    }

    summary.status = "OK";
    std::string line;
    int lineIndex = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        lineIndex++;
        summary.recordsRead++;

        RecordInspection rec;
        rec.recordIndex = lineIndex;
        rec.rawLine = line;
        rec.isValid = true;
        rec.modelType = "Review";

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 8) {
            rec.isValid = false;
            rec.modelType = "MALFORMED";
            rec.errorMessage = "Không đủ trường Review (yêu cầu 8 trường, thực tế: " + std::to_string(tokens.size()) + ")";
            summary.invalidRecords++;
            summary.records.push_back(rec);
            continue;
        }

        rec.fields.push_back({"id", tokens[0], "int", "Mã đánh giá nhận xét (Primary Key)"});
        rec.fields.push_back({"bookingId", tokens[1], "int", "Mã buổi học tương ứng (Foreign Key -> Booking.id)"});
        rec.fields.push_back({"studentId", tokens[2], "int", "Mã ID học viên đánh giá (Foreign Key -> User.id)"});
        rec.fields.push_back({"tutorId", tokens[3], "int", "Mã ID gia sư được nhận (Foreign Key -> User.id)"});
        rec.fields.push_back({"studentName", tokens[4], "string", "Họ và tên học viên gửi đánh giá"});
        rec.fields.push_back({"ratingScore", tokens[5] + " ★", "int", "Điểm số xếp hạng sao (thang 1 - 5 sao)"});
        rec.fields.push_back({"comment", tokens[6], "string", "Nội dung nhận xét chi tiết của học viên"});
        rec.fields.push_back({"createdAt", tokens[7], "string", "Ngày đăng nhận xét đánh giá (YYYY-MM-DD)"});

        summary.validRecords++;
        summary.records.push_back(rec);
    }
    in.close();

    return summary;
}

FileInspectionSummary DataFileInspector::inspectCourseBookingsFile(const std::string& dataDirHint) {
    FileInspectionSummary summary;
    summary.fileName = "course_bookings.dat";
    summary.formatDescription = "CSV (Plain-text comma-delimited, CourseBooking entity - 13 fields)";
    summary.recordsRead = 0;
    summary.validRecords = 0;
    summary.invalidRecords = 0;
    summary.fileModified = false;

    std::string baseDir = ApplicationPaths::resolveDataDirectory(dataDirHint);
    summary.resolvedPath = baseDir + "/course_bookings.dat";

    if (!std::filesystem::exists(summary.resolvedPath)) {
        summary.status = "ERROR_NOT_FOUND (Tệp không tồn tại)";
        return summary;
    }

    std::ifstream in(summary.resolvedPath, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        summary.status = "ERROR_CANNOT_OPEN (Không thể mở tệp)";
        return summary;
    }

    summary.status = "OK";
    std::string line;
    int lineIndex = 0;

    static const char* DAY_NAMES[] = {
        "Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6", "Thứ 7", "Chủ Nhật"
    };

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        lineIndex++;
        summary.recordsRead++;

        RecordInspection rec;
        rec.recordIndex = lineIndex;
        rec.rawLine = line;
        rec.isValid = true;
        rec.modelType = "CourseBooking";

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 13) {
            rec.isValid = false;
            rec.modelType = "MALFORMED";
            rec.errorMessage = "Không đủ trường CourseBooking (yêu cầu 13 trường, thực tế: " + std::to_string(tokens.size()) + ")";
            summary.invalidRecords++;
            summary.records.push_back(rec);
            continue;
        }

        rec.fields.push_back({"id", tokens[0], "int", "Mã khóa học định kỳ (Primary Key)"});
        rec.fields.push_back({"studentId", tokens[1], "int", "Mã ID học viên đăng ký (Foreign Key -> User.id)"});
        rec.fields.push_back({"tutorId", tokens[2], "int", "Mã ID gia sư đảm nhiệm (Foreign Key -> User.id)"});
        rec.fields.push_back({"studentName", tokens[3], "string", "Họ và tên học viên"});
        rec.fields.push_back({"tutorName", tokens[4], "string", "Họ và tên gia sư"});
        rec.fields.push_back({"subject", tokens[5], "string", "Tên môn học khóa học"});
        rec.fields.push_back({"hourlyRate", tokens[6] + " VNĐ/buổi", "double", "Học phí mỗi buổi học"});

        int dow = -1;
        try { dow = std::stoi(tokens[7]); } catch (...) {}
        std::string dowName = (dow >= 0 && dow <= 6) ? DAY_NAMES[dow] : "Không hợp lệ";
        rec.fields.push_back({"dayOfWeek", tokens[7] + " (" + dowName + ")", "int", "Thứ trong tuần cố định (0 = Thứ 2 .. 6 = CN)"});

        int slot = -1;
        try { slot = std::stoi(tokens[8]); } catch (...) {}
        std::string slotDesc = (dow >= 0 && dow <= 6 && slot >= 0 && slot < ScheduleGuard::SLOTS_PER_DAY)
            ? ScheduleGuard::formatSlotSingle(dow, slot)
            : ("Ca " + tokens[8]);
        rec.fields.push_back({"slotIndex", tokens[8] + " [" + slotDesc + "]", "int", "Chỉ số ca học trong ngày (0..7)"});

        rec.fields.push_back({"startDate", tokens[9], "string", "Ngày bắt đầu khóa học (YYYY-MM-DD)"});
        rec.fields.push_back({"weeksCount", tokens[10] + " tuần", "int", "Thời lượng khóa học (4, 8, 12 tuần)"});
        rec.fields.push_back({"status", tokens[11], "string", "Trạng thái khóa học (PENDING/ACCEPTED/REJECTED/COMPLETED/CANCELLED)"});
        rec.fields.push_back({"createdAt", tokens[12], "string", "Ngày tạo khóa học (YYYY-MM-DD)"});

        summary.validRecords++;
        summary.records.push_back(rec);
    }
    in.close();

    return summary;
}

void DataFileInspector::printFileSummary(const FileInspectionSummary& summary, std::ostream& out) {
    out << "============================================================" << std::endl;
    out << " TUTORMATCH DATA FILE INSPECTOR — " << summary.fileName << std::endl;
    out << "============================================================" << std::endl;
    out << "Tệp đường dẫn  : " << summary.resolvedPath << std::endl;
    out << "Định dạng      : " << summary.formatDescription << std::endl;
    out << "Trạng thái tệp : " << summary.status << std::endl;
    out << "Tổng bản ghi   : " << summary.recordsRead << std::endl;

    for (const auto& rec : summary.records) {
        out << "------------------------------------------------------------" << std::endl;
        out << "Bản ghi #" << rec.recordIndex;
        if (!rec.isValid) {
            out << " [LỖI ĐỊNH DẠNG / MALFORMED]" << std::endl;
            out << "  -> Lý do lỗi : " << rec.errorMessage << std::endl;
            out << "  -> Dòng thô  : " << rec.rawLine << std::endl;
            continue;
        }
        out << " [" << rec.modelType << "]" << std::endl;
        out << "------------------------------------------------------------" << std::endl;
        for (const auto& field : rec.fields) {
            out << formatField(field.name, field.value, field.type, field.description) << std::endl;
        }
    }

    out << "------------------------------------------------------------" << std::endl;
    out << "Tổng kết tệp: " << summary.fileName << std::endl;
    out << "------------------------------------------------------------" << std::endl;
    out << "Bản ghi đã đọc       : " << summary.recordsRead << std::endl;
    out << "Bản ghi hợp lệ (Valid): " << summary.validRecords << std::endl;
    out << "Bản ghi lỗi (Invalid): " << summary.invalidRecords << std::endl;
    out << "Tệp có bị chỉnh sửa : " << (summary.fileModified ? "YES (CẢNH BÁO)" : "NO (Hoàn toàn Read-Only)") << std::endl;
    out << "============================================================\n" << std::endl;
}

void DataFileInspector::inspectAll(const std::string& dataDirHint, std::ostream& out) {
    out << "\n";
    out << "************************************************************" << std::endl;
    out << "*   TUTORMATCH DATA STORAGE FORENSIC INSPECTION SUITE     *" << std::endl;
    out << "************************************************************\n" << std::endl;

    auto usersSummary = inspectUsersFile(dataDirHint);
    printFileSummary(usersSummary, out);

    auto bookingsSummary = inspectBookingsFile(dataDirHint);
    printFileSummary(bookingsSummary, out);

    auto reviewsSummary = inspectReviewsFile(dataDirHint);
    printFileSummary(reviewsSummary, out);

    auto courseBookingsSummary = inspectCourseBookingsFile(dataDirHint);
    printFileSummary(courseBookingsSummary, out);

    int totalRead = usersSummary.recordsRead + bookingsSummary.recordsRead + reviewsSummary.recordsRead + courseBookingsSummary.recordsRead;
    int totalValid = usersSummary.validRecords + bookingsSummary.validRecords + reviewsSummary.validRecords + courseBookingsSummary.validRecords;
    int totalInvalid = usersSummary.invalidRecords + bookingsSummary.invalidRecords + reviewsSummary.invalidRecords + courseBookingsSummary.invalidRecords;

    out << "============================================================" << std::endl;
    out << " TOÀN BỘ TỔNG KẾT HỆ THỐNG LƯU TRỮ (GLOBAL DATA SUMMARY)   " << std::endl;
    out << "============================================================" << std::endl;
    out << "Tổng số tệp dữ liệu đã kiểm tra : 4 (users.dat, bookings.dat, reviews.dat, course_bookings.dat)" << std::endl;
    out << "Tổng số bản ghi đọc được       : " << totalRead << std::endl;
    out << "Tổng số bản ghi hợp lệ         : " << totalValid << std::endl;
    out << "Tổng số bản ghi lỗi/hỏng       : " << totalInvalid << std::endl;
    out << "Tính toàn vẹn dữ liệu          : " << (totalInvalid == 0 ? "100% HOÀN HẢO (CHÍNH XÁC)" : "CÓ CẢNH BÁO") << std::endl;
    out << "Tất cả các tệp có bị sửa đổi?  : NO (Tuyệt đối an toàn Read-Only)" << std::endl;
    out << "============================================================\n" << std::endl;
}

} // namespace TutorMatch
