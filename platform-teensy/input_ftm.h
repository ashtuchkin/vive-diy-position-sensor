#pragma once
#include "input.h"

// Input node using Teensy's hardware timers. See description in .cpp file.
class InputFTMNode : public InputNode {
public:
    InputFTMNode(uint32_t input_idx, const InputDef &def);
    ~InputFTMNode();

    virtual void start();
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream& stream);    

private:
    int pin_;
    bool pulse_polarity_;

    int ftm_idx_;
    int ftm_ch_idx_;
    int ftm_ch_alt_;

    static CreatorRegistrar creator_;

    friend void _ftm_isr_handler(int ftm_idx);
};
