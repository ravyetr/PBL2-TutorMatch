#include "business/matching/TutorSearchMatcher.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>

namespace TutorMatch {

std::string TutorSearchMatcher::normalizeAccented(const std::string& input) {
    if (input.empty()) return "";
    QString qstr = QString::fromUtf8(input.c_str()).trimmed().toLower();

    // Normalize common punctuation and delimiters to spaces
    static const QRegularExpression punctRegex(QStringLiteral(R"([_,\-/:&+\;\*\?\!#\[\]\(\)\{\}\"]+)"));
    qstr.replace(punctRegex, QStringLiteral(" "));

    // Collapse multiple consecutive whitespaces into a single space
    static const QRegularExpression multiSpaceRegex(QStringLiteral(R"(\s+)"));
    qstr.replace(multiSpaceRegex, QStringLiteral(" "));

    return qstr.trimmed().toStdString();
}

std::string TutorSearchMatcher::normalizeUnaccented(const std::string& input) {
    if (input.empty()) return "";
    std::string accented = normalizeAccented(input);
    // Convert Vietnamese accented letters to base Latin (e.g. đ/Đ -> d, à/á -> a)
    std::string unacc = SubjectTaxonomy::stripAccents(accented);

    std::transform(unacc.begin(), unacc.end(), unacc.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    std::string res;
    bool inSpace = false;
    for (char c : unacc) {
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

std::vector<std::string> TutorSearchMatcher::tokenize(const std::string& normalizedInput) {
    std::vector<std::string> tokens;
    if (normalizedInput.empty()) return tokens;
    std::istringstream iss(normalizedInput);
    std::string token;
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

SearchQueryIntent TutorSearchMatcher::parseQuery(const std::string& rawQuery) {
    SearchQueryIntent intent;
    intent.rawQuery = rawQuery;
    intent.normalizedAccented = normalizeAccented(rawQuery);
    intent.normalizedUnaccented = normalizeUnaccented(rawQuery);
    intent.tokensAccented = tokenize(intent.normalizedAccented);
    intent.tokensUnaccented = tokenize(intent.normalizedUnaccented);

    if (intent.normalizedUnaccented.empty()) {
        return intent;
    }

    // 1. Extract Canonical Subjects from query using SubjectTaxonomy
    intent.targetSubjects = SubjectTaxonomy::getCanonicalSubjects(rawQuery);
    intent.isMultiSubject = (intent.targetSubjects.size() >= 2);

    // 2. Extract Grade Level from query tokens (1..12)
    for (int g = 12; g >= 1; --g) {
        std::string gStr = std::to_string(g);
        for (const auto& tok : intent.tokensUnaccented) {
            if (tok == gStr) {
                intent.targetGrade = g;
                break;
            }
        }
        if (intent.targetGrade > 0) break;
    }

    return intent;
}

TutorMatchEvaluation TutorSearchMatcher::evaluate(
    const SearchQueryIntent& intent,
    const std::shared_ptr<Tutor>& tutor,
    const std::string& explicitSubjectFilter) {

    TutorMatchEvaluation eval;
    eval.tutor = tutor;
    if (!tutor || !tutor->getIsActive()) {
        eval.tier = SearchRelevanceTier::NO_MATCH;
        eval.score = 0.0;
        eval.isEligible = false;
        return eval;
    }

    // 1. Explicit Subject Filter (from ComboBox / Category filter)
    if (!explicitSubjectFilter.empty()) {
        if (!SubjectTaxonomy::matchesFilter(tutor->getSubjectSpecialty(), explicitSubjectFilter)) {
            eval.tier = SearchRelevanceTier::NO_MATCH;
            eval.score = 0.0;
            eval.isEligible = false;
            return eval;
        }
    }

    // If query is empty: all active tutors matching explicitSubjectFilter are eligible with neutral score
    if (intent.normalizedUnaccented.empty()) {
        eval.tier = SearchRelevanceTier::GENERAL_TEXT_MATCH;
        eval.score = 100.0;
        eval.isEligible = true;
        return eval;
    }

    // Normalized tutor representations in memory
    std::string tNameUnacc = normalizeUnaccented(tutor->getFullName());
    std::string tSubjUnacc = normalizeUnaccented(tutor->getSubjectSpecialty());
    std::string tBioUnacc = normalizeUnaccented(tutor->getBio());

    auto tutorSubjects = SubjectTaxonomy::getCanonicalSubjects(tutor->getSubjectSpecialty());
    auto parsedSubj = SubjectTaxonomy::parseSubject(tutor->getSubjectSpecialty());

    // =========================================================================
    // CASE A: Query contains recognized Canonical Subjects (Subject Search Intent)
    // =========================================================================
    if (!intent.targetSubjects.empty()) {
        int matchedCount = 0;
        for (auto qs : intent.targetSubjects) {
            for (auto ts : tutorSubjects) {
                if (qs == ts) {
                    matchedCount++;
                    break;
                }
            }
        }

        // TIER 1: Full Subject Match (Matches ALL recognized subjects in query)
        if (matchedCount == static_cast<int>(intent.targetSubjects.size())) {
            eval.tier = SearchRelevanceTier::FULL_SUBJECT_MATCH;
            eval.score = 1000.0; // Tier 1 Base

            // Multi-subject bonus
            if (intent.isMultiSubject) {
                eval.score += 100.0;
            }

            // Grade match bonus
            if (intent.targetGrade > 0) {
                if (intent.targetGrade >= parsedSubj.minGrade && intent.targetGrade <= parsedSubj.maxGrade) {
                    eval.score += 100.0; // Exact grade coverage
                } else if (std::abs(intent.targetGrade - parsedSubj.minGrade) <= 1 ||
                           std::abs(intent.targetGrade - parsedSubj.maxGrade) <= 1) {
                    eval.score += 40.0;  // Adjacent grade
                }
            }

            // Exact phrase match in specialty
            if (tSubjUnacc.find(intent.normalizedUnaccented) != std::string::npos) {
                eval.score += 50.0;
            }

            // Name match bonus (e.g. query "Lê Toán")
            for (const auto& tok : intent.tokensUnaccented) {
                if (tNameUnacc.find(tok) != std::string::npos) {
                    eval.score += 20.0;
                }
            }

            eval.isEligible = true;
            return eval;
        }

        // TIER 2: Partial Subject Match (Matches SOME but not all recognized subjects)
        if (matchedCount > 0) {
            eval.tier = SearchRelevanceTier::PARTIAL_SUBJECT_MATCH;
            eval.score = 500.0; // Tier 2 Base (Never exceeds Tier 1 base of 1000.0)

            double ratio = static_cast<double>(matchedCount) / intent.targetSubjects.size();
            eval.score += ratio * 100.0;

            if (intent.targetGrade > 0 &&
                intent.targetGrade >= parsedSubj.minGrade && intent.targetGrade <= parsedSubj.maxGrade) {
                eval.score += 40.0;
            }

            if (tSubjUnacc.find(intent.normalizedUnaccented) != std::string::npos) {
                eval.score += 20.0;
            }

            eval.isEligible = true;
            return eval;
        }

        // matchedCount == 0: Tutor teaches NONE of the requested subjects
        // Exception: If the query matches the tutor's full name strongly (e.g. "Nguyễn Văn Toàn")
        if (tNameUnacc.find(intent.normalizedUnaccented) != std::string::npos) {
            eval.tier = SearchRelevanceTier::GENERAL_TEXT_MATCH;
            eval.score = 200.0;
            eval.isEligible = true;
            return eval;
        }

        eval.tier = SearchRelevanceTier::NO_MATCH;
        eval.score = 0.0;
        eval.isEligible = false;
        return eval;
    }

    // =========================================================================
    // CASE B: Query contains NO recognized subjects (Free-text, Name, Program, etc.)
    // =========================================================================
    bool nameExact = (tNameUnacc == intent.normalizedUnaccented);
    bool namePhrase = (tNameUnacc.find(intent.normalizedUnaccented) != std::string::npos);
    bool subjPhrase = (tSubjUnacc.find(intent.normalizedUnaccented) != std::string::npos);
    bool bioPhrase = (tBioUnacc.find(intent.normalizedUnaccented) != std::string::npos);

    int nameTokCount = 0;
    int subjTokCount = 0;
    int bioTokCount = 0;
    for (const auto& tok : intent.tokensUnaccented) {
        if (tNameUnacc.find(tok) != std::string::npos) nameTokCount++;
        if (tSubjUnacc.find(tok) != std::string::npos) subjTokCount++;
        if (tBioUnacc.find(tok) != std::string::npos) bioTokCount++;
    }

    int totalTokens = static_cast<int>(intent.tokensUnaccented.size());
    bool allTokensInName = (totalTokens > 0 && nameTokCount == totalTokens);
    bool allTokensInSubj = (totalTokens > 0 && subjTokCount == totalTokens);
    bool allTokensAcross = false;

    if (!allTokensInName && !allTokensInSubj) {
        int acrossCount = 0;
        std::string allText = tNameUnacc + " " + tSubjUnacc + " " + tBioUnacc;
        for (const auto& tok : intent.tokensUnaccented) {
            if (allText.find(tok) != std::string::npos) acrossCount++;
        }
        allTokensAcross = (totalTokens > 0 && acrossCount == totalTokens);
    }

    if (nameExact || namePhrase || subjPhrase || bioPhrase || allTokensInName || allTokensInSubj || allTokensAcross) {
        eval.tier = SearchRelevanceTier::GENERAL_TEXT_MATCH;
        eval.score = 100.0; // Tier 3 Base

        if (nameExact) eval.score += 400.0;
        else if (namePhrase) eval.score += 300.0;
        else if (allTokensInName) eval.score += 200.0;

        if (subjPhrase) eval.score += 150.0;
        else if (allTokensInSubj) eval.score += 100.0;

        if (bioPhrase) eval.score += 50.0;

        eval.isEligible = true;
        return eval;
    }

    eval.tier = SearchRelevanceTier::NO_MATCH;
    eval.score = 0.0;
    eval.isEligible = false;
    return eval;
}

std::vector<std::shared_ptr<Tutor>> TutorSearchMatcher::searchAndRank(
    const std::vector<std::shared_ptr<Tutor>>& tutors,
    const std::string& keyword,
    const std::string& explicitSubjectFilter,
    double minPrice, double maxPrice, double minRating) {

    auto intent = parseQuery(keyword);
    std::vector<TutorMatchEvaluation> evals;
    evals.reserve(tutors.size());

    for (const auto& t : tutors) {
        if (!t || !t->getIsActive()) continue;

        // Apply price and rating filters first
        if (minPrice > 0.0 && t->getHourlyRate() < minPrice) continue;
        if (maxPrice > 0.0 && t->getHourlyRate() > maxPrice) continue;
        if (minRating > 0.0 && t->getRating() < minRating) continue;

        auto ev = evaluate(intent, t, explicitSubjectFilter);
        if (ev.isEligible) {
            evals.push_back(ev);
        }
    }

    // Stable sort preserving relative order among tied evaluations
    std::stable_sort(evals.begin(), evals.end(), [](const TutorMatchEvaluation& a, const TutorMatchEvaluation& b) {
        if (a.tier != b.tier) {
            return static_cast<int>(a.tier) > static_cast<int>(b.tier);
        }
        return a.score > b.score;
    });

    std::vector<std::shared_ptr<Tutor>> results;
    results.reserve(evals.size());
    for (const auto& ev : evals) {
        results.push_back(ev.tutor);
    }
    return results;
}

} // namespace TutorMatch
