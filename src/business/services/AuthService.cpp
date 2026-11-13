#include "business/services/AuthService.h"
#include <stdexcept>

namespace TutorMatch {

AuthService::AuthService(std::shared_ptr<IUserRepository> repository)
    : userRepo(std::move(repository)), currentUser(nullptr), loggedInState(false) {}

UserRole AuthService::getCurrentRole() const {
    if (currentUser) {
        return currentUser->getRole();
    }
    return UserRole::STUDENT;
}

bool AuthService::login(const std::string& username, const std::string& password, std::string& outError) {
    if (username.empty() || password.empty()) {
        outError = "Tên đăng nhập và mật khẩu không được để trống!";
        return false;
    }

    if (!userRepo) {
        outError = "Cơ sở dữ liệu người dùng chưa được khởi tạo!";
        return false;
    }

    auto user = userRepo->findUserByUsername(username);
    if (!user) {
        outError = "Tên đăng nhập không tồn tại!";
        return false;
    }

    if (user->getPasswordHash() != password) {
        outError = "Mật khẩu không chính xác!";
        return false;
    }

    if (!user->getIsActive()) {
        outError = "Tài khoản của bạn đang BỊ KHÓA bởi Admin!";
        return false;
    }

    currentUser = user;
    loggedInState = true;
    outError = "";
    return true;
}

void AuthService::logout() {
    currentUser = nullptr;
    loggedInState = false;
}

bool AuthService::registerStudent(const std::string& username, const std::string& password,
                                  const std::string& fullName, const std::string& email,
                                  const std::string& gradeLevel, std::string& outError) {
    if (username.empty() || password.empty() || fullName.empty() || email.empty()) {
        outError = "Vui lòng điền đầy đủ các thông tin bắt buộc!";
        return false;
    }

    if (!userRepo) {
        outError = "Database repository null!";
        return false;
    }
    if (userRepo->findUserByUsername(username) != nullptr) {
        outError = "Tên đăng nhập đã tồn tại trong hệ thống!";
        return false;
    }

    int gLevel = 10;
    try {
        gLevel = std::stoi(gradeLevel);
    } catch (...) {
        gLevel = 10;
    }

    int newId = userRepo->getNextUserId();
    Student s(newId, username, password, email, fullName, gLevel, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    if (userRepo->saveUser(s)) {
        outError = "";
        return true;
    }
    outError = "Lỗi ghi dữ liệu tài khoản mới!";
    return false;
}

bool AuthService::registerStudent(const std::string& username, const std::string& password,
                                  const std::string& fullName, const std::string& email,
                                  const std::string& gradeLevel, const std::string& /*preferredSubject*/,
                                  double maxBudget, std::string& outError) {
    if (maxBudget < 0.0) {
        outError = "Ngân sách không thể là số âm!";
        return false;
    }
    return registerStudent(username, password, fullName, email, gradeLevel, outError);
}

bool AuthService::registerTutor(const std::string& username, const std::string& password,
                                const std::string& fullName, const std::string& email,
                                const std::string& subjectSpecialty, double hourlyRate,
                                const std::string& bio, std::string& outError) {
    if (username.empty() || password.empty() || fullName.empty() || email.empty() || subjectSpecialty.empty()) {
        outError = "Vui lòng điền đầy đủ các thông tin bắt buộc!";
        return false;
    }
    if (hourlyRate <= 0.0) {
        outError = "Học phí theo giờ phải lớn hơn 0!";
        return false;
    }

    if (!userRepo) {
        outError = "Database repository null!";
        return false;
    }
    if (userRepo->findUserByUsername(username) != nullptr) {
        outError = "Tên đăng nhập đã tồn tại trong hệ thống!";
        return false;
    }

    int newId = userRepo->getNextUserId();
    Tutor t(newId, username, password, email, fullName, bio, subjectSpecialty, hourlyRate, 5.0, 0, 0.0, ScheduleGuard::VALID_SCHEDULE_MASK, true);
    if (userRepo->saveUser(t)) {
        outError = "";
        return true;
    }
    outError = "Lỗi ghi dữ liệu gia sư mới!";
    return false;
}

} // namespace TutorMatch
