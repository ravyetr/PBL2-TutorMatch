#include "data_access/storage/DataFileInspector.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>

static void setupConsoleUTF8() {
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
}
#else
static void setupConsoleUTF8() {}
#endif

int main(int argc, char* argv[]) {
    setupConsoleUTF8();

    std::string mode = "--all";
    std::string dataDir = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << "TutorMatch Data File Inspector Tool\n"
                      << "Usage:\n"
                      << "  " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --all              Inspect all 4 .dat files (default)\n"
                      << "  --users            Inspect users.dat only\n"
                      << "  --bookings         Inspect bookings.dat only\n"
                      << "  --reviews          Inspect reviews.dat only\n"
                      << "  --course-bookings  Inspect course_bookings.dat only\n"
                      << "  --dir <path>       Custom data directory path\n"
                      << "  --help, -h         Display this help message\n";
            return 0;
        } else if (arg == "--users" || arg == "--bookings" || arg == "--reviews" || arg == "--course-bookings" || arg == "--all") {
            mode = arg;
        } else if (arg == "--dir" && i + 1 < argc) {
            dataDir = argv[++i];
        }
    }

    if (mode == "--users") {
        auto summary = TutorMatch::DataFileInspector::inspectUsersFile(dataDir);
        TutorMatch::DataFileInspector::printFileSummary(summary);
    } else if (mode == "--bookings") {
        auto summary = TutorMatch::DataFileInspector::inspectBookingsFile(dataDir);
        TutorMatch::DataFileInspector::printFileSummary(summary);
    } else if (mode == "--reviews") {
        auto summary = TutorMatch::DataFileInspector::inspectReviewsFile(dataDir);
        TutorMatch::DataFileInspector::printFileSummary(summary);
    } else if (mode == "--course-bookings") {
        auto summary = TutorMatch::DataFileInspector::inspectCourseBookingsFile(dataDir);
        TutorMatch::DataFileInspector::printFileSummary(summary);
    } else {
        TutorMatch::DataFileInspector::inspectAll(dataDir);
    }

    return 0;
}
