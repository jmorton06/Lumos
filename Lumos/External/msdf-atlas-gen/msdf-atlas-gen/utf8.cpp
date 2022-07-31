
#include "utf8.h"

namespace msdf_atlas {

void utf8Decode(std::vector<unicode_t> &codepoints, const char *utf8String) {
    bool start = true;
    int rBytes = 0;
    unicode_t cp = 0;

    for (const char *c = utf8String; *c; ++c) {
        if (rBytes > 0) {
            --rBytes;
            if ((*c&0xc0) == 0x80)
                cp |= (*c&0x3f)<<(6*rBytes);
            // else error
        } else if (!(*c&0x80)) {
            cp = *c;
            rBytes = 0;
        } else if (*c&0x40) {
            int block;
            for (block = 0; ((unsigned char) *c<<block)&0x40 && block < 4; ++block);
            if (block < 4) {
                cp = (*c&(0x3f>>block))<<(6*block);
                rBytes = block;
            } else
                continue; // error
        } else
            continue; // error
        if (!rBytes) {
            if (!(start && cp == 0xfeff)) // BOM
                codepoints.push_back(cp);
            start = false;
        }
    }
}

}
