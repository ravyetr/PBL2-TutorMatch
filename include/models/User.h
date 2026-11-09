#ifndef TUTOR_MATCH_USER_H
#define TUTOR_MATCH_USER_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "../ds/ScheduleGuard.h"

namespace TutorMatch {

enum class UserRole {
    STUDENT,
    TUTOR,
    ADMIN
};

inline std::string userRoleToString(UserRole role) {
    switch (role) {
        case UserRole::STUDENT: return "STUDENT";
        case UserRole::TUTOR:   return "TUTOR";
        case UserRole::ADMIN:   return "ADMIN";
        default: return "UNKNOWN";
    }
}

inline UserRole stringToUserRole(const std::string& str) {
    if (str == "STUDENT") return UserRole::STUDENT;
    if (str == "TUTOR")   return UserRole::TUTOR;
    if (str == "ADMIN")   return UserRole::ADMIN;
    return UserRole::STUDENT;
}

// ==========================================
// 1. ABSTRACT BASE CLASS: User (Encapsulation + Inheritance)
// ==========================================
class User {
protected:
    int id;
    std::string username;
    std::string passwordHash;
    std::string email;
    std::string fullName;
    UserRole role;
    bool isActive; // Soft delete flag for DB compaction / safe deletion

public:
    User() : id(0), role(UserRole::STUDENT), isActive(true) {}
    User(int id, std::string username, std::string passwordHash, std::string email, std::string fullName, UserRole role, bool isActive = true)
        : id(id), username(std::move(username)), passwordHash(std::move(passwordHash)),
          email(std::move(email)), fullName(std::move(fullName)), role(role), isActive(isActive) {}

    virtual ~User() = default;

    // Encapsulation - Getters & Setters
    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    const std::string& getUsername() const { return username; }
    void setUsername(const std::string& u) { username = u; }

    const std::string& getPasswordHash() const { return passwordHash; }
    void setPasswordHash(const std::string& p) { passwordHash = p; }

    const std::string& getEmail() const { return email; }
    void setEmail(const std::string& e) { email = e; }

    const std::string& getFullName() const { return fullName; }
    void setFullName(const std::string& name) { fullName = name; }

    UserRole getRole() const { return role; }

    bool getIsActive() const { return isActive; }
    void setIsActive(bool active) { isActive = active; }

    // Pure Virtual Functions for Polymorphism
    virtual std::string getRoleString() const = 0;
    virtual void displayProfile() const = 0;
};

// ==========================================
// 2. DERIVED CLASS: Student (Inheritance)
// ==========================================
class Student : public User {
private:
    int gradeLevel;
    std::string preferredSubject;
    double maxBudget;
    uint64_t scheduleBitmask;

public:
    Student() : User(0, "", "", "", "", UserRole::STUDENT), gradeLevel(10), maxBudget(0.0), scheduleBitmask(0) {}
    Student(int id, std::string username, std::string passwordHash, std::string email, std::string fullName,
            int gradeLevel, std::string preferredSubject, double maxBudget, uint64_t scheduleBitmask, bool isActive = true)
        : User(id, std::move(username), std::move(passwordHash), std::move(email), std::move(fullName), UserRole::STUDENT, isActive),
          gradeLevel(gradeLevel), preferredSubject(std::move(preferredSubject)), maxBudget(maxBudget),
          scheduleBitmask(ScheduleGuard::sanitizeMask(scheduleBitmask)) {}

    int getGradeLevel() const { return gradeLevel; }
    void setGradeLevel(int g) { gradeLevel = g; }

    const std::string& getPreferredSubject() const { return preferredSubject; }
    void setPreferredSubject(const std::string& subj) { preferredSubject = subj; }

    double getMaxBudget() const { return maxBudget; }
    void setMaxBudget(double b) { maxBudget = b; }

    uint64_t getScheduleBitmask() const { return scheduleBitmask; }
    void setScheduleBitmask(uint64_t mask) { scheduleBitmask = ScheduleGuard::sanitizeMask(mask); }

    std::string getRoleString() const override { return "Học viên"; }
    void displayProfile() const override;
};

// ==========================================
// 3. DERIVED CLASS: Tutor (Inheritance)
// ==========================================
class Tutor : public User {
private:
    std::string bio;
    std::string subjectSpecialty;
    double hourlyRate;
    double rating;
    int totalReviews;
    bool isVerified;
    double totalEarnings;
    uint64_t scheduleBitmask;

public:
    Tutor() : User(0, "", "", "", "", UserRole::TUTOR), hourlyRate(0.0), rating(5.0), totalReviews(0),
              isVerified(false), totalEarnings(0.0), scheduleBitmask(0) {}
    Tutor(int id, std::string username, std::string passwordHash, std::string email, std::string fullName,
          std::string bio, std::string subjectSpecialty, double hourlyRate, double rating, int totalReviews,
          bool isVerified, double totalEarnings, uint64_t scheduleBitmask, bool isActive = true)
        : User(id, std::move(username), std::move(passwordHash), std::move(email), std::move(fullName), UserRole::TUTOR, isActive),
          bio(std::move(bio)), subjectSpecialty(std::move(subjectSpecialty)), hourlyRate(hourlyRate),
          rating(rating), totalReviews(totalReviews), isVerified(isVerified), totalEarnings(totalEarnings),
          scheduleBitmask(ScheduleGuard::sanitizeMask(scheduleBitmask)) {}

    const std::string& getBio() const { return bio; }
    void setBio(const std::string& b) { bio = b; }

    const std::string& getSubjectSpecialty() const { return subjectSpecialty; }
    void setSubjectSpecialty(const std::string& s) { subjectSpecialty = s; }

    double getHourlyRate() const { return hourlyRate; }
    void setHourlyRate(double rate) { hourlyRate = rate; }

    double getRating() const { return rating; }
    void setRating(double r) { rating = r; }

    int getTotalReviews() const { return totalReviews; }
    void setTotalReviews(int cnt) { totalReviews = cnt; }

    bool getIsVerified() const { return isVerified; }
    void setIsVerified(bool v) { isVerified = v; }

    double getTotalEarnings() const { return totalEarnings; }
    void addEarnings(double amt) { totalEarnings += amt; }

    uint64_t getScheduleBitmask() const { return scheduleBitmask; }
    void setScheduleBitmask(uint64_t mask) { scheduleBitmask = ScheduleGuard::sanitizeMask(mask); }

    void addReviewRating(int newRatingScore);

    std::string getRoleString() const override { return "Gia sư"; }
    void displayProfile() const override;
};

// ==========================================
// 4. DERIVED CLASS: Admin (Inheritance)
// ==========================================
class Admin : public User {
private:
    int permissionLevel;

public:
    Admin() : User(0, "", "", "", "", UserRole::ADMIN), permissionLevel(1) {}
    Admin(int id, std::string username, std::string passwordHash, std::string email, std::string fullName, int permissionLevel, bool isActive = true)
        : User(id, std::move(username), std::move(passwordHash), std::move(email), std::move(fullName), UserRole::ADMIN, isActive),
          permissionLevel(permissionLevel) {}

    int getPermissionLevel() const { return permissionLevel; }
    void setPermissionLevel(int level) { permissionLevel = level; }

    std::string getRoleString() const override { return "Quản trị viên"; }
    void displayProfile() const override;
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_USER_H
