#ifndef TUTOR_MATCH_USER_SERIALIZER_H
#define TUTOR_MATCH_USER_SERIALIZER_H

#include "business/domain/User.h"
#include <string>
#include <memory>

namespace TutorMatch {

/**
 * @brief Dedicated Serializer for User domain entity hierarchy (Student, Tutor, Admin).
 *
 * Implements Phase 2 Step 2:
 * - Decouples serialization/deserialization from Domain Entities.
 * - Preserves 100% backward compatibility with legacy users.dat CSV format.
 * - Polymorphic deserialization factory returning std::shared_ptr<User>
 *   (instantiated as Student, Tutor, or Admin, preventing object slicing).
 * - Zero File I/O, zero caching, zero Qt dependencies.
 */
class UserSerializer {
public:
    // Serialization
    static std::string toCsv(const User& user);
    static std::string toCsv(const Student& student);
    static std::string toCsv(const Tutor& tutor);
    static std::string toCsv(const Admin& admin);

    // Polymorphic Deserialization
    // Returns std::shared_ptr<User> pointing to Student, Tutor, or Admin.
    // Returns nullptr if parsing fails or data is invalid.
    static std::shared_ptr<User> fromCsv(const std::string& csvLine, std::string* outError = nullptr);

    // Strongly-typed Deserialization helpers
    static std::shared_ptr<Student> studentFromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static std::shared_ptr<Tutor> tutorFromCsv(const std::string& csvLine, std::string* outError = nullptr);
    static std::shared_ptr<Admin> adminFromCsv(const std::string& csvLine, std::string* outError = nullptr);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_USER_SERIALIZER_H
