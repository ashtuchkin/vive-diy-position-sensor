#include "input.h"
#include "message_logging.h"

// Multiplexer method to create input node of correct type.
// Throws exceptions on incorrect values.
std::unique_ptr<InputNode> InputNode::create(uint32_t input_idx, const InputDef &input_def) {
    switch (input_def.input_type) {
        case InputType::kCMP: return createInputCmpNode(input_idx, input_def);
        case InputType::kPort: throw_printf("Port input type not implemented yet");
        case InputType::kFTM: throw_printf("FTM input type not implemented yet");
        default: throw_printf("Unknown input type");
    }
}


InputNode::InputNode(uint32_t input_idx)
    : input_idx_(input_idx)
    , pulses_buf_() {   
}

// In all types of inputs, pulses come from irq handlers. We don't want to run any other code in 
// irq context, so pulses go through pulses_buf_ circular buffer and are sent to other modules in main "thread".
void InputNode::do_work(Timestamp cur_time) {
    Pulse p;
    while (pulses_buf_.dequeue(&p)) {
        produce(p);
    }
}

void InputNode::enqueue_pulse(Timestamp start_time, TimeDelta len) {
    pulses_buf_.enqueue({
        .input_idx = input_idx_,
        .start_time = start_time,
        .pulse_len = len,
    });
}

bool InputNode::debug_cmd(HashedWord *input_words) {
    if (*input_words == "sensor#"_hash && input_words->idx == input_idx_) {
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


// ====  InputDef I/O ================================
#include "Print.h"
#include "primitives/string_utils.h"

HashedWord input_types[kInputTypeCount] = {
    [(int)InputType::kCMP]  = { "cmp", "cmp"_hash },
    [(int)InputType::kFTM]  = { "ftm", "ftm"_hash },
    [(int)InputType::kPort] = { "port_irq", "port_irq"_hash },
};

void InputDef::print_def(uint32_t idx, Print &stream) {
    stream.printf("sensor%d pin %d %s %s", idx, pin, pulse_polarity ? "positive" : "negative", input_types[(int)input_type].word);
    if (input_type == InputType::kCMP)
        stream.printf(" %d", initial_cmp_threshold);
    stream.printf("\n");
}

bool InputDef::parse_def(uint32_t idx, HashedWord *input_words, Print &err_stream) {
    if (*input_words == "pin"_hash)
        input_words++; // Ignore "pin" word
    
    if (!input_words++->as_uint32(&pin) || pin >= 256) {
        err_stream.printf("Invalid/missing pin number\n"); return false;
    }

    switch (*input_words++) {
    case 0: err_stream.printf("Missing polarity\n"); return false;
    case "positive"_hash: pulse_polarity = true; break;
    case "negative"_hash: pulse_polarity = false; break;
    default: 
        err_stream.printf("Unknown polarity. Only 'positive' and 'negative' supported.\n"); return false;
    }

    if (!*input_words) {
        input_type = InputType::kCMP;  // Default input type: Comparator
    } else {
        int i; 
        for (i = 0; i < kInputTypeCount; i++)
            if (*input_words == input_types[i]) {
                input_type = (InputType)i;
                break;
            }
        if (i == kInputTypeCount) {
            err_stream.printf("Unknown input type. Supported types: 'port_irq', 'ftm', 'cmp'.\n"); return false;
        }
    }
    input_words++;

    if (input_type == InputType::kCMP) {
        // For comparators, also read initial threshold level.
        if (!*input_words) {
            initial_cmp_threshold = 20; // Default threshold level.
        } else if (!input_words->as_uint32(&initial_cmp_threshold) || initial_cmp_threshold >= 64) {
            err_stream.printf("Invalid threshold level. Supported values: 0-63.\n"); return false;
        }
    }
    return true;
} 

