#include "storage/repositories/FileReviewRepository.h"
#include "storage/ApplicationPaths.h"
#include "storage/serializers/ReviewSerializer.h"

#include <iostream>
#include <sstream>

namespace TutorMatch {

FileReviewRepository::FileReviewRepository(const std::string& customFilePathOrDir)
    : nextReviewId(1) {
    if (customFilePathOrDir.empty()) {
        reviewsFilePath = ApplicationPaths::resolveDataDirectory("data") + "/reviews.dat";
    } else {
        std::filesystem::path p(customFilePathOrDir);
        if (p.has_extension()) {
            reviewsFilePath = customFilePathOrDir;
        } else {
            reviewsFilePath = ApplicationPaths::resolveDataDirectory(customFilePathOrDir) + "/reviews.dat";
        }
    }
}

bool FileReviewRepository::initialize() {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    try {
        std::filesystem::path p(reviewsFilePath);
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }

        // Ensure file exists
        if (!std::filesystem::exists(reviewsFilePath)) {
            std::ofstream rFile(reviewsFilePath, std::ios::out | std::ios::binary);
            rFile.close();
        }

        // Determine next review ID
        std::ifstream rIn(reviewsFilePath, std::ios::in);
        if (rIn.is_open()) {
            std::string line;
            int maxRId = 0;
            while (std::getline(rIn, line)) {
                if (line.empty()) continue;
                std::string parseErr;
                auto rOpt = ReviewSerializer::fromCsv(line, &parseErr);
                if (rOpt.has_value()) {
                    if (rOpt->id > maxRId) maxRId = rOpt->id;
                }
            }
            rIn.close();
            nextReviewId = maxRId + 1;
        }

        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[FileReviewRepository Error] Initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

int FileReviewRepository::getNextReviewId() {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    return nextReviewId++;
}

bool FileReviewRepository::saveReview(const Review& review) {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    std::ofstream outfile(reviewsFilePath, std::ios::out | std::ios::app);
    if (!outfile.is_open()) return false;

    outfile << ReviewSerializer::toCsv(review) << "\n";
    if (outfile.fail()) {
        outfile.close();
        return false;
    }
    outfile.close();

    if (review.id >= nextReviewId) {
        nextReviewId = review.id + 1;
    }
    return true;
}

bool FileReviewRepository::deleteReview(int reviewId) {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    std::ifstream infile(reviewsFilePath, std::ios::in);
    if (!infile.is_open()) return false;

    std::string tempPath = reviewsFilePath + ".tmp";
    std::string backupPath = reviewsFilePath + ".bak";
    std::ofstream outfile(tempPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outfile.is_open()) {
        infile.close();
        return false;
    }

    std::string line;
    bool found = false;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::string parseErr;
        auto opt = ReviewSerializer::fromCsv(line, &parseErr);
        if (opt.has_value()) {
            if (opt->id == reviewId) {
                found = true;
                continue;
            }
            outfile << ReviewSerializer::toCsv(*opt) << "\n";
        } else {
            // Malformed record preservation
            outfile << line << "\n";
        }
        if (outfile.fail()) {
            infile.close();
            outfile.close();
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
            return false;
        }
    }
    infile.close();

    outfile.flush();
    if (!outfile.good()) {
        outfile.close();
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
        return false;
    }
    outfile.close();

    std::error_code ec;
    if (std::filesystem::exists(reviewsFilePath)) {
        std::filesystem::remove(backupPath, ec);
        std::filesystem::rename(reviewsFilePath, backupPath, ec);
        if (ec) {
            std::cerr << "[FileReviewRepository Error] Backup creation failed: " << ec.message() << std::endl;
            std::filesystem::remove(tempPath, ec);
            return false;
        }
    }

    std::filesystem::rename(tempPath, reviewsFilePath, ec);
    if (ec) {
        std::ifstream src(tempPath, std::ios::binary);
        std::ofstream dst(reviewsFilePath, std::ios::binary | std::ios::trunc);
        dst << src.rdbuf();
        src.close();
        dst.close();
        std::filesystem::remove(tempPath, ec);

        if (!dst.good()) {
            std::error_code rEc;
            std::filesystem::remove(reviewsFilePath, rEc);
            std::filesystem::rename(backupPath, reviewsFilePath, rEc);
            return false;
        }
    }

    if (std::filesystem::exists(backupPath)) {
        std::filesystem::remove(backupPath, ec);
    }

    return found;
}

std::vector<Review> FileReviewRepository::getReviewsByTutorId(int tutorId) {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    std::vector<Review> result;
    auto all = getAllReviews();
    for (const auto& r : all) {
        if (r.tutorId == tutorId) {
            result.push_back(r);
        }
    }
    return result;
}

std::vector<Review> FileReviewRepository::getAllReviews() {
    std::lock_guard<std::recursive_mutex> lock(reviewRepoMutex);
    std::vector<Review> result;
    std::ifstream infile(reviewsFilePath, std::ios::in);
    if (!infile.is_open()) return result;

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::string parseErr;
        auto opt = ReviewSerializer::fromCsv(line, &parseErr);
        if (opt.has_value()) {
            result.push_back(*opt);
        } else {
            std::cerr << "[FileReviewRepository Warning] Skipping corrupted review line: " << parseErr << std::endl;
        }
    }
    infile.close();
    return result;
}

} // namespace TutorMatch
