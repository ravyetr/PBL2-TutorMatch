#include "data_access/storage/ApplicationPaths.h"

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
        if (QDir(appDataDir).exists() && (QFileInfo::exists(appDataDir + "/users.dat") || 
                                          QDir(appDataDir + "/seed").exists() || 
                                          QFileInfo::exists(appDataDir + "/.gitkeep"))) {
            std::cout << "[ApplicationPaths] Resolved to Executable Data Dir: " 
                      << appDataDir.toStdString() << std::endl;
            return appDataDir.toStdString();
        }

        // 4. Search parent directories of executable for project "/data" (up to 4 levels)
        QDir searchDir(appDir);
        for (int i = 0; i < 4; ++i) {
            searchDir.cdUp();
            QString parentDataDir = searchDir.absoluteFilePath("data");
            if (QDir(parentDataDir).exists() && (QFileInfo::exists(parentDataDir + "/users.dat") || 
                                                 QDir(parentDataDir + "/seed").exists() || 
                                                 QFileInfo::exists(parentDataDir + "/.gitkeep"))) {
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

std::string ApplicationPaths::resolveSeedDirectory(const std::string& hint) {
    // 1. Explicit hint check
    if (!hint.empty()) {
        std::filesystem::path p(hint);
        if (p.filename() == "seed" && std::filesystem::exists(p)) {
            return std::filesystem::absolute(p).string();
        }
        std::filesystem::path candidate = p / "seed";
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::absolute(candidate).string();
        }
        // If hint is a custom test directory (not canonical "data") without its own seed dir,
        // do not force global seed on it
        if (hint != "data") {
            std::filesystem::path canonicalData(resolveDataDirectory("data"));
            std::error_code ec;
            if (std::filesystem::absolute(p, ec) != std::filesystem::absolute(canonicalData, ec)) {
                return "";
            }
        }
    }

    // 2. Relative to canonical data directory
    std::string dataDir = resolveDataDirectory("data");
    std::filesystem::path seedInData = std::filesystem::path(dataDir) / "seed";
    if (std::filesystem::exists(seedInData)) {
        return seedInData.string();
    }

    // 3. Dev Mode via PROJECT_SOURCE_DIR macro
#ifdef PROJECT_SOURCE_DIR
    QString sourceSeedDir = QString::fromUtf8(PROJECT_SOURCE_DIR) + "/data/seed";
    if (QDir(sourceSeedDir).exists()) {
        return sourceSeedDir.toStdString();
    }
#endif

    // 4. Executable relative search
    QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        QString appSeedDir = appDir + "/data/seed";
        if (QDir(appSeedDir).exists()) {
            return appSeedDir.toStdString();
        }
        QDir searchDir(appDir);
        for (int i = 0; i < 4; ++i) {
            searchDir.cdUp();
            QString parentSeedDir = searchDir.absoluteFilePath("data/seed");
            if (QDir(parentSeedDir).exists()) {
                return parentSeedDir.toStdString();
            }
        }
    }

    return "";
}

} // namespace TutorMatch
