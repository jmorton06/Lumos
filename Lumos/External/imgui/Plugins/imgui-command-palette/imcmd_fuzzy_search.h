// Adapted from https://github.com/forrestthewoods/lib_fts/blob/master/code/fts_fuzzy_match.h
#pragma once

#include <cstdint>

namespace ImCmd
{

bool FuzzySearch(char const* pattern, char const* src, int& outScore);
bool FuzzySearch(char const* pattern, char const* src, int& outScore, uint8_t matches[], int maxMatches, int& outMatches);

} // namespace ImCmd
