
#include "Charset.h"

#include <cstdio>
#include <string>
#include "utf8.h"

namespace msdf_atlas {

static char escapedChar(char c) {
    switch (c) {
        case '0':
            return '\0';
        case 'n': case 'N':
            return '\n';
        case 'r': case 'R':
            return '\r';
        case 's': case 'S':
            return ' ';
        case 't': case 'T':
            return '\t';
        case '\\': case '"': case '\'':
        default:
            return c;
    }
}

static int readWord(std::string &str, FILE *f) {
    while (true) {
        int c = fgetc(f);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')
            str.push_back((char) c);
        else
            return c;
    }
}

static bool readString(std::string &str, FILE *f, char terminator) {
    bool escape = false;
    while (true) {
        int c = fgetc(f);
        if (c < 0)
            return false;
        if (escape) {
            str.push_back(escapedChar((char) c));
            escape = false;
        } else {
            if (c == terminator)
                return true;
            else if (c == '\\')
                escape = true;
            else
                str.push_back((char) c);
        }
    }
}

static bool parseInt(int &i, const char *str) {
    i = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { // hex
        str += 2;
        for (; *str; ++str) {
            if (*str >= '0' && *str <= '9') {
                i <<= 4;
                i += *str-'0';
            } else if (*str >= 'A' && *str <= 'F') {
                i <<= 4;
                i += *str-'A'+10;
            } else if (*str >= 'a' && *str <= 'f') {
                i <<= 4;
                i += *str-'a'+10;
            } else
                return false;
        }
    } else { // dec
        for (; *str; ++str) {
            if (*str >= '0' && *str <= '9') {
                i *= 10;
                i += *str-'0';
            } else
                return false;
        }
    }
    return true;
}

static std::string combinePath(const char *basePath, const char *relPath) {
    if (relPath[0] == '/' || (relPath[0] && relPath[1] == ':')) // absolute path?
        return relPath;
    int lastSlash = -1;
    for (int i = 0; basePath[i]; ++i)
        if (basePath[i] == '/' || basePath[i] == '\\')
            lastSlash = i;
    if (lastSlash < 0)
        return relPath;
    return std::string(basePath, lastSlash+1)+relPath;
}

bool Charset::load(const char *filename, bool disableCharLiterals) {

    if (FILE *f = fopen(filename, "rb")) {

        enum {
            CLEAR,
            TIGHT,
            RANGE_BRACKET,
            RANGE_START,
            RANGE_SEPARATOR,
            RANGE_END
        } state = CLEAR;

        std::string buffer;
        std::vector<unicode_t> unicodeBuffer;
        unicode_t rangeStart = 0;
        for (int c = fgetc(f), start = true; c >= 0; start = false) {
            switch (c) {
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': // number
                    if (!(state == CLEAR || state == RANGE_BRACKET || state == RANGE_SEPARATOR))
                        goto FAIL;
                    buffer.push_back((char) c);
                    c = readWord(buffer, f);
                    {
                        int cp;
                        if (!parseInt(cp, buffer.c_str()))
                            goto FAIL;
                        switch (state) {
                            case CLEAR:
                                if (cp >= 0)
                                    add((unicode_t) cp);
                                state = TIGHT;
                                break;
                            case RANGE_BRACKET:
                                rangeStart = (unicode_t) cp;
                                state = RANGE_START;
                                break;
                            case RANGE_SEPARATOR:
                                for (unicode_t u = rangeStart; (int) u <= cp; ++u)
                                    add(u);
                                state = RANGE_END;
                                break;
                            default:;
                        }
                    }
                    buffer.clear();
                    continue; // next character already read
                case '\'': // single UTF-8 character
                    if (!(state == CLEAR || state == RANGE_BRACKET || state == RANGE_SEPARATOR) || disableCharLiterals)
                        goto FAIL;
                    if (!readString(buffer, f, '\''))
                        goto FAIL;
                    utf8Decode(unicodeBuffer, buffer.c_str());
                    if (unicodeBuffer.size() == 1) {
                        switch (state) {
                            case CLEAR:
                                if (unicodeBuffer[0] > 0)
                                    add(unicodeBuffer[0]);
                                state = TIGHT;
                                break;
                            case RANGE_BRACKET:
                                rangeStart = unicodeBuffer[0];
                                state = RANGE_START;
                                break;
                            case RANGE_SEPARATOR:
                                for (unicode_t u = rangeStart; u <= unicodeBuffer[0]; ++u)
                                    add(u);
                                state = RANGE_END;
                                break;
                            default:;
                        }
                    } else
                        goto FAIL;
                    unicodeBuffer.clear();
                    buffer.clear();
                    break;
                case '"': // string of UTF-8 characters
                    if (state != CLEAR || disableCharLiterals)
                        goto FAIL;
                    if (!readString(buffer, f, '"'))
                        goto FAIL;
                    utf8Decode(unicodeBuffer, buffer.c_str());
                    for (unicode_t cp : unicodeBuffer)
                        add(cp);
                    unicodeBuffer.clear();
                    buffer.clear();
                    state = TIGHT;
                    break;
                case '[': // character range start
                    if (state != CLEAR)
                        goto FAIL;
                    state = RANGE_BRACKET;
                    break;
                case ']': // character range end
                    if (state == RANGE_END)
                        state = TIGHT;
                    else
                        goto FAIL;
                    break;
                case '@': // annotation
                    if (state != CLEAR)
                        goto FAIL;
                    c = readWord(buffer, f);
                    if (buffer == "include") {
                        while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                            c = fgetc(f);
                        if (c != '"')
                            goto FAIL;
                        buffer.clear();
                        if (!readString(buffer, f, '"'))
                            goto FAIL;
                        load(combinePath(filename, buffer.c_str()).c_str());
                        state = TIGHT;
                    } else
                        goto FAIL;
                    buffer.clear();
                    break;
                case ',': case ';': // separator
                    if (!(state == CLEAR || state == TIGHT)) {
                        if (state == RANGE_START)
                            state = RANGE_SEPARATOR;
                        else
                            goto FAIL;
                    } // else treat as whitespace
                case ' ': case '\n': case '\r': case '\t': // whitespace
                    if (state == TIGHT)
                        state = CLEAR;
                    break;
                case 0xef: // UTF-8 byte order mark
                    if (start) {
                        if (!(fgetc(f) == 0xbb && fgetc(f) == 0xbf))
                            goto FAIL;
                        break;
                    }
                default: // unexpected character
                    goto FAIL;
            }
            c = fgetc(f);
        }

        fclose(f);
        return state == CLEAR || state == TIGHT;

    FAIL:
        fclose(f);
        return false;
    }

    return false;
}

}
