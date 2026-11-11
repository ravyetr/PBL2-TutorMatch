#ifndef TUTOR_MATCH_APPLICATION_PATHS_H
#define TUTOR_MATCH_APPLICATION_PATHS_H

#include <string>

namespace TutorMatch {

class ApplicationPaths {
public:
    /**
     * Resolves the predictable, canonical data directory for TutorMatch.
     * Prevents reliance on Current Working Directory (CWD).
     * 
     * Priority:
     * 1. If explicit non-empty absolute path is provided and valid, use it.
     * 2. If PROJECT_SOURCE_DIR compiler macro is set and PROJECT_SOURCE_DIR/data exists, use it (Dev Mode).
     * 3. Check application executable directory + "/data" (Standalone Deployed Mode).
     * 4. Check parent directories of executable up to 4 levels for "/data".
     * 5. Fallback to executable directory + "/data".
     */
    static std::string resolveDataDirectory(const std::string& hint = "");

    /**
     * Resolves the version-controlled seed data directory for TutorMatch.
     * Contains deterministic, sanitized initial data files (users.dat, course_bookings.dat, reviews.dat).
     */
    static std::string resolveSeedDirectory(const std::string& dataDirHint = "");
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_APPLICATION_PATHS_H
