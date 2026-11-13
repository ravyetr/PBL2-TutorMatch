#include "data_access/serializers/SerializerUtils.h"
#include <sstream>

namespace TutorMatch {

std::string SerializerUtils::escapeCsv(const std::string& str) {
    std::string res = str;
    size_t pos = 0;
    while ((pos = res.find(',', pos)) != std::string::npos) {
        res.replace(pos, 1, " ");
        pos += 1;
    }
    return res;
}

std::vector<std::string> SerializerUtils::splitCsvLine(const std::string& csvLine) {
    std::string line = csvLine;
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
    }
    if (line.empty()) {
        return {};
    }
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        tokens.push_back(item);
    }
    if (line.back() == ',') {
        tokens.push_back("");
    }
    return tokens;
}

bool SerializerUtils::safeStringToInt(const std::string& str, int& outVal) {
    if (str.empty()) return false;
    try {
        size_t idx = 0;
        int val = std::stoi(str, &idx);
        for (; idx < str.size(); ++idx) {
            if (str[idx] != ' ' && str[idx] != '\r' && str[idx] != '\t' && str[idx] != '\n') {
                return false;
            }
        }
        outVal = val;
        return true;
    } catch (...) {
        return false;
    }
}

bool SerializerUtils::safeStringToDouble(const std::string& str, double& outVal) {
    if (str.empty()) return false;
    try {
        size_t idx = 0;
        double val = std::stod(str, &idx);
        for (; idx < str.size(); ++idx) {
            if (str[idx] != ' ' && str[idx] != '\r' && str[idx] != '\t' && str[idx] != '\n') {
                return false;
            }
        }
        outVal = val;
        return true;
    } catch (...) {
        return false;
    }
}

bool SerializerUtils::safeStringToUll(const std::string& str, uint64_t& outVal) {
    if (str.empty()) return false;
    size_t firstNonSpace = str.find_first_not_of(" \t\r\n");
    if (firstNonSpace != std::string::npos && str[firstNonSpace] == '-') {
        return false;
    }
    try {
        size_t idx = 0;
        unsigned long long val = std::stoull(str, &idx);
        for (; idx < str.size(); ++idx) {
            if (str[idx] != ' ' && str[idx] != '\r' && str[idx] != '\t' && str[idx] != '\n') {
                return false;
            }
        }
        outVal = static_cast<uint64_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace TutorMatch
