#ifndef TUTOR_MATCH_CONSOLE_APP_H
#define TUTOR_MATCH_CONSOLE_APP_H

#include "../storage/MockDatabaseEngine.h"
#include "../algorithms/MatchingEngine.h"
#include "../models/User.h"
#include "../models/CourseBooking.h"
#include "../models/Review.h"
#include "../utils/DateUtils.h"

#include "../services/AuthService.h"
#include "../services/UserService.h"
#include "../services/BookingService.h"
#include "../services/ReviewService.h"
#include "../services/MatchingService.h"
#include "../services/AdminService.h"

#include <memory>
#include <string>
#include <vector>

namespace TutorMatch {

class ConsoleApp {
private:
    std::shared_ptr<AuthService> authService;
    std::shared_ptr<UserService> userService;
    std::shared_ptr<BookingService> bookingService;
    std::shared_ptr<ReviewService> reviewService;
    std::shared_ptr<MatchingService> matchingService;
    std::shared_ptr<AdminService> adminService;
    std::shared_ptr<MockDatabaseEngine> dbEngine;
    MatchingEngine matchingEngine;

    // Active Session State
    std::shared_ptr<User> currentUser;
    bool isLoggedIn;

    // Helper formatting & validation methods
    void printHeader(const std::string& title) const;
    void printDivider() const;
    void printSuccess(const std::string& msg) const;
    void printError(const std::string& msg) const;
    void printWarning(const std::string& msg) const;
    void printInfo(const std::string& msg) const;

    bool isValidEmail(const std::string& email) const;
    void renderTimetableGrid(int userId, UserRole role) const;
    void renderStudentOverview() const;
    void renderTutorOverview() const;
    void renderTutorTable(const std::vector<std::shared_ptr<Tutor>>& tutors) const;

    int readIntInput(const std::string& prompt);
    int getValidIntChoice(int minVal, int maxVal, const std::string& prompt);
    double readDoubleInput(const std::string& prompt);
    std::string readStringInput(const std::string& prompt);

    // Auth Flow
    void handleLogin();
    void handleRegister();
    void handleBenchmarkStatus();
    void handleChangePassword();

    // Student Flow
    void renderStudentMenu();
    void handleStudentProfileUpdate();
    void handleTutorDiscoveryAVL();
    void handleSmartMatchingMaxHeap();
    void handleBookLessonScheduleGuard();
    void handleStudentScheduleMatrixEditor();
    void handleStudentBookings();
    void handleLeaveReviewAndRating();

    // Tutor Flow
    void renderTutorMenu();
    void handleTutorProfileUpdate();
    void handleTutorScheduleMatrixEditor();
    void handleTutorPendingBookings();
    void handleTutorEarningsStats();

    // Admin Flow
    void renderAdminMenu();
    void handleAdminSystemMetrics();
    void handleAdminUserManagement();
    void handleAdminDBStorageStatus();

public:
    ConsoleApp();
    ~ConsoleApp();

    bool initialize(std::shared_ptr<AuthService> auth,
                    std::shared_ptr<UserService> user,
                    std::shared_ptr<BookingService> booking,
                    std::shared_ptr<ReviewService> review,
                    std::shared_ptr<MatchingService> matching,
                    std::shared_ptr<AdminService> admin,
                    std::shared_ptr<MockDatabaseEngine> db = nullptr);
    bool initialize(std::shared_ptr<MockDatabaseEngine> engine);
    void run();
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_CONSOLE_APP_H
