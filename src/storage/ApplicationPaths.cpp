#include "../../include/storage/ApplicationPaths.h"

#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <filesystem>

namespace TutorMatch {

std::string ApplicationPaths::resolveDataDirectory(const std::string& hint) {
    // 1. If explicit path passed as hint, resolve and use it
    if (!hint.empty() && hint != "data") {
        std::filesystem::path p(hint);
        return std::filesystem::absolute(p).string();
    }

    // 2. Check if compile macro PROJECT_SOURCE_DIR is available (Dev Mode)
#ifdef PROJECT_SOURCE_DIR
    QString sourceDataDir = QString::fromUtf8(PROJECT_SOURCE_DIR) + "/data";
    if (QDir(sourceDataDir).exists()) {
        std::cout << "[ApplicationPaths] Resolved to Project Source Data Dir: " 
                  << sourceDataDir.toStdString() << std::endl;
        return sourceDataDir.toStdString();
    }
#endif

    // 3. Check executable directory + "/data"
    QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        QString appDataDir = appDir + "/data";
        if (QDir(appDataDir).exists() && QFileInfo::exists(appDataDir + "/bookings.dat")) {
            std::cout << "[ApplicationPaths] Resolved to Executable Data Dir: " 
                      << appDataDir.toStdString() << std::endl;
            return appDataDir.toStdString();
        }

        // 4. Search parent directories of executable for project "/data" (up to 4 levels)
        QDir searchDir(appDir);
        for (int i = 0; i < 4; ++i) {
            searchDir.cdUp();
            QString parentDataDir = searchDir.absoluteFilePath("data");
            if (QDir(parentDataDir).exists() && QFileInfo::exists(parentDataDir + "/bookings.dat")) {
                std::cout << "[ApplicationPaths] Resolved to Ancestor Data Dir: " 
                          << parentDataDir.toStdString() << std::endl;
                return parentDataDir.toStdString();
            }
        }

        // Fallback: appDir + "/data"
        return appDataDir.toStdString();
    }

    // Ultimate fallback if QCoreApplication is not initialized yet
    std::filesystem::path cwdPath = std::filesystem::current_path() / "data";
    return cwdPath.string();
}

} // namespace TutorMatch
