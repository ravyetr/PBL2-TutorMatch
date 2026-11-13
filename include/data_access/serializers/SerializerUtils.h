#ifndef TUTOR_MATCH_SERIALIZER_UTILS_H
#define TUTOR_MATCH_SERIALIZER_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace TutorMatch {

/**
 * @brief Utility helpers for CSV serialization, token parsing, and type conversions.
 *
 * Encapsulates common string manipulation logic matching legacy TutorMatch rules:
 * - escapeCsv: replaces ',' with ' ' within field contents (preserves legacy comma delimiter).
 * - splitCsvLine: tokenizes a CSV line by ',', properly trimming trailing CR/LF.
 * - safeStringToInt / Double / Ull: non-throwing, robust type converters.
 */
class SerializerUtils {
public:
    static std::string escapeCsv(const std::string& str);
    static std::vector<std::string> splitCsvLine(const std::string& csvLine);
    static bool safeStringToInt(const std::string& str, int& outVal);
    static bool safeStringToDouble(const std::string& str, double& outVal);
    static bool safeStringToUll(const std::string& str, uint64_t& outVal);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_SERIALIZER_UTILS_H
