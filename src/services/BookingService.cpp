#include "services/BookingService.h"
#include "storage/repositories/FileCourseBookingRepository.h"
#include "utils/DateUtils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <set>
#include <algorithm>

namespace TutorMatch {

BookingService::BookingService(std::shared_ptr<IUserRepository> uRepo,
                               std::shared_ptr<IReviewRepository> rRepo,
                               std::shared_ptr<ICourseBookingRepository> cbRepo)
    : userRepo(std::move(uRepo)),
      reviewRepo(std::move(rRepo)),
      courseBookingRepo(std::move(cbRepo)) {
    if (!courseBookingRepo) {
        auto concreteCbRepo = std::make_shared<FileCourseBookingRepository>();
        concreteCbRepo->initialize();
        courseBookingRepo = concreteCbRepo;
    }
}

bool BookingService::hasCourseConflict(int studentId, int tutorId, int dayOfWeek, int slotIndex,
                                       const std::string& startDate, int weeksCount,
                                       int excludeBookingId,
                                       std::string* outConflictDetails,
                                       std::string* outConflictDate) {
    return hasCourseConflictMulti(studentId, tutorId, { {dayOfWeek, slotIndex} }, startDate, weeksCount,
                                  excludeBookingId, outConflictDetails, outConflictDate);
}

bool BookingService::hasCourseConflictMulti(int studentId, int tutorId,
                                            const std::vector<std::pair<int, int>>& weeklySlots,
                                            const std::string& startDate, int weeksCount,
                                            int excludeBookingId,
                                            std::string* outConflictDetails,
                                            std::string* outConflictDate) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!DateUtils::isValidDateString(startDate) || weeksCount <= 0 || weeklySlots.empty()) {
        if (outConflictDetails) *outConflictDetails = "Thông tin lịch học không hợp lệ!";
        return true;
    }

    auto targetSessions = DateUtils::generateMultiSlotSessions(startDate, weeksCount, weeklySlots);

    // Check against active ACCEPTED course bookings
    if (courseBookingRepo) {
        auto allCourses = courseBookingRepo->getAllCourseBookings();
        for (const auto& cb : allCourses) {
            if (cb.id == excludeBookingId) continue;
            if (cb.status != "ACCEPTED") continue; // PENDING, REJECTED, CANCELLED, COMPLETED, EARLY_TERMINATED do NOT reserve

            bool isTutor = (tutorId > 0 && cb.tutorId == tutorId);
            bool isStudent = (studentId > 0 && cb.studentId == studentId);
            if (!isTutor && !isStudent) continue;

            auto cbSlots = cb.getWeeklySlots();
            auto cbSessions = DateUtils::generateMultiSlotSessions(cb.startDate, cb.weeksCount, cbSlots);

            for (const auto& existingSession : cbSessions) {
                // If early terminated, future sessions after actualEndDate were cancelled
                if (cb.isEarlyTerminated() && !cb.actualEndDate.empty()) {
                    if (DateUtils::compareDates(existingSession.date, cb.actualEndDate) > 0) {
                        continue;
                    }
                }

                for (const auto& targetSession : targetSessions) {
                    if (existingSession.date == targetSession.date &&
                        existingSession.slotIndex == targetSession.slotIndex) {
                        if (outConflictDate) *outConflictDate = targetSession.date;
                        if (outConflictDetails) {
                            if (isTutor) {
                                *outConflictDetails = "Gia sư đã có lịch dạy khóa #" + std::to_string(cb.id) +
                                                      " (học viên: " + cb.studentName + ") vào ngày " + targetSession.date + "!";
                            } else {
                                *outConflictDetails = "Học viên đã có lịch học khóa #" + std::to_string(cb.id) +
                                                      " (gia sư: " + cb.tutorName + ") vào ngày " + targetSession.date + "!";
                            }
                        }
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool BookingService::bookCourse(int studentId, int tutorId, const std::string& subject,
                                int dayOfWeek, int slotIndex, const std::string& startDate,
                                int weeksCount, bool studentAcknowledgeCommitment,
                                std::string& outMsg, int* outBookingId) {
    return bookCourseMultiSlot(studentId, tutorId, subject, { {dayOfWeek, slotIndex} },
                               startDate, weeksCount, studentAcknowledgeCommitment,
                               outMsg, outBookingId);
}

bool BookingService::bookCourse(int studentId, int tutorId, const std::string& subject,
                                int dayOfWeek, int slotIndex, const std::string& startDate,
                                int weeksCount, std::string& outMsg, int* outBookingId) {
    return bookCourse(studentId, tutorId, subject, dayOfWeek, slotIndex,
                      startDate, weeksCount, false, outMsg, outBookingId);
}

bool BookingService::bookCourseMultiSlot(int studentId, int tutorId, const std::string& subject,
                                         const std::vector<std::pair<int, int>>& weeklySlots,
                                         const std::string& startDate, int weeksCount,
                                         std::string& outMsg, int* outBookingId) {
    return bookCourseMultiSlot(studentId, tutorId, subject, weeklySlots, startDate, weeksCount,
                               false, outMsg, outBookingId);
}

bool BookingService::bookCourseMultiSlot(int studentId, int tutorId, const std::string& subject,
                                         const std::vector<std::pair<int, int>>& weeklySlots,
                                         const std::string& startDate, int weeksCount,
                                         bool studentAcknowledgeCommitment,
                                         std::string& outMsg, int* outBookingId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo || !userRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    if (!studentAcknowledgeCommitment) {
        outMsg = "Bạn phải xác nhận đã đọc và đồng ý với chính sách cam kết kết thúc sớm trước khi gửi yêu cầu!";
        return false;
    }

    if (weeklySlots.empty()) {
        outMsg = "Bạn phải chọn ít nhất một ca học trong tuần!";
        return false;
    }

    // Validate slots bounds and duplicates
    std::set<std::pair<int, int>> uniqueSlots;
    for (const auto& p : weeklySlots) {
        int dow = p.first;
        int slot = p.second;
        if (dow < 0 || dow >= ScheduleGuard::DAYS_PER_WEEK ||
            slot < 0 || slot >= ScheduleGuard::SLOTS_PER_DAY) {
            outMsg = "Ca học được chọn không hợp lệ (ngoài phạm vi 7 ngày x 8 ca)!";
            return false;
        }
        if (uniqueSlots.count(p)) {
            outMsg = "Không được chọn trùng ca học trong cùng một ngày!";
            return false;
        }
        uniqueSlots.insert(p);
    }

    if (weeksCount != 4 && weeksCount != 8 && weeksCount != 12) {
        outMsg = "Số tuần học phải là 4, 8 hoặc 12 tuần!";
        return false;
    }

    if (!DateUtils::isValidDateString(startDate)) {
        outMsg = "Ngày bắt đầu không hợp lệ (định dạng yêu cầu: YYYY-MM-DD)!";
        return false;
    }

    // For single-slot courses, validate that startDate matches the chosen day of week
    if (weeklySlots.size() == 1) {
        int startDayOfWeek = DateUtils::dayOfWeek(startDate);
        int expectedDow = weeklySlots[0].first;
        if (startDayOfWeek != expectedDow) {
            static const char* DAY_NAMES[7] = {
                "Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6", "Thứ 7", "Chủ Nhật"
            };
            outMsg = "Ngày bắt đầu (" + startDate + ") là " + DAY_NAMES[startDayOfWeek] +
                     ", không khớp với thứ đã chọn (" + DAY_NAMES[expectedDow] + ")!";
            return false;
        }
    }

    std::string today = DateUtils::getCurrentDateString();
    if (DateUtils::compareDates(startDate, today) < 0) {
        outMsg = "Ngày bắt đầu không được ở trong quá khứ!";
        return false;
    }

    if (studentId == tutorId) {
        outMsg = "Học viên không thể tự đặt lịch với chính mình!";
        return false;
    }

    auto sUser = userRepo->findUserById(studentId);
    if (!sUser || sUser->getRole() != UserRole::STUDENT) {
        outMsg = "Tài khoản học viên không hợp lệ!";
        return false;
    }
    if (!sUser->getIsActive()) {
        outMsg = "Tài khoản học viên đang bị khóa!";
        return false;
    }

    auto tUser = userRepo->findUserById(tutorId);
    if (!tUser || tUser->getRole() != UserRole::TUTOR) {
        outMsg = "Tài khoản gia sư không hợp lệ!";
        return false;
    }
    if (!tUser->getIsActive()) {
        outMsg = "Tài khoản gia sư đang bị khóa!";
        return false;
    }

    auto student = std::static_pointer_cast<Student>(sUser);
    auto tutor = std::static_pointer_cast<Tutor>(tUser);

    // Validate bitmasks for all chosen slots
    for (const auto& p : weeklySlots) {
        uint64_t reqSlotMask = ScheduleGuard::getSlotMask(p.first, p.second);
        if (!ScheduleGuard::isSlotAvailable(tutor->getScheduleBitmask(), reqSlotMask)) {
            outMsg = "Gia sư không rảnh vào khung giờ này theo thời khóa biểu!";
            return false;
        }
        if (!ScheduleGuard::isSlotAvailable(student->getScheduleBitmask(), reqSlotMask)) {
            outMsg = "Bạn đang BẬN vào khung giờ này theo thời khóa biểu cá nhân!";
            return false;
        }
    }

    std::ostringstream schedOss;
    for (size_t i = 0; i < weeklySlots.size(); ++i) {
        if (i > 0) schedOss << ";";
        schedOss << weeklySlots[i].first << ":" << weeklySlots[i].second;
    }

    int primaryDow = weeklySlots[0].first;
    int primarySlot = weeklySlots[0].second;
    int nextId = courseBookingRepo->getNextCourseBookingId();
    std::string courseSubject = subject.empty() ? tutor->getSubjectSpecialty() : subject;

    CourseBooking newBooking(nextId, studentId, tutorId, student->getFullName(), tutor->getFullName(),
                             courseSubject, tutor->getHourlyRate(), primaryDow, primarySlot,
                             startDate, weeksCount, "PENDING", today, schedOss.str(),
                             0, "NONE", "", "",
                             true, DateUtils::getCurrentTimestampString(),
                             false, "", CURRENT_COMMITMENT_VERSION);

    if (courseBookingRepo->saveCourseBooking(newBooking)) {
        if (outBookingId) *outBookingId = nextId;
        outMsg = "Đăng ký khóa học thành công! Yêu cầu đang chờ gia sư phản hồi.";
        return true;
    }

    outMsg = "Lỗi lưu đăng ký khóa học vào cơ sở dữ liệu!";
    return false;
}

bool BookingService::acknowledgeStudentCommitment(int studentId, int courseBookingId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Khóa học không tồn tại!";
        return false;
    }

    if (bPtr->studentId != studentId) {
        outMsg = "Bạn không phải là học viên của khóa học này!";
        return false;
    }

    if (bPtr->status != "PENDING") {
        outMsg = "Không thể xác nhận cam kết cho khóa học không ở trạng thái chờ duyệt (PENDING)!";
        return false;
    }

    bPtr->studentAcknowledged = true;
    bPtr->studentAckTimestamp = DateUtils::getCurrentTimestampString();
    bPtr->commitmentVersion = CURRENT_COMMITMENT_VERSION;

    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã ghi nhận xác nhận cam kết của học viên thành công!";
        return true;
    }

    outMsg = "Lỗi lưu xác nhận cam kết của học viên!";
    return false;
}

bool BookingService::acknowledgeTutorCommitment(int tutorId, int courseBookingId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Khóa học không tồn tại!";
        return false;
    }

    if (bPtr->tutorId != tutorId) {
        outMsg = "Bạn không có quyền xử lý yêu cầu của gia sư khác!";
        return false;
    }

    if (bPtr->status != "PENDING") {
        outMsg = "Không thể xác nhận cam kết cho khóa học không ở trạng thái chờ duyệt (PENDING)!";
        return false;
    }

    bPtr->tutorAcknowledged = true;
    bPtr->tutorAckTimestamp = DateUtils::getCurrentTimestampString();
    if (bPtr->commitmentVersion.empty()) {
        bPtr->commitmentVersion = CURRENT_COMMITMENT_VERSION;
    }

    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã ghi nhận xác nhận cam kết của gia sư thành công!";
        return true;
    }

    outMsg = "Lỗi lưu xác nhận cam kết của gia sư!";
    return false;
}

bool BookingService::acceptCourseBooking(int tutorId, int courseBookingId, bool tutorAcknowledgeCommitment,
                                        std::string& outMsg, std::string* outConflictDate) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Khóa học không tồn tại!";
        return false;
    }

    if (bPtr->tutorId != tutorId) {
        outMsg = "Bạn không có quyền chấp nhận yêu cầu của gia sư khác!";
        return false;
    }

    if (bPtr->status == "ACCEPTED") {
        outMsg = "Khóa học đã được chấp nhận trước đó.";
        return true;
    }

    if (bPtr->status != "PENDING") {
        outMsg = "Yêu cầu khóa học không ở trạng thái chờ duyệt (PENDING)!";
        return false;
    }

    // Record tutor acknowledgment if provided
    if (tutorAcknowledgeCommitment) {
        bPtr->tutorAcknowledged = true;
        bPtr->tutorAckTimestamp = DateUtils::getCurrentTimestampString();
        if (bPtr->commitmentVersion.empty()) {
            bPtr->commitmentVersion = CURRENT_COMMITMENT_VERSION;
        }
    }

    // 1. Verify tutor commitment
    if (!bPtr->tutorAcknowledged || bPtr->tutorAckTimestamp.empty()) {
        outMsg = "Gia sư phải xác nhận đã đọc và đồng ý với chính sách cam kết kết thúc sớm trước khi chấp nhận yêu cầu!";
        return false;
    }

    // 2. Verify student commitment
    if (!bPtr->studentAcknowledged || bPtr->studentAckTimestamp.empty()) {
        outMsg = "Học viên chưa hoàn tất xác nhận cam kết kết thúc sớm! Không thể chấp nhận khóa học.";
        return false;
    }

    // 3. Verify commitment version
    if (bPtr->commitmentVersion != CURRENT_COMMITMENT_VERSION) {
        outMsg = "Phiên bản điều khoản cam kết không hợp lệ hoặc đã lỗi thời!";
        return false;
    }

    // 4. Strict multi-slot conflict check
    std::string conflictDetails;
    std::string conflictDate;
    if (hasCourseConflictMulti(bPtr->studentId, bPtr->tutorId, bPtr->getWeeklySlots(),
                               bPtr->startDate, bPtr->weeksCount, bPtr->id,
                               &conflictDetails, &conflictDate)) {
        if (outConflictDate) *outConflictDate = conflictDate;
        outMsg = "Không thể chấp nhận: " + conflictDetails;
        return false;
    }

    bPtr->status = "ACCEPTED";
    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã chấp nhận khóa học định kỳ thành công! Lịch học đã được khóa cho cả 2 bên.";
        return true;
    }

    outMsg = "Lỗi lưu trạng thái chấp nhận vào cơ sở dữ liệu!";
    return false;
}

bool BookingService::acceptCourseBooking(int tutorId, int courseBookingId, std::string& outMsg,
                                        std::string* outConflictDate) {
    return acceptCourseBooking(tutorId, courseBookingId, false, outMsg, outConflictDate);
}

bool BookingService::rejectCourseBooking(int tutorId, int courseBookingId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Không tìm thấy thông tin khóa học!";
        return false;
    }

    if (bPtr->tutorId != tutorId) {
        outMsg = "Bạn không có quyền từ chối yêu cầu của gia sư khác!";
        return false;
    }

    if (bPtr->status != "PENDING") {
        outMsg = "Yêu cầu khóa học không ở trạng thái chờ duyệt (PENDING)!";
        return false;
    }

    bPtr->status = "REJECTED";
    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã từ chối yêu cầu đăng ký khóa học.";
        return true;
    }

    outMsg = "Lỗi lưu trạng thái từ chối vào cơ sở dữ liệu!";
    return false;
}

bool BookingService::cancelCourseBooking(int operatorUserId, UserRole operatorRole,
                                         int courseBookingId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Không tìm thấy thông tin khóa học!";
        return false;
    }

    if (operatorRole != UserRole::ADMIN && bPtr->studentId != operatorUserId && bPtr->tutorId != operatorUserId) {
        outMsg = "Bạn không có quyền hủy khóa học này!";
        return false;
    }

    if (bPtr->status != "PENDING" && bPtr->status != "ACCEPTED") {
        outMsg = "Khóa học không ở trạng thái có thể hủy!";
        return false;
    }

    bPtr->status = "CANCELLED";
    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã hủy khóa học thành công!";
        return true;
    }

    outMsg = "Lỗi cập nhật trạng thái hủy khóa học!";
    return false;
}

bool BookingService::completeCourseBooking(int operatorUserId, UserRole operatorRole,
                                           int courseBookingId, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Không tìm thấy thông tin khóa học!";
        return false;
    }

    if (operatorRole != UserRole::ADMIN && bPtr->studentId != operatorUserId && bPtr->tutorId != operatorUserId) {
        outMsg = "Bạn không có quyền hoàn thành khóa học này!";
        return false;
    }

    if (bPtr->status != "ACCEPTED") {
        outMsg = "Chỉ có thể hoàn thành khóa học đang diễn ra (ACCEPTED)!";
        return false;
    }

    bPtr->status = "COMPLETED";
    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Khóa học đã được hoàn thành thành công!";
        return true;
    }

    outMsg = "Lỗi cập nhật trạng thái hoàn thành khóa học!";
    return false;
}

bool BookingService::terminateCourseBooking(int operatorUserId, UserRole operatorRole,
                                            int courseBookingId, std::string& outMsg) {
    return completeCourseBooking(operatorUserId, operatorRole, courseBookingId, outMsg);
}

bool BookingService::requestEarlyTermination(int operatorUserId, UserRole operatorRole,
                                             int courseBookingId, const std::string& reason,
                                             std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Không tìm thấy thông tin khóa học!";
        return false;
    }

    if (operatorRole != UserRole::ADMIN && bPtr->studentId != operatorUserId && bPtr->tutorId != operatorUserId) {
        outMsg = "Bạn không có quyền yêu cầu kết thúc khóa học này!";
        return false;
    }

    if (bPtr->status != "ACCEPTED") {
        outMsg = "Chỉ có thể yêu cầu kết thúc sớm đối với khóa học đang diễn ra (ACCEPTED)!";
        return false;
    }

    if (bPtr->earlyTerminationStatus == "PENDING") {
        outMsg = "Khóa học này đã có một yêu cầu kết thúc sớm đang chờ phản hồi!";
        return false;
    }

    bPtr->earlyTerminationRequesterId = operatorUserId;
    bPtr->earlyTerminationStatus = "PENDING";
    bPtr->earlyTerminationReason = reason.empty() ? "Thỏa thuận kết thúc sớm" : reason;

    if (courseBookingRepo->updateCourseBooking(*bPtr)) {
        outMsg = "Đã gửi yêu cầu kết thúc sớm. Đang chờ đối phương xác nhận.";
        return true;
    }

    outMsg = "Lỗi lưu yêu cầu kết thúc sớm vào cơ sở dữ liệu!";
    return false;
}

bool BookingService::respondEarlyTermination(int operatorUserId, UserRole operatorRole,
                                             int courseBookingId, bool agree,
                                             std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) {
        outMsg = "Dịch vụ cơ sở dữ liệu chưa sẵn sàng!";
        return false;
    }

    auto bPtr = courseBookingRepo->findCourseBookingById(courseBookingId);
    if (!bPtr) {
        outMsg = "Không tìm thấy thông tin khóa học!";
        return false;
    }

    if (bPtr->status != "ACCEPTED") {
        outMsg = "Khóa học không ở trạng thái có thể kết thúc sớm!";
        return false;
    }

    if (bPtr->earlyTerminationStatus != "PENDING") {
        outMsg = "Không có yêu cầu kết thúc sớm nào đang chờ phản hồi!";
        return false;
    }

    // Only the OTHER party can respond (or Admin)
    if (operatorRole != UserRole::ADMIN) {
        if (operatorUserId == bPtr->earlyTerminationRequesterId) {
            outMsg = "Bạn không thể tự phản hồi yêu cầu do chính mình gửi!";
            return false;
        }
        if (operatorUserId != bPtr->studentId && operatorUserId != bPtr->tutorId) {
            outMsg = "Bạn không có quyền xử lý yêu cầu của khóa học này!";
            return false;
        }
    }

    if (!agree) {
        // Refusal: Course continues on initial schedule
        bPtr->earlyTerminationStatus = "REJECTED";
        if (courseBookingRepo->updateCourseBooking(*bPtr)) {
            outMsg = "Đã từ chối yêu cầu kết thúc sớm. Khóa học tiếp tục theo lịch ban đầu.";
            return true;
        }
        outMsg = "Lỗi cập nhật từ chối kết thúc sớm!";
        return false;
    } else {
        // Mutual agreement: End early, release future reservations
        bPtr->earlyTerminationStatus = "ACCEPTED";
        bPtr->status = "EARLY_TERMINATED";
        bPtr->actualEndDate = DateUtils::getCurrentDateString();

        if (courseBookingRepo->updateCourseBooking(*bPtr)) {
            outMsg = "Khóa học đã kết thúc sớm theo đồng thuận hai bên. Các ca học tương lai đã được giải phóng.";
            return true;
        }
        outMsg = "Lỗi cập nhật trạng thái kết thúc sớm!";
        return false;
    }
}

void BookingService::autoCompleteExpiredCourses() {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) return;

    std::string today = DateUtils::getCurrentDateString();
    auto allCourses = courseBookingRepo->getAllCourseBookings();
    for (auto& cb : allCourses) {
        if (cb.status == "ACCEPTED") {
            auto slots = cb.getWeeklySlots();
            auto sessions = DateUtils::generateMultiSlotSessions(cb.startDate, cb.weeksCount, slots);
            if (!sessions.empty()) {
                const std::string& endDate = sessions.back().date;
                if (DateUtils::compareDates(today, endDate) > 0) {
                    cb.status = "COMPLETED";
                    courseBookingRepo->updateCourseBooking(cb);
                }
            }
        }
    }
}

bool BookingService::submitReview(int studentId, int bookingId, int ratingScore,
                                  const std::string& comment, std::string& outMsg) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo || !userRepo || !reviewRepo) return false;

    auto bPtr = courseBookingRepo->findCourseBookingById(bookingId);
    if (!bPtr || bPtr->studentId != studentId || (bPtr->status != "COMPLETED" && bPtr->status != "EARLY_TERMINATED")) {
        outMsg = "Khóa học chưa hoàn thành hoặc không thuộc về bạn!";
        return false;
    }

    auto sUser = userRepo->findUserById(studentId);
    if (!sUser) {
        outMsg = "Không tìm thấy thông tin học viên!";
        return false;
    }

    auto existingRevs = reviewRepo->getAllReviews();
    for (const auto& r : existingRevs) {
        if (r.bookingId == bookingId) {
            outMsg = "Bạn đã đánh giá khóa học này rồi!";
            return false;
        }
    }

    int nextRevId = reviewRepo->getNextReviewId();
    Review rev(nextRevId, bookingId, studentId, bPtr->tutorId, sUser->getFullName(),
               ratingScore, comment, DateUtils::getCurrentDateString());

    if (!reviewRepo->saveReview(rev)) {
        outMsg = "Lỗi lưu đánh giá vào cơ sở dữ liệu!";
        return false;
    }

    // Recalculate tutor rating
    auto tutorUser = userRepo->findUserById(bPtr->tutorId);
    if (tutorUser && tutorUser->getRole() == UserRole::TUTOR) {
        auto tutor = std::static_pointer_cast<Tutor>(tutorUser);
        auto tutorRevs = reviewRepo->getReviewsByTutorId(bPtr->tutorId);
        double totalScore = 0.0;
        for (const auto& r : tutorRevs) {
            totalScore += r.ratingScore;
        }
        double newRating = tutorRevs.empty() ? 5.0 : (totalScore / tutorRevs.size());
        tutor->setRating(newRating);
        tutor->setTotalReviews(static_cast<int>(tutorRevs.size()));
        userRepo->updateUser(*tutor);
    }

    outMsg = "Gửi đánh giá thành công!";
    return true;
}

std::vector<CourseBooking> BookingService::getStudentCourseBookings(int studentId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) return {};
    autoCompleteExpiredCourses();
    return courseBookingRepo->getCourseBookingsByStudentId(studentId);
}

std::vector<CourseBooking> BookingService::getTutorCourseBookings(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) return {};
    autoCompleteExpiredCourses();
    return courseBookingRepo->getCourseBookingsByTutorId(tutorId);
}

std::vector<CourseBooking> BookingService::getAllCourseBookings() {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) return {};
    autoCompleteExpiredCourses();
    return courseBookingRepo->getAllCourseBookings();
}

std::shared_ptr<CourseBooking> BookingService::findCourseBookingById(int courseBookingId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!courseBookingRepo) return nullptr;
    return courseBookingRepo->findCourseBookingById(courseBookingId);
}

StudentBookingMetrics BookingService::getStudentBookingMetrics(int studentId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    StudentBookingMetrics metrics;
    if (!courseBookingRepo) return metrics;

    autoCompleteExpiredCourses();
    auto courses = getStudentCourseBookings(studentId);
    for (const auto& cb : courses) {
        if (cb.status == "ACCEPTED") {
            metrics.acceptedCount++;
            metrics.weeklyCost += cb.hourlyRate * static_cast<double>(cb.getWeeklySlots().size());
        } else if (cb.status == "PENDING") {
            metrics.pendingCount++;
        } else if (cb.status == "COMPLETED" || cb.status == "CANCELLED" ||
                   cb.status == "REJECTED" || cb.status == "EARLY_TERMINATED") {
            metrics.historyCount++;
        }
    }
    return metrics;
}

std::vector<TimetableSessionDTO> BookingService::getAcceptedSessionsForWeek(int userId, UserRole role, const std::string& mondayDateStr) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    std::vector<TimetableSessionDTO> result;
    if (!courseBookingRepo) return result;

    autoCompleteExpiredCourses();

    std::string monday = DateUtils::isValidDateString(mondayDateStr) ? DateUtils::getWeekMonday(mondayDateStr) : DateUtils::getWeekMonday();
    std::string sunday = DateUtils::addDays(monday, 6);

    std::vector<CourseBooking> courses;
    if (role == UserRole::STUDENT) {
        courses = getStudentCourseBookings(userId);
    } else if (role == UserRole::TUTOR) {
        courses = getTutorCourseBookings(userId);
    } else {
        courses = getAllCourseBookings();
    }

    for (const auto& c : courses) {
        // Show ACCEPTED, or EARLY_TERMINATED/COMPLETED for past/current week history
        if (c.status != "ACCEPTED" && c.status != "EARLY_TERMINATED" && c.status != "COMPLETED") continue;

        auto slots = c.getWeeklySlots();
        auto allSessions = DateUtils::generateMultiSlotSessions(c.startDate, c.weeksCount, slots);

        for (size_t i = 0; i < allSessions.size(); ++i) {
            const auto& s = allSessions[i];

            // If early terminated, future sessions after actualEndDate were cancelled
            if (c.status == "EARLY_TERMINATED" && !c.actualEndDate.empty()) {
                if (DateUtils::compareDates(s.date, c.actualEndDate) > 0) {
                    continue; // Skip cancelled future session
                }
            }

            if (DateUtils::compareDates(s.date, monday) >= 0 && DateUtils::compareDates(s.date, sunday) <= 0) {
                TimetableSessionDTO dto;
                dto.courseBookingId = c.id;
                dto.sessionNumber = static_cast<int>(i + 1);
                dto.totalSessions = static_cast<int>(allSessions.size());
                dto.date = s.date;
                dto.dayOfWeek = s.dayOfWeek;
                dto.slotIndex = s.slotIndex;
                dto.subject = c.subject;
                dto.studentId = c.studentId;
                dto.tutorId = c.tutorId;
                dto.hourlyRate = c.hourlyRate;
                dto.status = c.status;

                std::string sName = c.studentName;
                std::string tName = c.tutorName;
                if (userRepo) {
                    if (sName.empty()) {
                        auto sUser = userRepo->findUserById(c.studentId);
                        if (sUser) sName = sUser->getFullName();
                    }
                    if (tName.empty()) {
                        auto tUser = userRepo->findUserById(c.tutorId);
                        if (tUser) tName = tUser->getFullName();
                    }
                }
                dto.studentName = sName;
                dto.tutorName = tName;

                result.push_back(dto);
            }
        }
    }
    return result;
}

std::vector<Review> BookingService::getTutorReviews(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!reviewRepo) return {};
    return reviewRepo->getReviewsByTutorId(tutorId);
}

std::vector<Review> BookingService::getAllReviews() {
    std::lock_guard<std::recursive_mutex> lock(serviceMutex);
    if (!reviewRepo) return {};
    return reviewRepo->getAllReviews();
}

} // namespace TutorMatch
