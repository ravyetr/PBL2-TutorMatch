#ifndef TUTOR_MATCH_AUTH_SERVICE_H
#define TUTOR_MATCH_AUTH_SERVICE_H

#include "business/domain/User.h"
#include "data_access/repositories/IRepository.h"
#include <memory>
#include <string>

namespace TutorMatch {

/**
 * AuthService:
 * Implements Single Responsibility Principle (SRP) for user authentication,
 * session lifecycle, and account registration.
 *
 * Implements Dependency Inversion Principle (DIP) by depending on IUserRepository
 * rather than concrete database classes.
 */
class AuthService {
private:
    std::shared_ptr<IUserRepository> userRepo;
    std::shared_ptr<User> currentUser;
    bool loggedInState;

public:
    explicit AuthService(std::shared_ptr<IUserRepository> repository);
    virtual ~AuthService() = default;

    bool login(const std::string& username, const std::string& password, std::string& outError);
    void logout();

    bool isLoggedIn() const { return loggedInState; }
    std::shared_ptr<User> getCurrentUser() const { return currentUser; }
    UserRole getCurrentRole() const;

    bool registerStudent(const std::string& username, const std::string& password,
                         const std::string& fullName, const std::string& email,
                         const std::string& gradeLevel, std::string& outError);

    bool registerStudent(const std::string& username, const std::string& password,
                         const std::string& fullName, const std::string& email,
                         const std::string& gradeLevel, const std::string& preferredSubject,
                         double maxBudget, std::string& outError);

    bool registerTutor(const std::string& username, const std::string& password,
                       const std::string& fullName, const std::string& email,
                       const std::string& subjectSpecialty, double hourlyRate,
                       const std::string& bio, std::string& outError);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_AUTH_SERVICE_H
