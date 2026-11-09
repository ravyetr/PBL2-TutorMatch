#include "storage/repositories/FileUserRepository.h"
#include "storage/ApplicationPaths.h"
#include "storage/serializers/UserSerializer.h"
#include "ds/ScheduleGuard.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

namespace TutorMatch {

FileUserRepository::FileUserRepository(const std::string& customFilePathOrDir)
    : ramHashIndex(32),
      profileLruCache(50),
      nextAutoIncrementId(1) {
    if (customFilePathOrDir.empty()) {
        usersFilePath = ApplicationPaths::resolveDataDirectory("data") + "/users.dat";
    } else {
        std::filesystem::path p(customFilePathOrDir);
        if (p.has_extension()) {
            usersFilePath = customFilePathOrDir;
        } else {
            usersFilePath = ApplicationPaths::resolveDataDirectory(customFilePathOrDir) + "/users.dat";
        }
    }
}

bool FileUserRepository::initialize() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    try {
        std::filesystem::path p(usersFilePath);
        if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
            std::filesystem::create_directories(p.parent_path());
        }

        // Ensure file exists
        if (!std::filesystem::exists(usersFilePath)) {
            std::ofstream uFile(usersFilePath, std::ios::out | std::ios::binary);
            uFile.close();
        }

        rebuildRamIndex();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[FileUserRepository Error] Initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

void FileUserRepository::rebuildRamIndex() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    ramHashIndex.clear();
    priceRateIndex.clear();
    ratingIndex.clear();
    profileLruCache.clear();
    rawUnparsedLines.clear();

    struct LineRecord {
        int id;
        std::streampos pos;
        std::string line;
        bool isTutor;
        bool isActive;
        double rate;
        double rating;
    };

    std::ifstream infile(usersFilePath, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        return;
    }

    std::string line;
    int maxId = 0;
    int totalLines = 0;
    std::unordered_map<int, LineRecord> latestRecords;
    std::vector<int> idOrder;

    while (true) {
        std::streampos pos = infile.tellg();
        if (!std::getline(infile, line)) break;
        if (line.empty()) continue;

        totalLines++;

        std::string parseErr;
        auto user = UserSerializer::fromCsv(line, &parseErr);
        if (!user) {
            std::cerr << "[FileUserRepository Warning] Skipping corrupted line in " 
                      << usersFilePath << ": " << parseErr << std::endl;
            rawUnparsedLines.push_back(line);
            continue;
        }

        int id = user->getId();
        if (id > maxId) maxId = id;

        LineRecord rec;
        rec.id = id;
        rec.pos = pos;
        rec.line = line;
        rec.isTutor = (user->getRole() == UserRole::TUTOR);
        rec.isActive = user->getIsActive();
        rec.rate = 0.0;
        rec.rating = 0.0;

        if (rec.isTutor) {
            auto t = std::static_pointer_cast<Tutor>(user);
            rec.rate = t->getHourlyRate();
            rec.rating = t->getRating();
        }

        if (latestRecords.find(id) == latestRecords.end()) {
            idOrder.push_back(id);
        }
        latestRecords[id] = rec;
    }
    infile.close();
    nextAutoIncrementId = maxId + 1;

    // Populate ramHashIndex and AVL indexes strictly from canonical latest records
    for (const auto& kv : latestRecords) {
        const auto& rec = kv.second;
        ramHashIndex.insert(rec.id, rec.pos);
        if (rec.isTutor && rec.isActive) {
            priceRateIndex.insert(rec.rate, rec.id);
            ratingIndex.insert(rec.rating, rec.id);
        }
    }

    // Auto-compact if duplicate lines detected
    if (totalLines > static_cast<int>(latestRecords.size())) {
        std::cout << "[FileUserRepository Notice] Auto-compacting users: detected "
                  << totalLines << " lines, deduplicating to " << latestRecords.size() << " records." << std::endl;
        std::vector<std::shared_ptr<User>> canonicalUsers;
        std::sort(idOrder.begin(), idOrder.end());
        for (int id : idOrder) {
            auto u = findUserById(id);
            if (u) canonicalUsers.push_back(u);
        }
        rewriteUsersFileAtomic(canonicalUsers);
    }
}

bool FileUserRepository::rewriteUsersFileAtomic(const std::vector<std::shared_ptr<User>>& usersToPersist) {
    std::string tempPath = usersFilePath + ".tmp";
    std::ofstream tempOut(tempPath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!tempOut.is_open()) {
        std::cerr << "[FileUserRepository Error] Failed to create temp file for atomic rewrite: " << tempPath << std::endl;
        return false;
    }

    std::vector<std::pair<int, std::streampos>> newOffsets;
    std::vector<std::pair<int, std::string>> newCachedLines;
    newOffsets.reserve(usersToPersist.size());
    newCachedLines.reserve(usersToPersist.size());

    for (const auto& user : usersToPersist) {
        if (!user) continue;
        std::streampos pos = tempOut.tellp();
        std::string line = UserSerializer::toCsv(*user);
        tempOut << line << "\n";
        if (tempOut.fail()) {
            std::cerr << "[FileUserRepository Error] Write stream failed while writing user ID: " << user->getId() << std::endl;
            tempOut.close();
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
            return false;
        }
        newOffsets.push_back({user->getId(), pos});
        newCachedLines.push_back({user->getId(), line});
    }

    // Preserve unparsed / malformed lines at end of file
    for (const auto& rawLine : rawUnparsedLines) {
        tempOut << rawLine << "\n";
    }

    tempOut.flush();
    if (!tempOut.good()) {
        std::cerr << "[FileUserRepository Error] Flush failed for temp file: " << tempPath << std::endl;
        tempOut.close();
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
        return false;
    }
    tempOut.close();

    // Verify temp file
    std::error_code ec;
    auto tempSize = std::filesystem::file_size(tempPath, ec);
    if (ec || ((!usersToPersist.empty() || !rawUnparsedLines.empty()) && tempSize == 0)) {
        std::cerr << "[FileUserRepository Error] Temp file verification failed: " << ec.message() << std::endl;
        std::filesystem::remove(tempPath, ec);
        return false;
    }

    // Atomic replacement with backup and rollback
    std::string backupPath = usersFilePath + ".bak";
    if (std::filesystem::exists(usersFilePath)) {
        std::filesystem::remove(backupPath, ec);
        std::filesystem::rename(usersFilePath, backupPath, ec);
        if (ec) {
            std::cerr << "[FileUserRepository Error] Failed to create backup file: " << ec.message() << std::endl;
            std::filesystem::remove(tempPath, ec);
            return false;
        }
    }

    std::filesystem::rename(tempPath, usersFilePath, ec);
    if (ec) {
        std::cerr << "[FileUserRepository Warning] Rename failed, attempting fallback copy: " << ec.message() << std::endl;
        std::ifstream src(tempPath, std::ios::binary);
        std::ofstream dst(usersFilePath, std::ios::binary | std::ios::trunc);
        dst << src.rdbuf();
        src.close();
        dst.close();
        std::filesystem::remove(tempPath, ec);

        if (!dst.good()) {
            std::cerr << "[FileUserRepository Error] Fallback copy failed! Rolling back from backup." << std::endl;
            std::error_code rEc;
            std::filesystem::remove(usersFilePath, rEc);
            std::filesystem::rename(backupPath, usersFilePath, rEc);
            return false;
        }
    }

    // Operation succeeded, safely remove backup
    if (std::filesystem::exists(backupPath)) {
        std::filesystem::remove(backupPath, ec);
    }

    // Update RAM Hash Index and LRU Cache with verified offsets
    ramHashIndex.clear();
    for (const auto& entry : newOffsets) {
        ramHashIndex.insert(entry.first, entry.second);
    }
    for (const auto& entry : newCachedLines) {
        profileLruCache.put(entry.first, entry.second);
    }

    return true;
}

int FileUserRepository::getNextUserId() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    return nextAutoIncrementId++;
}

bool FileUserRepository::saveUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);

    std::streampos existingOffset;
    if (ramHashIndex.find(user.getId(), existingOffset)) {
        return updateUser(user);
    }

    auto allUsers = getAllUsers();
    std::shared_ptr<User> newUser;
    if (user.getRole() == UserRole::STUDENT) {
        newUser = std::make_shared<Student>(static_cast<const Student&>(user));
    } else if (user.getRole() == UserRole::TUTOR) {
        newUser = std::make_shared<Tutor>(static_cast<const Tutor&>(user));
    } else if (user.getRole() == UserRole::ADMIN) {
        newUser = std::make_shared<Admin>(static_cast<const Admin&>(user));
    } else {
        return false;
    }

    allUsers.push_back(newUser);

    if (user.getRole() == UserRole::TUTOR && user.getIsActive()) {
        const Tutor& tutor = static_cast<const Tutor&>(user);
        priceRateIndex.insert(tutor.getHourlyRate(), tutor.getId());
        ratingIndex.insert(tutor.getRating(), tutor.getId());
    }

    if (user.getId() >= nextAutoIncrementId) {
        nextAutoIncrementId = user.getId() + 1;
    }

    return rewriteUsersFileAtomic(allUsers);
}

std::shared_ptr<User> FileUserRepository::findUserById(int userId) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);

    try {
        // 1. Check LRU Cache
        std::string cachedLine;
        if (profileLruCache.get(userId, cachedLine)) {
            auto u = UserSerializer::fromCsv(cachedLine);
            if (u) return u;
        }

        // 2. RAM Hash Index Seek
        std::streampos offset;
        if (!ramHashIndex.find(userId, offset)) {
            return nullptr;
        }

        std::ifstream infile(usersFilePath, std::ios::in | std::ios::binary);
        if (!infile.is_open()) return nullptr;

        infile.seekg(offset);
        std::string line;
        if (std::getline(infile, line)) {
            infile.close();
            profileLruCache.put(userId, line);
            return UserSerializer::fromCsv(line);
        }
        infile.close();
    } catch (const std::exception& e) {
        std::cerr << "[FileUserRepository Error] findUserById failed for ID " << userId << ": " << e.what() << std::endl;
    }

    return nullptr;
}

std::shared_ptr<User> FileUserRepository::findUserByUsername(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    auto allIndex = ramHashIndex.getAll();
    for (const auto& kv : allIndex) {
        auto u = findUserById(kv.first);
        if (u && u->getUsername() == username) {
            return u;
        }
    }
    return nullptr;
}

bool FileUserRepository::updateUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);

    std::streampos existingOffset;
    if (!ramHashIndex.find(user.getId(), existingOffset)) {
        return saveUser(user);
    }

    // Clean up old AVL tree index entries if previously a tutor
    auto existingUser = findUserById(user.getId());
    if (existingUser && existingUser->getRole() == UserRole::TUTOR) {
        auto oldTutor = std::static_pointer_cast<Tutor>(existingUser);
        priceRateIndex.remove(oldTutor->getHourlyRate(), oldTutor->getId());
        ratingIndex.remove(oldTutor->getRating(), oldTutor->getId());
    }

    auto allUsers = getAllUsers();
    bool replaced = false;
    for (size_t i = 0; i < allUsers.size(); ++i) {
        if (allUsers[i]->getId() == user.getId()) {
            if (user.getRole() == UserRole::STUDENT) {
                allUsers[i] = std::make_shared<Student>(static_cast<const Student&>(user));
            } else if (user.getRole() == UserRole::TUTOR) {
                allUsers[i] = std::make_shared<Tutor>(static_cast<const Tutor&>(user));
            } else if (user.getRole() == UserRole::ADMIN) {
                allUsers[i] = std::make_shared<Admin>(static_cast<const Admin&>(user));
            }
            replaced = true;
            break;
        }
    }

    if (!replaced) {
        if (user.getRole() == UserRole::STUDENT) {
            allUsers.push_back(std::make_shared<Student>(static_cast<const Student&>(user)));
        } else if (user.getRole() == UserRole::TUTOR) {
            allUsers.push_back(std::make_shared<Tutor>(static_cast<const Tutor&>(user)));
        } else if (user.getRole() == UserRole::ADMIN) {
            allUsers.push_back(std::make_shared<Admin>(static_cast<const Admin&>(user)));
        }
    }

    if (user.getRole() == UserRole::TUTOR && user.getIsActive()) {
        const Tutor& tutor = static_cast<const Tutor&>(user);
        priceRateIndex.insert(tutor.getHourlyRate(), tutor.getId());
        ratingIndex.insert(tutor.getRating(), tutor.getId());
    }

    return rewriteUsersFileAtomic(allUsers);
}

bool FileUserRepository::deleteUserSoft(int userId) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    auto user = findUserById(userId);
    if (user) {
        user->setIsActive(false);
        return updateUser(*user);
    }
    return false;
}

std::vector<std::shared_ptr<Tutor>> FileUserRepository::getAllTutors() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    std::vector<std::shared_ptr<Tutor>> tutors;
    auto allIndex = ramHashIndex.getAll();
    std::sort(allIndex.begin(), allIndex.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (const auto& kv : allIndex) {
        auto u = findUserById(kv.first);
        if (u && u->getRole() == UserRole::TUTOR && u->getIsActive()) {
            tutors.push_back(std::static_pointer_cast<Tutor>(u));
        }
    }
    return tutors;
}

std::vector<std::shared_ptr<Student>> FileUserRepository::getAllStudents() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    std::vector<std::shared_ptr<Student>> students;
    auto allIndex = ramHashIndex.getAll();
    std::sort(allIndex.begin(), allIndex.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (const auto& kv : allIndex) {
        auto u = findUserById(kv.first);
        if (u && u->getRole() == UserRole::STUDENT && u->getIsActive()) {
            students.push_back(std::static_pointer_cast<Student>(u));
        }
    }
    return students;
}

std::vector<std::shared_ptr<User>> FileUserRepository::getAllUsers() {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    std::vector<std::shared_ptr<User>> users;
    auto allIndex = ramHashIndex.getAll();
    std::sort(allIndex.begin(), allIndex.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (const auto& kv : allIndex) {
        auto u = findUserById(kv.first);
        if (u) {
            users.push_back(u);
        }
    }
    return users;
}

std::vector<int> FileUserRepository::findTutorIdsByPriceRange(double minRate, double maxRate) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    return priceRateIndex.rangeSearch(minRate, maxRate);
}

std::vector<int> FileUserRepository::findTutorIdsByMinRating(double minRating) {
    std::lock_guard<std::recursive_mutex> lock(userRepoMutex);
    return ratingIndex.rangeSearch(minRating, 5.0);
}

} // namespace TutorMatch
