#ifndef TUTOR_MATCH_SUBJECT_TAXONOMY_H
#define TUTOR_MATCH_SUBJECT_TAXONOMY_H

#include <string>
#include <vector>
#include <utility>

namespace TutorMatch {

class Student;
class Tutor;

enum class CanonicalSubject {
    MATH,
    PHYSICS,
    CHEMISTRY,
    ENGLISH,
    COMPUTER_SCIENCE,
    BIOLOGY,
    LITERATURE,
    HISTORY,
    GEOGRAPHY,
    OTHER
};

struct ParsedSubject {
    CanonicalSubject code = CanonicalSubject::OTHER;
    std::string canonicalCodeStr = "OTHER";
    std::string displayName = "Môn học khác";
    int minGrade = 1;  // 1..12
    int maxGrade = 12; // 1..12
    std::string targetProgram = "";
    std::string rawInput = "";
};

class SubjectTaxonomy {
public:
    static std::string stripAccents(const std::string& input);
    static std::string normalizeText(const std::string& input);
    static std::vector<CanonicalSubject> getCanonicalSubjects(const std::string& subjectStr);
    static ParsedSubject parseSubject(const std::string& subjectStr);
    
    static std::string getCanonicalCodeString(CanonicalSubject code);
    static std::string getDisplayName(CanonicalSubject code);
    static std::string canonicalCodeToDisplay(const std::string& codeOrText);

    static bool matchesFilter(const std::string& tutorSubjectStr, const std::string& filterSubjectStr);
    static double calculateAffinityScore(const std::string& targetSubject, int targetGrade, const Tutor& tutor);
    static double calculateAffinityScore(const Student& student, const Tutor& tutor);

    static std::vector<std::pair<std::string, std::string>> getAvailableCategories();
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_SUBJECT_TAXONOMY_H
