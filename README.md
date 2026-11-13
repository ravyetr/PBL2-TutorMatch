# TutorMatch — Tutor–Student Matching & Weekly Timetable Management Platform

[![Platform](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://github.com/ravyetr/PBL2-TutorMatch)
[![C++](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Framework](https://img.shields.io/badge/Framework-Qt%206.11-green.svg)](https://www.qt.io/)
[![UI](https://img.shields.io/badge/UI-QML%20%2F%20QtQuick-lightgrey.svg)](https://doc.qt.io/qt-6/qtquick-index.html)
[![Storage](https://img.shields.io/badge/Storage-Flat--File%20%28CSV%20%2B%20AVL%29-orange.svg)](#5-storage--database-architecture)
[![Tests](https://img.shields.io/badge/Tests-34%20Suites%20Passing-brightgreen.svg)](#8-running-tests--forensic-inspector)

---

## 1. Project Overview & Academic Context

**TutorMatch** is an academic software engineering platform developed for **Project Based Learning 2 (PBL2 - Dự án Cơ sở Ngành Công nghệ Thông tin)** at the **University of Science and Technology – The University of Danang (DUT)**.

The project models an academic matchmaking and recurring schedule negotiation system between students and tutors, adhering strictly to **SOLID architectural principles**, clean object-oriented design, custom memory-efficient data structures, and deterministic timetable math.

### Core Product Positioning
> **Matching & Scheduling Platform, NOT an LMS:**  
> TutorMatch is designed specifically for **Tutor–Student Matching, Timetable Negotiation, and Mutual Commitments**.  
> The application deliberately does not track attendance, grade student assignments, process online payments, or collect platform fees. It manages schedule agreements and mutual commitments between students and tutors.

### Comprehensive Technical Documentation
A definitive, fully verified technical documentation rebuild is available in the [`docs/`](docs/README.md) directory:
- [**Documentation Portal & Reading Guide (docs/README.md)**](docs/README.md)
- [01. Introduction & Overview](docs/01_introduction/01_product_overview_VI.md)
- [02. Three-Layer Architecture](docs/02_system_architecture/01_overall_architecture_VI.md)
- [03. Functional Specifications (STU / TUT / ADM)](docs/03_functional_specification/01_student_features_VI.md)
- [04. Data Model & Data Dictionary](docs/04_data_and_database/02_data_dictionary_VI.md)
- [05. Business Logic & SmartMatch Engine](docs/05_business_logic/03_smartmatch_engine_algorithm_VI.md)
- [06. Comprehensive SOLID Principles Audit](docs/06_solid_analysis/06_class_level_solid_matrix_VI.md)
- [07. Complete Code Reference & Symbol Index](docs/07_code_reference/01_complete_repository_inventory_VI.md)
- [08. Quality, Testing & Technical Debt](docs/08_quality_and_testing/01_test_architecture_and_infrastructure_VI.md)
- [Architectural Diagrams (20 Mermaid Diagrams)](docs/diagrams/)

---

## 2. Key Architecture & Features

### Role-Based User Hierarchy (LSP & OCP)
- **Student:** Discover tutors via intelligent search or SmartMatch, inspect 56-slot schedule alignments, send multi-slot recurring course requests, acknowledge mutual commitment terms, and review completed courses.
- **Tutor:** Configure hourly rates, subject specialties, biographical details, and weekly availability bitmasks; review course requests; accept/reject with commitment rules; initiate or consent to early termination.
- **Admin:** Moderate user accounts, manage platform user locks, inspect forensic data integrity, and view platform metrics.

### 56-Slot Weekly Timetable Engine (`ScheduleGuard`)
- Weekly schedules are mathematically modeled as 7 days $\times$ 8 slots/day = **56 slots/week**, encoded into a single 64-bit unsigned integer (`uint64_t`).
- Bitwise math enables $O(1)$ overlap checks (`reqSlotMask & scheduleBitmask`), boundary sanitization (enforcing maximum $2^{56}-1$), and zero-overhead timetable synchronization.

### SmartMatch Engine with Mandatory Subject Filtering
- Filtering strictly separates **eligibility** from **weighted ranking**.
- Tutors not matching the requested subject are excluded prior to scoring. Eligible tutors are then scored using normalized multi-criteria weighting: price compatibility, rating, review count, experience, and weekly schedule overlap.

### Multi-Token Semantic Search Engine
- Accent-insensitive and case-insensitive Vietnamese text search (`Toán 12`, `toan vat ly`, `luyện thi THPT`).
- Normalized token extraction supporting conjunction delimiters (`&`, `,`, `và`), Grade Level recognition (10, 11, 12), and relevance score ranking.

### Multi-Week Recurring Course Lifecycle
- Recurring courses span 4, 8, or 12 weeks with deterministic calendar date calculation.
- Two-sided policy acknowledgment (`COMMITMENT_V1`) required before state enters `ACCEPTED`.
- Consensual early termination workflow requiring mutual agreement from both parties.

---

## 3. Technology Stack & Prerequisites

| Component | Minimum Requirement | Recommended |
|---|---|---|
| **OS** | Windows 10 / 11 (64-bit) | Windows 11 (64-bit) |
| **Language** | C++17 compliant compiler | GCC 13.1.0+ (MinGW-w64 64-bit) |
| **UI Framework** | Qt 6.2+ | Qt 6.11.2 (Qt Quick, QML, Core, Gui) |
| **Build System** | CMake 3.16+ | CMake 3.25+ |
| **Persistence** | Zero external DBMS | Flat-File CSV with atomic file replacement |
| **IDE** | Any (CLI / VS Code / Qt Creator) | Qt Creator 13+ |

---

## 4. Quick Start — Qt Creator Workflow (Zero Configuration)

TutorMatch is designed to be **100% clone-and-run ready**. A collaborator or evaluator can clone the repository, open it in Qt Creator, configure the desktop kit, build, and run immediately.

### Step-by-Step Instructions:

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ravyetr/PBL2-TutorMatch.git
   cd PBL2-TutorMatch
   ```

2. **Open the project in Qt Creator:**
   - Launch **Qt Creator**.
   - Click **File** > **Open File or Project...** (or `Ctrl+O`).
   - Navigate to the cloned folder and select `CMakeLists.txt`.

3. **Configure Project:**
   - In the **Configure Project** screen, choose the **Desktop Qt 6.11.2 MinGW 64-bit** kit (or any installed Qt 6.2+ MinGW 64-bit kit).
   - Ensure the build type is set to **Debug** or **Release**.
   - Click the **Configure Project** button.

4. **Select Target & Run:**
   - In the bottom-left target selector, ensure **TutorMatch_GUI** is selected.
   - Press **Ctrl+R** (or click the green **Run** triangle).
   - The application will compile, link, auto-seed the canonical database from `data/seed/`, and launch the GUI dashboard immediately!

---

## 5. Storage & Database Architecture

TutorMatch uses a deterministic, dependency-free flat-file storage engine:

- **Clean Seed Separation:** The repository ships with a pristine, version-controlled seed dataset in `data/seed/`:
  - `data/seed/users.dat`: 52 standardized user accounts (3 Admin, 13 Tutor, 36 Student).
  - `data/seed/course_bookings.dat`: 23 multi-slot recurring course contracts.
  - `data/seed/reviews.dat`: 15 completed course reviews.
- **Automatic First-Run Migration:** On first launch, `MockDatabaseEngine::seedDefaultDataIfEmpty()` detects that active runtime storage in `data/` is uninitialized and automatically copies the seed dataset into `data/*.dat`.
- **Zero Runtime Overwrite:** Once initialized, subsequent application restarts strictly preserve all newly created users, bookings, ratings, and timetable changes.
- **Atomic File Writes:** File updates write to a temporary file (`.tmp`) before atomic rename (`MoveFileExW` with `MOVEFILE_WRITE_THROUGH`) to prevent corruption during unexpected shutdowns.
- **Git-Ignored Runtime Data:** Active runtime files (`data/*.dat`) and local temporary files are ignored by version control to avoid repository churn.

---

## 6. Standardized Demo Accounts

All demo accounts have been standardized with coherent credentials:

| Role | Username | Password | Notes / Specialty |
|---|---|---|---|
| **Admin** | `admin` | `admin123` | System Administrator (Full platform moderation) |
| **Admin** | `qt_phuongt92` | `admin123` | Support Administrator |
| **Admin** | `qt_chaul50` | `admin123` | Content Administrator |
| **Tutor** | `tutor_math` | `pass123` | ThS. Lê Văn Toàn (Toán 12 — Luyện thi ĐH, 150k/buổi) |
| **Tutor** | `tutor_phys` | `pass123` | TS. Trần Thị Hồng Nhung (Vật lý 10-12, 200k/buổi) |
| **Tutor** | `tutor_eng` | `pass123` | Ms. Nguyễn Anh Văn (Tiếng Anh IELTS, 120k/buổi) |
| **Tutor** | `tutor_chem` | `pass123` | Thầy Phạm Hóa Học (Hóa học 10-12, 100k/buổi) |
| **Tutor** | `sinh_thng84` | `pass123` | Cô Nguyễn Thị Sinh (Sinh học 10-12, 130k/buổi) |
| **Tutor** | `tinvo.edu` | `pass123` | Thầy Võ Minh Tin (Tin học C++, 180k/buổi) |
| **Student** | `student_nam` | `pass123` | Nguyễn Văn Nam (Học sinh Lớp 12, Lịch rảnh 31/56 ca) |
| **Student** | `student_thanh` | `pass123` | Phạm Tuấn Thành (Học sinh Lớp 11, Lịch rảnh 48/56 ca) |
| **Student** | `tmanh964` | `pass123` | Trần Minh Anh (Học sinh Lớp 11) |

---

## 7. Command-Line Build (CLI / Terminal)

If you prefer building and running from the command line:

### Using CMake Presets (CMake 3.20+)
```powershell
# Setup environment (adjust Qt/MinGW paths if necessary)
$env:PATH = "C:\Qt\6.11.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;" + $env:PATH

# Configure and build Debug target
cmake --preset default
cmake --build --preset default -j4
```

### Manual CMake Commands
```powershell
# 1. Configure build directory
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# 2. Build all 3 targets
cmake --build build --config Debug -j4

# 3. Launch GUI
.\build\TutorMatch_GUI.exe
```

---

## 8. Running Tests & Forensic Inspector

### Automated Verification Test Suite (`TutorMatch_Tests`)
TutorMatch features an exhaustive, zero-dependency C++ verification runner with **34 comprehensive test suites** covering:
- Open/Closed & Liskov Substitution Principles (OCP / LSP)
- Dependency Inversion & Single Responsibility (DIP / ISP / SRP)
- 56-slot bitmask math, phantom bit protection, and collision detection
- AVL Tree multi-value indexing & deletion
- Atomic persistence, self-healing rating reconciliation, and multi-slot course workflows
- SmartMatch mandatory subject filtering and semantic search engine ranking
- Complete tutor verification decommission invariants
- Generic Data Structures & Algorithmic Invariants (HashTable, AVLTree, LRUCache, ScheduleGuard, MaxHeap, DoublyLinkedList)
- Unified Root Admin authorization & permission level removal
- Tutor accumulated income single source of truth & startup reconciliation
- Course registration start-date chronological calculation & order invariance
- Consensual early termination workflows & completion guard protection

```powershell
.\build\TutorMatch_Tests.exe
```

Expected output:
```text
============================================================
    TUTORMATCH SOLID ARCHITECTURE VERIFICATION TEST RUNNER   
============================================================
...
  >>> ALL TEST SUITES PASSED SUCCESSFULLY! (100% PASS) <<<  
============================================================
```

### Data Storage Forensic Inspector (`TutorMatch_Inspector`)
To inspect the internal structure, field layouts, and validation status of all storage records in read-only mode:

```powershell
# Full summary of all data files
.\build\TutorMatch_Inspector.exe --all

# Inspect specific records
.\build\TutorMatch_Inspector.exe --users
.\build\TutorMatch_Inspector.exe --course-bookings
.\build\TutorMatch_Inspector.exe --reviews
```

---

## 9. Project Directory Layout

```text
PBL2-TutorMatch/
├── CMakeLists.txt              # Top-level portable CMake build script
├── CMakePresets.json           # Standardized CMake configure & build presets
├── README.md                   # Project documentation & evaluator quick-start
├── data/
│   ├── .gitkeep                # Tracks data directory structure
│   ├── archive/                # Static historical audit archives (legacy bookings/users)
│   └── seed/                   # Pristine version-controlled seed dataset
│       ├── users.dat           # 52 standardized seed user records
│       ├── course_bookings.dat # 23 seed course booking records
│       └── reviews.dat         # 15 seed course review records
├── docs/                       # Architecture Decision Records (ADRs) & reports
├── include/                    # C++ Header Files (Strict Three-Layer Architecture)
│   ├── presentation/           # 1. Presentation Layer
│   │   └── bridges/            # GUIBridge QObject controller
│   ├── business/               # 2. Business Logic Layer
│   │   ├── domain/             # User, CourseBooking, BookingDTOs, Review, DateUtils
│   │   ├── services/           # AuthService, BookingService, UserService, ReviewService, MatchingService, AdminService
│   │   ├── matching/           # MatchingEngine, IMatchingStrategy, SubjectTaxonomy, TutorSearchMatcher
│   │   └── algorithms/         # ScheduleGuard, AVLTree, HashTable, LRUCache, MaxHeap, DoublyLinkedList
│   └── data_access/            # 3. Data Access Layer
│       ├── repositories/       # IRepository, FileUserRepository, FileCourseBookingRepository, FileReviewRepository
│       ├── serializers/        # SerializerUtils, UserSerializer, CourseBookingSerializer, ReviewSerializer
│       └── storage/            # MockDatabaseEngine, ApplicationPaths, DataFileInspector
├── src/                        # C++ Implementation Files
│   ├── presentation/           # gui_main.cpp, bridges/GUIBridge.cpp
│   ├── business/               # domain/, services/, matching/, algorithms/
│   ├── data_access/            # repositories/, serializers/, storage/
│   └── tools/                  # inspect_main.cpp (TutorMatch_Inspector CLI entry point)
├── tests/                      # TestRunner.cpp (34 verification test suites)
└── ui/
    └── qml/                    # Qt Quick / QML components, views, dialogs, and Theme
```

---

## 10. Troubleshooting & FAQ

<details>
<summary><b>1. Qt Creator says "No suitable kits found" or "CMake configuration failed"</b></summary>

- Ensure you have installed **Qt 6.2 or newer** with the **MinGW 64-bit** component selected in the Qt Online Installer.
- Open Qt Creator **Edit** > **Preferences** > **Kits** and verify that your MinGW compiler (GCC 13+) and Qt 6.x.x MinGW 64-bit version are detected.
</details>

<details>
<summary><b>2. Application launches with black screen or QML module errors</b></summary>

- TutorMatch utilizes `qt_add_qml_module` with the URI `TutorMatch` and requires `QT_QUICK_CONTROLS_STYLE=Basic`, which is automatically set in `src/presentation/gui_main.cpp`.
- If running from outside Qt Creator, ensure Qt's `bin/` directory and QML plugin directory are accessible, or run the application from within Qt Creator.
</details>

<details>
<summary><b>3. How do I reset the application data to initial sample state?</b></summary>

- Simply delete the active runtime files `data/users.dat`, `data/course_bookings.dat`, and `data/reviews.dat` in your project `data/` folder.
- On the next launch, `MockDatabaseEngine` will automatically re-seed pristine copies from `data/seed/`.
</details>

---

## 11. Author & Acknowledgments

- **Author / Developer:** Lê Thị Hồng Ngọc Yến Trang ([@ravyetr](https://github.com/ravyetr))
- **Student ID:** 102230103
- **Class:** 23T_DT2
- **Supervisor:** TS. Ninh Khánh Duy
- **Course:** Project Based Learning 2 (PBL2) — Học kỳ I, Năm học 2024–2025
- **Faculty:** Khoa Công nghệ Thông tin (IT)
- **Institution:** Trường Đại học Bách Khoa — Đại học Đà Nẵng (DUT)
