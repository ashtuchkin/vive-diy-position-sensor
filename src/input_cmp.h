#pragma once
#include "input.h"

struct ComparatorPorts;
struct ComparatorInputPin;

class InputCmpNode : public InputNode {
public:
    InputCmpNode(uint32_t input_idx, const InputDefinition &def);
    ~InputCmpNode();

    virtual void start();
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(Print& stream);    

    void _isr_handler(volatile uint8_t *scr);

private:
    void setCmpThreshold(uint32_t level);

    Timestamp rise_time_;
    bool rise_valid_;
    uint32_t cmp_threshold_;
    bool pulse_polarity_;
    const ComparatorPorts *ports_;
    const ComparatorInputPin *pin_;
};
