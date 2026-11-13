#ifndef TUTOR_MATCH_FILE_COURSE_BOOKING_REPOSITORY_H
#define TUTOR_MATCH_FILE_COURSE_BOOKING_REPOSITORY_H

#include "data_access/repositories/IRepository.h"
#include "data_access/serializers/CourseBookingSerializer.h"
#include "business/domain/CourseBooking.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace TutorMatch {

/**
 * @brief Concrete File-based Repository for Recurring CourseBooking entities.
 *
 * Implements Data Access Layer for Recurring Course Bookings:
 * - Implements ICourseBookingRepository contract (ISP & DIP).
 * - Manages persistent storage in course_bookings.dat (CSV format).
 * - Delegates CSV formatting/parsing to CourseBookingSerializer (SRP).
 * - Thread-safe CRUD with recursive_mutex.
 * - Atomic persistence on update with temporary file replacement.
 */
class FileCourseBookingRepository : public ICourseBookingRepository {
private:
    std::string filePath;
    mutable std::recursive_mutex repoMutex;
    int nextId;

public:
    explicit FileCourseBookingRepository(const std::string& customFilePathOrDir = "");
    ~FileCourseBookingRepository() override = default;

    bool initialize();

    // ICourseBookingRepository Implementation
    int getNextCourseBookingId() override;
    bool saveCourseBooking(const CourseBooking& booking) override;
    bool updateCourseBooking(const CourseBooking& booking) override;
    std::shared_ptr<CourseBooking> findCourseBookingById(int id) override;
    std::vector<CourseBooking> getAllCourseBookings() override;
    std::vector<CourseBooking> getCourseBookingsByStudentId(int studentId) override;
    std::vector<CourseBooking> getCourseBookingsByTutorId(int tutorId) override;

    const std::string& getFilePath() const { return filePath; }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_FILE_COURSE_BOOKING_REPOSITORY_H
