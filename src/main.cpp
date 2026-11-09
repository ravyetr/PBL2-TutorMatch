#include "../include/ui/ConsoleApp.h"
#include "../include/ds/HashTable.h"
#include "../include/ds/DoublyLinkedList.h"
#include "../include/ds/LRUCache.h"
#include "../include/ds/AVLTree.h"
#include "../include/ds/ScheduleGuard.h"
#include "../include/ds/MaxHeap.h"
#include "../include/storage/ApplicationPaths.h"
#include "../include/storage/repositories/FileUserRepository.h"
#include "../include/storage/repositories/FileReviewRepository.h"
#include "../include/services/AuthService.h"
#include "../include/services/UserService.h"
#include "../include/services/BookingService.h"
#include "../include/services/ReviewService.h"
#include "../include/services/MatchingService.h"
#include "../include/services/AdminService.h"

#include <iostream>
#include <cassert>

#ifdef _WIN32
#include <windows.h>

void setupConsoleUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hIn, &dwMode)) {
            dwMode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS;
            SetConsoleMode(hIn, dwMode);
        }
    }
}
#else
void setupConsoleUTF8() {}
#endif

using namespace TutorMatch;

void runCoreDataStructureUnitTests() {
    // 1. HashTable O(1) Test
    HashTable<std::string, int> hashTab;
    hashTab.insert("student_1", 101);
    hashTab.insert("tutor_99", 202);
    int val = 0;
    assert(hashTab.find("student_1", val) && val == 101);
    assert(hashTab.find("tutor_99", val) && val == 202);

    // 2. AVL Tree Range Query Test
    AVLTree<double, std::string> avlTree;
    avlTree.insert(25.0, "Tutor_Cheap");
    avlTree.insert(45.0, "Tutor_Mid");
    avlTree.insert(80.0, "Tutor_Exp");
    avlTree.insert(35.0, "Tutor_Fair");

    auto rangeResults = avlTree.rangeSearch(20.0, 50.0);
    assert(rangeResults.size() == 3); // 25, 35, 45

    // 3. LRU Cache Eviction Test
    LRUCache<int, std::string> lruCache(2); // Capacity = 2
    lruCache.put(1, "Profile_1");
    lruCache.put(2, "Profile_2");
    std::string profile;
    assert(lruCache.get(1, profile));  // Access 1 -> 1 becomes MRU, 2 becomes LRU
    lruCache.put(3, "Profile_3");      // Evicts 2
    assert(!lruCache.get(2, profile)); // 2 was evicted!
    assert(lruCache.get(1, profile));   // 1 is present
    assert(lruCache.get(3, profile));   // 3 is present

    // 4. Bitmask Schedule Guard Test
    uint64_t tutorSchedule = ScheduleGuard::getSlotMask(0, 0) | ScheduleGuard::getSlotMask(0, 1);
    uint64_t reqValid = ScheduleGuard::getSlotMask(0, 0);
    uint64_t reqInvalid = ScheduleGuard::getSlotMask(1, 0);

    assert(ScheduleGuard::isSlotAvailable(tutorSchedule, reqValid) == true);
    assert(ScheduleGuard::isSlotAvailable(tutorSchedule, reqInvalid) == false);

    // 5. Max-Heap Priority Queue Ranking Test
    MaxHeap<int> maxHeap;
    maxHeap.push(45);
    maxHeap.push(98);
    maxHeap.push(72);
    maxHeap.push(15);

    auto top2 = maxHeap.extractSortedTopK(2);
    assert(top2.size() == 2 && top2[0] == 98 && top2[1] == 72);
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    setupConsoleUTF8();
    #endif

    //std::cout << "[SYSTEM] Initializing Storage Engine...\n" << std::flush;
    try {
        // Chạy tự động các bài tự kiểm tra cấu trúc dữ liệu
        runCoreDataStructureUnitTests();

        // Composition Root: Repositories & Services
        std::string dataDir = ApplicationPaths::resolveDataDirectory();
        auto dbEngine = std::make_shared<MockDatabaseEngine>(dataDir);
        dbEngine->initialize();

        auto userRepo = dbEngine->getUserRepository();
        auto reviewRepo = dbEngine->getReviewRepository();
        auto courseBookingRepo = dbEngine->getCourseBookingRepository();

        auto authService = std::make_shared<AuthService>(userRepo);
        auto userService = std::make_shared<UserService>(userRepo);
        auto bookingService = std::make_shared<BookingService>(userRepo, reviewRepo, courseBookingRepo);
        auto reviewService = std::make_shared<ReviewService>(reviewRepo, userRepo, courseBookingRepo);
        auto matchingService = std::make_shared<MatchingService>(userRepo);
        auto adminService = std::make_shared<AdminService>(userRepo, reviewRepo, bookingService);

        // ADR-007 Startup Reconciliation
        reviewService->reconcileRatingsOnStartup();

        ConsoleApp app;
        if (app.initialize(authService, userService, bookingService, reviewService, matchingService, adminService, dbEngine)) {
            app.run();
        } else {
            std::cerr << "[FATAL ERROR] ConsoleApp initialization failed!\n";
            return 1;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL CRASH] " << ex.what() << "\n";
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    return 0;
}
