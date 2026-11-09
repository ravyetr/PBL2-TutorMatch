#include "storage/repositories/FileCourseBookingRepository.h"
#include "storage/ApplicationPaths.h"
#include "storage/serializers/CourseBookingSerializer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace TutorMatch {

FileCourseBookingRepository::FileCourseBookingRepository(const std::string& customFilePathOrDir)
    : nextId(1001) {
    if (customFilePathOrDir.empty()) {
        filePath = ApplicationPaths::resolveDataDirectory("data") + "/course_bookings.dat";
    } else {
        std::filesystem::path p(customFilePathOrDir);
        if (p.has_extension()) {
            filePath = customFilePathOrDir;
        } else {
            filePath = ApplicationPaths::resolveDataDirectory(customFilePathOrDir) + "/course_bookings.dat";
        }
    }
}

bool FileCourseBookingRepository::initialize() {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    try {
        std::filesystem::path p(filePath);
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }

        // Ensure file exists
        if (!std::filesystem::exists(filePath)) {
            std::ofstream f(filePath, std::ios::out | std::ios::binary);
            f.close();
        }

        // Scan for max ID
        std::ifstream in(filePath, std::ios::in);
        if (in.is_open()) {
            std::string line;
            int maxFound = 1000;
            while (std::getline(in, line)) {
                if (line.empty()) continue;
                std::string parseErr;
                auto cbOpt = CourseBookingSerializer::fromCsv(line, &parseErr);
                if (cbOpt.has_value()) {
                    if (cbOpt->id > maxFound) maxFound = cbOpt->id;
                }
            }
            in.close();
            nextId = maxFound + 1;
        }

        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[FileCourseBookingRepository Error] Initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

int FileCourseBookingRepository::getNextCourseBookingId() {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    return nextId++;
}

bool FileCourseBookingRepository::saveCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    try {
        std::ofstream out(filePath, std::ios::app | std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "[FileCourseBookingRepository Error] Cannot open for append: " << filePath << std::endl;
            return false;
        }

        std::string csvLine = CourseBookingSerializer::toCsv(booking);
        out << csvLine << "\n";
        out.flush();
        bool ok = out.good();
        out.close();

        if (ok && booking.id >= nextId) {
            nextId = booking.id + 1;
        }
        return ok;
    } catch (const std::exception& ex) {
        std::cerr << "[FileCourseBookingRepository Error] saveCourseBooking failed: " << ex.what() << std::endl;
        return false;
    }
}

bool FileCourseBookingRepository::updateCourseBooking(const CourseBooking& booking) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    try {
        std::vector<CourseBooking> all = getAllCourseBookings();
        bool found = false;
        for (auto& item : all) {
            if (item.id == booking.id) {
                item = booking;
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }

        // Atomic write to temporary file
        std::string tempPath = filePath + ".tmp";
        std::ofstream out(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "[FileCourseBookingRepository Error] Cannot open temp file: " << tempPath << std::endl;
            return false;
        }

        for (const auto& item : all) {
            out << CourseBookingSerializer::toCsv(item) << "\n";
        }
        out.flush();
        if (!out.good()) {
            out.close();
            std::filesystem::remove(tempPath);
            return false;
        }
        out.close();

#ifdef _WIN32
        std::wstring wTemp(tempPath.begin(), tempPath.end());
        std::wstring wDest(filePath.begin(), filePath.end());
        if (!MoveFileExW(wTemp.c_str(), wDest.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::error_code ec;
            std::filesystem::rename(tempPath, filePath, ec);
            if (ec) {
                std::cerr << "[FileCourseBookingRepository Error] Atomic replace failed: " << ec.message() << std::endl;
                std::filesystem::remove(tempPath);
                return false;
            }
        }
#else
        std::error_code ec;
        std::filesystem::rename(tempPath, filePath, ec);
        if (ec) {
            std::cerr << "[FileCourseBookingRepository Error] Atomic rename failed: " << ec.message() << std::endl;
            std::filesystem::remove(tempPath);
            return false;
        }
#endif
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[FileCourseBookingRepository Error] updateCourseBooking failed: " << ex.what() << std::endl;
        return false;
    }
}

std::shared_ptr<CourseBooking> FileCourseBookingRepository::findCourseBookingById(int id) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    std::ifstream in(filePath, std::ios::in);
    if (!in.is_open()) return nullptr;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::string err;
        auto cbOpt = CourseBookingSerializer::fromCsv(line, &err);
        if (cbOpt.has_value() && cbOpt->id == id) {
            return std::make_shared<CourseBooking>(*cbOpt);
        }
    }
    return nullptr;
}

std::vector<CourseBooking> FileCourseBookingRepository::getAllCourseBookings() {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    std::vector<CourseBooking> list;
    std::ifstream in(filePath, std::ios::in);
    if (!in.is_open()) return list;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::string err;
        auto cbOpt = CourseBookingSerializer::fromCsv(line, &err);
        if (cbOpt.has_value()) {
            list.push_back(*cbOpt);
        } else if (!err.empty()) {
            std::cerr << "[FileCourseBookingRepository Warning] Skipping line: " << err << std::endl;
        }
    }
    return list;
}

std::vector<CourseBooking> FileCourseBookingRepository::getCourseBookingsByStudentId(int studentId) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    std::vector<CourseBooking> list;
    auto all = getAllCourseBookings();
    for (const auto& item : all) {
        if (item.studentId == studentId) {
            list.push_back(item);
        }
    }
    return list;
}

std::vector<CourseBooking> FileCourseBookingRepository::getCourseBookingsByTutorId(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(repoMutex);
    std::vector<CourseBooking> list;
    auto all = getAllCourseBookings();
    for (const auto& item : all) {
        if (item.tutorId == tutorId) {
            list.push_back(item);
        }
    }
    return list;
}

} // namespace TutorMatch
