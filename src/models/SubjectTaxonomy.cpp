#include "models/SubjectTaxonomy.h"
#include "models/User.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace TutorMatch {

std::string SubjectTaxonomy::stripAccents(const std::string& input) {
    std::string str = input;
    static const std::vector<std::pair<std::string, char>> ACCENT_REPLACEMENTS = {
        {"à", 'a'}, {"á", 'a'}, {"ả", 'a'}, {"ã", 'a'}, {"ạ", 'a'},
        {"ă", 'a'}, {"ằ", 'a'}, {"ắ", 'a'}, {"ẳ", 'a'}, {"ẵ", 'a'}, {"ặ", 'a'},
        {"â", 'a'}, {"ầ", 'a'}, {"ấ", 'a'}, {"ẩ", 'a'}, {"ẫ", 'a'}, {"ậ", 'a'},
        {"À", 'a'}, {"Á", 'a'}, {"Ả", 'a'}, {"Ã", 'a'}, {"Ạ", 'a'},
        {"Ă", 'a'}, {"Ằ", 'a'}, {"Ắ", 'a'}, {"Ẳ", 'a'}, {"Ẵ", 'a'}, {"Ặ", 'a'},
        {"Â", 'a'}, {"Ầ", 'a'}, {"Ấ", 'a'}, {"Ẩ", 'a'}, {"Ẫ", 'a'}, {"Ậ", 'a'},
        {"đ", 'd'}, {"Đ", 'd'},
        {"è", 'e'}, {"é", 'e'}, {"ẻ", 'e'}, {"ẽ", 'e'}, {"ẹ", 'e'},
        {"ê", 'e'}, {"ề", 'e'}, {"ế", 'e'}, {"ể", 'e'}, {"ễ", 'e'}, {"ệ", 'e'},
        {"È", 'e'}, {"É", 'e'}, {"Ẻ", 'e'}, {"Ẽ", 'e'}, {"Ẹ", 'e'},
        {"Ê", 'e'}, {"Ề", 'e'}, {"Ế", 'e'}, {"Ể", 'e'}, {"Ễ", 'e'}, {"Ệ", 'e'},
        {"ì", 'i'}, {"í", 'i'}, {"ỉ", 'i'}, {"ĩ", 'i'}, {"ị", 'i'},
        {"Ì", 'i'}, {"Í", 'i'}, {"Ỉ", 'i'}, {"Ĩ", 'i'}, {"Ị", 'i'},
        {"ò", 'o'}, {"ó", 'o'}, {"ỏ", 'o'}, {"õ", 'o'}, {"ọ", 'o'},
        {"ô", 'o'}, {"ồ", 'o'}, {"ố", 'o'}, {"ổ", 'o'}, {"ỗ", 'o'}, {"ộ", 'o'},
        {"ơ", 'o'}, {"ờ", 'o'}, {"ớ", 'o'}, {"ở", 'o'}, {"ỡ", 'o'}, {"ợ", 'o'},
        {"Ò", 'o'}, {"Ó", 'o'}, {"Ỏ", 'o'}, {"Õ", 'o'}, {"Ọ", 'o'},
        {"Ô", 'o'}, {"Ồ", 'o'}, {"Ố", 'o'}, {"Ổ", 'o'}, {"Ỗ", 'o'}, {"Ộ", 'o'},
        {"Ơ", 'o'}, {"Ờ", 'o'}, {"Ớ", 'o'}, {"Ở", 'o'}, {"Ỡ", 'o'}, {"Ợ", 'o'},
        {"ù", 'u'}, {"ú", 'u'}, {"ủ", 'u'}, {"ũ", 'u'}, {"ụ", 'u'},
        {"ư", 'u'}, {"ừ", 'u'}, {"ứ", 'u'}, {"ử", 'u'}, {"ữ", 'u'}, {"ự", 'u'},
        {"Ù", 'u'}, {"Ú", 'u'}, {"Ủ", 'u'}, {"Ũ", 'u'}, {"Ụ", 'u'},
        {"Ư", 'u'}, {"Ừ", 'u'}, {"Ứ", 'u'}, {"Ử", 'u'}, {"Ữ", 'u'}, {"Ự", 'u'},
        {"ỳ", 'y'}, {"ý", 'y'}, {"ỷ", 'y'}, {"ỹ", 'y'}, {"ỵ", 'y'},
        {"Ỳ", 'y'}, {"Ý", 'y'}, {"Ỷ", 'y'}, {"Ỹ", 'y'}, {"Ỵ", 'y'}
    };

    for (const auto& item : ACCENT_REPLACEMENTS) {
        size_t pos = 0;
        while ((pos = str.find(item.first, pos)) != std::string::npos) {
            str.replace(pos, item.first.length(), 1, item.second);
            pos += 1;
        }
    }
    return str;
}

std::string SubjectTaxonomy::normalizeText(const std::string& input) {
    std::string s = stripAccents(input);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    for (char& c : s) {
        if (c == '_' || c == '-' || c == '/' || c == ',' || c == '(' || c == ')' ||
            c == ':' || c == '&' || c == '+' || c == ';' || c == '*' || c == '?' ||
            c == '!' || c == '#' || c == '[' || c == ']' || c == '{' || c == '}') {
            c = ' ';
        }
    }
    std::string res;
    bool inSpace = false;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!inSpace) {
                res += ' ';
                inSpace = true;
            }
        } else {
            res += c;
            inSpace = false;
        }
    }
    if (!res.empty() && res.front() == ' ') res.erase(res.begin());
    if (!res.empty() && res.back() == ' ') res.pop_back();
    return res;
}

static bool containsKeyword(const std::string& text, const std::string& kw) {
    return text.find(kw) != std::string::npos;
}

std::vector<CanonicalSubject> SubjectTaxonomy::getCanonicalSubjects(const std::string& subjectStr) {
    std::vector<CanonicalSubject> result;
    if (subjectStr.empty()) return result;

    std::string norm = normalizeText(subjectStr);
    if (norm.empty()) return result;

    auto containsWordOrPhrase = [](const std::string& text, const std::string& phrase) -> bool {
        if (phrase.empty() || text.empty()) return false;
        size_t pos = 0;
        while ((pos = text.find(phrase, pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || text[pos - 1] == ' ');
            size_t end = pos + phrase.length();
            bool rightOk = (end == text.length() || text[end] == ' ');
            if (leftOk && rightOk) return true;
            pos += 1;
        }
        return false;
    };

    // 1. MATH
    bool hasMath = containsKeyword(norm, "math") || containsKeyword(norm, "calculus") ||
                   containsKeyword(norm, "algebra") || containsKeyword(norm, "geometry") ||
                   containsKeyword(norm, "giai tich") || containsKeyword(norm, "dai so") ||
                   containsKeyword(norm, "hinh hoc");
    if (!hasMath) {
        size_t pos = 0;
        while ((pos = norm.find("toan", pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || norm[pos - 1] == ' ');
            size_t end = pos + 4;
            bool rightOk = (end == norm.length() || norm[end] == ' ');
            if (leftOk && rightOk) {
                bool isAnToan = (pos >= 3 && norm.substr(pos - 3, 3) == "an ");
                bool isHoanToan = (pos >= 5 && norm.substr(pos - 5, 5) == "hoan ");
                if (!isAnToan && !isHoanToan) {
                    hasMath = true;
                    break;
                }
            }
            pos += 1;
        }
    }
    if (hasMath) result.push_back(CanonicalSubject::MATH);

    // 2. PHYSICS
    bool hasPhys = containsKeyword(norm, "physics") || containsKeyword(norm, "vat ly") ||
                   containsKeyword(norm, "co hoc") || containsKeyword(norm, "quang hoc") ||
                   containsKeyword(norm, "dien tu");
    if (!hasPhys) {
        size_t pos = 0;
        while ((pos = norm.find("ly", pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || norm[pos - 1] == ' ');
            size_t end = pos + 2;
            bool rightOk = (end == norm.length() || norm[end] == ' ');
            if (leftOk && rightOk) {
                bool isDiaLy = (pos >= 4 && norm.substr(pos - 4, 4) == "dia ");
                bool isTamLy = (pos >= 4 && norm.substr(pos - 4, 4) == "tam ");
                bool isQuanLy = (pos >= 5 && norm.substr(pos - 5, 5) == "quan ");
                bool isDaiLy = (pos >= 4 && norm.substr(pos - 4, 4) == "dai ");
                bool isPhapLy = (pos >= 5 && norm.substr(pos - 5, 5) == "phap ");
                bool isHocLy = (pos >= 4 && norm.substr(pos - 4, 4) == "hoc ");
                if (!isDiaLy && !isTamLy && !isQuanLy && !isDaiLy && !isPhapLy && !isHocLy) {
                    hasPhys = true;
                    break;
                }
            }
            pos += 1;
        }
    }
    if (hasPhys) result.push_back(CanonicalSubject::PHYSICS);

    // 3. CHEMISTRY
    bool hasChem = containsWordOrPhrase(norm, "chemistry") || containsWordOrPhrase(norm, "hoa hoc") ||
                   containsWordOrPhrase(norm, "hoa huu co") || containsWordOrPhrase(norm, "hoa vo co") ||
                   containsWordOrPhrase(norm, "hoa");
    if (hasChem) result.push_back(CanonicalSubject::CHEMISTRY);

    // 4. ENGLISH
    bool hasEng = containsKeyword(norm, "english") || containsKeyword(norm, "tieng anh") ||
                  containsKeyword(norm, "ngoai ngu") || containsKeyword(norm, "ielts") ||
                  containsKeyword(norm, "toeic") || containsKeyword(norm, "toefl");
    if (hasEng) result.push_back(CanonicalSubject::ENGLISH);

    // 5. COMPUTER_SCIENCE
    bool hasCS = containsKeyword(norm, "computer science") || containsKeyword(norm, "tin hoc") ||
                 containsKeyword(norm, "lap trinh") || containsKeyword(norm, "programming") ||
                 containsKeyword(norm, "coding") || containsKeyword(norm, "c++") ||
                 containsKeyword(norm, "python") || containsKeyword(norm, "cntt") ||
                 containsWordOrPhrase(norm, "tin");
    if (hasCS) result.push_back(CanonicalSubject::COMPUTER_SCIENCE);

    // 6. BIOLOGY
    bool hasBio = containsKeyword(norm, "biology") || containsKeyword(norm, "sinh hoc");
    if (!hasBio) {
        size_t pos = 0;
        while ((pos = norm.find("sinh", pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || norm[pos - 1] == ' ');
            size_t end = pos + 4;
            bool rightOk = (end == norm.length() || norm[end] == ' ');
            if (leftOk && rightOk) {
                bool isHocSinh = (pos >= 4 && norm.substr(pos - 4, 4) == "hoc ");
                if (!isHocSinh) {
                    hasBio = true;
                    break;
                }
            }
            pos += 1;
        }
    }
    if (hasBio) result.push_back(CanonicalSubject::BIOLOGY);

    // 7. LITERATURE
    bool hasLit = containsKeyword(norm, "literature") || containsKeyword(norm, "van hoc") ||
                  containsKeyword(norm, "ngu van") || containsWordOrPhrase(norm, "van");
    if (hasLit) result.push_back(CanonicalSubject::LITERATURE);

    // 8. HISTORY
    bool hasHist = containsWordOrPhrase(norm, "history") || containsWordOrPhrase(norm, "lich su") ||
                   containsWordOrPhrase(norm, "su hoc");
    if (!hasHist) {
        size_t pos = 0;
        while ((pos = norm.find("su", pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || norm[pos - 1] == ' ');
            size_t end = pos + 2;
            bool rightOk = (end == norm.length() || norm[end] == ' ');
            if (leftOk && rightOk) {
                bool isKySu = (pos >= 3 && norm.substr(pos - 3, 3) == "ky ");
                bool isGiaoSu = (pos >= 5 && norm.substr(pos - 5, 5) == "giao ");
                bool isLuatSu = (pos >= 5 && norm.substr(pos - 5, 5) == "luat ");
                bool isBacSi = (pos >= 4 && norm.substr(pos - 4, 4) == "bac ");
                if (!isKySu && !isGiaoSu && !isLuatSu && !isBacSi) {
                    hasHist = true;
                    break;
                }
            }
            pos += 1;
        }
    }
    if (hasHist) result.push_back(CanonicalSubject::HISTORY);

    // 9. GEOGRAPHY
    bool hasGeo = containsWordOrPhrase(norm, "geography") || containsWordOrPhrase(norm, "dia ly") ||
                  containsWordOrPhrase(norm, "dia li") || containsWordOrPhrase(norm, "dia chat");
    if (!hasGeo) {
        size_t pos = 0;
        while ((pos = norm.find("dia", pos)) != std::string::npos) {
            bool leftOk = (pos == 0 || norm[pos - 1] == ' ');
            size_t end = pos + 3;
            bool rightOk = (end == norm.length() || norm[end] == ' ');
            if (leftOk && rightOk) {
                bool isDiaDiem = (end + 5 <= norm.length() && norm.substr(end, 5) == " diem");
                bool isDiaChi = (end + 4 <= norm.length() && norm.substr(end, 4) == " chi");
                bool isDiaPhuong = (end + 7 <= norm.length() && norm.substr(end, 7) == " phuong");
                if (!isDiaDiem && !isDiaChi && !isDiaPhuong) {
                    hasGeo = true;
                    break;
                }
            }
            pos += 1;
        }
    }
    if (hasGeo) result.push_back(CanonicalSubject::GEOGRAPHY);

    return result;
}

ParsedSubject SubjectTaxonomy::parseSubject(const std::string& subjectStr) {
    ParsedSubject res;
    res.rawInput = subjectStr;
    std::string norm = normalizeText(subjectStr);
    if (norm.empty()) {
        res.code = CanonicalSubject::OTHER;
        res.canonicalCodeStr = "OTHER";
        res.displayName = "Môn học khác";
        return res;
    }

    auto canonicals = getCanonicalSubjects(subjectStr);
    if (!canonicals.empty()) {
        res.code = canonicals.front();
    } else {
        res.code = CanonicalSubject::OTHER;
    }

    res.canonicalCodeStr = getCanonicalCodeString(res.code);
    res.displayName = getDisplayName(res.code);

    // Extract Grade Range
    if (containsKeyword(norm, "10 12") || containsKeyword(norm, "thpt")) {
        res.minGrade = 10;
        res.maxGrade = 12;
    } else if (containsKeyword(norm, "6 9") || containsKeyword(norm, "thcs")) {
        res.minGrade = 6;
        res.maxGrade = 9;
    } else {
        bool foundSpecific = false;
        for (int g = 12; g >= 1; --g) {
            std::string gStr = std::to_string(g);
            if (norm.find(gStr) != std::string::npos) {
                res.minGrade = g;
                res.maxGrade = g;
                foundSpecific = true;
                break;
            }
        }
        if (!foundSpecific) {
            res.minGrade = 1;
            res.maxGrade = 12;
        }
    }

    // Extract Program Target
    if (containsKeyword(norm, "dai hoc") || containsKeyword(norm, "luyen thi")) {
        res.targetProgram = "Luyện thi Đại học";
    } else if (containsKeyword(norm, "ielts")) {
        res.targetProgram = "Luyện thi IELTS";
    } else if (containsKeyword(norm, "hsg")) {
        res.targetProgram = "Luyện thi HSG";
    }

    return res;
}

std::string SubjectTaxonomy::getCanonicalCodeString(CanonicalSubject code) {
    switch (code) {
        case CanonicalSubject::MATH:             return "MATH";
        case CanonicalSubject::PHYSICS:          return "PHYSICS";
        case CanonicalSubject::CHEMISTRY:        return "CHEMISTRY";
        case CanonicalSubject::ENGLISH:          return "ENGLISH";
        case CanonicalSubject::COMPUTER_SCIENCE: return "COMPUTER_SCIENCE";
        case CanonicalSubject::BIOLOGY:          return "BIOLOGY";
        case CanonicalSubject::LITERATURE:       return "LITERATURE";
        case CanonicalSubject::HISTORY:          return "HISTORY";
        case CanonicalSubject::GEOGRAPHY:        return "GEOGRAPHY";
        default:                                 return "OTHER";
    }
}

std::string SubjectTaxonomy::getDisplayName(CanonicalSubject code) {
    switch (code) {
        case CanonicalSubject::MATH:             return "Toán";
        case CanonicalSubject::PHYSICS:          return "Vật lý";
        case CanonicalSubject::CHEMISTRY:        return "Hóa học";
        case CanonicalSubject::ENGLISH:          return "Tiếng Anh";
        case CanonicalSubject::COMPUTER_SCIENCE: return "Tin học";
        case CanonicalSubject::BIOLOGY:          return "Sinh học";
        case CanonicalSubject::LITERATURE:       return "Văn học";
        case CanonicalSubject::HISTORY:          return "Lịch sử";
        case CanonicalSubject::GEOGRAPHY:        return "Địa lý";
        default:                                 return "Môn học khác";
    }
}

std::string SubjectTaxonomy::canonicalCodeToDisplay(const std::string& codeOrText) {
    ParsedSubject parsed = parseSubject(codeOrText);
    return parsed.displayName;
}

bool SubjectTaxonomy::matchesFilter(const std::string& tutorSubjectStr, const std::string& filterSubjectStr) {
    if (tutorSubjectStr.empty() || filterSubjectStr.empty()) {
        return false;
    }

    std::string normFilter = normalizeText(filterSubjectStr);
    std::string normTutor = normalizeText(tutorSubjectStr);
    if (normFilter.empty() || normTutor.empty()) {
        return false;
    }

    auto filterSubjects = getCanonicalSubjects(filterSubjectStr);
    if (filterSubjects.empty()) {
        return false;
    }

    auto tutorSubjects = getCanonicalSubjects(tutorSubjectStr);
    if (tutorSubjects.empty()) {
        return false;
    }

    for (const auto& fs : filterSubjects) {
        for (const auto& ts : tutorSubjects) {
            if (fs == ts) {
                return true;
            }
        }
    }

    return false;
}

double SubjectTaxonomy::calculateAffinityScore(const std::string& targetSubject, int targetGrade, const Tutor& tutor) {
    if (targetSubject.empty()) {
        return 100.0;
    }

    if (!matchesFilter(tutor.getSubjectSpecialty(), targetSubject)) {
        return 0.0;
    }

    ParsedSubject studentSubj = parseSubject(targetSubject);
    ParsedSubject tutorSubj = parseSubject(tutor.getSubjectSpecialty());

    int studentTargetGrade = targetGrade;
    if (studentSubj.minGrade == studentSubj.maxGrade && studentSubj.minGrade >= 1 && studentSubj.minGrade <= 12) {
        studentTargetGrade = studentSubj.minGrade;
    }

    double score = 80.0;
    if (studentTargetGrade >= tutorSubj.minGrade && studentTargetGrade <= tutorSubj.maxGrade) {
        score += 20.0; // 100.0 Total
    } else if (std::abs(studentTargetGrade - tutorSubj.minGrade) <= 1 || std::abs(studentTargetGrade - tutorSubj.maxGrade) <= 1) {
        score += 10.0; // 90.0 Total
    } else {
        score += 5.0;  // 85.0 Total
    }
    return std::min(100.0, score);
}

double SubjectTaxonomy::calculateAffinityScore(const Student& student, const Tutor& tutor) {
    return calculateAffinityScore("", student.getGradeLevel(), tutor);
}

std::vector<std::pair<std::string, std::string>> SubjectTaxonomy::getAvailableCategories() {
    return {
        {"", "Tất cả môn"},
        {"math", "Toán"},
        {"physics", "Vật lý"},
        {"chemistry", "Hóa học"},
        {"english", "Tiếng Anh"},
        {"computer_science", "Tin học"},
        {"biology", "Sinh học"},
        {"literature", "Văn học"},
        {"history", "Lịch sử"},
        {"geography", "Địa lý"}
    };
}

} // namespace TutorMatch
