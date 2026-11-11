#ifndef TUTOR_MATCH_GUI_BRIDGE_H
#define TUTOR_MATCH_GUI_BRIDGE_H

#include "models/User.h"
#include "models/CourseBooking.h"
#include "models/Review.h"
#include "storage/IRepository.h"
#include "storage/MockDatabaseEngine.h"
#include "algorithms/MatchingEngine.h"
#include "ds/ScheduleGuard.h"
#include "models/BookingDTOs.h"
#include "services/AuthService.h"
#include "services/UserService.h"
#include "services/BookingService.h"
#include "services/ReviewService.h"
#include "services/MatchingService.h"
#include "services/AdminService.h"

#include <memory>
#include <string>
#include <vector>

#ifdef QT_CORE_LIB
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#endif

namespace TutorMatch {

#ifdef QT_CORE_LIB
class GUIBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY stateChanged)
    Q_PROPERTY(QString currentUserFullName READ getCurrentUserFullName NOTIFY stateChanged)
    Q_PROPERTY(QString currentUserRole READ getCurrentUserRole NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ getStatusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QVariantMap currentStudentProfile READ getCurrentStudentProfile NOTIFY studentProfileChanged)
    Q_PROPERTY(QVariantMap currentTutorProfile READ getCurrentTutorProfile NOTIFY tutorProfileChanged)
#else
class GUIBridge {
#endif

private:
    std::shared_ptr<TutorMatch::AuthService> authService;
    std::shared_ptr<TutorMatch::UserService> userService;
    std::shared_ptr<TutorMatch::BookingService> bookingService;
    std::shared_ptr<TutorMatch::ReviewService> reviewService;
    std::shared_ptr<TutorMatch::MatchingService> matchingService;
    std::shared_ptr<TutorMatch::AdminService> adminService;

    std::string statusMessageStr;

public:
#ifdef QT_CORE_LIB
    explicit GUIBridge(QObject *parent = nullptr);
#else
    GUIBridge();
#endif
    ~GUIBridge();

    bool initialize(std::shared_ptr<TutorMatch::AuthService> aService,
                    std::shared_ptr<TutorMatch::UserService> uService,
                    std::shared_ptr<TutorMatch::BookingService> bService,
                    std::shared_ptr<TutorMatch::ReviewService> rService,
                    std::shared_ptr<TutorMatch::MatchingService> mService,
                    std::shared_ptr<TutorMatch::AdminService> admService);

    bool initialize(std::shared_ptr<TutorMatch::IUserRepository> uRepo,
                    std::shared_ptr<TutorMatch::IReviewRepository> rRepo,
                    std::shared_ptr<TutorMatch::ICourseBookingRepository> cbRepo);
    bool initialize(std::shared_ptr<TutorMatch::MockDatabaseEngine> engine);

    // Authentication
    bool login(const std::string& username, const std::string& password, std::string& outError);
    void logout();
    bool isLoggedIn() const;
    std::shared_ptr<TutorMatch::User> getCurrentUser() const;
    TutorMatch::UserRole getCurrentRole() const;

#ifdef QT_CORE_LIB
    // Qt Helper Getters
    QString getCurrentUserFullName() const;
    QString getCurrentUserRole() const;
    QString getStatusMessage() const;
    QVariantMap getCurrentStudentProfile() const;
    QVariantMap getCurrentTutorProfile() const;

    // Notification State API
    Q_INVOKABLE void clearStatusMessage();

    // Q_INVOKABLE Methods for QML Binding
    Q_INVOKABLE bool qmlLogin(const QString& username, const QString& password);
    Q_INVOKABLE void qmlLogout();
    Q_INVOKABLE bool qmlRegisterStudent(const QString& username, const QString& password,
                                       const QString& fullName, const QString& email,
                                       int gradeLevel, const QString& preferredSubject = "", double maxBudget = 0.0);
    Q_INVOKABLE bool qmlRegisterTutor(const QString& username, const QString& password,
                                     const QString& fullName, const QString& email,
                                     const QString& subjectSpecialty, double hourlyRate, const QString& bio);

    Q_INVOKABLE QVariantList qmlGetAllTutors();
    Q_INVOKABLE QVariantList qmlGetSmartMatches(int topK = 3);
    Q_INVOKABLE QVariantList qmlSearchSmartMatches(const QVariantMap& criteriaMap, int topK = 5);
    Q_INVOKABLE QVariantList qmlGetStudentBookings();
    Q_INVOKABLE QVariantMap qmlGetStudentBookingMetrics(int studentId = 0);
    Q_INVOKABLE QVariantList qmlGetTutorBookings();
    Q_INVOKABLE QVariantList qmlGetAllUsers();
    Q_INVOKABLE QVariantMap qmlGetAdminMetrics();
    Q_INVOKABLE QVariantMap qmlGetAdminPlatformStats();
    Q_INVOKABLE QVariantList qmlGetAdminUsers(const QString& filter = "ALL", const QString& searchKeyword = "");
    Q_INVOKABLE QVariantMap qmlGetUserDetailsForAdmin(int userId);
    Q_INVOKABLE bool qmlToggleUserLock(int userId);
    Q_INVOKABLE QVariantList qmlGetRecentConnections(int limit = 5);
    Q_INVOKABLE QVariantList qmlGetAllMatches();
    Q_INVOKABLE bool qmlAdminTerminateMatch(int bookingId);
    Q_INVOKABLE QVariantList qmlGetAllReviews();
    Q_INVOKABLE bool qmlDeleteReview(int reviewId);

    Q_INVOKABLE bool qmlToggleUserActiveStatus(int userId);
    Q_INVOKABLE bool qmlToggleStudentScheduleSlot(int day, int slot);
    Q_INVOKABLE bool qmlToggleTutorScheduleSlot(int day, int slot);
    Q_INVOKABLE bool qmlBookLesson(int tutorId, int day, int slot);
    Q_INVOKABLE bool qmlCancelBooking(int bookingId);
    Q_INVOKABLE bool qmlAcceptBooking(int bookingId, bool acknowledgeCommitment = true);
    Q_INVOKABLE bool qmlRejectBooking(int bookingId);
    Q_INVOKABLE bool qmlCompleteLesson(int bookingId);
    Q_INVOKABLE bool terminateCourse(int bookingId);
    Q_INVOKABLE bool qmlSubmitReview(int bookingId, int ratingScore, const QString& comment);
    Q_INVOKABLE QString getCommitmentPolicyText() const;
    Q_INVOKABLE QString getCommitmentVersion() const;

    // Tab 0 Student Profile & Timetable APIs
    Q_INVOKABLE bool updateStudentProfile(const QString& name, const QString& email, int grade, const QString& targetSubject = "", double budget = 0.0);
    Q_INVOKABLE QVariantList getStudentWeeklySchedule(const QString& weekStartDateStr = "");
    Q_INVOKABLE QVariantList getWeeklyScheduleMatrix(const QString& weekStartDateStr = "");
    Q_INVOKABLE bool clearStudentBusySchedule();
    Q_INVOKABLE bool clearAllBusySlots();

    // Tutor Profile, Schedule & Dashboard APIs
    Q_INVOKABLE bool updateTutorProfile(const QString& name, const QString& email, const QString& bio, const QString& subjectSpecialty, double hourlyRate);
    Q_INVOKABLE bool updateTutorProfile(const QString& title, const QString& name, const QString& email, const QString& bio, const QString& subjectSpecialty, double hourlyRate);
    Q_INVOKABLE QVariantList getTutorWeeklySchedule(const QString& weekStartDateStr = "");
    Q_INVOKABLE QVariantMap qmlGetTutorOverview();
    Q_INVOKABLE QVariantList qmlGetTutorScheduleLessons();
    Q_INVOKABLE QVariantList qmlGetTutorStudents();
    Q_INVOKABLE QVariantList qmlGetTutorActiveStudents();
    Q_INVOKABLE QVariantList qmlGetTutorCompletedStudents();
    Q_INVOKABLE QVariantMap qmlGetTutorStatistics();
    Q_INVOKABLE QVariantList qmlGetTutorReviews();

    // Calendar Week Navigation & Date Display APIs
    Q_INVOKABLE QString getWeekMonday(const QString& refDateStr = "");
    Q_INVOKABLE QString getWeekSunday(const QString& refDateStr = "");
    Q_INVOKABLE QString shiftWeek(const QString& currentMondayStr, int weekOffset);
    Q_INVOKABLE QString formatWeekRangeDisplay(const QString& mondayStr);
    Q_INVOKABLE QString getCurrentFullDateString();
    Q_INVOKABLE QVariantList getWeekDaysInfo(const QString& mondayStr);

    // Tab 1 Student Marketplace & Booking APIs
    Q_INVOKABLE QVariantList searchAndFilterTutors(const QString& keyword, const QString& subject, double minPrice, double maxPrice, double minRating);
    Q_INVOKABLE QVariantMap getTutorDetail(int tutorId);
    Q_INVOKABLE QVariantList getTutorAvailableSlots(int tutorId);
    Q_INVOKABLE QVariantList getTutorAvailableBookingSlots(int tutorId, int studentId = 0);
    Q_INVOKABLE bool requestCourseBooking(int tutorId, const QString& subject, int dayIndex, int slotIndex, bool acknowledgeCommitment = true);
    Q_INVOKABLE bool requestRecurringCourseBooking(int tutorId, const QString& subject, int dayIndex, int slotIndex, const QString& startDate, int weeksCount, bool acknowledgeCommitment = true);
    Q_INVOKABLE bool requestCourseBookingMulti(int tutorId, const QString& subject, const QVariantList& chosenSlots, const QString& startDate, int weeksCount, bool acknowledgeCommitment = false);
    Q_INVOKABLE bool requestEarlyTermination(int courseBookingId, const QString& reason);
    Q_INVOKABLE bool respondEarlyTermination(int courseBookingId, bool agree);
    Q_INVOKABLE QVariantList calculateMultiSlotSessionDates(const QString& startDate, int weeksCount, const QVariantList& chosenSlots);
    Q_INVOKABLE QVariantList getUpcomingStartDatesForDay(int targetDayOfWeek, int count = 4);
    Q_INVOKABLE QVariantList calculateCourseSessionDates(const QString& startDate, int weeksCount);
    Q_INVOKABLE int getDayOfWeekFromDate(const QString& dateStr);
    Q_INVOKABLE QString getDayNameFromDate(const QString& dateStr);
    Q_INVOKABLE QString getTodayDateString();
    Q_INVOKABLE bool isValidDateString(const QString& dateStr);

Q_SIGNALS:
    void stateChanged();
    void statusMessageChanged();
    void dataChanged();
    void studentProfileChanged();
    void studentWeeklyScheduleChanged();
    void tutorProfileChanged();
    void tutorScheduleChanged();
#endif

public:
    // Registration
    bool registerStudent(const std::string& username, const std::string& password, const std::string& fullName,
                         const std::string& email, const std::string& gradeLevel, const std::string& preferredSubject,
                         double maxBudget, std::string& outError);

    bool registerTutor(const std::string& username, const std::string& password, const std::string& fullName,
                       const std::string& email, const std::string& subjectSpecialty, double hourlyRate,
                       const std::string& bio, std::string& outError);

    // Student Actions
    std::vector<std::shared_ptr<TutorMatch::Tutor>> getAllTutors();
    std::vector<std::shared_ptr<TutorMatch::Tutor>> filterTutorsByPrice(double minP, double maxP);
    std::vector<std::shared_ptr<TutorMatch::Tutor>> filterTutorsByRating(double minR);
    std::vector<TutorMatch::TutorMatchResult> getSmartMatches(int topK = 3);
    
    bool toggleStudentScheduleSlot(int day, int slot, std::string& outMsg);
    bool resetStudentSchedule(std::string& outMsg);
    
    bool bookLesson(int tutorId, int day, int slot, std::string& outMsg);
    bool cancelBooking(int bookingId, std::string& outMsg);
    bool submitReview(int bookingId, int ratingScore, const std::string& comment, std::string& outMsg);

    std::vector<TutorMatch::CourseBooking> getStudentBookings();

    // Tutor Actions
    bool toggleTutorScheduleSlot(int day, int slot, std::string& outMsg);
    bool acceptBooking(int bookingId, std::string& outMsg);
    bool rejectBooking(int bookingId, std::string& outMsg);

    std::vector<TutorMatch::CourseBooking> getTutorBookings();
    std::vector<TutorMatch::Review> getTutorReviews(int tutorId);

    // Admin Actions
    TutorMatch::AdminMetrics getAdminMetrics();
    std::vector<std::shared_ptr<TutorMatch::User>> getAllUsers();
    bool toggleUserActiveStatus(int userId, std::string& outMsg);
    bool deleteViolatingReview(int reviewId, std::string& outMsg);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_GUI_BRIDGE_H
