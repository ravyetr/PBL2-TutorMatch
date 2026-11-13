#include "business/domain/User.h"
#include "business/domain/Review.h"
#include "business/algorithms/ScheduleGuard.h"

#include <iostream>
#include <iomanip>

namespace TutorMatch {

// Student Derived Class
void Student::displayProfile() const {
    std::cout << "\n========================================================" << std::endl;
    std::cout << " [HỒ SƠ HỌC VIÊN] Mã ID: " << id << " | " << fullName << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " Tên tài khoản  : " << username << std::endl;
    std::cout << " Địa chỉ Email   : " << email << std::endl;
    std::cout << " Khối lớp        : Lớp " << gradeLevel << std::endl;
    std::cout << " Trạng thái TK   : " << (isActive ? "HOẠT ĐỘNG" : "BỊ KHÓA") << std::endl;
    std::cout << "========================================================\n" << std::endl;
}

// Tutor Derived Class
void Tutor::addReviewRating(int newRatingScore) {
    double totalPoints = (rating * totalReviews) + newRatingScore;
    totalReviews++;
    rating = totalPoints / totalReviews;
}

void Tutor::displayProfile() const {
    std::cout << "\n========================================================" << std::endl;
    std::cout << " [HỒ SƠ GIA SƯ] Mã ID: " << id << " | " << fullName << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " Tên tài khoản  : " << username << std::endl;
    std::cout << " Địa chỉ Email   : " << email << std::endl;
    std::cout << " Chuyên môn      : " << subjectSpecialty << std::endl;
    std::cout << " Học phí         : " << (long long)hourlyRate << " VNĐ/buổi" << std::endl;
    if (totalReviews == 0 || rating == 0.0) {
        std::cout << " Điểm đánh giá   : Chưa có đánh giá (0 lượt đánh giá)" << std::endl;
    } else {
        std::cout << " Điểm đánh giá   : " << std::fixed << std::setprecision(1) << rating << " * (" << totalReviews << " lượt đánh giá)" << std::endl;
    }
    std::cout << " Giới thiệu      : " << bio << std::endl;
    std::cout << " Trạng thái TK   : " << (isActive ? "HOẠT ĐỘNG" : "BỊ KHÓA") << std::endl;
    std::cout << "========================================================\n" << std::endl;
}

// Admin Derived Class
void Admin::displayProfile() const {
    std::cout << "\n========================================================" << std::endl;
    std::cout << " [HỒ SƠ QUẢN TRỊ VIÊN] Mã ID: " << id << " | " << fullName << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " Vai trò: Quản trị viên hệ thống (Toàn quyền quản trị)" << std::endl;
    std::cout << "========================================================\n" << std::endl;
}

} // namespace TutorMatch
