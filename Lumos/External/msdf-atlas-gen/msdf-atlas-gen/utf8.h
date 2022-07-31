
#pragma once

#include <vector>
#include "types.h"

namespace msdf_atlas {

/// Decodes the UTF-8 string into an array of Unicode codepoints
void utf8Decode(std::vector<unicode_t> &codepoints, const char *utf8String);

}
