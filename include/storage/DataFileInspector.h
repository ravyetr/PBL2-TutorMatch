#ifndef TUTOR_MATCH_DATA_FILE_INSPECTOR_H
#define TUTOR_MATCH_DATA_FILE_INSPECTOR_H

#include <string>
#include <vector>
#include <iostream>

namespace TutorMatch {

struct FieldInspection {
    std::string name;
    std::string value;
    std::string type;
    std::string description;
};

struct RecordInspection {
    int recordIndex;
    std::string modelType; // "ADMIN", "STUDENT", "TUTOR", "Booking", "Review", "MALFORMED"
    bool isValid;
    std::string rawLine;
    std::string errorMessage;
    std::vector<FieldInspection> fields;
};

struct FileInspectionSummary {
    std::string fileName;
    std::string resolvedPath;
    std::string formatDescription;
    std::string status; // "OK", "ERROR_NOT_FOUND", "ERROR_CANNOT_OPEN", "EMPTY"
    int recordsRead;
    int validRecords;
    int invalidRecords;
    bool fileModified;
    std::vector<RecordInspection> records;
};

class DataFileInspector {
public:
    static FileInspectionSummary inspectUsersFile(const std::string& dataDirHint = "");
    static FileInspectionSummary inspectBookingsFile(const std::string& dataDirHint = "");
    static FileInspectionSummary inspectReviewsFile(const std::string& dataDirHint = "");
    static FileInspectionSummary inspectCourseBookingsFile(const std::string& dataDirHint = "");

    static void printFileSummary(const FileInspectionSummary& summary, std::ostream& out = std::cout);
    static void inspectAll(const std::string& dataDirHint = "", std::ostream& out = std::cout);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_DATA_FILE_INSPECTOR_H
