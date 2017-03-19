#pragma once
#include "input.h"
#include <stm32f2xx.h>

// Input node using STM32F2xx TIM timer modules. See description in .cpp file.
class InputTimNode : public InputNode {
public:
    InputTimNode(uint32_t input_idx, const InputDef &def);
    ~InputTimNode();

    virtual void start();
    virtual bool debug_cmd(HashedWord *input_words);
    virtual void debug_print(PrintStream& stream);    

private:
    void _irq_handler();

    // Denormalized configuration
    uint32_t pin_;
    bool pulse_polarity_;
    TIM_TypeDef *timer_;
    uint32_t timer_idx_;
    uint32_t channel_idx_;

    static CreatorRegistrar creator_;
};
