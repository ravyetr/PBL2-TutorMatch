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

# Các file mã nguồn C++ Backend
set(CORE_SOURCES
    src/models/User.cpp
    src/models/SubjectTaxonomy.cpp
    src/ds/ScheduleGuard.cpp
    src/utils/DateUtils.cpp
    src/storage/ApplicationPaths.cpp
    src/storage/MockDatabaseEngine.cpp
    src/storage/DataFileInspector.cpp
    src/storage/serializers/SerializerUtils.cpp
    src/storage/serializers/UserSerializer.cpp
    src/storage/serializers/CourseBookingSerializer.cpp
    src/storage/serializers/ReviewSerializer.cpp
    src/storage/repositories/FileUserRepository.cpp
    src/storage/repositories/FileCourseBookingRepository.cpp
    src/storage/repositories/FileReviewRepository.cpp
    src/algorithms/MatchingEngine.cpp
    src/algorithms/TutorSearchMatcher.cpp
    src/services/AuthService.cpp
    src/services/UserService.cpp
    src/services/BookingService.cpp
    src/services/ReviewService.cpp
    src/services/MatchingService.cpp
    src/services/AdminService.cpp
    src/gui/GUIBridge.cpp
)

# File main C++ cho ứng dụng Qt
set(GUI_SOURCES
    ${CORE_SOURCES}
    src/gui/main.cpp
)

# File Header
set(HEADERS
    include/ds/HashTable.h
    include/ds/DoublyLinkedList.h
    include/ds/LRUCache.h
    include/ds/AVLTree.h
    include/ds/MaxHeap.h
    include/ds/ScheduleGuard.h
    include/models/User.h
    include/models/SubjectTaxonomy.h
    include/models/CourseBooking.h
    include/models/BookingDTOs.h
    include/models/Review.h
    include/utils/DateUtils.h
    include/storage/IRepository.h
    include/storage/ApplicationPaths.h
    include/storage/MockDatabaseEngine.h
    include/storage/DataFileInspector.h
    include/storage/serializers/SerializerUtils.h
    include/storage/serializers/UserSerializer.h
    include/storage/serializers/CourseBookingSerializer.h
    include/storage/serializers/ReviewSerializer.h
    include/storage/repositories/FileUserRepository.h
    include/storage/repositories/FileCourseBookingRepository.h
    include/storage/repositories/FileReviewRepository.h
    include/algorithms/IMatchingStrategy.h
    include/algorithms/MatchingEngine.h
    include/algorithms/TutorSearchMatcher.h
    include/services/AuthService.h
    include/services/UserService.h
    include/services/BookingService.h
    include/services/ReviewService.h
    include/services/MatchingService.h
    include/services/AdminService.h
    include/gui/GUIBridge.h
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
        src/models/User.cpp
        src/models/SubjectTaxonomy.cpp
        src/ds/ScheduleGuard.cpp
        src/utils/DateUtils.cpp
        src/storage/ApplicationPaths.cpp
        src/storage/MockDatabaseEngine.cpp
        src/storage/DataFileInspector.cpp
        src/storage/serializers/SerializerUtils.cpp
        src/storage/serializers/UserSerializer.cpp
        src/storage/serializers/CourseBookingSerializer.cpp
        src/storage/serializers/ReviewSerializer.cpp
        src/storage/repositories/FileUserRepository.cpp
        src/storage/repositories/FileCourseBookingRepository.cpp
        src/storage/repositories/FileReviewRepository.cpp
        src/algorithms/MatchingEngine.cpp
        src/algorithms/TutorSearchMatcher.cpp
        src/services/AuthService.cpp
        src/services/UserService.cpp
        src/services/BookingService.cpp
        src/services/ReviewService.cpp
        src/services/MatchingService.cpp
        src/services/AdminService.cpp
        include/gui/GUIBridge.h
        src/gui/GUIBridge.cpp
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
    src/storage/DataFileInspector.cpp
    src/storage/ApplicationPaths.cpp
    src/ds/ScheduleGuard.cpp
)

target_compile_definitions(TutorMatch_Inspector PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(TutorMatch_Inspector PRIVATE
    Qt6::Core
)

# ============================================================================
# Standalone Console Application CLI Target
# ============================================================================
add_executable(TutorMatch_CLI
    src/main.cpp
    src/ui/ConsoleApp.cpp
    ${CORE_SOURCES}
    ${HEADERS}
)

target_compile_definitions(TutorMatch_CLI PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(TutorMatch_CLI PRIVATE
    Qt6::Core
)