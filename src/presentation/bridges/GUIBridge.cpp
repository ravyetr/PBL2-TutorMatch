#include "presentation/bridges/GUIBridge.h"
#include "business/matching/SubjectTaxonomy.h"
#include "business/domain/DateUtils.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace TutorMatch {

// ============================================================================
// Centralized Presentation Time & Slot Constants (DRY Principle)
// ============================================================================
static const char* DAY_NAMES[7] = {
    "Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6", "Thứ 7", "Chủ Nhật"
};

static const char* SLOT_TIMES[8] = {
    "07:30 - 09:00", "09:15 - 10:45", "13:30 - 15:00", "15:15 - 16:45",
    "17:00 - 18:30", "18:45 - 20:15", "20:30 - 22:00", "22:00 - 23:30"
};

static void decodeSlotMask(uint64_t slotMask, int& outDay, int& outSlot) {
    outDay = 0;
    outSlot = 0;
    for (int d = 0; d < ScheduleGuard::DAYS_PER_WEEK; ++d) {
        for (int s = 0; s < ScheduleGuard::SLOTS_PER_DAY; ++s) {
            if ((slotMask & ScheduleGuard::getSlotMask(d, s)) != 0) {
                outDay = d;
                outSlot = s;
                return;
            }
        }
    }
}

// ============================================================================
// GUIBridge Implementation
// ============================================================================

#ifdef QT_CORE_LIB
GUIBridge::GUIBridge(QObject *parent)
    : QObject(parent), authService(nullptr), userService(nullptr), bookingService(nullptr),
      reviewService(nullptr), matchingService(nullptr), adminService(nullptr),
      statusMessageStr("Sẵn sàng!") {}
#else
GUIBridge::GUIBridge()
    : authService(nullptr), userService(nullptr), bookingService(nullptr),
      reviewService(nullptr), matchingService(nullptr), adminService(nullptr),
      statusMessageStr("Sẵn sàng!") {}
#endif

GUIBridge::~GUIBridge() {}

bool GUIBridge::initialize(std::shared_ptr<AuthService> aService,
                          std::shared_ptr<UserService> uService,
                          std::shared_ptr<BookingService> bService,
                          std::shared_ptr<ReviewService> rService,
                          std::shared_ptr<MatchingService> mService,
                          std::shared_ptr<AdminService> admService) {
    if (!aService || !uService || !bService || !rService || !mService || !admService) return false;
    authService = std::move(aService);
    userService = std::move(uService);
    bookingService = std::move(bService);
    reviewService = std::move(rService);
    matchingService = std::move(mService);
    adminService = std::move(admService);
    if (matchingService && bookingService) {
        matchingService->setCourseBookingRepository(bookingService->getCourseBookingRepository());
    }
    if (bookingService) {
        bookingService->autoCompleteExpiredCourses();
        bookingService->reconcileAllTutorEarningsOnStartup();
    }
    return true;
}

bool GUIBridge::initialize(std::shared_ptr<IUserRepository> uRepo,
                          std::shared_ptr<IReviewRepository> rRepo,
                          std::shared_ptr<ICourseBookingRepository> cbRepo) {
    if (!uRepo || !rRepo) return false;

    auto aService = std::make_shared<AuthService>(uRepo);
    auto uService = std::make_shared<UserService>(uRepo);
    auto bService = std::make_shared<BookingService>(uRepo, rRepo, cbRepo);
    auto rService = std::make_shared<ReviewService>(rRepo, uRepo, cbRepo);
    auto mService = std::make_shared<MatchingService>(uRepo, cbRepo);
    auto admService = std::make_shared<AdminService>(uRepo, rRepo, bService);

    return initialize(aService, uService, bService, rService, mService, admService);
}

bool GUIBridge::initialize(std::shared_ptr<MockDatabaseEngine> engine) {
    if (!engine) return false;
    return initialize(engine->getUserRepository(), engine->getReviewRepository(), engine->getCourseBookingRepository());
}

// ============================================================================
// Authentication & Session (Delegates to AuthService - SRP)
// ============================================================================

bool GUIBridge::login(const std::string& username, const std::string& password, std::string& outError) {
    if (!authService) {
        outError = "Dịch vụ xác thực chưa khởi tạo!";
        return false;
    }
    return authService->login(username, password, outError);
}

void GUIBridge::logout() {
    if (authService) {
        authService->logout();
    }
}

bool GUIBridge::isLoggedIn() const {
    return authService ? authService->isLoggedIn() : false;
}

std::shared_ptr<User> GUIBridge::getCurrentUser() const {
    return authService ? authService->getCurrentUser() : nullptr;
}

UserRole GUIBridge::getCurrentRole() const {
    return authService ? authService->getCurrentRole() : UserRole::STUDENT;
}

#ifdef QT_CORE_LIB
QString GUIBridge::getCurrentUserFullName() const {
    auto u = getCurrentUser();
    if (u) {
        return QString::fromStdString(u->getFullName());
    }
    return QString();
}

QString GUIBridge::getCurrentUserRole() const {
    auto u = getCurrentUser();
    if (u) {
        return QString::fromStdString(u->getRoleString());
    }
    return QString();
}

QString GUIBridge::getStatusMessage() const {
    return QString::fromStdString(statusMessageStr);
}

void GUIBridge::clearStatusMessage() {
    statusMessageStr.clear();
    Q_EMIT statusMessageChanged();
}

bool GUIBridge::qmlLogin(const QString& username, const QString& password) {
    std::string outErr;
    bool res = login(username.toStdString(), password.toStdString(), outErr);
    if (res) {
        statusMessageStr = "Đăng nhập thành công!";
    } else {
        statusMessageStr = outErr;
    }
    Q_EMIT stateChanged();
    Q_EMIT statusMessageChanged();
    return res;
}

void GUIBridge::qmlLogout() {
    logout();
    statusMessageStr = "Đã đăng xuất!";
    Q_EMIT stateChanged();
    Q_EMIT statusMessageChanged();
}

bool GUIBridge::qmlRegisterStudent(const QString& username, const QString& password,
                                   const QString& fullName, const QString& email,
                                   int gradeLevel, const QString& preferredSubject, double maxBudget) {
    if (!authService) return false;
    std::string outErr;
    std::string gStr = std::to_string(gradeLevel);
    bool ok = authService->registerStudent(username.toStdString(), password.toStdString(), fullName.toStdString(),
                                          email.toStdString(), gStr, preferredSubject.toStdString(),
                                          maxBudget, outErr);
    if (ok) {
        statusMessageStr = "Đăng ký tài khoản học viên thành công! Vui lòng đăng nhập.";
    } else {
        statusMessageStr = outErr.empty() ? "Đăng ký học viên thất bại!" : outErr;
    }
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return ok;
}

bool GUIBridge::qmlRegisterTutor(const QString& username, const QString& password,
                                 const QString& fullName, const QString& email,
                                 const QString& subjectSpecialty, double hourlyRate, const QString& bio) {
    if (!authService) return false;
    std::string outErr;
    bool ok = authService->registerTutor(username.toStdString(), password.toStdString(), fullName.toStdString(),
                                        email.toStdString(), subjectSpecialty.toStdString(), hourlyRate,
                                        bio.toStdString(), outErr);
    if (ok) {
        statusMessageStr = "Đăng ký tài khoản gia sư thành công! Vui lòng đăng nhập.";
    } else {
        statusMessageStr = outErr.empty() ? "Đăng ký gia sư thất bại!" : outErr;
    }
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return ok;
}
#endif

bool GUIBridge::registerStudent(const std::string& username, const std::string& password, const std::string& fullName,
                                const std::string& email, const std::string& gradeLevel, const std::string& preferredSubject,
                                double maxBudget, std::string& outError) {
    if (!authService) {
        outError = "AuthService chưa sẵn sàng!";
        return false;
    }
    return authService->registerStudent(username, password, fullName, email, gradeLevel, preferredSubject, maxBudget, outError);
}

bool GUIBridge::registerTutor(const std::string& username, const std::string& password, const std::string& fullName,
                              const std::string& email, const std::string& subjectSpecialty, double hourlyRate,
                              const std::string& bio, std::string& outError) {
    if (!authService) {
        outError = "AuthService chưa sẵn sàng!";
        return false;
    }
    return authService->registerTutor(username, password, fullName, email, subjectSpecialty, hourlyRate, bio, outError);
}

// ============================================================================
// Student Profile & Timetable Management (Delegates to UserService)
// ============================================================================

#ifdef QT_CORE_LIB
QVariantMap GUIBridge::getCurrentStudentProfile() const {
    QVariantMap map;
    auto u = getCurrentUser();
    if (u && u->getRole() == UserRole::STUDENT) {
        auto student = std::static_pointer_cast<Student>(u);
        map["id"] = student->getId();
        map["fullName"] = QString::fromStdString(student->getFullName());
        map["username"] = QString::fromStdString(student->getUsername());
        map["email"] = QString::fromStdString(student->getEmail());
        map["gradeLevel"] = student->getGradeLevel();
        map["isActive"] = student->getIsActive();
        map["statusString"] = student->getIsActive() ? QString::fromUtf8("HOẠT ĐỘNG") : QString::fromUtf8("ĐÃ BỊ KHÓA");
    } else {
        map["id"] = 0;
        map["fullName"] = QString::fromUtf8("Chưa đăng nhập");
        map["username"] = "";
        map["email"] = "";
        map["gradeLevel"] = 0;
        map["isActive"] = false;
        map["statusString"] = QString::fromUtf8("KHÔNG XÁC ĐỊNH");
    }
    return map;
}

bool GUIBridge::updateStudentProfile(const QString& name, const QString& email, int grade, const QString& targetSubject, double budget) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !userService) {
        statusMessageStr = "Chỉ học viên đang đăng nhập mới có thể chỉnh sửa hồ sơ!";
        Q_EMIT statusMessageChanged();
        return false;
    }

    auto student = std::static_pointer_cast<Student>(u);
    std::string outErr;
    bool ok = false;
    if (targetSubject.trimmed().isEmpty() && budget <= 0.0) {
        ok = userService->updateStudentProfile(*student, name.trimmed().toStdString(), email.trimmed().toStdString(),
                                               grade, outErr);
    } else {
        ok = userService->updateStudentProfile(*student, name.trimmed().toStdString(), email.trimmed().toStdString(),
                                               grade, targetSubject.trimmed().toStdString(), budget, outErr);
    }
    if (ok) {
        statusMessageStr = "Cập nhật hồ sơ học viên thành công!";
        Q_EMIT studentProfileChanged();
        Q_EMIT stateChanged();
        Q_EMIT dataChanged();
    } else {
        statusMessageStr = outErr;
    }
    Q_EMIT statusMessageChanged();
    return ok;
}

QVariantList GUIBridge::getStudentWeeklySchedule(const QString& weekStartDateStr) {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !bookingService) return list;

    auto student = std::static_pointer_cast<Student>(u);
    uint64_t studentMask = student->getScheduleBitmask();

    std::string refDate = weekStartDateStr.trimmed().toStdString();
    std::string monday = DateUtils::isValidDateString(refDate) ? DateUtils::getWeekMonday(refDate) : DateUtils::getWeekMonday();
    std::string today = DateUtils::getCurrentDateString();
    auto weekDays = DateUtils::getWeekDays(monday);

    auto courseSessions = bookingService->getAcceptedSessionsForWeek(student->getId(), UserRole::STUDENT, monday);
    const TimetableSessionDTO* sessionGrid[8][7] = { nullptr };
    for (const auto& s : courseSessions) {
        if (s.slotIndex >= 0 && s.slotIndex < 8 && s.dayOfWeek >= 0 && s.dayOfWeek < 7) {
            sessionGrid[s.slotIndex][s.dayOfWeek] = &s;
        }
    }

    for (int slot = 0; slot < 8; ++slot) {
        for (int day = 0; day < 7; ++day) {
            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
            std::string cellDate = (day < static_cast<int>(weekDays.size())) ? weekDays[day] : "";
            bool isCellToday = (cellDate == today);

            QVariantMap cellMap;
            cellMap["dayIndex"] = day;
            cellMap["slotIndex"] = slot;
            cellMap["date"] = QString::fromStdString(cellDate);
            cellMap["isToday"] = isCellToday;
            cellMap["dayName"] = QString::fromUtf8(DAY_NAMES[day]);
            cellMap["timeRange"] = QString::fromUtf8(SLOT_TIMES[slot]);

            const TimetableSessionDTO* cs = sessionGrid[slot][day];
            if (cs != nullptr) {
                cellMap["status"] = "BOOKED";
                cellMap["isCourse"] = true;
                cellMap["bookingId"] = cs->courseBookingId;
                cellMap["courseBookingId"] = cs->courseBookingId;
                cellMap["sessionNumber"] = cs->sessionNumber;
                cellMap["totalSessions"] = cs->totalSessions;
                cellMap["sessionLabel"] = QString("Buổi %1/%2").arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["subjectName"] = QString::fromStdString(cs->subject);
                cellMap["tutorName"] = QString::fromStdString(cs->tutorName);
                cellMap["studentName"] = QString::fromStdString(cs->studentName);
                cellMap["displayName"] = QString::fromStdString(cs->tutorName);
                cellMap["primaryText"] = QString::fromStdString(cs->tutorName);
                cellMap["secondaryText"] = QString("%1 (Buổi %2/%3)").arg(QString::fromStdString(cs->subject)).arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["label"] = QString::fromStdString(cs->subject);
                cellMap["subLabel"] = QString("Gia sư: %1 (Buổi %2/%3)").arg(QString::fromStdString(cs->tutorName)).arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["hourlyRate"] = cs->hourlyRate;
                cellMap["editable"] = false;
                cellMap["locked"] = true;
            } else {
                bool isFree = ((studentMask & sMask) != 0);
                cellMap["isCourse"] = false;
                cellMap["bookingId"] = 0;
                cellMap["sessionNumber"] = 0;
                cellMap["totalSessions"] = 0;
                cellMap["sessionLabel"] = "";
                cellMap["displayName"] = "";
                cellMap["subjectName"] = "";
                cellMap["tutorName"] = "";
                cellMap["primaryText"] = "";
                cellMap["secondaryText"] = "";
                cellMap["editable"] = true;
                cellMap["locked"] = false;
                if (isFree) {
                    cellMap["status"] = "FREE";
                    cellMap["label"] = QString::fromUtf8("RẢNH");
                    cellMap["subLabel"] = "";
                } else {
                    cellMap["status"] = "BUSY";
                    cellMap["label"] = QString::fromUtf8("[BẬN]");
                    cellMap["subLabel"] = "";
                }
            }
            list.append(cellMap);
        }
    }
    return list;
}

QVariantList GUIBridge::getWeeklyScheduleMatrix(const QString& weekStartDateStr) {
    auto u = getCurrentUser();
    if (u && u->getRole() == UserRole::TUTOR) {
        return getTutorWeeklySchedule(weekStartDateStr);
    }
    return getStudentWeeklySchedule(weekStartDateStr);
}

bool GUIBridge::clearStudentBusySchedule() {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !userService) return false;
    auto student = std::static_pointer_cast<Student>(u);
    
    uint64_t fullMask = ScheduleGuard::VALID_SCHEDULE_MASK;
    student->setScheduleBitmask(fullMask);
    if (userService) {
        userService->updateUser(*student);
    }
    statusMessageStr = "Đã đặt lại thời khóa biểu học viên thành rảnh toàn bộ!";
    Q_EMIT statusMessageChanged();
    Q_EMIT studentWeeklyScheduleChanged();
    Q_EMIT dataChanged();
    return true;
}

bool GUIBridge::clearAllBusySlots() {
    auto u = getCurrentUser();
    if (u && u->getRole() == UserRole::TUTOR) {
        auto tutor = std::static_pointer_cast<Tutor>(u);
        uint64_t fullMask = ScheduleGuard::VALID_SCHEDULE_MASK;
        tutor->setScheduleBitmask(fullMask);
        if (userService) {
            userService->updateUser(*tutor);
        }
        statusMessageStr = "Đã đặt lại toàn bộ lịch rảnh giảng dạy!";
        Q_EMIT statusMessageChanged();
        Q_EMIT tutorScheduleChanged();
        Q_EMIT dataChanged();
        return true;
    }
    return clearStudentBusySchedule();
}

bool GUIBridge::qmlToggleStudentScheduleSlot(int day, int slot) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !userService) {
        statusMessageStr = "Chỉ học viên mới có quyền đổi lịch!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    auto student = std::static_pointer_cast<Student>(u);
    std::string msg;
    bool res = userService->toggleStudentScheduleSlot(*student, day, slot, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT studentWeeklyScheduleChanged();
    Q_EMIT dataChanged();
    return res;
}
#endif

bool GUIBridge::toggleStudentScheduleSlot(int day, int slot, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !userService) {
        outMsg = "Chỉ học viên mới có quyền đổi lịch!";
        return false;
    }
    auto student = std::static_pointer_cast<Student>(u);
    return userService->toggleStudentScheduleSlot(*student, day, slot, outMsg);
}

bool GUIBridge::resetStudentSchedule(std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !userService) {
        outMsg = "Chỉ học viên mới có quyền đổi lịch!";
        return false;
    }
    auto student = std::static_pointer_cast<Student>(u);
    return userService->resetStudentSchedule(*student, outMsg);
}

// ============================================================================
// Tutor Profile & Schedule Management (Delegates to UserService)
// ============================================================================

#ifdef QT_CORE_LIB
QVariantMap GUIBridge::getCurrentTutorProfile() const {
    QVariantMap map;
    auto u = getCurrentUser();
    if (u && u->getRole() == UserRole::TUTOR) {
        auto tutor = std::static_pointer_cast<Tutor>(u);
        map["id"] = tutor->getId();
        map["fullName"] = QString::fromStdString(tutor->getFullName());
        map["username"] = QString::fromStdString(tutor->getUsername());
        map["email"] = QString::fromStdString(tutor->getEmail());
        map["bio"] = QString::fromStdString(tutor->getBio());
        map["subjectSpecialty"] = QString::fromStdString(tutor->getSubjectSpecialty());
        map["hourlyRate"] = tutor->getHourlyRate();
        map["rating"] = tutor->getRating();
        map["totalReviews"] = tutor->getTotalReviews();
        map["totalEarnings"] = tutor->getTotalEarnings();
        map["isActive"] = tutor->getIsActive();
        map["statusString"] = tutor->getIsActive() ? QString::fromUtf8("HOẠT ĐỘNG") : QString::fromUtf8("ĐÃ BỊ KHÓA");
    }
    return map;
}

bool GUIBridge::updateTutorProfile(const QString& name, const QString& email, const QString& bio, const QString& subjectSpecialty, double hourlyRate) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !userService) {
        statusMessageStr = "Chỉ gia sư đang đăng nhập mới có thể chỉnh sửa hồ sơ!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    auto tutor = std::static_pointer_cast<Tutor>(u);
    std::string outErr;
    bool ok = userService->updateTutorProfile(*tutor, name.trimmed().toStdString(), email.trimmed().toStdString(),
                                            bio.trimmed().toStdString(), subjectSpecialty.trimmed().toStdString(),
                                            hourlyRate, outErr);
    if (ok) {
        statusMessageStr = "Cập nhật hồ sơ gia sư thành công!";
        Q_EMIT tutorProfileChanged();
        Q_EMIT stateChanged();
        Q_EMIT dataChanged();
    } else {
        statusMessageStr = outErr;
    }
    Q_EMIT statusMessageChanged();
    return ok;
}

bool GUIBridge::updateTutorProfile(const QString& title, const QString& name, const QString& email, const QString& bio, const QString& subjectSpecialty, double hourlyRate) {
    QString full = name.trimmed();
    QString t = title.trimmed();
    if (!t.isEmpty() && t != QString::fromUtf8("Không") && !full.startsWith(t)) {
        full = t + " " + full;
    }
    return updateTutorProfile(full, email, bio, subjectSpecialty, hourlyRate);
}

QVariantList GUIBridge::getTutorWeeklySchedule(const QString& weekStartDateStr) {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return list;

    auto tutor = std::static_pointer_cast<Tutor>(u);
    uint64_t tutorMask = tutor->getScheduleBitmask();

    std::string refDate = weekStartDateStr.trimmed().toStdString();
    std::string monday = DateUtils::isValidDateString(refDate) ? DateUtils::getWeekMonday(refDate) : DateUtils::getWeekMonday();
    std::string today = DateUtils::getCurrentDateString();
    auto weekDays = DateUtils::getWeekDays(monday);

    auto courseSessions = bookingService->getAcceptedSessionsForWeek(tutor->getId(), UserRole::TUTOR, monday);
    const TimetableSessionDTO* sessionGrid[8][7] = { nullptr };
    for (const auto& s : courseSessions) {
        if (s.slotIndex >= 0 && s.slotIndex < 8 && s.dayOfWeek >= 0 && s.dayOfWeek < 7) {
            sessionGrid[s.slotIndex][s.dayOfWeek] = &s;
        }
    }

    for (int slot = 0; slot < 8; ++slot) {
        for (int day = 0; day < 7; ++day) {
            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
            std::string cellDate = (day < static_cast<int>(weekDays.size())) ? weekDays[day] : "";
            bool isCellToday = (cellDate == today);

            QVariantMap cellMap;
            cellMap["dayIndex"] = day;
            cellMap["slotIndex"] = slot;
            cellMap["date"] = QString::fromStdString(cellDate);
            cellMap["isToday"] = isCellToday;
            cellMap["dayName"] = QString::fromUtf8(DAY_NAMES[day]);
            cellMap["timeRange"] = QString::fromUtf8(SLOT_TIMES[slot]);

            const TimetableSessionDTO* cs = sessionGrid[slot][day];
            if (cs != nullptr) {
                cellMap["status"] = "BOOKED";
                cellMap["isCourse"] = true;
                cellMap["bookingId"] = cs->courseBookingId;
                cellMap["courseBookingId"] = cs->courseBookingId;
                cellMap["sessionNumber"] = cs->sessionNumber;
                cellMap["totalSessions"] = cs->totalSessions;
                cellMap["sessionLabel"] = QString("Buổi %1/%2").arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["subjectName"] = QString::fromStdString(cs->subject);
                cellMap["studentName"] = QString::fromStdString(cs->studentName);
                cellMap["tutorName"] = QString::fromStdString(cs->tutorName);
                cellMap["displayName"] = QString::fromStdString(cs->studentName);
                cellMap["primaryText"] = QString::fromStdString(cs->studentName);
                cellMap["secondaryText"] = QString("%1 (Buổi %2/%3)").arg(QString::fromStdString(cs->subject)).arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["label"] = QString::fromStdString(cs->subject);
                cellMap["subLabel"] = QString("Học viên: %1 (Buổi %2/%3)").arg(QString::fromStdString(cs->studentName)).arg(cs->sessionNumber).arg(cs->totalSessions);
                cellMap["hourlyRate"] = cs->hourlyRate;
                cellMap["editable"] = false;
                cellMap["locked"] = true;
            } else {
                bool isFree = ((tutorMask & sMask) != 0);
                cellMap["isCourse"] = false;
                cellMap["bookingId"] = 0;
                cellMap["sessionNumber"] = 0;
                cellMap["totalSessions"] = 0;
                cellMap["sessionLabel"] = "";
                cellMap["displayName"] = "";
                cellMap["subjectName"] = "";
                cellMap["studentName"] = "";
                cellMap["primaryText"] = "";
                cellMap["secondaryText"] = "";
                cellMap["editable"] = true;
                cellMap["locked"] = false;
                if (isFree) {
                    cellMap["status"] = "FREE";
                    cellMap["label"] = QString::fromUtf8("RẢNH");
                    cellMap["subLabel"] = "";
                } else {
                    cellMap["status"] = "BUSY";
                    cellMap["label"] = QString::fromUtf8("[BẬN]");
                    cellMap["subLabel"] = "";
                }
            }
            list.append(cellMap);
        }
    }
    return list;
}

bool GUIBridge::qmlToggleTutorScheduleSlot(int day, int slot) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !userService) {
        statusMessageStr = "Chỉ gia sư mới có quyền thao tác!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    auto tutor = std::static_pointer_cast<Tutor>(u);
    std::string msg;
    bool res = userService->toggleTutorScheduleSlot(*tutor, day, slot, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT tutorScheduleChanged();
    Q_EMIT dataChanged();
    return res;
}
#endif

bool GUIBridge::toggleTutorScheduleSlot(int day, int slot, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !userService) {
        outMsg = "Chỉ gia sư mới có quyền thao tác!";
        return false;
    }
    auto tutor = std::static_pointer_cast<Tutor>(u);
    return userService->toggleTutorScheduleSlot(*tutor, day, slot, outMsg);
}

// ============================================================================
// Marketplace & Search (Delegates to UserService)
// ============================================================================

#ifdef QT_CORE_LIB
QVariantList GUIBridge::qmlGetAllTutors() {
    QVariantList list;
    if (!userService) return list;
    auto tutors = userService->getAllActiveTutors();
    for (const auto& t : tutors) {
        QVariantMap map;
        map["id"] = t->getId();
        map["fullName"] = QString::fromStdString(t->getFullName());
        map["subject"] = QString::fromStdString(t->getSubjectSpecialty());
        map["hourlyRate"] = t->getHourlyRate();
        map["rating"] = t->getRating();
        map["totalReviews"] = t->getTotalReviews();
        list.append(map);
    }
    return list;
}

QVariantList GUIBridge::searchAndFilterTutors(const QString& keyword, const QString& subject, double minPrice, double maxPrice, double minRating) {
    QVariantList list;
    if (!userService) return list;

    auto tutors = userService->searchAndFilterTutors(keyword.trimmed().toStdString(),
                                                    subject.trimmed().toStdString(),
                                                    minPrice, maxPrice, minRating);
    for (const auto& t : tutors) {
        QString fullName = QString::fromStdString(t->getFullName());
        QString initials = fullName.isEmpty() ? "G" : fullName.left(1).toUpper();

        QVariantMap map;
        map["id"] = t->getId();
        map["tutorId"] = t->getId();
        map["fullName"] = fullName;
        map["initials"] = initials;
        map["subject"] = QString::fromStdString(t->getSubjectSpecialty());
        map["hourlyRate"] = t->getHourlyRate();
        map["rating"] = t->getRating();
        map["totalReviews"] = t->getTotalReviews();
        map["bio"] = QString::fromStdString(t->getBio());

        list.append(map);
    }
    return list;
}

QVariantMap GUIBridge::getTutorDetail(int tutorId) {
    QVariantMap result;
    if (!userService || !bookingService) return result;

    auto tutor = userService->getTutorById(tutorId);
    if (!tutor) return result;

    result["id"] = tutor->getId();
    result["tutorId"] = tutor->getId();
    result["fullName"] = QString::fromStdString(tutor->getFullName());
    result["email"] = QString::fromStdString(tutor->getEmail());
    result["bio"] = QString::fromStdString(tutor->getBio());
    result["subjectSpecialty"] = QString::fromStdString(tutor->getSubjectSpecialty());
    result["hourlyRate"] = tutor->getHourlyRate();
    result["rating"] = tutor->getRating();
    result["totalReviews"] = tutor->getTotalReviews();
    result["isActive"] = tutor->getIsActive();
    result["scheduleBitmask"] = QString::number(tutor->getScheduleBitmask());
    result["availableSlots"] = getTutorAvailableSlots(tutorId);

    QVariantList reviewList;
    auto reviews = bookingService->getTutorReviews(tutorId);
    for (const auto& r : reviews) {
        QVariantMap rMap;
        rMap["id"] = r.id;
        rMap["studentName"] = QString::fromStdString(r.studentName);
        rMap["ratingScore"] = r.ratingScore;
        rMap["comment"] = QString::fromStdString(r.comment);
        rMap["createdAt"] = QString::fromStdString(r.createdAt);
        reviewList.append(rMap);
    }
    result["reviews"] = reviewList;
    return result;
}

QVariantList GUIBridge::getTutorAvailableSlots(int tutorId) {
    QVariantList result;
    if (!userService) return result;

    auto t = userService->getTutorById(tutorId);
    if (!t) return result;

    uint64_t mask = t->getScheduleBitmask();
    for (int day = 0; day < ScheduleGuard::DAYS_PER_WEEK; ++day) {
        for (int slot = 0; slot < ScheduleGuard::SLOTS_PER_DAY; ++slot) {
            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
            if ((mask & sMask) != 0) {
                QVariantMap item;
                item["day"] = QString::fromUtf8(DAY_NAMES[day]);
                item["dayIndex"] = day;
                item["slotIndex"] = slot;
                item["timeText"] = QString::fromUtf8(SLOT_TIMES[slot]);
                result.append(item);
            }
        }
    }
    return result;
}

QVariantList GUIBridge::getTutorAvailableBookingSlots(int tutorId, int studentId) {
    QVariantList result;
    if (!userService || !bookingService) return result;

    auto t = userService->getTutorById(tutorId);
    if (!t || !t->getIsActive()) return result;

    uint64_t tutorMask = t->getScheduleBitmask();

    int targetStudentId = studentId;
    auto u = getCurrentUser();
    if (targetStudentId <= 0 && u && u->getRole() == UserRole::STUDENT) {
        targetStudentId = u->getId();
    }
    auto student = targetStudentId > 0 ? userService->getStudentById(targetStudentId) : nullptr;

    for (int day = 0; day < ScheduleGuard::DAYS_PER_WEEK; ++day) {
        for (int slot = 0; slot < ScheduleGuard::SLOTS_PER_DAY; ++slot) {
            uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);

            if ((tutorMask & sMask) == 0) continue;

            bool isStudentBusy = false;
            if (student) {
                if ((student->getScheduleBitmask() & sMask) == 0) {
                    isStudentBusy = true;
                }
            }

            QVariantMap item;
            item["dayIndex"] = day;
            item["slotIndex"] = slot;
            item["dayName"] = QString::fromUtf8(DAY_NAMES[day]);
            item["timeRange"] = QString::fromUtf8(SLOT_TIMES[slot]);
            item["slotLabel"] = QString::fromUtf8(DAY_NAMES[day]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slot]);
            item["isAvailable"] = !isStudentBusy;
            item["busyReason"] = isStudentBusy ? QString::fromUtf8("Học viên bận / đã có lịch") : QString();

            result.append(item);
        }
    }
    return result;
}
#endif

std::vector<std::shared_ptr<Tutor>> GUIBridge::getAllTutors() {
    if (!userService) return {};
    return userService->getAllActiveTutors();
}

std::vector<std::shared_ptr<Tutor>> GUIBridge::filterTutorsByPrice(double minP, double maxP) {
    if (!userService) return {};
    return userService->filterTutorsByPrice(minP, maxP);
}

std::vector<std::shared_ptr<Tutor>> GUIBridge::filterTutorsByRating(double minR) {
    if (!userService) return {};
    return userService->filterTutorsByRating(minR);
}

// ============================================================================
// Smart Matching (Delegates to MatchingService - OCP & Strategy Pattern)
// ============================================================================

std::vector<TutorMatchResult> GUIBridge::getSmartMatches(int topK) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !matchingService) return {};
    auto student = std::static_pointer_cast<Student>(u);
    return matchingService->findMatches(*student, topK);
}

#ifdef QT_CORE_LIB
QVariantList GUIBridge::qmlGetSmartMatches(int topK) {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT) return list;

    auto student = std::static_pointer_cast<Student>(u);
    auto matches = getSmartMatches(topK);
    int rank = 1;

    for (const auto& m : matches) {
        if (!m.tutor) continue;
        QVariantMap map;
        map["rank"] = rank++;
        map["id"] = m.tutor->getId();
        map["tutorId"] = m.tutor->getId();
        map["fullName"] = QString::fromStdString(m.tutor->getFullName());
        map["tutorName"] = QString::fromStdString(m.tutor->getFullName());
        map["subject"] = QString::fromStdString(m.tutor->getSubjectSpecialty());
        map["hourlyRate"] = m.tutor->getHourlyRate();
        map["rating"] = m.tutor->getRating();
        map["totalReviews"] = m.tutor->getTotalReviews();
        map["bio"] = QString::fromStdString(m.tutor->getBio());
        map["initials"] = (!m.tutor->getFullName().empty()) ? QString::fromStdString(m.tutor->getFullName()).left(1).toUpper() : QString("G");
        map["compositeScore"] = (int)m.compositeScore;
        map["subjectScore"] = (int)m.subjectScore;
        map["priceScore"] = (int)m.priceScore;
        map["scheduleScore"] = (int)m.scheduleScore;
        map["ratingScore"] = (int)m.ratingScore;
        map["experienceScore"] = (int)m.experienceScore;
        map["credibilityScore"] = (int)m.credibilityScore;

        int overlap = (m.matchedSlotCount > 0) ? m.matchedSlotCount : ScheduleGuard::countMatchingSlots(m.tutor->getScheduleBitmask(), student->getScheduleBitmask());
        map["scheduleOverlapCount"] = overlap;
        map["matchedSlotCount"] = m.matchedSlotCount;
        map["requestedSlotCount"] = m.requestedSlotCount;

        QStringList expList;
        for (const auto& exp : m.explanations) {
            expList.append(QString::fromStdString(exp));
        }
        map["explanations"] = expList;

        list.append(map);
    }
    return list;
}

QVariantList GUIBridge::qmlSearchSmartMatches(const QVariantMap& criteriaMap, int topK) {
    QVariantList list;
    if (!matchingService) return list;

    SmartMatchCriteria criteria;
    criteria.subject = criteriaMap.value("subject").toString().trimmed().toStdString();
    criteria.gradeLevel = criteriaMap.value("gradeLevel", 10).toInt();
    criteria.maxBudget = criteriaMap.value("maxBudget", 200000.0).toDouble();

    QVariant maskVar = criteriaMap.value("preferredScheduleMask");
    if (maskVar.canConvert<double>()) {
        criteria.preferredScheduleMask = static_cast<uint64_t>(maskVar.toULongLong());
    } else {
        bool ok = false;
        uint64_t maskParsed = maskVar.toString().toULongLong(&ok);
        criteria.preferredScheduleMask = ok ? maskParsed : 0ULL;
    }

    QVariant slotsVar = criteriaMap.value("selectedSlots");
    if (slotsVar.canConvert<QVariantList>()) {
        QVariantList sList = slotsVar.toList();
        for (const auto& item : sList) {
            QVariantMap sm = item.toMap();
            int d = sm.value("day", -1).toInt();
            int s = sm.value("slot", -1).toInt();
            if (d >= 0 && d <= 6 && s >= 0 && s <= 7) {
                criteria.preferredScheduleMask |= (1ULL << (d * 8 + s));
            }
        }
    }

    QString startStr = criteriaMap.value("startDate").toString().trimmed();
    if (startStr.isEmpty()) {
        criteria.startDate = DateUtils::getCurrentDateString();
    } else {
        criteria.startDate = startStr.toStdString();
    }

    criteria.weeksCount = criteriaMap.value("weeksCount", 4).toInt();
    if (criteria.weeksCount != 4 && criteria.weeksCount != 8 && criteria.weeksCount != 12) {
        criteria.weeksCount = 4;
    }

    criteria.strictSchedule = criteriaMap.value("strictSchedule", true).toBool();
    criteria.minRating = criteriaMap.value("minRating", 0.0).toDouble();

    auto matches = matchingService->findMatchesWithCriteria(criteria, topK);
    int rank = 1;

    for (const auto& m : matches) {
        if (!m.tutor) continue;
        QVariantMap map;
        map["rank"] = rank++;
        map["id"] = m.tutor->getId();
        map["tutorId"] = m.tutor->getId();
        map["fullName"] = QString::fromStdString(m.tutor->getFullName());
        map["tutorName"] = QString::fromStdString(m.tutor->getFullName());
        map["subject"] = QString::fromStdString(m.tutor->getSubjectSpecialty());
        map["hourlyRate"] = m.tutor->getHourlyRate();
        map["rating"] = m.tutor->getRating();
        map["totalReviews"] = m.tutor->getTotalReviews();
        map["bio"] = QString::fromStdString(m.tutor->getBio());
        map["initials"] = (!m.tutor->getFullName().empty()) ? QString::fromStdString(m.tutor->getFullName()).left(1).toUpper() : QString("G");

        // 100-point explainable scores
        map["compositeScore"] = static_cast<int>(std::round(m.compositeScore));
        map["scheduleScore"] = static_cast<int>(std::round(m.scheduleScore));
        map["priceScore"] = static_cast<int>(std::round(m.priceScore));
        map["ratingScore"] = static_cast<int>(std::round(m.ratingScore));
        map["experienceScore"] = static_cast<int>(std::round(m.experienceScore));
        map["credibilityScore"] = static_cast<int>(std::round(m.credibilityScore));

        map["matchedSlotCount"] = m.matchedSlotCount;
        map["requestedSlotCount"] = m.requestedSlotCount;
        map["scheduleOverlapCount"] = m.matchedSlotCount;

        QVariantList matchedSlotsList;
        for (const auto& p : m.matchedSlots) {
            QVariantMap slotMap;
            slotMap["day"] = p.first;
            slotMap["slot"] = p.second;
            slotMap["label"] = QString::fromUtf8(ScheduleGuard::formatSlotSingle(p.first, p.second).c_str());
            matchedSlotsList.append(slotMap);
        }
        map["matchedSlots"] = matchedSlotsList;

        QStringList expList;
        for (const auto& exp : m.explanations) {
            expList.append(QString::fromStdString(exp));
        }
        map["explanations"] = expList;

        list.append(map);
    }
    return list;
}
#endif

// ============================================================================
// Booking & Course Lifecycle (Delegates to BookingService - SRP)
// ============================================================================

bool GUIBridge::bookLesson(int tutorId, int day, int slot, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !bookingService) {
        outMsg = "Bạn không phải là Học viên!";
        return false;
    }
    if (day < 0 || day > 6) {
        outMsg = "Thứ không hợp lệ!";
        return false;
    }
    std::string today = DateUtils::getCurrentDateString();
    int currentDow = DateUtils::dayOfWeek(today);
    int daysUntil = (day - currentDow + 7) % 7;
    std::string startDate = DateUtils::addDays(today, daysUntil);
    auto student = std::static_pointer_cast<Student>(u);
    int newCourseId = 0;
    std::string tutorSubj = "";
    if (userService) {
        auto tutorUser = userService->getUserById(tutorId);
        if (tutorUser && tutorUser->getRole() == UserRole::TUTOR) {
            tutorSubj = std::static_pointer_cast<Tutor>(tutorUser)->getSubjectSpecialty();
        }
    }
    return bookingService->bookCourse(student->getId(), tutorId, tutorSubj, day, slot, startDate, 4, outMsg, &newCourseId);
}

#ifdef QT_CORE_LIB
bool GUIBridge::requestCourseBooking(int tutorId, const QString& subject, int dayIndex, int slotIndex, bool acknowledgeCommitment) {
    if (dayIndex < 0 || dayIndex > 6) return false;
    std::string today = DateUtils::getCurrentDateString();
    int currentDow = DateUtils::dayOfWeek(today);
    int daysUntil = (dayIndex - currentDow + 7) % 7;
    std::string startDate = DateUtils::addDays(today, daysUntil);
    return requestRecurringCourseBooking(tutorId, subject, dayIndex, slotIndex, QString::fromStdString(startDate), 4, acknowledgeCommitment);
}

bool GUIBridge::requestRecurringCourseBooking(int tutorId, const QString& subject, int dayIndex, int slotIndex, const QString& startDate, int weeksCount, bool acknowledgeCommitment) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !bookingService) {
        statusMessageStr = "Chỉ học viên mới có thể đặt khóa học định kỳ!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    std::string msg;
    int bookingId = 0;
    bool ok = bookingService->bookCourse(u->getId(), tutorId, subject.trimmed().toStdString(),
                                         dayIndex, slotIndex, startDate.trimmed().toStdString(),
                                         weeksCount, acknowledgeCommitment, msg, &bookingId);
    statusMessageStr = msg;
    if (ok) {
        Q_EMIT dataChanged();
        Q_EMIT studentWeeklyScheduleChanged();
    }
    Q_EMIT statusMessageChanged();
    return ok;
}

bool GUIBridge::requestCourseBookingMulti(int tutorId, const QString& subject, const QVariantList& chosenSlots,
                                         const QString& startDate, int weeksCount, bool acknowledgeCommitment) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT || !bookingService) {
        statusMessageStr = "Chỉ học viên mới có thể đặt khóa học định kỳ!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    std::vector<std::pair<int, int>> chosenSlotsPairs;
    for (const auto& var : chosenSlots) {
        QVariantMap sm = var.toMap();
        int d = sm.value("dayIndex", sm.value("dayOfWeek", -1)).toInt();
        int s = sm.value("slotIndex", -1).toInt();
        if (d >= 0 && d <= 6 && s >= 0 && s <= 7) {
            chosenSlotsPairs.push_back({d, s});
        }
    }
    std::string msg;
    int bookingId = 0;
    bool ok = bookingService->bookCourseMultiSlot(u->getId(), tutorId, subject.trimmed().toStdString(),
                                                  chosenSlotsPairs, startDate.trimmed().toStdString(),
                                                  weeksCount, acknowledgeCommitment, msg, &bookingId);
    statusMessageStr = msg;
    if (ok) {
        Q_EMIT dataChanged();
        Q_EMIT studentWeeklyScheduleChanged();
    }
    Q_EMIT statusMessageChanged();
    return ok;
}

bool GUIBridge::requestEarlyTermination(int courseBookingId, const QString& reason) {
    auto u = getCurrentUser();
    if (!u || !bookingService) {
        statusMessageStr = "Người dùng chưa đăng nhập hoặc dịch vụ chưa sẵn sàng!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    std::string msg;
    bool res = bookingService->requestEarlyTermination(u->getId(), u->getRole(), courseBookingId, reason.trimmed().toStdString(), msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    if (res) {
        Q_EMIT dataChanged();
        Q_EMIT studentWeeklyScheduleChanged();
        Q_EMIT tutorScheduleChanged();
    }
    return res;
}

bool GUIBridge::respondEarlyTermination(int courseBookingId, bool agree) {
    auto u = getCurrentUser();
    if (!u || !bookingService) {
        statusMessageStr = "Người dùng chưa đăng nhập hoặc dịch vụ chưa sẵn sàng!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    std::string msg;
    bool res = bookingService->respondEarlyTermination(u->getId(), u->getRole(), courseBookingId, agree, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    if (res) {
        Q_EMIT dataChanged();
        Q_EMIT studentWeeklyScheduleChanged();
        Q_EMIT tutorScheduleChanged();
    }
    return res;
}

QVariantList GUIBridge::calculateMultiSlotSessionDates(const QString& startDate, int weeksCount, const QVariantList& chosenSlots) {
    QVariantList list;
    std::vector<std::pair<int, int>> chosenSlotsPairs;
    for (const auto& var : chosenSlots) {
        QVariantMap sm = var.toMap();
        int d = sm.value("dayIndex", sm.value("dayOfWeek", -1)).toInt();
        int s = sm.value("slotIndex", -1).toInt();
        if (d >= 0 && d <= 6 && s >= 0 && s <= 7) {
            chosenSlotsPairs.push_back({d, s});
        }
    }
    auto sessions = DateUtils::generateMultiSlotSessions(startDate.trimmed().toStdString(), weeksCount, chosenSlotsPairs);
    for (const auto& sess : sessions) {
        QVariantMap m;
        m["date"] = QString::fromStdString(sess.date);
        m["dayOfWeek"] = sess.dayOfWeek;
        m["slotIndex"] = sess.slotIndex;
        m["dayName"] = QString::fromUtf8(DAY_NAMES[sess.dayOfWeek]);
        m["timeRange"] = QString::fromUtf8(SLOT_TIMES[sess.slotIndex]);
        m["label"] = QString::fromStdString(sess.date) + " (" + QString::fromUtf8(DAY_NAMES[sess.dayOfWeek]) + " · " + QString::fromUtf8(SLOT_TIMES[sess.slotIndex]) + ")";
        list.append(m);
    }
    return list;
}

QVariantList GUIBridge::getUpcomingStartDatesForDay(int targetDayOfWeek, int count) {
    QVariantList list;
    if (targetDayOfWeek < 0 || targetDayOfWeek > 6 || count <= 0) return list;
    std::string today = DateUtils::getCurrentDateString();
    int currentDow = DateUtils::dayOfWeek(today);
    int daysUntil = (targetDayOfWeek - currentDow + 7) % 7;
    // If target day is today, start from today (i=0) or next week
    for (int i = 0; i < count; ++i) {
        std::string d = DateUtils::addDays(today, daysUntil + 7 * i);
        QVariantMap item;
        item["date"] = QString::fromStdString(d);
        item["label"] = QString::fromStdString(d) + (i == 0 && daysUntil == 0 ? QString::fromUtf8(" (Hôm nay)") :
                                                    (i == 0 ? QString::fromUtf8(" (Tuần tới)") : QString("")));
        list.append(item);
    }
    return list;
}

QVariantList GUIBridge::getUpcomingStartDatesForSlots(const QVariantList& chosenSlots, int count) {
    QVariantList list;
    if (chosenSlots.isEmpty() || count <= 0) return list;

    std::vector<std::pair<int, int>> chosenSlotsPairs;
    for (const auto& var : chosenSlots) {
        QVariantMap sm = var.toMap();
        int d = sm.value("dayIndex", sm.value("dayOfWeek", -1)).toInt();
        int s = sm.value("slotIndex", -1).toInt();
        if (d >= 0 && d <= 6 && s >= 0 && s <= 7) {
            chosenSlotsPairs.push_back({d, s});
        }
    }
    if (chosenSlotsPairs.empty()) return list;

    std::string today = DateUtils::getCurrentDateString();
    auto dates = DateUtils::getUpcomingStartDatesForSlots(chosenSlotsPairs, count);

    for (size_t i = 0; i < dates.size(); ++i) {
        const std::string& d = dates[i];
        int dow = DateUtils::dayOfWeek(d);
        QVariantMap item;
        item["date"] = QString::fromStdString(d);
        item["dayOfWeek"] = dow;
        if (dow >= 0 && dow < 7) {
            item["dayName"] = QString::fromUtf8(DAY_NAMES[dow]);
        }

        QString label = QString::fromStdString(d);
        if (i == 0) {
            if (d == today) {
                label += QString::fromUtf8(" (Hôm nay)");
            } else {
                label += QString::fromUtf8(" (Sớm nhất)");
            }
        }
        item["label"] = label;
        list.append(item);
    }
    return list;
}

QVariantList GUIBridge::calculateCourseSessionDates(const QString& startDate, int weeksCount) {
    QVariantList list;
    auto dates = DateUtils::generateSessionDates(startDate.trimmed().toStdString(), weeksCount);
    for (const auto& d : dates) {
        list.append(QString::fromStdString(d));
    }
    return list;
}

int GUIBridge::getDayOfWeekFromDate(const QString& dateStr) {
    return DateUtils::dayOfWeek(dateStr.trimmed().toStdString());
}

QString GUIBridge::getDayNameFromDate(const QString& dateStr) {
    int dow = DateUtils::dayOfWeek(dateStr.trimmed().toStdString());
    if (dow >= 0 && dow < 7) {
        return QString::fromUtf8(DAY_NAMES[dow]);
    }
    return QString();
}

QString GUIBridge::getTodayDateString() {
    return QString::fromStdString(DateUtils::getCurrentDateString());
}

bool GUIBridge::isValidDateString(const QString& dateStr) {
    return DateUtils::isValidDateString(dateStr.trimmed().toStdString());
}

bool GUIBridge::qmlBookLesson(int tutorId, int day, int slot) {
    return requestCourseBooking(tutorId, "", day, slot);
}

bool GUIBridge::qmlCancelBooking(int bookingId) {
    auto u = getCurrentUser();
    if (!u || !bookingService) return false;
    std::string msg;
    bool res = bookingService->cancelCourseBooking(u->getId(), u->getRole(), bookingId, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    Q_EMIT studentWeeklyScheduleChanged();
    Q_EMIT tutorScheduleChanged();
    return res;
}

bool GUIBridge::qmlAcceptBooking(int bookingId, bool acknowledgeCommitment) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return false;
    std::string msg;
    std::string conflictDate;
    bool res = bookingService->acceptCourseBooking(u->getId(), bookingId, acknowledgeCommitment, msg, &conflictDate);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    Q_EMIT studentWeeklyScheduleChanged();
    Q_EMIT tutorScheduleChanged();
    return res;
}

QString GUIBridge::getCommitmentPolicyText() const {
    return QString::fromUtf8(COMMITMENT_POLICY_TEXT);
}

QString GUIBridge::getCommitmentVersion() const {
    return QString::fromLatin1(CURRENT_COMMITMENT_VERSION);
}

bool GUIBridge::qmlRejectBooking(int bookingId) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return false;
    std::string msg;
    bool res = bookingService->rejectCourseBooking(u->getId(), bookingId, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return res;
}

bool GUIBridge::terminateCourse(int bookingId) {
    auto u = getCurrentUser();
    if (!u || !bookingService) {
        statusMessageStr = "Vui lòng đăng nhập để thực hiện thao tác!";
        Q_EMIT statusMessageChanged();
        return false;
    }
    std::string msg;
    bool res = bookingService->completeCourseBooking(u->getId(), u->getRole(), bookingId, msg);
    statusMessageStr = msg;
    if (res) {
        Q_EMIT dataChanged();
        Q_EMIT studentWeeklyScheduleChanged();
        Q_EMIT tutorScheduleChanged();
        Q_EMIT studentProfileChanged();
        Q_EMIT tutorProfileChanged();
    }
    Q_EMIT statusMessageChanged();
    return res;
}

bool GUIBridge::qmlCompleteLesson(int bookingId) {
    return terminateCourse(bookingId);
}

bool GUIBridge::qmlSubmitReview(int bookingId, int ratingScore, const QString& comment) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT) return false;
    std::string msg;
    bool res = false;
    if (reviewService) {
        res = reviewService->submitReview(u->getId(), bookingId, ratingScore, comment.toStdString(), msg);
    } else if (bookingService) {
        res = bookingService->submitReview(u->getId(), bookingId, ratingScore, comment.toStdString(), msg);
    }
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return res;
}
#endif

bool GUIBridge::cancelBooking(int bookingId, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || !bookingService) return false;
    return bookingService->cancelCourseBooking(u->getId(), u->getRole(), bookingId, outMsg);
}

bool GUIBridge::acceptBooking(int bookingId, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return false;
    return bookingService->acceptCourseBooking(u->getId(), bookingId, outMsg);
}

bool GUIBridge::rejectBooking(int bookingId, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return false;
    return bookingService->rejectCourseBooking(u->getId(), bookingId, outMsg);
}

bool GUIBridge::submitReview(int bookingId, int ratingScore, const std::string& comment, std::string& outMsg) {
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::STUDENT) return false;
    if (reviewService) {
        return reviewService->submitReview(u->getId(), bookingId, ratingScore, comment, outMsg);
    }
    if (bookingService) {
        return bookingService->submitReview(u->getId(), bookingId, ratingScore, comment, outMsg);
    }
    return false;
}

std::vector<CourseBooking> GUIBridge::getStudentBookings() {
    auto u = getCurrentUser();
    if (!u || !bookingService) return {};
    return bookingService->getStudentCourseBookings(u->getId());
}

std::vector<CourseBooking> GUIBridge::getTutorBookings() {
    auto u = getCurrentUser();
    if (!u || !bookingService) return {};
    return bookingService->getTutorCourseBookings(u->getId());
}

std::vector<Review> GUIBridge::getTutorReviews(int tutorId) {
    if (reviewService) return reviewService->getReviewsByTutorId(tutorId);
    if (bookingService) return bookingService->getTutorReviews(tutorId);
    return {};
}

#ifdef QT_CORE_LIB
QVariantList GUIBridge::qmlGetStudentBookings() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || !bookingService) return list;

    bookingService->autoCompleteExpiredCourses();

    auto courseBookings = bookingService->getStudentCourseBookings(u->getId());
    for (const auto& cb : courseBookings) {
        QVariantMap map;
        map["id"] = cb.id;
        map["bookingId"] = cb.id;
        map["studentId"] = cb.studentId;
        map["tutorId"] = cb.tutorId;
        map["tutorName"] = QString::fromStdString(cb.tutorName);
        map["subject"] = QString::fromStdString(cb.subject);
        map["hourlyRate"] = cb.hourlyRate;
        map["status"] = QString::fromStdString(cb.status);
        map["createdAt"] = QString::fromStdString(cb.createdAt);
        map["isCourse"] = true;

        map["isMultiSlot"] = cb.isMultiSlot();
        map["weeklySchedule"] = QString::fromStdString(cb.weeklySchedule);
        map["earlyTerminationStatus"] = QString::fromStdString(cb.earlyTerminationStatus);
        map["earlyTerminationRequesterId"] = cb.earlyTerminationRequesterId;
        map["earlyTerminationReason"] = QString::fromStdString(cb.earlyTerminationReason);
        map["actualEndDate"] = QString::fromStdString(cb.actualEndDate);

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;

        map["dayIndex"] = dayIdx;
        map["slotIndex"] = slotIdx;
        map["dayName"] = QString::fromUtf8(DAY_NAMES[dayIdx]);
        map["timeRange"] = QString::fromUtf8(SLOT_TIMES[slotIdx]);
        if (cb.isMultiSlot()) {
            map["slotLabel"] = QString::fromStdString(cb.getWeeklyScheduleSummary());
        } else {
            map["slotLabel"] = QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]);
        }

        map["startDate"] = QString::fromStdString(cb.startDate);
        map["endDate"] = QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount));
        map["weeksCount"] = cb.weeksCount;
        map["totalSessions"] = cb.getTotalSessions();
        map["totalCost"] = cb.getTotalCost();
        map["dateRangeLabel"] = QString::fromUtf8("Từ ") + QString::fromStdString(cb.startDate) + QString::fromUtf8(" đến ") +
                                QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount)) +
                                QString::fromUtf8(" (") + QString::number(cb.weeksCount) + QString::fromUtf8(" tuần)");

        map["canCancel"] = (cb.status == "PENDING" || cb.status == "ACCEPTED");
        map["canRequestEarlyTermination"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus != "PENDING");
        map["canRespondEarlyTermination"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus == "PENDING" && cb.earlyTerminationRequesterId != u->getId());
        map["isPendingEarlyTerminationFromMe"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus == "PENDING" && cb.earlyTerminationRequesterId == u->getId());

        bool isReviewed = false;
        int reviewRating = 0;
        QString reviewComment = "";

        auto tutorReviews = reviewService ? reviewService->getReviewsByTutorId(cb.tutorId)
                                          : (bookingService ? bookingService->getTutorReviews(cb.tutorId) : std::vector<Review>{});
        for (const auto& r : tutorReviews) {
            if (r.bookingId == cb.id && r.studentId == u->getId()) {
                isReviewed = true;
                reviewRating = r.ratingScore;
                reviewComment = QString::fromStdString(r.comment);
                break;
            }
        }

        map["isReviewed"] = isReviewed;
        map["canReview"] = ((cb.status == "COMPLETED" || cb.status == "EARLY_TERMINATED") && !isReviewed);
        map["reviewRating"] = reviewRating;
        map["reviewComment"] = reviewComment;
        map["tutorInitials"] = (!cb.tutorName.empty()) ? QString::fromStdString(cb.tutorName).left(1).toUpper() : QString("G");

        QVariantList sessionList;
        if (cb.isMultiSlot()) {
            auto sDates = DateUtils::generateMultiSlotSessions(cb.startDate, cb.weeksCount, cb.getWeeklySlots());
            for (const auto& s : sDates) sessionList.append(QString::fromStdString(s.date));
        } else {
            auto sDates = DateUtils::generateSessionDates(cb.startDate, cb.weeksCount);
            for (const auto& d : sDates) sessionList.append(QString::fromStdString(d));
        }
        map["sessionDates"] = sessionList;

        list.append(map);
    }
    return list;
}

QVariantMap GUIBridge::qmlGetStudentBookingMetrics(int studentId) {
    QVariantMap map;
    if (studentId == 0) {
        auto u = getCurrentUser();
        if (u) studentId = u->getId();
    }
    if (!bookingService || studentId <= 0) return map;
    auto metrics = bookingService->getStudentBookingMetrics(studentId);
    map["acceptedCount"] = metrics.acceptedCount;
    map["pendingCount"] = metrics.pendingCount;
    map["historyCount"] = metrics.historyCount;
    map["weeklyCost"] = metrics.weeklyCost;
    return map;
}

QVariantList GUIBridge::qmlGetTutorBookings() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return list;

    bookingService->autoCompleteExpiredCourses();

    auto courseBookings = bookingService->getTutorCourseBookings(u->getId());
    for (const auto& cb : courseBookings) {
        QVariantMap map;
        map["id"] = cb.id;
        map["bookingId"] = cb.id;
        map["studentId"] = cb.studentId;
        map["tutorId"] = cb.tutorId;
        map["studentName"] = QString::fromStdString(cb.studentName);
        map["tutorName"] = QString::fromStdString(cb.tutorName);
        map["subject"] = QString::fromStdString(cb.subject);
        map["hourlyRate"] = cb.hourlyRate;
        map["totalCost"] = cb.getTotalCost();
        map["status"] = QString::fromStdString(cb.status);
        map["createdAt"] = QString::fromStdString(cb.createdAt);
        map["isCourse"] = true;

        map["isMultiSlot"] = cb.isMultiSlot();
        map["weeklySchedule"] = QString::fromStdString(cb.weeklySchedule);
        map["earlyTerminationStatus"] = QString::fromStdString(cb.earlyTerminationStatus);
        map["earlyTerminationRequesterId"] = cb.earlyTerminationRequesterId;
        map["earlyTerminationReason"] = QString::fromStdString(cb.earlyTerminationReason);
        map["actualEndDate"] = QString::fromStdString(cb.actualEndDate);

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;

        map["dayIndex"] = dayIdx;
        map["slotIndex"] = slotIdx;
        map["dayName"] = QString::fromUtf8(DAY_NAMES[dayIdx]);
        map["timeRange"] = QString::fromUtf8(SLOT_TIMES[slotIdx]);
        if (cb.isMultiSlot()) {
            map["slotLabel"] = QString::fromStdString(cb.getWeeklyScheduleSummary());
        } else {
            map["slotLabel"] = QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]);
        }

        map["startDate"] = QString::fromStdString(cb.startDate);
        map["endDate"] = QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount));
        map["weeksCount"] = cb.weeksCount;
        map["totalSessions"] = cb.getTotalSessions();
        map["dateRangeLabel"] = QString::fromUtf8("Từ ") + QString::fromStdString(cb.startDate) + QString::fromUtf8(" đến ") +
                                QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount)) +
                                QString::fromUtf8(" (") + QString::number(cb.weeksCount) + QString::fromUtf8(" tuần)");

        map["canCancel"] = (cb.status == "PENDING" || cb.status == "ACCEPTED");
        map["canRequestEarlyTermination"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus != "PENDING");
        map["canRespondEarlyTermination"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus == "PENDING" && cb.earlyTerminationRequesterId != u->getId());
        map["isPendingEarlyTerminationFromMe"] = (cb.status == "ACCEPTED" && cb.earlyTerminationStatus == "PENDING" && cb.earlyTerminationRequesterId == u->getId());
        map["studentInitials"] = (!cb.studentName.empty()) ? QString::fromStdString(cb.studentName).left(1).toUpper() : QString("H");

        QVariantList sessionList;
        if (cb.isMultiSlot()) {
            auto sDates = DateUtils::generateMultiSlotSessions(cb.startDate, cb.weeksCount, cb.getWeeklySlots());
            for (const auto& s : sDates) sessionList.append(QString::fromStdString(s.date));
        } else {
            auto sDates = DateUtils::generateSessionDates(cb.startDate, cb.weeksCount);
            for (const auto& d : sDates) sessionList.append(QString::fromStdString(d));
        }
        map["sessionDates"] = sessionList;

        list.append(map);
    }
    return list;
}

QVariantMap GUIBridge::qmlGetTutorOverview() {
    QVariantMap map;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return map;

    auto tutor = std::static_pointer_cast<Tutor>(u);

    std::unordered_set<int> activeStudents;
    int upcomingCount = 0;
    int pendingCount = 0;

    auto courseBookings = bookingService->getTutorCourseBookings(tutor->getId());
    for (const auto& cb : courseBookings) {
        if (cb.status == "ACCEPTED") {
            activeStudents.insert(cb.studentId);
            upcomingCount++;
        } else if (cb.status == "PENDING") {
            pendingCount++;
        }
    }

    map["totalStudents"] = (int)activeStudents.size();
    map["upcomingLessonsCount"] = upcomingCount;
    map["pendingBookingsCount"] = pendingCount;
    map["rating"] = tutor->getRating();
    map["totalReviews"] = tutor->getTotalReviews();
    double accumulatedIncome = bookingService->calculateTutorAccumulatedIncome(tutor->getId());
    if (std::abs(tutor->getTotalEarnings() - accumulatedIncome) > 0.01) {
        bookingService->reconcileTutorEarnings(tutor->getId());
        tutor->setTotalEarnings(accumulatedIncome);
    }
    map["totalEarnings"] = accumulatedIncome;
    map["hourlyRate"] = tutor->getHourlyRate();
    map["fullName"] = QString::fromStdString(tutor->getFullName());
    map["subjectSpecialty"] = QString::fromStdString(tutor->getSubjectSpecialty());
    map["id"] = tutor->getId();
    map["username"] = QString::fromStdString(tutor->getUsername());
    map["email"] = QString::fromStdString(tutor->getEmail());
    map["bio"] = QString::fromStdString(tutor->getBio());

    return map;
}

QVariantList GUIBridge::qmlGetTutorScheduleLessons() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return list;

    // 1. Course Bookings
    auto courseBookings = bookingService->getTutorCourseBookings(u->getId());
    for (const auto& cb : courseBookings) {
        if (cb.status != "ACCEPTED" && cb.status != "COMPLETED") continue;

        std::string sName = cb.studentName;
        if (sName.empty() && userService) {
            auto sUser = userService->getUserById(cb.studentId);
            if (sUser) sName = sUser->getFullName();
        }

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;

        QVariantMap item;
        item["id"] = cb.id;
        item["bookingId"] = cb.id;
        item["studentId"] = cb.studentId;
        item["studentName"] = QString::fromStdString(sName);
        item["subject"] = QString::fromStdString(cb.subject);
        item["hourlyRate"] = cb.hourlyRate;
        item["totalCost"] = cb.getTotalCost();
        item["status"] = QString::fromStdString(cb.status);
        item["dayIndex"] = dayIdx;
        item["slotIndex"] = slotIdx;
        item["dayName"] = QString::fromUtf8(DAY_NAMES[dayIdx]);
        item["timeRange"] = QString::fromUtf8(SLOT_TIMES[slotIdx]);
        QString schedLabel = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]));
        item["slotLabel"] = schedLabel + QString(" (Khóa %1 tuần, từ %2)").arg(cb.weeksCount).arg(QString::fromStdString(cb.startDate));
        item["studentInitials"] = (!sName.empty()) ? QString::fromStdString(sName).left(1).toUpper() : QString("H");
        item["isCourse"] = true;

        list.append(item);
    }
    return list;
}

QString GUIBridge::getWeekMonday(const QString& refDateStr) {
    std::string ref = refDateStr.trimmed().toStdString();
    return QString::fromStdString(DateUtils::getWeekMonday(ref));
}

QString GUIBridge::getWeekSunday(const QString& refDateStr) {
    std::string ref = refDateStr.trimmed().toStdString();
    return QString::fromStdString(DateUtils::getWeekSunday(ref));
}

QString GUIBridge::shiftWeek(const QString& currentMondayStr, int weekOffset) {
    std::string ref = currentMondayStr.trimmed().toStdString();
    return QString::fromStdString(DateUtils::shiftWeek(ref, weekOffset));
}

QString GUIBridge::formatWeekRangeDisplay(const QString& mondayStr) {
    std::string ref = mondayStr.trimmed().toStdString();
    std::string monday = DateUtils::isValidDateString(ref) ? DateUtils::getWeekMonday(ref) : DateUtils::getWeekMonday();
    std::string sunday = DateUtils::addDays(monday, 6);

    int y1 = 0, m1 = 0, d1 = 0;
    int y2 = 0, m2 = 0, d2 = 0;
    DateUtils::parseDate(monday, y1, m1, d1);
    DateUtils::parseDate(sunday, y2, m2, d2);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d - %02d/%02d/%04d", d1, m1, y1, d2, m2, y2);
    return QString::fromUtf8(buf);
}

QString GUIBridge::getCurrentFullDateString() {
    std::string today = DateUtils::getCurrentDateString();
    int y = 0, m = 0, d = 0;
    DateUtils::parseDate(today, y, m, d);
    int dow = DateUtils::dayOfWeek(today);
    const char* dowName = (dow >= 0 && dow < 7) ? DAY_NAMES[dow] : "";

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s, %02d/%02d/%04d", dowName, d, m, y);
    return QString::fromUtf8(buf);
}

QVariantList GUIBridge::getWeekDaysInfo(const QString& mondayStr) {
    QVariantList result;
    std::string ref = mondayStr.trimmed().toStdString();
    std::string monday = DateUtils::isValidDateString(ref) ? DateUtils::getWeekMonday(ref) : DateUtils::getWeekMonday();
    std::string today = DateUtils::getCurrentDateString();
    auto weekDays = DateUtils::getWeekDays(monday);

    for (int day = 0; day < 7; ++day) {
        std::string dStr = (day < static_cast<int>(weekDays.size())) ? weekDays[day] : "";
        bool isToday = (dStr == today);
        int y = 0, m = 0, d = 0;
        DateUtils::parseDate(dStr, y, m, d);

        char dispBuf[32];
        std::snprintf(dispBuf, sizeof(dispBuf), "%02d/%02d", d, m);

        QVariantMap dayMap;
        dayMap["dayIndex"] = day;
        dayMap["dayName"] = QString::fromUtf8(DAY_NAMES[day]);
        dayMap["date"] = QString::fromStdString(dStr);
        dayMap["displayDate"] = QString::fromUtf8(dispBuf);
        dayMap["dayNumber"] = d;
        dayMap["month"] = m;
        dayMap["year"] = y;
        dayMap["isToday"] = isToday;
        result.append(dayMap);
    }
    return result;
}

QVariantList GUIBridge::qmlGetTutorActiveStudents() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService || !userService) return list;

    auto courseBookings = bookingService->getTutorCourseBookings(u->getId());
    for (const auto& cb : courseBookings) {
        if (cb.status != "ACCEPTED") continue;

        QVariantMap item;
        item["id"] = cb.id;
        item["bookingId"] = cb.id;
        item["studentId"] = cb.studentId;
        item["studentName"] = QString::fromStdString(cb.studentName);
        item["subject"] = QString::fromStdString(cb.subject);
        item["hourlyRate"] = cb.hourlyRate;
        item["totalCost"] = cb.getTotalCost();
        item["status"] = QString::fromStdString(cb.status);
        item["earlyTerminationStatus"] = QString::fromStdString(cb.earlyTerminationStatus);
        item["statusString"] = QString::fromUtf8("ĐANG HỌC");
        item["studentInitials"] = (!cb.studentName.empty()) ? QString::fromStdString(cb.studentName).left(1).toUpper() : QString("H");

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;
        item["dayIndex"] = dayIdx;
        item["slotIndex"] = slotIdx;
        item["startDate"] = QString::fromStdString(cb.startDate);
        item["weeksCount"] = cb.weeksCount;
        QString schedLabel = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]));
        item["nextLessonSlot"] = schedLabel + QString(" (Khóa %1 tuần, từ %2)").arg(cb.weeksCount).arg(QString::fromStdString(cb.startDate));

        auto s = userService->getStudentById(cb.studentId);
        if (s) {
            item["email"] = QString::fromStdString(s->getEmail());
            item["gradeLevel"] = s->getGradeLevel();
        } else {
            item["email"] = "";
            item["gradeLevel"] = 10;
        }

        list.append(item);
    }
    return list;
}

QVariantList GUIBridge::qmlGetTutorCompletedStudents() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService || !userService) return list;

    auto courseBookings = bookingService->getTutorCourseBookings(u->getId());
    auto reviews = reviewService ? reviewService->getReviewsByTutorId(u->getId())
                                 : (bookingService ? bookingService->getTutorReviews(u->getId()) : std::vector<Review>{});

    for (const auto& cb : courseBookings) {
        if (cb.status != "COMPLETED") continue;

        QVariantMap item;
        item["id"] = cb.id;
        item["bookingId"] = cb.id;
        item["studentId"] = cb.studentId;
        item["studentName"] = QString::fromStdString(cb.studentName);
        item["subject"] = QString::fromStdString(cb.subject);
        item["hourlyRate"] = cb.hourlyRate;
        item["totalCost"] = cb.getTotalCost();
        item["status"] = QString::fromStdString(cb.status);
        item["statusString"] = QString::fromUtf8("ĐÃ HOÀN THÀNH");
        item["completedDate"] = QString::fromStdString(cb.createdAt);
        item["studentInitials"] = (!cb.studentName.empty()) ? QString::fromStdString(cb.studentName).left(1).toUpper() : QString("H");

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;
        item["dayIndex"] = dayIdx;
        item["slotIndex"] = slotIdx;
        item["startDate"] = QString::fromStdString(cb.startDate);
        item["weeksCount"] = cb.weeksCount;
        QString schedLabel = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]));
        item["nextLessonSlot"] = schedLabel + QString(" (Khóa %1 tuần, từ %2)").arg(cb.weeksCount).arg(QString::fromStdString(cb.startDate));

        bool isReviewed = false;
        int ratingScore = 0;
        std::string comment = "";
        for (const auto& r : reviews) {
            if (r.bookingId == cb.id) {
                isReviewed = true;
                ratingScore = r.ratingScore;
                comment = r.comment;
                break;
            }
        }
        item["isReviewed"] = isReviewed;
        item["ratingScore"] = ratingScore;
        item["comment"] = QString::fromStdString(comment);

        auto s = userService->getStudentById(cb.studentId);
        if (s) {
            item["email"] = QString::fromStdString(s->getEmail());
            item["gradeLevel"] = s->getGradeLevel();
        } else {
            item["email"] = "";
            item["gradeLevel"] = 10;
        }

        list.append(item);
    }
    return list;
}

QVariantList GUIBridge::qmlGetTutorStudents() {
    return qmlGetTutorActiveStudents();
}

QVariantMap GUIBridge::qmlGetTutorStatistics() {
    QVariantMap map;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR || !bookingService) return map;

    auto tutor = std::static_pointer_cast<Tutor>(u);
    auto courseBookings = bookingService->getTutorCourseBookings(tutor->getId());
    auto reviews = reviewService ? reviewService->getReviewsByTutorId(tutor->getId())
                                 : (bookingService ? bookingService->getTutorReviews(tutor->getId()) : std::vector<Review>{});

    std::unordered_set<int> activeStudents;
    int completedCount = 0;
    int upcomingCount = 0;
    int cancelledCount = 0;

    for (const auto& cb : courseBookings) {
        if (cb.status == "ACCEPTED") {
            activeStudents.insert(cb.studentId);
            upcomingCount++;
        } else if (cb.status == "COMPLETED") {
            completedCount++;
        } else if (cb.status == "CANCELLED" || cb.status == "REJECTED") {
            cancelledCount++;
        }
    }

    int star5 = 0, star4 = 0, star3 = 0, star2 = 0, star1 = 0;
    for (const auto& r : reviews) {
        if (r.ratingScore == 5) star5++;
        else if (r.ratingScore == 4) star4++;
        else if (r.ratingScore == 3) star3++;
        else if (r.ratingScore == 2) star2++;
        else if (r.ratingScore == 1) star1++;
    }

    map["totalStudents"] = (int)activeStudents.size();
    map["completedLessons"] = completedCount;
    map["upcomingLessons"] = upcomingCount;
    map["cancelledLessons"] = cancelledCount;
    map["totalEarnings"] = bookingService ? bookingService->calculateTutorAccumulatedIncome(tutor->getId()) : tutor->getTotalEarnings();
    map["averageRating"] = tutor->getRating();
    map["totalReviews"] = (int)reviews.size();
    map["star5"] = star5;
    map["star4"] = star4;
    map["star3"] = star3;
    map["star2"] = star2;
    map["star1"] = star1;

    return map;
}

QVariantList GUIBridge::qmlGetTutorReviews() {
    QVariantList list;
    auto u = getCurrentUser();
    if (!u || u->getRole() != UserRole::TUTOR) return list;

    auto reviews = reviewService ? reviewService->getReviewsByTutorId(u->getId())
                                 : (bookingService ? bookingService->getTutorReviews(u->getId()) : std::vector<Review>{});
    for (const auto& r : reviews) {
        QVariantMap map;
        map["id"] = r.id;
        map["reviewId"] = r.id;
        map["bookingId"] = r.bookingId;
        map["studentId"] = r.studentId;
        map["studentName"] = QString::fromStdString(r.studentName);
        map["ratingScore"] = r.ratingScore;
        map["comment"] = QString::fromStdString(r.comment);
        map["createdAt"] = QString::fromStdString(r.createdAt);
        map["studentInitials"] = (!r.studentName.empty()) ? QString::fromStdString(r.studentName).left(1).toUpper() : QString("H");
        list.append(map);
    }
    return list;
}
#endif

// ============================================================================
// Admin Platform Management (Delegates to AdminService - SRP)
// ============================================================================

AdminMetrics GUIBridge::getAdminMetrics() {
    if (!adminService) return AdminMetrics();
    return adminService->calculateMetrics();
}

std::vector<std::shared_ptr<User>> GUIBridge::getAllUsers() {
    if (!adminService) return {};
    return adminService->getAllUsers();
}

bool GUIBridge::toggleUserActiveStatus(int userId, std::string& outMsg) {
    if (!adminService) return false;
    return adminService->toggleUserLock(userId, outMsg);
}

bool GUIBridge::deleteViolatingReview(int reviewId, std::string& outMsg) {
    if (!adminService) return false;
    return adminService->deleteViolatingReview(reviewId, outMsg);
}

#ifdef QT_CORE_LIB
QVariantList GUIBridge::qmlGetAllUsers() {
    QVariantList list;
    if (!adminService) return list;
    auto users = adminService->getAllUsers();
    for (const auto& u : users) {
        QVariantMap map;
        map["id"] = u->getId();
        map["fullName"] = QString::fromStdString(u->getFullName());
        map["role"] = QString::fromStdString(u->getRoleString());
        map["isActive"] = u->getIsActive();

        list.append(map);
    }
    return list;
}

QVariantMap GUIBridge::qmlGetAdminMetrics() {
    AdminMetrics am = getAdminMetrics();
    QVariantMap map;
    map["totalTutors"] = am.totalTutors;
    map["totalStudents"] = am.totalStudents;
    map["totalBookings"] = am.totalBookings;
    map["completedBookings"] = am.completedBookings;
    map["activeMatches"] = am.activeMatches;
    return map;
}

QVariantMap GUIBridge::qmlGetAdminPlatformStats() {
    AdminMetrics am = getAdminMetrics();
    QVariantMap map;
    map["studentCount"] = am.totalStudents;
    map["tutorCount"] = am.totalTutors;
    map["activeMatches"] = am.activeMatches;
    map["completedMatches"] = am.completedBookings;
    map["completedBookings"] = am.completedBookings;
    map["totalBookings"] = am.totalBookings;
    return map;
}

QVariantList GUIBridge::qmlGetAdminUsers(const QString& filter, const QString& searchKeyword) {
    QVariantList list;
    if (!adminService) return list;

    auto users = adminService->getFilteredUsers(filter.toStdString(), searchKeyword.toStdString());
    for (const auto& u : users) {
        if (!u) continue;
        QVariantMap map;
        map["id"] = u->getId();
        map["userId"] = u->getId();
        map["username"] = QString::fromStdString(u->getUsername());
        map["fullName"] = QString::fromStdString(u->getFullName());
        map["email"] = QString::fromStdString(u->getEmail());
        map["role"] = QString::fromStdString(u->getRoleString());
        map["roleEnum"] = QString::fromStdString(userRoleToString(u->getRole()));
        map["roleKey"] = QString::fromStdString(userRoleToString(u->getRole()));
        map["isActive"] = u->getIsActive();
        map["isLocked"] = !u->getIsActive();
        map["statusText"] = u->getIsActive() ? QString::fromUtf8("HOẠT ĐỘNG") : QString::fromUtf8("BỊ KHÓA");

        QString subj = "";
        double rate = 0.0;
        double rating = 0.0;
        int totalRevs = 0;

        if (u->getRole() == UserRole::TUTOR) {
            auto t = std::static_pointer_cast<Tutor>(u);
            subj = QString::fromStdString(t->getSubjectSpecialty());
            rate = t->getHourlyRate();
            rating = t->getRating();
            totalRevs = t->getTotalReviews();
        } else if (u->getRole() == UserRole::STUDENT) {
            auto s = std::static_pointer_cast<Student>(u);
            subj = QString::fromUtf8("Lớp %1").arg(s->getGradeLevel());
            rate = 0.0;
        } else if (u->getRole() == UserRole::ADMIN) {
            subj = QString::fromUtf8("Quản trị hệ thống");
        }

        map["subject"] = subj;
        map["subjectOrField"] = subj;
        map["hourlyRate"] = rate;
        map["hourlyRateOrBudget"] = rate;
        map["rating"] = rating;
        map["totalReviews"] = totalRevs;
        map["initials"] = (!u->getFullName().empty()) ? QString::fromStdString(u->getFullName()).left(1).toUpper() : QString("U");

        list.append(map);
    }
    return list;
}

QVariantMap GUIBridge::qmlGetUserDetailsForAdmin(int userId) {
    QVariantMap result;
    if (!userService || !bookingService) {
        result["found"] = false;
        result["errorMessage"] = QString::fromUtf8("Lỗi kết nối cơ sở dữ liệu!");
        return result;
    }

    auto u = userService->getUserById(userId);
    if (!u) {
        result["found"] = false;
        result["errorMessage"] = QString::fromUtf8("Không tìm thấy thông tin người dùng với ID #%1!").arg(userId);
        return result;
    }

    result["found"] = true;
    result["id"] = u->getId();
    result["userId"] = u->getId();
    result["username"] = QString::fromStdString(u->getUsername());
    result["fullName"] = QString::fromStdString(u->getFullName());
    result["email"] = QString::fromStdString(u->getEmail());
    result["role"] = QString::fromStdString(u->getRoleString());
    result["roleEnum"] = QString::fromStdString(userRoleToString(u->getRole()));
    result["roleKey"] = QString::fromStdString(userRoleToString(u->getRole()));
    result["isActive"] = u->getIsActive();
    result["isLocked"] = !u->getIsActive();
    result["statusText"] = u->getIsActive() ? QString::fromUtf8("HOẠT ĐỘNG") : QString::fromUtf8("BỊ KHÓA");
    result["initials"] = (!u->getFullName().empty()) ? QString::fromStdString(u->getFullName()).left(1).toUpper() : QString("U");

    // Profile object specifically constructed for UserDetailModal.qml
    QVariantMap profile;
    profile["id"] = u->getId();
    profile["userId"] = u->getId();
    profile["username"] = QString::fromStdString(u->getUsername());
    profile["fullName"] = QString::fromStdString(u->getFullName());
    profile["email"] = QString::fromStdString(u->getEmail());
    profile["role"] = QString::fromStdString(u->getRoleString());
    profile["roleEnum"] = QString::fromStdString(userRoleToString(u->getRole()));
    profile["roleKey"] = QString::fromStdString(userRoleToString(u->getRole()));
    profile["isActive"] = u->getIsActive();
    profile["isLocked"] = !u->getIsActive();
    profile["statusText"] = u->getIsActive() ? QString::fromUtf8("HOẠT ĐỘNG") : QString::fromUtf8("BỊ KHÓA");
    profile["initials"] = (!u->getFullName().empty()) ? QString::fromStdString(u->getFullName()).left(1).toUpper() : QString("U");
    profile["createdAt"] = QString::fromUtf8("2026-09-01");

    uint64_t userMask = 0;
    if (u->getRole() == UserRole::STUDENT) {
        auto s = std::static_pointer_cast<Student>(u);
        profile["gradeLevel"] = s->getGradeLevel();
        profile["gradeDisplay"] = QString::number(s->getGradeLevel());
        profile["gradeText"] = QString::fromUtf8("Lớp %1").arg(s->getGradeLevel());
        userMask = s->getScheduleBitmask();

        result["gradeLevel"] = s->getGradeLevel();
        result["gradeDisplay"] = QString::number(s->getGradeLevel());
        result["gradeText"] = QString::fromUtf8("Lớp %1").arg(s->getGradeLevel());
    } else if (u->getRole() == UserRole::TUTOR) {
        auto t = std::static_pointer_cast<Tutor>(u);
        profile["bio"] = QString::fromStdString(t->getBio());
        profile["subjectSpecialty"] = QString::fromStdString(t->getSubjectSpecialty());
        profile["subject"] = QString::fromStdString(t->getSubjectSpecialty());
        profile["hourlyRate"] = t->getHourlyRate();
        profile["rating"] = t->getRating();
        profile["ratingFormatted"] = QString::number(t->getRating(), 'f', 1);
        profile["totalReviews"] = t->getTotalReviews();
        profile["totalEarnings"] = t->getTotalEarnings();
        userMask = t->getScheduleBitmask();

        result["bio"] = QString::fromStdString(t->getBio());
        result["subjectSpecialty"] = QString::fromStdString(t->getSubjectSpecialty());
        result["subject"] = QString::fromStdString(t->getSubjectSpecialty());
        result["hourlyRate"] = t->getHourlyRate();
        result["rating"] = t->getRating();
        result["ratingFormatted"] = QString::number(t->getRating(), 'f', 1);
        result["totalReviews"] = t->getTotalReviews();
        result["totalEarnings"] = t->getTotalEarnings();
    } else if (u->getRole() == UserRole::ADMIN) {
        // Unified root administrator - uses 7 base User profile attributes
    }

    result["profile"] = profile;

    // Schedule matrix
    QVariantList scheduleList;
    if (u->getRole() == UserRole::STUDENT || u->getRole() == UserRole::TUTOR) {
        auto courseBookings = (u->getRole() == UserRole::STUDENT)
                            ? bookingService->getStudentCourseBookings(userId)
                            : bookingService->getTutorCourseBookings(userId);

        for (int slot = 0; slot < 8; ++slot) {
            for (int day = 0; day < 7; ++day) {
                uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
                QVariantMap cell;
                cell["dayIndex"] = day;
                cell["slotIndex"] = slot;
                cell["dayName"] = QString::fromUtf8(DAY_NAMES[day]);
                cell["timeRange"] = QString::fromUtf8(SLOT_TIMES[slot]);

                bool isBooked = false;
                std::string partnerName = "";
                std::string subjName = "";
                int bookingId = 0;

                for (const auto& cb : courseBookings) {
                    if (cb.status == "ACCEPTED") {
                        auto ws = cb.getWeeklySlots();
                        for (const auto& p : ws) {
                            if (p.first == day && p.second == slot) {
                                isBooked = true;
                                subjName = cb.subject;
                                partnerName = (u->getRole() == UserRole::STUDENT) ? cb.tutorName : cb.studentName;
                                bookingId = cb.id;
                                break;
                            }
                        }
                        if (isBooked) break;
                    }
                }

                if (isBooked) {
                    cell["status"] = "BOOKED";
                    cell["statusLabel"] = QString::fromUtf8("ĐÃ ĐẶT");
                    cell["label"] = QString::fromStdString(subjName);
                    cell["subLabel"] = QString::fromStdString(partnerName);
                    cell["displayName"] = QString::fromStdString(subjName);
                    cell["subjectName"] = QString::fromStdString(subjName);
                    cell["tutorName"] = QString::fromStdString(partnerName);
                    cell["studentName"] = QString::fromStdString(partnerName);
                    cell["bookingId"] = bookingId;
                    cell["editable"] = false;
                    cell["locked"] = true;
                } else if ((userMask & sMask) != 0) {
                    cell["status"] = "FREE";
                    cell["statusLabel"] = QString::fromUtf8("RẢNH");
                    cell["label"] = QString::fromUtf8("RẢNH");
                    cell["subLabel"] = "";
                    cell["displayName"] = QString::fromUtf8("RẢNH");
                    cell["editable"] = false;
                    cell["locked"] = false;
                } else {
                    cell["status"] = "BUSY";
                    cell["statusLabel"] = QString::fromUtf8("BẬN");
                    cell["label"] = QString::fromUtf8("BẬN");
                    cell["subLabel"] = "";
                    cell["displayName"] = QString::fromUtf8("BẬN");
                    cell["editable"] = false;
                    cell["locked"] = true;
                }
                scheduleList.append(cell);
            }
        }
    }
    result["schedule"] = scheduleList;
    result["scheduleMatrix"] = scheduleList;

    // Connections
    QVariantList connList;
    auto userBookings = (u->getRole() == UserRole::STUDENT)
                            ? bookingService->getStudentCourseBookings(userId)
                            : bookingService->getTutorCourseBookings(userId);

    for (const auto& cb : userBookings) {
        QVariantMap bMap;
        bMap["id"] = cb.id;
        bMap["bookingId"] = cb.id;
        bMap["studentId"] = cb.studentId;
        bMap["tutorId"] = cb.tutorId;
        bMap["studentName"] = QString::fromStdString(cb.studentName);
        bMap["tutorName"] = QString::fromStdString(cb.tutorName);
        bMap["subject"] = QString::fromStdString(cb.subject);
        bMap["hourlyRate"] = cb.hourlyRate;
        bMap["totalCost"] = cb.getTotalCost();
        bMap["status"] = QString::fromStdString(cb.status);
        bMap["createdAt"] = QString::fromStdString(cb.createdAt);
        bMap["isCourse"] = true;
        bMap["startDate"] = QString::fromStdString(cb.startDate);
        bMap["weeksCount"] = cb.weeksCount;

        QString statusText = QString::fromStdString(cb.status);
        if (cb.status == "ACCEPTED") statusText = QString::fromUtf8("ĐANG HỌC");
        else if (cb.status == "PENDING") statusText = QString::fromUtf8("CHỜ DUYỆT");
        else if (cb.status == "COMPLETED") statusText = QString::fromUtf8("HOÀN THÀNH");
        else if (cb.status == "CANCELLED") statusText = QString::fromUtf8("ĐÃ HỦY");
        else if (cb.status == "REJECTED") statusText = QString::fromUtf8("TỪ CHỐI");
        bMap["statusText"] = statusText;

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;
        QString schedLabel = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]));
        QString slotStr = schedLabel + QString(" (Khóa %1 tuần, từ %2)").arg(cb.weeksCount).arg(QString::fromStdString(cb.startDate));
        bMap["slotLabel"] = slotStr;
        bMap["slotFormatted"] = slotStr;

        connList.append(bMap);
    }
    result["connections"] = connList;

    // Reviews
    QVariantList reviewList;
    if (u->getRole() == UserRole::TUTOR) {
        auto reviews = reviewService ? reviewService->getReviewsByTutorId(userId)
                                     : (bookingService ? bookingService->getTutorReviews(userId) : std::vector<Review>{});
        for (const auto& r : reviews) {
            QVariantMap rMap;
            rMap["id"] = r.id;
            rMap["reviewId"] = r.id;
            rMap["studentName"] = QString::fromStdString(r.studentName);
            rMap["ratingScore"] = r.ratingScore;
            rMap["comment"] = QString::fromStdString(r.comment);
            rMap["createdAt"] = QString::fromStdString(r.createdAt);
            reviewList.append(rMap);
        }
    } else if (u->getRole() == UserRole::STUDENT) {
        auto allReviews = reviewService ? reviewService->getAllReviews()
                                        : (bookingService ? bookingService->getAllReviews() : std::vector<Review>{});
        for (const auto& r : allReviews) {
            if (r.studentId == userId) {
                QVariantMap rMap;
                rMap["id"] = r.id;
                rMap["reviewId"] = r.id;
                rMap["studentName"] = QString::fromStdString(r.studentName);
                rMap["ratingScore"] = r.ratingScore;
                rMap["comment"] = QString::fromStdString(r.comment);
                rMap["createdAt"] = QString::fromStdString(r.createdAt);

                std::string tName = "Gia sư #" + std::to_string(r.tutorId);
                if (userService) {
                    auto tut = userService->getUserById(r.tutorId);
                    if (tut) {
                        tName = tut->getFullName();
                    }
                }
                rMap["tutorName"] = QString::fromStdString(tName);
                reviewList.append(rMap);
            }
        }
    }
    result["reviews"] = reviewList;

    return result;
}

bool GUIBridge::qmlToggleUserLock(int userId) {
    if (!adminService) return false;
    std::string msg;
    bool res = adminService->toggleUserLock(userId, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return res;
}

QVariantList GUIBridge::qmlGetRecentConnections(int limit) {
    QVariantList list;
    if (!bookingService) return list;

    auto courseBookings = bookingService->getAllCourseBookings();
    int count = 0;
    for (auto it = courseBookings.rbegin(); it != courseBookings.rend(); ++it) {
        if (count >= limit) break;
        const auto& cb = *it;

        QVariantMap map;
        map["id"] = cb.id;
        map["bookingId"] = cb.id;
        map["studentName"] = QString::fromStdString(cb.studentName);
        map["tutorName"] = QString::fromStdString(cb.tutorName);
        map["subject"] = QString::fromStdString(cb.subject);
        map["hourlyRate"] = cb.hourlyRate;
        map["totalCost"] = cb.getTotalCost();
        map["status"] = QString::fromStdString(cb.status);
        map["createdAt"] = QString::fromStdString(cb.createdAt);

        QString statusText = QString::fromStdString(cb.status);
        if (cb.status == "ACCEPTED") statusText = QString::fromUtf8("ĐANG HỌC");
        else if (cb.status == "PENDING") statusText = QString::fromUtf8("CHỜ DUYỆT");
        else if (cb.status == "COMPLETED") statusText = QString::fromUtf8("HOÀN THÀNH");
        else if (cb.status == "CANCELLED") statusText = QString::fromUtf8("ĐÃ HỦY");
        else if (cb.status == "REJECTED") statusText = QString::fromUtf8("TỪ CHỐI");
        map["statusText"] = statusText;

        auto ws = cb.getWeeklySlots();
        int dayIdx = ws.empty() ? 0 : ws[0].first;
        int slotIdx = ws.empty() ? 0 : ws[0].second;
        QString schedLabel = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dayIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[slotIdx]));
        QString slotStr = schedLabel + QString(" (Khóa %1 tuần, từ %2)").arg(cb.weeksCount).arg(QString::fromStdString(cb.startDate));
        map["slotLabel"] = slotStr;
        map["isCourse"] = true;

        list.append(map);
        count++;
    }
    return list;
}

QVariantList GUIBridge::qmlGetAllMatches() {
    QVariantList list;
    if (!bookingService) return list;

    auto courseBookings = bookingService->getAllCourseBookings();
    for (const auto& cb : courseBookings) {
        QVariantMap map;
        map["id"] = cb.id;
        map["bookingId"] = cb.id;
        map["studentId"] = cb.studentId;
        map["tutorId"] = cb.tutorId;
        map["studentName"] = QString::fromStdString(cb.studentName);
        map["tutorName"] = QString::fromStdString(cb.tutorName);
        map["subject"] = QString::fromStdString(cb.subject);
        map["hourlyRate"] = cb.hourlyRate;
        map["status"] = QString::fromStdString(cb.status);
        map["createdAt"] = QString::fromStdString(cb.createdAt);
        map["isCourse"] = true;
        map["startDate"] = QString::fromStdString(cb.startDate);
        map["endDate"] = QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount));
        map["weeksCount"] = cb.weeksCount;
        map["totalSessions"] = cb.getTotalSessions();
        map["totalCost"] = cb.getTotalCost();
        map["dateRangeLabel"] = QString::fromUtf8("Từ ") + QString::fromStdString(cb.startDate) + QString::fromUtf8(" đến ") +
                                QString::fromStdString(DateUtils::getEndDate(cb.startDate, cb.weeksCount)) +
                                QString::fromUtf8(" (") + QString::number(cb.weeksCount) + QString::fromUtf8(" tuần)");

        QString statusText = QString::fromStdString(cb.status);
        if (cb.status == "ACCEPTED") statusText = QString::fromUtf8("ĐANG HỌC");
        else if (cb.status == "PENDING") statusText = QString::fromUtf8("CHỜ DUYỆT");
        else if (cb.status == "COMPLETED") statusText = QString::fromUtf8("HOÀN THÀNH");
        else if (cb.status == "CANCELLED") statusText = QString::fromUtf8("ĐÃ HỦY");
        else if (cb.status == "REJECTED") statusText = QString::fromUtf8("TỪ CHỐI");
        map["statusText"] = statusText;

        auto wsCb = cb.getWeeklySlots();
        int dIdx = wsCb.empty() ? 0 : wsCb[0].first;
        int sIdx = wsCb.empty() ? 0 : wsCb[0].second;
        map["slotText"] = cb.isMultiSlot()
            ? QString::fromStdString(cb.getWeeklyScheduleSummary())
            : (QString::fromUtf8(DAY_NAMES[dIdx]) + QString::fromUtf8(" · ") + QString::fromUtf8(SLOT_TIMES[sIdx]));
        map["slotLabel"] = map["slotText"];
        map["canTerminate"] = (cb.status == "ACCEPTED");

        list.append(map);
    }
    return list;
}

bool GUIBridge::qmlAdminTerminateMatch(int bookingId) {
    if (!adminService) return false;
    std::string msg;
    bool res = false;
    if (bookingService && bookingService->findCourseBookingById(bookingId)) {
        res = bookingService->cancelCourseBooking(1, UserRole::ADMIN, bookingId, msg);
    } else {
        res = adminService->terminateMatch(bookingId, msg);
    }
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    Q_EMIT studentWeeklyScheduleChanged();
    Q_EMIT tutorScheduleChanged();
    return res;
}

QVariantList GUIBridge::qmlGetAllReviews() {
    QVariantList list;
    auto reviews = reviewService ? reviewService->getAllReviews() : std::vector<Review>{};
    for (const auto& r : reviews) {
        QVariantMap map;
        map["id"] = r.id;
        map["reviewId"] = r.id;
        map["bookingId"] = r.bookingId;
        map["studentId"] = r.studentId;
        map["tutorId"] = r.tutorId;
        map["studentName"] = QString::fromStdString(r.studentName);
        map["ratingScore"] = r.ratingScore;
        map["comment"] = QString::fromStdString(r.comment);
        map["createdAt"] = QString::fromStdString(r.createdAt);

        QString tName = QString("Gia sư #%1").arg(r.tutorId);
        if (userService) {
            auto u = userService->getUserById(r.tutorId);
            if (u) tName = QString::fromStdString(u->getFullName());
        }
        map["tutorName"] = tName;

        list.append(map);
    }
    return list;
}

bool GUIBridge::qmlDeleteReview(int reviewId) {
    if (!adminService) return false;
    std::string msg;
    bool res = adminService->deleteViolatingReview(reviewId, msg);
    statusMessageStr = msg;
    Q_EMIT statusMessageChanged();
    Q_EMIT dataChanged();
    return res;
}

bool GUIBridge::qmlToggleUserActiveStatus(int userId) {
    return qmlToggleUserLock(userId);
}
#endif

} // namespace TutorMatch
