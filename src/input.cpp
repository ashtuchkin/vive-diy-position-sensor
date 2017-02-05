#include "input.h"
#include "message_logging.h"

bool InputNode::debug_cmd(HashedWord *input_words) {
    if (*input_words == "inp#"_hash && input_words->idx == input_idx_) {
        input_words++;
        switch (*input_words++) {
            case "pulses"_hash: return producer_debug_cmd<Pulse>(this, input_words, "Pulse", input_idx_);
        }
    }
    return false;
}

void InputNode::debug_print(Print& stream) {
    producer_debug_print<Pulse>(this, stream);
}