#include "imcmd_fuzzy_search.h"

#include <cctype>
#include <cstring>

namespace ImCmd
{

namespace
{
    bool FuzzySearchRecursive(const char* pattern, const char* src, int& outScore, const char* strBegin, const uint8_t srcMatches[], uint8_t newMatches[], int maxMatches, int& nextMatch, int& recursionCount, int recursionLimit);
} // namespace

bool FuzzySearch(char const* pattern, char const* haystack, int& outScore)
{
    uint8_t matches[256];
    int matchCount = 0;
    return FuzzySearch(pattern, haystack, outScore, matches, sizeof(matches), matchCount);
}

bool FuzzySearch(char const* pattern, char const* haystack, int& outScore, uint8_t matches[], int maxMatches, int& outMatches)
{
    int recursionCount = 0;
    int recursionLimit = 10;
    int newMatches = 0;
    bool result = FuzzySearchRecursive(pattern, haystack, outScore, haystack, nullptr, matches, maxMatches, newMatches, recursionCount, recursionLimit);
    outMatches = newMatches;
    return result;
}

namespace
{
    bool FuzzySearchRecursive(const char* pattern, const char* src, int& outScore, const char* strBegin, const uint8_t srcMatches[], uint8_t newMatches[], int maxMatches, int& nextMatch, int& recursionCount, int recursionLimit)
    {
        // Count recursions
        ++recursionCount;
        if (recursionCount >= recursionLimit) {
            return false;
        }

        // Detect end of strings
        if (*pattern == '\0' || *src == '\0') {
            return false;
        }

        // Recursion params
        bool recursiveMatch = false;
        uint8_t bestRecursiveMatches[256];
        int bestRecursiveScore = 0;

        // Loop through pattern and str looking for a match
        bool firstMatch = true;
        while (*pattern != '\0' && *src != '\0') {
            // Found match
            if (tolower(*pattern) == tolower(*src)) {
                // Supplied matches buffer was too short
                if (nextMatch >= maxMatches) {
                    return false;
                }

                // "Copy-on-Write" srcMatches into matches
                if (firstMatch && srcMatches) {
                    memcpy(newMatches, srcMatches, nextMatch);
                    firstMatch = false;
                }

                // Recursive call that "skips" this match
                uint8_t recursiveMatches[256];
                int recursiveScore;
                int recursiveNextMatch = nextMatch;
                if (FuzzySearchRecursive(pattern, src + 1, recursiveScore, strBegin, newMatches, recursiveMatches, sizeof(recursiveMatches), recursiveNextMatch, recursionCount, recursionLimit)) {
                    // Pick the best recursive score
                    if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                        memcpy(bestRecursiveMatches, recursiveMatches, 256);
                        bestRecursiveScore = recursiveScore;
                    }
                    recursiveMatch = true;
                }

                // Advance
                newMatches[nextMatch++] = (uint8_t)(src - strBegin);
                ++pattern;
            }
            ++src;
        }

        // Determine if full pattern was matched
        bool matched = *pattern == '\0';

        // Calculate score
        if (matched) {
            const int sequentialBonus = 15; // bonus for adjacent matches
            const int separatorBonus = 30; // bonus if match occurs after a separator
            const int camelBonus = 30; // bonus if match is uppercase and prev is lower
            const int firstLetterBonus = 15; // bonus if the first letter is matched

            const int leadingLetterPenalty = -5; // penalty applied for every letter in str before the first match
            const int maxLeadingLetterPenalty = -15; // maximum penalty for leading letters
            const int unmatchedLetterPenalty = -1; // penalty for every letter that doesn't matter

            // Iterate str to end
            while (*src != '\0') {
                ++src;
            }

            // Initialize score
            outScore = 100;

            // Apply leading letter penalty
            int penalty = leadingLetterPenalty * newMatches[0];
            if (penalty < maxLeadingLetterPenalty) {
                penalty = maxLeadingLetterPenalty;
            }
            outScore += penalty;

            // Apply unmatched penalty
            int unmatched = (int)(src - strBegin) - nextMatch;
            outScore += unmatchedLetterPenalty * unmatched;

            // Apply ordering bonuses
            for (int i = 0; i < nextMatch; ++i) {
                uint8_t currIdx = newMatches[i];

                if (i > 0) {
                    uint8_t prevIdx = newMatches[i - 1];

                    // Sequential
                    if (currIdx == (prevIdx + 1))
                        outScore += sequentialBonus;
                }

                // Check for bonuses based on neighbor character value
                if (currIdx > 0) {
                    // Camel case
                    char neighbor = strBegin[currIdx - 1];
                    char curr = strBegin[currIdx];
                    if (::islower(neighbor) && ::isupper(curr)) {
                        outScore += camelBonus;
                    }

                    // Separator
                    bool neighborSeparator = neighbor == '_' || neighbor == ' ';
                    if (neighborSeparator) {
                        outScore += separatorBonus;
                    }
                } else {
                    // First letter
                    outScore += firstLetterBonus;
                }
            }
        }

        // Return best result
        if (recursiveMatch && (!matched || bestRecursiveScore > outScore)) {
            // Recursive score is better than "this"
            memcpy(newMatches, bestRecursiveMatches, maxMatches);
            outScore = bestRecursiveScore;
            return true;
        } else if (matched) {
            // "this" score is better than recursive
            return true;
        } else {
            // no match
            return false;
        }
    }
} // namespace
} // namespace ImCmd
