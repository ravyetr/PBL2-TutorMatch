#ifndef TUTOR_MATCH_TUTOR_SEARCH_MATCHER_H
#define TUTOR_MATCH_TUTOR_SEARCH_MATCHER_H

#include "models/User.h"
#include "models/SubjectTaxonomy.h"
#include <string>
#include <vector>
#include <memory>

namespace TutorMatch {

/**
 * @brief Categorical priority tiers for search relevance ranking.
 *
 * Tier 1: FULL_SUBJECT_MATCH — Tutor teaches ALL canonical subjects recognized in the search query.
 * Tier 2: PARTIAL_SUBJECT_MATCH — Tutor teaches a subset of canonical subjects recognized in the search query.
 * Tier 3: GENERAL_TEXT_MATCH — Tutor matches free-text keywords, tutor full name, bio, or non-subject tokens.
 * Tier 4: NO_MATCH — Irrelevant to the search query (excluded from search results).
 */
enum class SearchRelevanceTier {
    NO_MATCH = 0,
    GENERAL_TEXT_MATCH = 1,   // Tier 3
    PARTIAL_SUBJECT_MATCH = 2, // Tier 2
    FULL_SUBJECT_MATCH = 3    // Tier 1 (Highest priority)
};

/**
 * @brief Structured representation of the student's search intent.
 */
struct SearchQueryIntent {
    std::string rawQuery;
    std::string normalizedAccented;
    std::string normalizedUnaccented;
    std::vector<std::string> tokensAccented;
    std::vector<std::string> tokensUnaccented;
    std::vector<CanonicalSubject> targetSubjects; // Recognized canonical subjects
    int targetGrade = 0;                          // Target grade (1..12) if recognized
    bool isMultiSubject = false;                  // True if query includes >= 2 subjects
};

/**
 * @brief Evaluation result for a specific tutor against a search query.
 */
struct TutorMatchEvaluation {
    std::shared_ptr<Tutor> tutor;
    SearchRelevanceTier tier = SearchRelevanceTier::NO_MATCH;
    double score = 0.0;
    bool isEligible = false;
};

/**
 * @brief Independent search engine matcher for TutorMatch regular tutor discovery.
 *
 * Provides query understanding, text normalization (Unicode, case-folding, accent-stripping,
 * whitespace collapsing), multi-subject parsing, and relevance ranking across multiple fields.
 */
class TutorSearchMatcher {
public:
    // 1. Text Normalization Utilities
    static std::string normalizeAccented(const std::string& input);
    static std::string normalizeUnaccented(const std::string& input);
    static std::vector<std::string> tokenize(const std::string& normalizedInput);

    // 2. Query Understanding
    static SearchQueryIntent parseQuery(const std::string& rawQuery);

    // 3. Single Tutor Evaluation
    static TutorMatchEvaluation evaluate(const SearchQueryIntent& intent,
                                         const std::shared_ptr<Tutor>& tutor,
                                         const std::string& explicitSubjectFilter);

    // 4. Batch Filtering & Relevance Ranking
    static std::vector<std::shared_ptr<Tutor>> searchAndRank(
        const std::vector<std::shared_ptr<Tutor>>& tutors,
        const std::string& keyword,
        const std::string& explicitSubjectFilter,
        double minPrice = 0.0,
        double maxPrice = 0.0,
        double minRating = 0.0);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_TUTOR_SEARCH_MATCHER_H
