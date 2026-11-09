#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QIcon>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <memory>
#include <iostream>

#include "storage/MockDatabaseEngine.h"
#include "storage/ApplicationPaths.h"
#include "gui/GUIBridge.h"
#include "storage/DataFileInspector.h"
#include "storage/repositories/FileUserRepository.h"
#include "storage/repositories/FileReviewRepository.h"
#include "services/AuthService.h"
#include "services/UserService.h"
#include "services/BookingService.h"
#include "services/ReviewService.h"
#include "services/MatchingService.h"
#include "services/AdminService.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--inspect-data" || arg == "--inspect" || arg == "-i") {
#ifdef _WIN32
            if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                freopen("CONOUT$", "w", stdout);
                freopen("CONOUT$", "w", stderr);
                freopen("CONIN$", "r", stdin);
                SetConsoleOutputCP(CP_UTF8);
                SetConsoleCP(CP_UTF8);
            }
#endif
            TutorMatch::DataFileInspector::inspectAll();
            return 0;
        }
    }

    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QGuiApplication app(argc, argv);
    app.setOrganizationName("TutorMatch");
    app.setOrganizationDomain("tutormatch.vn");
    app.setApplicationName("TutorMatch_GUI");

    qDebug() << "================================================";
    qDebug() << "TUTORMATCH_FORENSIC_BUILD_2026_10_01_V9";
    qDebug() << "EXECUTABLE:" << QCoreApplication::applicationFilePath();
    qDebug() << "WORKING_DIR:" << QDir::currentPath();
    qDebug() << "================================================";

    // Initialize Core Storage Engine & GUIBridge Controller with resolved predictable data path
    std::string dataDir = TutorMatch::ApplicationPaths::resolveDataDirectory();
    std::cout << "[TutorMatch_GUI] Starting app with Data Directory: " << dataDir << std::endl;

    // Composition Root: 1. Storage Repositories & Facade
    auto dbEngine = std::make_shared<TutorMatch::MockDatabaseEngine>(dataDir);
    dbEngine->initialize();

    auto userRepo = dbEngine->getUserRepository();
    auto reviewRepo = dbEngine->getReviewRepository();
    auto courseBookingRepo = dbEngine->getCourseBookingRepository();

    // Composition Root: 2. Core Application Services
    auto authService = std::make_shared<TutorMatch::AuthService>(userRepo);
    auto userService = std::make_shared<TutorMatch::UserService>(userRepo);
    auto bookingService = std::make_shared<TutorMatch::BookingService>(userRepo, reviewRepo, courseBookingRepo);
    auto reviewService = std::make_shared<TutorMatch::ReviewService>(reviewRepo, userRepo, courseBookingRepo);
    auto matchingService = std::make_shared<TutorMatch::MatchingService>(userRepo);
    auto adminService = std::make_shared<TutorMatch::AdminService>(userRepo, reviewRepo, bookingService);

    // ADR-007 Startup Reconciliation: re-evaluate tutor ratings from canonical reviews
    reviewService->reconcileRatingsOnStartup();

    // Composition Root: 3. GUIBridge Presentation Adapter
    auto bridge = std::make_shared<TutorMatch::GUIBridge>();
    bridge->initialize(authService, userService, bookingService, reviewService, matchingService, adminService);

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");
    engine.addImportPath("qrc:/qt/qml");

    // Register GUIBridge instance to QML Context
    engine.rootContext()->setContextProperty("guiBridge", bridge.get());

    // Load QML main window directly from registered module path
    const QUrl mainUrl(QStringLiteral("qrc:/TutorMatch/ui/qml/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &err : warnings) {
            std::cerr << "[QML WARNING] " << err.toString().toStdString() << std::endl;
        }
    });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                std::cerr << "[QML ERROR] Failed to load QML root object from: " << objUrl.toString().toStdString() << std::endl;
            }
        },
        Qt::QueuedConnection);

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        std::cout << "[TutorMatch_GUI] Loading QML fallback from filesystem ui/qml/main.qml" << std::endl;
        engine.load(QUrl::fromLocalFile("ui/qml/main.qml"));
    }

    return app.exec();
}