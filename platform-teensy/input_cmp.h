#pragma once
#include "input.h"

struct ComparatorPorts;
struct ComparatorDef;

// Input node using Teensy's comparator modules. See description in .cpp file.
class InputCmpNode : public InputNode {
public:
    InputCmpNode(uint32_t input_idx, const InputDef &def);
    ~InputCmpNode();

    virtual void start();
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream& stream);    

    void _isr_handler(volatile uint8_t *scr);

private:
    void setCmpThreshold(uint32_t level);

    Timestamp rise_time_;
    bool rise_valid_;
    uint32_t cmp_threshold_;
    bool pulse_polarity_;

    int cmp_idx_;
    int cmp_input_idx_;
    volatile ComparatorPorts *ports_;
    const ComparatorDef *cmp_def_;

    static CreatorRegistrar creator_;
};
