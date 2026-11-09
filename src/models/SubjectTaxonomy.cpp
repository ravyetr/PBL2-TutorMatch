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
        if (c == '_' || c == '-' || c == '/' || c == ',' || c == '(' || c == ')' || c == ':') {
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

ParsedSubject SubjectTaxonomy::parseSubject(const std::string& subjectStr) {
    ParsedSubject res;
    res.rawInput = subjectStr;
    std::string norm = normalizeText(subjectStr);

    if (containsKeyword(norm, "math") || containsKeyword(norm, "toan") || containsKeyword(norm, "calculus") ||
        containsKeyword(norm, "algebra") || containsKeyword(norm, "geometry") || containsKeyword(norm, "giai tich") ||
        containsKeyword(norm, "dai so") || containsKeyword(norm, "hinh hoc")) {
        res.code = CanonicalSubject::MATH;
    } else if (containsKeyword(norm, "physics") || containsKeyword(norm, "vat ly") || containsKeyword(norm, "co hoc") ||
               containsKeyword(norm, "quang hoc") || containsKeyword(norm, "dien tu") || norm == "ly" || containsKeyword(norm, " ly ")) {
        res.code = CanonicalSubject::PHYSICS;
    } else if (containsKeyword(norm, "chemistry") || containsKeyword(norm, "hoa hoc") || containsKeyword(norm, "hoa huu co") ||
               containsKeyword(norm, "hoa vo co") || norm == "hoa" || containsKeyword(norm, " hoa ")) {
        res.code = CanonicalSubject::CHEMISTRY;
    } else if (containsKeyword(norm, "english") || containsKeyword(norm, "tieng anh") || containsKeyword(norm, "ngoai ngu") ||
               containsKeyword(norm, "ielts") || containsKeyword(norm, "toeic") || containsKeyword(norm, "toefl")) {
        res.code = CanonicalSubject::ENGLISH;
    } else if (containsKeyword(norm, "computer science") || containsKeyword(norm, "tin hoc") || containsKeyword(norm, "lap trinh") ||
               containsKeyword(norm, "programming") || containsKeyword(norm, "coding") || containsKeyword(norm, "c++") ||
               containsKeyword(norm, "python") || containsKeyword(norm, "it")) {
        res.code = CanonicalSubject::COMPUTER_SCIENCE;
    } else if (containsKeyword(norm, "biology") || containsKeyword(norm, "sinh hoc") || norm == "sinh" || containsKeyword(norm, " sinh ")) {
        res.code = CanonicalSubject::BIOLOGY;
    } else if (containsKeyword(norm, "literature") || containsKeyword(norm, "van hoc") || containsKeyword(norm, "ngu van") ||
               norm == "van" || containsKeyword(norm, " van ")) {
        res.code = CanonicalSubject::LITERATURE;
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
        default:                                 return "Môn học khác";
    }
}

std::string SubjectTaxonomy::canonicalCodeToDisplay(const std::string& codeOrText) {
    ParsedSubject parsed = parseSubject(codeOrText);
    return parsed.displayName;
}

bool SubjectTaxonomy::matchesFilter(const std::string& tutorSubjectStr, const std::string& filterSubjectStr) {
    std::string normFilter = normalizeText(filterSubjectStr);
    if (normFilter.empty() || normFilter == "tat ca mon" || normFilter == "tat ca" || filterSubjectStr == "Tất cả môn") {
        return true;
    }

    ParsedSubject filterParsed = parseSubject(filterSubjectStr);
    ParsedSubject tutorParsed = parseSubject(tutorSubjectStr);

    if (filterParsed.code != CanonicalSubject::OTHER && tutorParsed.code != CanonicalSubject::OTHER) {
        if (filterParsed.code == tutorParsed.code) {
            return true;
        }
    }

    std::string normTutor = normalizeText(tutorSubjectStr);
    if (normTutor.find(normFilter) != std::string::npos) {
        return true;
    }

    return false;
}

double SubjectTaxonomy::calculateAffinityScore(const Student& student, const Tutor& tutor) {
    std::string prefSubjStr = student.getPreferredSubject();
    if (prefSubjStr.empty()) {
        return 100.0;
    }

    ParsedSubject studentSubj = parseSubject(prefSubjStr);
    ParsedSubject tutorSubj = parseSubject(tutor.getSubjectSpecialty());

    int studentTargetGrade = student.getGradeLevel();
    if (studentSubj.minGrade == studentSubj.maxGrade && studentSubj.minGrade >= 1 && studentSubj.minGrade <= 12) {
        studentTargetGrade = studentSubj.minGrade;
    }

    if (studentSubj.code == tutorSubj.code && studentSubj.code != CanonicalSubject::OTHER) {
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

    std::string normTutor = normalizeText(tutor.getSubjectSpecialty());
    std::string normPref = normalizeText(prefSubjStr);

    if (!normPref.empty() && normTutor.find(normPref) != std::string::npos) {
        return 70.0;
    }

    return 20.0;
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
        {"literature", "Văn học"}
    };
}

} // namespace TutorMatch
