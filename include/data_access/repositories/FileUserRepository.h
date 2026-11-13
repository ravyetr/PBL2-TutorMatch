#ifndef TUTOR_MATCH_FILE_USER_REPOSITORY_H
#define TUTOR_MATCH_FILE_USER_REPOSITORY_H

#include "data_access/repositories/IRepository.h"
#include "data_access/serializers/UserSerializer.h"
#include "business/domain/User.h"
#include "business/algorithms/HashTable.h"
#include "business/algorithms/LRUCache.h"
#include "business/algorithms/AVLTree.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>
#include <filesystem>

namespace TutorMatch {

/**
 * @brief Concrete File-based Repository for User hierarchy (Student, Tutor, Admin).
 *
 * Implements Phase 2 Step 3 (Data Access Layer):
 * - Implements IUserRepository contract (ISP & DIP).
 * - Manages persistent storage in users.dat (CSV format).
 * - Delegates CSV formatting/parsing to UserSerializer (SRP).
 * - RAM Hash Index (HashTable<int, std::streampos>): O(1) disk seeking by user ID.
 * - Profile LRU Cache (LRUCache<int, std::string>): In-memory profile line cache.
 * - AVL Trees (AVLTree<double, int>): In-memory balanced binary search trees for
 *   O(log N + K) range searches on tutor hourly rates and rating scores.
 * - Atomic persistence with temporary file renaming for crash resistance.
 * - Thread-safe operations protected by recursive mutex.
 */
class FileUserRepository : public IUserRepository {
private:
    std::string usersFilePath;

    // RAM Hash Index: User ID -> Byte Offset in users.dat
    HashTable<int, std::streampos> ramHashIndex;

    // LRU Cache for raw user CSV lines: User ID -> Raw CSV line
    LRUCache<int, std::string> profileLruCache;

    // AVL Trees for price and rating range queries
    AVLTree<double, int> priceRateIndex; // HourlyRate -> Tutor ID
    AVLTree<double, int> ratingIndex;    // Rating -> Tutor ID

    // Preserved unparsed / malformed lines to prevent data loss on atomic rewrite
    std::vector<std::string> rawUnparsedLines;

    mutable std::recursive_mutex userRepoMutex;
    int nextAutoIncrementId;

    bool rewriteUsersFileAtomic(const std::vector<std::shared_ptr<User>>& usersToPersist);

public:
    explicit FileUserRepository(const std::string& customFilePathOrDir = "");
    ~FileUserRepository() override = default;

    bool initialize();
    void rebuildRamIndex();

    // IUserRepository Implementation
    int getNextUserId() override;
    bool saveUser(const User& user) override;
    std::shared_ptr<User> findUserById(int userId) override;
    std::shared_ptr<User> findUserByUsername(const std::string& username) override;
    bool updateUser(const User& user) override;
    bool deleteUserSoft(int userId) override;

    std::vector<std::shared_ptr<Tutor>> getAllTutors() override;
    std::vector<std::shared_ptr<Student>> getAllStudents() override;
    std::vector<std::shared_ptr<User>> getAllUsers() override;

    // Range Queries via AVL Tree
    std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) override;
    std::vector<int> findTutorIdsByMinRating(double minRating) override;

    // Diagnostics & Inspection Helpers
    size_t getCacheSize() const { return profileLruCache.size(); }
    size_t getIndexCount() const { return ramHashIndex.size(); }
    const std::string& getFilePath() const { return usersFilePath; }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_FILE_USER_REPOSITORY_H
