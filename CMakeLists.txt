cmake_minimum_required(VERSION 3.16)
project(TutorMatch_GUI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bật tính năng tự động biên dịch MOC, RCC và UIC của Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tìm các module Qt6 cần thiết
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

# Thêm thư mục include của dự án
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ============================================================================
# C++ Mã nguồn Backend — Kiến trúc 3 lớp (Strict Three-Layer Architecture)
# ============================================================================
set(CORE_SOURCES
    # 1. Business Logic Layer — Domain
    src/business/domain/User.cpp
    src/business/domain/DateUtils.cpp

    # 1. Business Logic Layer — Matching
    src/business/matching/SubjectTaxonomy.cpp
    src/business/matching/MatchingEngine.cpp
    src/business/matching/TutorSearchMatcher.cpp

    # 1. Business Logic Layer — Algorithms
    src/business/algorithms/ScheduleGuard.cpp

    # 1. Business Logic Layer — Services
    src/business/services/AuthService.cpp
    src/business/services/UserService.cpp
    src/business/services/BookingService.cpp
    src/business/services/ReviewService.cpp
    src/business/services/MatchingService.cpp
    src/business/services/AdminService.cpp

    # 2. Data Access Layer — Storage & Path Management
    src/data_access/storage/ApplicationPaths.cpp
    src/data_access/storage/MockDatabaseEngine.cpp
    src/data_access/storage/DataFileInspector.cpp

    # 2. Data Access Layer — Serializers
    src/data_access/serializers/SerializerUtils.cpp
    src/data_access/serializers/UserSerializer.cpp
    src/data_access/serializers/CourseBookingSerializer.cpp
    src/data_access/serializers/ReviewSerializer.cpp

    # 2. Data Access Layer — Repositories
    src/data_access/repositories/FileUserRepository.cpp
    src/data_access/repositories/FileCourseBookingRepository.cpp
    src/data_access/repositories/FileReviewRepository.cpp

    # 3. Presentation Layer — Bridges
    src/presentation/bridges/GUIBridge.cpp
)

# File main C++ cho ứng dụng Qt Quick GUI
set(GUI_SOURCES
    ${CORE_SOURCES}
    src/presentation/gui_main.cpp
)

# Danh sách Header Files — Kiến trúc 3 lớp
set(HEADERS
    # Presentation Layer
    include/presentation/bridges/GUIBridge.h

    # Business Logic Layer — Domain
    include/business/domain/User.h
    include/business/domain/CourseBooking.h
    include/business/domain/BookingDTOs.h
    include/business/domain/Review.h
    include/business/domain/DateUtils.h

    # Business Logic Layer — Services
    include/business/services/AuthService.h
    include/business/services/UserService.h
    include/business/services/BookingService.h
    include/business/services/ReviewService.h
    include/business/services/MatchingService.h
    include/business/services/AdminService.h

    # Business Logic Layer — Matching
    include/business/matching/IMatchingStrategy.h
    include/business/matching/MatchingEngine.h
    include/business/matching/SubjectTaxonomy.h
    include/business/matching/TutorSearchMatcher.h

    # Business Logic Layer — Algorithms & Generic Data Structures
    include/business/algorithms/ScheduleGuard.h
    include/business/algorithms/HashTable.h
    include/business/algorithms/DoublyLinkedList.h
    include/business/algorithms/LRUCache.h
    include/business/algorithms/AVLTree.h
    include/business/algorithms/MaxHeap.h

    # Data Access Layer — Repositories
    include/data_access/repositories/IRepository.h
    include/data_access/repositories/FileUserRepository.h
    include/data_access/repositories/FileCourseBookingRepository.h
    include/data_access/repositories/FileReviewRepository.h

    # Data Access Layer — Serializers
    include/data_access/serializers/SerializerUtils.h
    include/data_access/serializers/UserSerializer.h
    include/data_access/serializers/CourseBookingSerializer.h
    include/data_access/serializers/ReviewSerializer.h

    # Data Access Layer — Storage
    include/data_access/storage/ApplicationPaths.h
    include/data_access/storage/MockDatabaseEngine.h
    include/data_access/storage/DataFileInspector.h
)

# Tệp giao diện QML
set(QML_FILES
    ui/qml/Theme.qml
    ui/qml/main.qml
    ui/qml/LoginView.qml
    ui/qml/MainDashboard.qml
    ui/qml/AdminOverviewView.qml
    ui/qml/AdminUsersView.qml
    ui/qml/AdminMatchesView.qml
    ui/qml/AdminReviewsView.qml
    ui/qml/StudentOverviewView.qml
    ui/qml/StudentProfileCard.qml
    ui/qml/ProfileEditModal.qml
    ui/qml/TimetableGrid.qml
    ui/qml/TimetableCell.qml
    ui/qml/StudentTutorSearchView.qml
    ui/qml/StudentSmartMatchView.qml
    ui/qml/TutorCard.qml
    ui/qml/TutorDetailModal.qml
    ui/qml/BookingModal.qml
    ui/qml/ReviewModal.qml
    ui/qml/StudentBookingsView.qml
    ui/qml/TutorOverview.qml
    ui/qml/TutorBookings.qml
    ui/qml/TutorStudents.qml
    ui/qml/TutorStatistics.qml
    ui/qml/SegmentedFilterButton.qml
    ui/qml/UserDetailModal.qml
)

# Đánh dấu Theme.qml là Singleton type cho Qt QML Engine
set_source_files_properties(ui/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Tạo Target Thực thi và QML Module cho Qt6
if (${Qt6_VERSION} VERSION_GREATER_EQUAL "6.2.0")
    qt_add_executable(TutorMatch_GUI
        ${GUI_SOURCES}
        ${HEADERS}
    )
    qt_add_qml_module(TutorMatch_GUI
        URI TutorMatch
        VERSION 1.0
        QML_FILES ${QML_FILES}
    )
else()
    add_executable(TutorMatch_GUI
        ${GUI_SOURCES}
        ${HEADERS}
        ${QML_FILES}
    )
endif()

# Pass Project Source Directory Macro to C++ Runtime
target_compile_definitions(TutorMatch_GUI PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Link các thư viện Qt6 vào ứng dụng
target_link_libraries(TutorMatch_GUI PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# Automatically Sync Canonical Data Directory to Output Directory on Build
add_custom_command(TARGET TutorMatch_GUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:TutorMatch_GUI>/data
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:TutorMatch_GUI>
        ${CMAKE_CURRENT_SOURCE_DIR}/TutorMatch_GUI.exe
    COMMENT "Syncing canonical data directory and executable to root project directory"
)

if (WIN32)
    set_target_properties(TutorMatch_GUI PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ============================================================================
# Unit & Integration Tests Target (SOLID Verification)
# ============================================================================
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestRunner.cpp")
    add_executable(TutorMatch_Tests
        tests/TestRunner.cpp
        ${CORE_SOURCES}
        include/presentation/bridges/GUIBridge.h
    )

    target_compile_definitions(TutorMatch_Tests PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(TutorMatch_Tests PRIVATE
        Qt6::Core
    )
endif()

# ============================================================================
# Standalone Data File Diagnostic Inspector Tool
# ============================================================================
add_executable(TutorMatch_Inspector
    src/tools/inspect_main.cpp
    src/data_access/storage/DataFileInspector.cpp
    src/data_access/storage/ApplicationPaths.cpp
    src/business/algorithms/ScheduleGuard.cpp
)

target_compile_definitions(TutorMatch_Inspector PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(TutorMatch_Inspector PRIVATE
    Qt6::Core
)
