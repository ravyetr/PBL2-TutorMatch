#include "services/UserService.h"
#include <algorithm>
#include <cctype>

namespace TutorMatch {

UserService::UserService(std::shared_ptr<IUserRepository> repository)
    : userRepo(std::move(repository)) {}

std::shared_ptr<User> UserService::getUserById(int userId) {
    if (!userRepo) return nullptr;
    return userRepo->findUserById(userId);
}

std::shared_ptr<Student> UserService::getStudentById(int studentId) {
    auto u = getUserById(studentId);
    if (u && u->getRole() == UserRole::STUDENT) {
        return std::static_pointer_cast<Student>(u);
    }
    return nullptr;
}

std::shared_ptr<Tutor> UserService::getTutorById(int tutorId) {
    auto u = getUserById(tutorId);
    if (u && u->getRole() == UserRole::TUTOR) {
        return std::static_pointer_cast<Tutor>(u);
    }
    return nullptr;
}

bool UserService::updateStudentProfile(Student& student, const std::string& name, const std::string& email,
                                      int grade, const std::string& targetSubject, double budget, std::string& outError) {
    if (name.empty()) {
        outError = "Họ và tên không được để trống!";
        return false;
    }
    if (email.empty()) {
        outError = "Email không được để trống!";
        return false;
    }
    if (budget < 0.0) {
        outError = "Ngân sách không hợp lệ!";
        return false;
    }

    student.setFullName(name);
    student.setEmail(email);
    student.setGradeLevel(grade);
    student.setPreferredSubject(targetSubject);
    student.setMaxBudget(budget);

    if (!userRepo || !userRepo->updateUser(student)) {
        outError = "Lỗi lưu thông tin cập nhật vào tệp users.dat!";
        return false;
    }

    outError = "";
    return true;
}

bool UserService::toggleStudentScheduleSlot(Student& student, int day, int slot, std::string& outMsg) {
    if (!userRepo) {
        outMsg = "Database repository null!";
        return false;
    }

    uint64_t mask = student.getScheduleBitmask();
    uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
    bool isAvail = (mask & sMask) != 0;
    mask = ScheduleGuard::setSlot(mask, day, slot, !isAvail);

    student.setScheduleBitmask(mask);
    userRepo->updateUser(student);
    outMsg = "Đã đổi ca " + ScheduleGuard::formatSlotSingle(day, slot) + " thành " + (!isAvail ? "RẢNH" : "BẬN");
    return true;
}

bool UserService::resetStudentSchedule(Student& student, std::string& outMsg) {
    if (!userRepo) return false;
    student.setScheduleBitmask(ScheduleGuard::VALID_SCHEDULE_MASK);
    userRepo->updateUser(student);
    outMsg = "Đã đặt lại tất cả ca học thành RẢNH!";
    return true;
}

bool UserService::updateTutorProfile(Tutor& tutor, const std::string& name, const std::string& email,
                                    const std::string& bio, const std::string& subjectSpecialty,
                                    double hourlyRate, std::string& outError) {
    if (name.empty() || name.length() < 2) {
        outError = "Họ và tên gia sư phải có ít nhất 2 ký tự!";
        return false;
    }
    if (email.empty() || email.find('@') == std::string::npos || email.find('.') == std::string::npos) {
        outError = "Địa chỉ email không đúng định dạng!";
        return false;
    }
    if (subjectSpecialty.empty() || subjectSpecialty.length() < 2) {
        outError = "Môn chuyên môn giảng dạy phải có ít nhất 2 ký tự!";
        return false;
    }
    if (hourlyRate < 1000.0 || hourlyRate > 50000000.0) {
        outError = "Học phí đề xuất phải từ 1.000 đến 50.000.000 VNĐ / buổi!";
        return false;
    }
    if (bio.length() > 2000) {
        outError = "Giới thiệu bản thân không được vượt quá 2000 ký tự!";
        return false;
    }

    tutor.setFullName(name);
    tutor.setEmail(email);
    tutor.setBio(bio);
    tutor.setSubjectSpecialty(subjectSpecialty);
    tutor.setHourlyRate(hourlyRate);

    if (!userRepo || !userRepo->updateUser(tutor)) {
        outError = "Lỗi lưu cập nhật hồ sơ gia sư!";
        return false;
    }

    outError = "";
    return true;
}

bool UserService::toggleTutorScheduleSlot(Tutor& tutor, int day, int slot, std::string& outMsg) {
    if (!userRepo) return false;
    uint64_t mask = tutor.getScheduleBitmask();
    uint64_t sMask = ScheduleGuard::getSlotMask(day, slot);
    bool isAvail = (mask & sMask) != 0;
    mask = ScheduleGuard::setSlot(mask, day, slot, !isAvail);

    tutor.setScheduleBitmask(mask);
    userRepo->updateUser(tutor);
    outMsg = "Đã đổi ca " + ScheduleGuard::formatSlotSingle(day, slot) + " thành " + (!isAvail ? "RẢNH" : "BẬN");
    return true;
}

std::vector<std::shared_ptr<Tutor>> UserService::searchAndFilterTutors(const std::string& keyword,
                                                                      const std::string& subject,
                                                                      double minPrice, double maxPrice,
                                                                      double minRating) {
    std::vector<std::shared_ptr<Tutor>> result;
    if (!userRepo) return result;

    auto tutors = userRepo->getAllTutors();
    std::string kw = keyword;
    std::transform(kw.begin(), kw.end(), kw.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const auto& t : tutors) {
        if (!t || !t->getIsActive()) continue;

        std::string tName = t->getFullName();
        std::string tSubject = t->getSubjectSpecialty();
        std::string tBio = t->getBio();

        // Keyword filter
        if (!kw.empty()) {
            std::string lowerName = tName;
            std::string lowerSubj = tSubject;
            std::string lowerBio = tBio;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerSubj.begin(), lowerSubj.end(), lowerSubj.begin(), [](unsigned char c) { return std::tolower(c); });
            std::transform(lowerBio.begin(), lowerBio.end(), lowerBio.begin(), [](unsigned char c) { return std::tolower(c); });

            if (lowerName.find(kw) == std::string::npos &&
                lowerSubj.find(kw) == std::string::npos &&
                lowerBio.find(kw) == std::string::npos) {
                continue;
            }
        }

        // Subject filter using taxonomy
        if (!subject.empty()) {
            if (!SubjectTaxonomy::matchesFilter(tSubject, subject)) {
                continue;
            }
        }

        // Price filter
        if (minPrice > 0.0 && t->getHourlyRate() < minPrice) {
            continue;
        }
        if (maxPrice > 0.0 && t->getHourlyRate() > maxPrice) {
            continue;
        }

        // Rating filter
        if (minRating > 0.0 && t->getRating() < minRating) {
            continue;
        }

        result.push_back(t);
    }
    return result;
}

std::vector<std::shared_ptr<Tutor>> UserService::getAllActiveTutors() {
    std::vector<std::shared_ptr<Tutor>> res;
    if (!userRepo) return res;
    auto tutors = userRepo->getAllTutors();
    for (const auto& t : tutors) {
        if (t && t->getIsActive()) {
            res.push_back(t);
        }
    }
    return res;
}

std::vector<std::shared_ptr<Tutor>> UserService::getAllTutors() {
    if (!userRepo) return {};
    return userRepo->getAllTutors();
}

std::vector<std::shared_ptr<Student>> UserService::getAllStudents() {
    if (!userRepo) return {};
    return userRepo->getAllStudents();
}

std::vector<std::shared_ptr<User>> UserService::getAllUsers() {
    if (!userRepo) return {};
    return userRepo->getAllUsers();
}

std::vector<std::shared_ptr<Tutor>> UserService::filterTutorsByPrice(double minP, double maxP) {
    if (!userRepo) return {};
    auto ids = userRepo->findTutorIdsByPriceRange(minP, maxP);
    std::vector<std::shared_ptr<Tutor>> res;
    for (int id : ids) {
        auto u = userRepo->findUserById(id);
        if (u && u->getRole() == UserRole::TUTOR && u->getIsActive()) {
            res.push_back(std::static_pointer_cast<Tutor>(u));
        }
    }
    return res;
}

std::vector<std::shared_ptr<Tutor>> UserService::filterTutorsByRating(double minR) {
    if (!userRepo) return {};
    auto ids = userRepo->findTutorIdsByMinRating(minR);
    std::vector<std::shared_ptr<Tutor>> res;
    for (int id : ids) {
        auto u = userRepo->findUserById(id);
        if (u && u->getRole() == UserRole::TUTOR && u->getIsActive()) {
            res.push_back(std::static_pointer_cast<Tutor>(u));
        }
    }
    return res;
}

bool UserService::updateUser(const User& user) {
    if (!userRepo) return false;
    return userRepo->updateUser(user);
}

} // namespace TutorMatch
