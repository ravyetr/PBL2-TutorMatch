#ifndef TUTOR_MATCH_USER_SERVICE_H
#define TUTOR_MATCH_USER_SERVICE_H

#include "../models/User.h"
#include "../storage/IRepository.h"
#include "../ds/ScheduleGuard.h"
#include "../models/SubjectTaxonomy.h"
#include <memory>
#include <string>
#include <vector>

namespace TutorMatch {

/**
 * UserService:
 * Handles user profiles, timetable slots, and tutor searching.
 * Adheres to SRP by encapsulating user-specific business logic.
 * Adheres to DIP by interacting with IUserRepository.
 */
class UserService {
private:
    std::shared_ptr<IUserRepository> userRepo;

public:
    explicit UserService(std::shared_ptr<IUserRepository> repository);
    virtual ~UserService() = default;

    std::shared_ptr<User> getUserById(int userId);
    std::shared_ptr<Student> getStudentById(int studentId);
    std::shared_ptr<Tutor> getTutorById(int tutorId);

    bool updateStudentProfile(Student& student, const std::string& name, const std::string& email,
                              int grade, const std::string& targetSubject, double budget, std::string& outError);

    bool toggleStudentScheduleSlot(Student& student, int day, int slot, std::string& outMsg);
    bool resetStudentSchedule(Student& student, std::string& outMsg);

    bool updateTutorProfile(Tutor& tutor, const std::string& name, const std::string& email,
                            const std::string& bio, const std::string& subjectSpecialty,
                            double hourlyRate, std::string& outError);

    bool toggleTutorScheduleSlot(Tutor& tutor, int day, int slot, std::string& outMsg);

    std::vector<std::shared_ptr<Tutor>> searchAndFilterTutors(const std::string& keyword,
                                                              const std::string& subject,
                                                              double minPrice, double maxPrice,
                                                              double minRating);

    std::vector<std::shared_ptr<Tutor>> getAllActiveTutors();
    std::vector<std::shared_ptr<Tutor>> getAllTutors();
    std::vector<std::shared_ptr<Student>> getAllStudents();
    std::vector<std::shared_ptr<User>> getAllUsers();
    std::vector<std::shared_ptr<Tutor>> filterTutorsByPrice(double minP, double maxP);
    std::vector<std::shared_ptr<Tutor>> filterTutorsByRating(double minR);
    bool updateUser(const User& user);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_USER_SERVICE_H
