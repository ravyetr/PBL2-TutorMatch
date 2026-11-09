# TutorMatch — Tutor-Student Matching & Weekly Timetable Management Platform

[![Platform](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://github.com/ravyetr/PBL2-TutorMatch)
[![C++](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/Framework-Qt%206.11-green.svg)](https://www.qt.io/)
[![UI](https://img.shields.io/badge/UI-QML%20%2F%20QtQuick-lightgrey.svg)](https://doc.qt.io/qt-6/qtquick-index.html)
[![Storage](https://img.shields.io/badge/Storage-Flat--File%20%28CSV%29-orange.svg)](#storage--persistence)

---

## 1. Project Overview & Academic Context

**TutorMatch** is an academic software engineering project developed for **Project Based Learning 2 (PBL2)**. The platform focuses on object-oriented system modeling, SOLID architectural principles, custom data structures, and deterministic scheduling.

TutorMatch connects students with qualified tutors based on subject specialty, pricing, schedule availability, and verified ratings. It features bidirectional timetable synchronization, recurring course bookings over fixed multi-week durations, mutual commitment agreements, and consensual early termination.

> **Important Product Positioning:**  
> TutorMatch is strictly a **Tutor-Student Matching and Connection Platform (Matching & Scheduling Platform)**, **NOT** a Learning Management System (LMS).  
> The system does not track physical lesson attendance, monitor classroom progress, process financial payments, or collect platform commissions. It manages schedule agreements and mutual commitments between students and tutors.

---

## 2. Key Features

- **User Hierarchy & Role-Based Access (LSP/OCP):**
  - **Student:** Search tutors, inspect schedule overlaps, request single or multi-slot recurring courses with mandatory policy acknowledgment, submit course reviews, and request consensual early termination.
  - **Tutor:** Set hourly rate, subject specialty, and weekly availability bitmask (56 time slots); review pending course requests; accept or reject with commitment enforcement; request or respond to early termination.
  - **Admin:** Moderate user accounts, manage platform user locks, verify tutor credentials, and view system metrics.

- **56-Slot Weekly Timetable Engine (`ScheduleGuard`):**
  - Standardized weekly schedule representation: 7 days $\times$ 8 slots/day = 56 slots/week encoded into an unsigned 64-bit bitmask (`uint64_t`).
  - Bitwise math operations for $O(1)$ overlap detection and availability checking (`reqSlotMask & scheduleBitmask`).

- **Multi-Slot Recurring Course Bookings:**
  - Support for 1 to 56 weekly time slots over 4, 8, or 12 weeks duration.
  - Deterministic calendar date generation based on start date and chosen weekday slots.
  - Multi-slot conflict prevention across student and tutor schedules.

- **Commitment Acknowledgment & Consensual Early Termination:**
  - Strict two-sided policy acknowledgment (`COMMITMENT_V1`) with server-generated timestamps before any course enters `ACCEPTED`.
  - Mutual-consent early termination workflow: either party can initiate, but the course only terminates early (`EARLY_TERMINATED`) if the other party confirms.

- **Review & Rating Pipeline:**
  - Single review per student per completed or early-terminated course booking.
  - Automated incremental rating recalculation for tutors with self-healing reconciliation.

- **Dual User Interface:**
  - **Desktop GUI:** Modern desktop application built with Qt Quick / QML (Fluent-inspired design).
  - **Interactive CLI:** Full-featured console terminal application (`TutorMatch_CLI`) with role-based command loops.

---

## 3. Technology Stack

- **Core Language:** C++17
- **UI Framework:** Qt 6.11 (Qt Quick / QML, Qt Core, Qt Gui, Qt Network)
- **Build System:** CMake (3.16+)
- **Compiler:** MinGW 64-bit (GCC 13+) / Clang / MSVC
- **Storage Layer:** ACID-inspired flat-file persistence (CSV serialization with temporary file writes and atomic replacement)
- **Indexing Data Structures:** In-memory AVL Tree for key-value indexing and fast lookups

---

## 4. Architecture & Layer Responsibilities

The codebase adheres to SOLID principles and layered design:

```
TutorMatch/
├── include/              # Public C++ Header Specifications
│   ├── ds/               # Data structures (ScheduleGuard, AVLTree)
│   ├── gui/              # GUI Bridge & Qt QML model bindings
│   ├── models/           # Domain entities (User, Student, Tutor, Admin, CourseBooking, Review)
│   ├── services/         # Business services (Auth, Booking, Review, Admin, Matching)
│   ├── storage/          # Repositories, serializers, and MockDatabaseEngine
│   └── utils/            # Utility helpers (DateUtils)
├── src/                  # Implementation Files (.cpp)
├── ui/                   # QML Files & Resources
│   └── qml/              # UI views, modal dialogs, theme tokens, and custom components
├── tests/                # Comprehensive Test Suite (24 suites, TestRunner.cpp)
├── docs/                 # Architecture Decision Records (ADRs) & Design Specifications
├── scripts/              # Migration and verification utility scripts
└── data/                 # Local flat-file storage directory (tracked via .gitkeep)
```

### Layer Responsibilities
- **Domain Models (`include/models/`):** Pure entity representations (`User` polymorphic hierarchy, `CourseBooking`, `Review`).
- **Repositories (`include/storage/repositories/`):** Flat-file repositories implementing abstract repository interfaces (`IUserRepository`, `ICourseBookingRepository`, `IReviewRepository`).
- **Serializers (`include/storage/serializers/`):** Specialized CSV serializers with backward-compatible token-count parsing (13, 18, and 23 fields).
- **Services (`include/services/`):** Business logic coordinators (`BookingService`, `AuthService`, `ReviewService`, `AdminService`, `MatchingEngine`).
- **GUI Bridge (`include/gui/GUIBridge.h`):** Decoupled `QObject` interface bridging QML interactions to backend domain services.

---

## 5. Storage & Persistence Notes

TutorMatch uses file-based persistence for lightweight, zero-dependency deployment:

- **Storage Location:** `data/` directory relative to the executable or project root (`users.dat`, `course_bookings.dat`, `reviews.dat`).
- **Atomic File Writes:** Updates write to temporary files (`.tmp`) before performing atomic file replacement (`MoveFileExW` with `MOVEFILE_WRITE_THROUGH` on Windows) to prevent corrupting data during unexpected terminations.
- **Auto-Recovery & Data Seeding:** If data files are missing or empty on application startup, `MockDatabaseEngine::seedDefaultDataIfEmpty()` automatically recovers a default dataset containing administrative and sample accounts.
- **Sensitive Data Exclusion:** Actual runtime database files (`*.dat`), backups, and archives are excluded from Git version control via `.gitignore`.

---

## 6. Prerequisites

To build and run TutorMatch from source, ensure the following tools are installed:

- **CMake:** Version 3.16 or higher
- **C++ Compiler:** C++17 compliant compiler (e.g., MinGW-w64 GCC 13+, MSVC 2019+, or Clang)
- **Qt 6 Framework:** Qt 6.5+ (Qt 6.11 recommended) with components:
  - `Qt6::Core`
  - `Qt6::Gui`
  - `Qt6::Qml`
  - `Qt6::Quick`
  - `Qt6::QuickControls2`
  - `Qt6::Network`

---

## 7. Build Instructions

### Configure with CMake
Open a terminal in the project root directory:

```bash
# Configure build in a build/ directory using MinGW
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

*(On Windows with MSVC, omit `-G "MinGW Makefiles"` to use the Visual Studio generator).*

### Build All Targets
```bash
cmake --build build --config Debug -j4
```

This compiles all four executable targets:
1. `TutorMatch_GUI`: Desktop QML Graphical User Interface.
2. `TutorMatch_CLI`: Console Terminal Application.
3. `TutorMatch_Tests`: Comprehensive Verification Test Suite.
4. `TutorMatch_Inspector`: Standalone Flat-File Forensic Inspector.

---

## 8. Running the Application

### Running the Desktop GUI
Ensure Qt runtime DLLs are in your system `PATH`, or deploy them using `windeployqt`:

```bash
# Add Qt bin to PATH (example for MinGW)
set PATH=C:\Qt\6.11.2\mingw_64\bin;%PATH%

# Run GUI
./build/TutorMatch_GUI.exe
```

### Running the Console CLI
```bash
./build/TutorMatch_CLI.exe
```

**Default Test Credentials:**
- **Admin:** `username: admin` | `password: admin123`
- **Tutor:** `username: tutor_math` | `password: 123456`
- **Student:** `username: student_nam` | `password: 123456`

*(These accounts are automatically seeded if `data/users.dat` is empty or missing).*

---

## 9. Running Tests & Data Diagnostics

### Running the Automated Test Suite
TutorMatch includes 24 comprehensive test suites covering unit logic, bitmask math, persistence, concurrency safety, workflows, and edge-case matrixes:

```bash
./build/TutorMatch_Tests.exe
```

### Running the Data Inspector
To verify data storage integrity, field layouts, and detect record corruption:

```bash
./build/TutorMatch_Inspector.exe --all
```

---

## 10. Known Technical Limitations

1. **Multi-Process Concurrency:** Flat-file repositories use in-process mutexes and atomic file replacement. Concurrent write operations across multiple independent running processes without OS-level advisory locks may lead to lost updates.
2. **Time-Dependent Test Fixtures:** Certain static test cases in `TestRunner.cpp` use explicit calendar dates (`YYYY-MM-DD`). Because `BookingService` rejects dates in the past, running historical tests without adjusting fixture dates or using relative date utilities may trigger validation boundaries.
3. **No Embedded Payment Gateway:** Financial metrics (Hourly Rate, Gross Course Value) represent agreed contractual figures between tutor and student; payments are handled directly between parties offline.

---

## 11. Authors & License

- **Author / Developer:** [ravyetr](https://github.com/ravyetr) (`hongocyentrang83@gmail.com`)
- **Project:** PBL2 - Software Engineering Project
- **Institution:** The University of Danang - University of Science and Technology (DUT)
- **License:** Open for academic and educational evaluation.
