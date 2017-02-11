#include "string_utils.h"
#include <utility>
#include <Arduino.h>

constexpr int max_input_str_len = 256;
constexpr int max_words = 64;

// Non-blocking version of Stream.readBytesUntil('\n', ...). Returns line if found, or NULL if no line.
char *read_line(Stream &stream) {
    static char input_string_buf[max_input_str_len];
    static int input_string_pos = 0;
    while (stream.available()) {
        char next_char = stream.read();
        if (next_char == '\n') {
            input_string_buf[input_string_pos] = 0;
            input_string_pos = 0;
            return input_string_buf;
        } else if (input_string_pos < max_input_str_len - 1) {
            input_string_buf[input_string_pos++] = next_char;
        }
    }
    return NULL;
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
    char *p = word + strlen(word) - 1; // Last char of the word.
    bool seen_digit = false;
    while (p != word && '0' <= *p && *p <= '9') {  // Go backwards if seeing digits.
        p--; 
        seen_digit = true; 
    }
    // Fail if either no digits found, or all of them are digits.
    return seen_digit && p != word && parse_uint32(*first_digit = p+1, value);
}

// Return a static, zero-terminated array of hashes of provided words.
HashedWord *hash_words(char *str) {
    static HashedWord hashes[max_words+1];
    uint32_t i = 0;
    char *cur_word, *first_digit;
    for (; (cur_word = next_word(&str)) && i < max_words; i++) {
        hashes[i].word = cur_word;
        if (suffixed_by_int(cur_word, &first_digit, &hashes[i].idx)) {
            char c = '#';
            // Create hash of curword + '#'
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
