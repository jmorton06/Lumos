
#pragma once

#include <cstdlib>
#include <set>
#include "types.h"

namespace msdf_atlas {

/// Represents a set of Unicode codepoints (characters)
class Charset {

public:
    /// The set of the 95 printable ASCII characters
    static const Charset ASCII;

    /// Adds a codepoint
    void add(unicode_t cp);
    /// Removes a codepoint
    void remove(unicode_t cp);

    size_t size() const;
    bool empty() const;
    std::set<unicode_t>::const_iterator begin() const;
    std::set<unicode_t>::const_iterator end() const;

    /// Load character set from a text file with the correct syntax
    bool load(const char *filename, bool disableCharLiterals = false);

private:
    std::set<unicode_t> codepoints;

};

}
