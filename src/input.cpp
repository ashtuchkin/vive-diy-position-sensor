#include "input.h"
#include "message_logging.h"
#include "input_cmp.h"

// Multiplexer method to create input node of correct type.
// Throws exceptions on incorrect values.
InputNode *InputNode::create(uint32_t input_idx, const InputDefinition &input_def) {
    switch (input_def.input_type) {
        case kCMP: return new InputCmpNode(input_idx, input_def);
        case kPort: throw std::runtime_error("Port input type not implemented yet");
        case kFTM: throw std::runtime_error("FTM input type not implemented yet");
        default: throw std::runtime_error("Unknown input type");
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
        p.input_idx = input_idx_;
        produce(p);
    }
}

void InputNode::enqueue_pulse(Timestamp start_time, TimeDelta len) {
    pulses_buf_.enqueue({
        .input_idx = 0,  // This will be set on dequeue.
        .start_time = start_time,
        .pulse_len = len,
    });
}

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


// ====  InputDefinition I/O ================================
#include "Print.h"
#include "primitives/string_utils.h"

uint32_t input_type_hashes[kMaxInputType] = {"cmp"_hash, "ftm"_hash, "port_irq"_hash};
const char *input_type_names[kMaxInputType] = {"cmp", "ftm", "port_irq"};

void InputDefinition::print_def(uint32_t idx, Print &stream) {
    stream.printf("i%d pin %d %s %s", idx, pin, pulse_polarity ? "positive" : "negative", input_type_names[input_type]);
    if (input_type == kCMP)
        stream.printf(" %d", initial_cmp_threshold);
    stream.println();
}
bool InputDefinition::parse_def(HashedWord *input_words, Print &stream) {
    if (*input_words == "pin"_hash)
        input_words++; // Ignore "pin" word
    
    if (!input_words++->as_uint32(&pin) || pin >= CORE_NUM_TOTAL_PINS) {
        stream.printf("Invalid/missing pin number\n"); return false;
    }

    switch (*input_words++) {
    case 0: stream.printf("Missing polarity\n"); return false;
    case "positive"_hash: pulse_polarity = true; break;
    case "negative"_hash: pulse_polarity = false; break;
    default: 
        stream.printf("Unknown polarity. Only 'positive' and 'negative' supported.\n"); return false;
    }

    if (!*input_words) {
        // Use default input type: Comparator
        input_type = kCMP;
    } else {
        int i; 
        for (i = 0; i < kMaxInputType; i++)
            if (*input_words == input_type_hashes[i]) {
                input_type = (InputType)i;
                break;
            }
        if (i == kMaxInputType) {
            stream.printf("Unknown input type. Supported types: 'port_irq', 'ftm', 'cmp'.\n"); return false;
        }
    }
    input_words++;

    if (input_type == kCMP) {
        // For comparators, also read initial threshold level.
        if (!*input_words) {
            initial_cmp_threshold = 20; // Default threshold level.
        } else if (!input_words->as_uint32(&initial_cmp_threshold) || initial_cmp_threshold >= 64) {
            stream.printf("Invalid threshold level. Supported values: 0-63.\n"); return false;
        }
    } else {
        stream.printf("ftm and port_irq input types are not supported yet.\n"); return false;
    }
    return true;
} 

