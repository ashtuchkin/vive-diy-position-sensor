#include "primitives/string_utils.h"
#include "primitives/vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <utility>

int PrintStream::printf(const char *format, ...) {
    static char printf_buf[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(printf_buf, sizeof(printf_buf), format, args);
    va_end(args);
    if (len > 0)
        write(printf_buf, len);
    return len;
}

// Parses provided string to null-terminated array of trimmed strings.
char **parse_words(char *str) {
    static char *words[max_words+1];
    int i = 0;
    char *word;
    while ((word = next_word(&str)) && i < max_words) {
        words[i++] = word;
    }
    words[i] = 0;
    return words;
}


// Returns pointer to a null-terminated next word in *str (or NULL); updates *str to point to remaining string.
char* next_word(char **str) {
    char *start = *str;
    if (start == NULL)
        return NULL;
    while (*start != 0 && *start <= ' ')
        start++;
    if (*start == 0) {
        *str = NULL;
        return NULL;
    }
    char *end = start+1;
    while (*end > ' ')
        end++;
    if (*end != 0) {
        *end = 0;
        *str = end+1;
    } else {
        *str = NULL;
    }
    return start;
}

// Parses given string into a uint32 and returns if the parsing is successful.
bool parse_uint32(const char *str, uint32_t *res) {
    if (!str || *str == 0)
        return false;
    char *endparse;
    *res = strtoul(str, &endparse, 10);
    return (*endparse == 0);
}

bool parse_float(const char *str, float *res) {
    if (!str || *str == 0)
        return false;
    char *endparse;
    *res = strtof(str, &endparse);
    return (*endparse == 0);
}

// Returns true if the word is suffixed by a valid number.
bool suffixed_by_int(char *word, char **first_digit, uint32_t *value) {
    char *p = word + strlen(word); // \0 char after the word.
    while (p != word && '0' <= *(p-1) && *(p-1) <= '9')  // Go backwards if seeing digits.
        p--;
    // Fail if either no digits found, or all of them are digits.
    return *p && p != word && parse_uint32(*first_digit = p, value);
}

// Return a static, zero-terminated array of hashes of provided words.
HashedWord *hash_words(char *str) {
    static HashedWord hashes[max_words+1];
    uint32_t i = 0;
    for (char *cur_word; (cur_word = next_word(&str)) && i < max_words; i++) {
        if (cur_word[0] == '#') break;  // Comment.
        hashes[i].word = cur_word;
        char *first_digit;
        if (suffixed_by_int(cur_word, &first_digit, &hashes[i].idx)) {
            char c = '#';
            // Create hash of cur_word without digits, but with prefix '#'
            std::swap(*first_digit, c); 
            hashes[i].hash = runtime_hash(cur_word, first_digit-cur_word+1);
            std::swap(*first_digit, c);
            // hashes[i].idx is set in suffixed_by_int
        } else { // Regular case.
            hashes[i].hash = runtime_hash(cur_word, strlen(cur_word));
            hashes[i].idx = -1;  // To avoid clashing with existing indexes.
        }
    }
    hashes[i] = {0, 0, 0};
    return hashes;
}

// Throws custom exception with a printf-formatted string. Uses static buffer to avoid mem allocation
// and std::string-related errors.
[[noreturn]] void throw_printf(const char* format, ...) {
    static char throw_printf_message[128];
    va_list args;
    va_start(args, format);
    vsnprintf(throw_printf_message, sizeof(throw_printf_message), format, args);
    va_end(args);
    throw ValidationException(throw_printf_message);
}
