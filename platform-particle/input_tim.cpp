#include "input_tim.h"
#include <application.h>

// Reference Manual: http://www.st.com/content/ccc/resource/technical/document/reference_manual/51/f7/f3/06/cd/b6/46/ec/CD00225773.pdf/files/CD00225773.pdf/jcr:content/translations/en.CD00225773.pdf
// 14.3.5 Input capture mode
// See also stm32f2xx_tim.c:1840

//  Timer   CH1     CH2     CH3     CH4     Control     Bits    Max Frequency
// --------------------------------------------------------------------------------------
//  TIM1    --      TX*     RX*     --      Advanced    16bit   SystemCoreClock (120MHz)
//  TIM2    --      LED:R   LED:G  LED:B    Basic       32bit   SystemCoreClock/2
//  TIM3    D3*/A4  D2/A5   --      --      Basic       16bit   SystemCoreClock/2
//  TIM4    D1*     D0      --      --      Basic       16bit   SystemCoreClock/2
//  TIM5    WKP*    --      --      --      Basic       32bit   SystemCoreClock/2
//  TIM8    B1      --      B0      --      Advanced    16bit   SystemCoreClock
// NOTE: Recommended pins are marked with a '*' character.
// NOTE: Pins D0-D7 -> 0..7; A0-A7 -> 10..17; RX/TX -> 18/19
// NOTE: Electron Pins: B0-B5 -> 24..29; C0-C5 -> 30-35

struct TimerConfig {
    TIM_TypeDef *timer;
    uint16_t gpio_af;  // Alternative Function
    uint16_t rcc_apb;
    uint32_t rcc_apb_periph;
    uint16_t clock_divider; 
    uint16_t cc_irq;   // # of Capture/Compare IRQ
    hal_irq_t irq_by_channel[4]; 
};

static const uint32_t num_timer_configs = 6; 
static const TimerConfig timer_config[num_timer_configs] = {
    {TIM1, GPIO_AF_TIM1, 2, RCC_APB2Periph_TIM1, 1, TIM1_CC_IRQn, {SysInterrupt_TIM1_Compare1, SysInterrupt_TIM1_Compare2, SysInterrupt_TIM1_Compare3, SysInterrupt_TIM1_Compare4}},
    {TIM2, GPIO_AF_TIM2, 1, RCC_APB1Periph_TIM2, 2, TIM2_IRQn,    {SysInterrupt_TIM2_Compare1, SysInterrupt_TIM2_Compare2, SysInterrupt_TIM2_Compare3, SysInterrupt_TIM2_Compare4}}, 
    {TIM3, GPIO_AF_TIM3, 1, RCC_APB1Periph_TIM3, 2, TIM3_IRQn,    {SysInterrupt_TIM3_Compare1, SysInterrupt_TIM3_Compare2, SysInterrupt_TIM3_Compare3, SysInterrupt_TIM3_Compare4}}, 
    {TIM4, GPIO_AF_TIM4, 1, RCC_APB1Periph_TIM4, 2, TIM4_IRQn,    {SysInterrupt_TIM4_Compare1, SysInterrupt_TIM4_Compare2, SysInterrupt_TIM4_Compare3, SysInterrupt_TIM4_Compare4}}, 
    {TIM5, GPIO_AF_TIM5, 1, RCC_APB1Periph_TIM5, 2, TIM5_IRQn,    {SysInterrupt_TIM5_Compare1, SysInterrupt_TIM5_Compare2, SysInterrupt_TIM5_Compare3, SysInterrupt_TIM5_Compare4}}, 
    {TIM8, GPIO_AF_TIM8, 2, RCC_APB2Periph_TIM8, 1, TIM8_CC_IRQn, {SysInterrupt_TIM8_Compare1, SysInterrupt_TIM8_Compare2, SysInterrupt_TIM8_Compare3, SysInterrupt_TIM8_Compare4}}, 
};

struct TimerChannelConfig {
    uint16_t channel;
    uint16_t paired_channel;
    uint16_t it_flag; // TIM_IT_CC1
};

static const uint32_t num_timer_channels = 4;
static const TimerChannelConfig timer_channel_config[num_timer_channels] = {
    {TIM_Channel_1, TIM_Channel_2, TIM_IT_CC1},
    {TIM_Channel_2, TIM_Channel_1, TIM_IT_CC2},
    {TIM_Channel_3, TIM_Channel_4, TIM_IT_CC3},
    {TIM_Channel_4, TIM_Channel_3, TIM_IT_CC4},
};

// Track usage of channel pairs (we use 2 channels per pin to get timing of start and end of the pulse)
static uint8_t channel_pair_used_by_pin[num_timer_configs][num_timer_channels/2] = {0};


InputTimNode::InputTimNode(uint32_t input_idx, const InputDef &input_def)
    : InputNode(input_idx)
    , pin_(input_def.pin)
    , pulse_polarity_(input_def.pulse_polarity) {

    // Sanity checks.
    assert(input_def.input_type == InputType::kTimer);
    assert((SystemCoreClock/2) % sec == 0); // Time resolution must be whole number of timer ticks

    if (pin_ >= TOTAL_PINS)
        throw_printf("Pin number too large: %d", pin_);
    
    // Find the timer that serves given pin.
    STM32_Pin_Info& pin_info = HAL_Pin_Map()[pin_];
    timer_ = pin_info.timer_peripheral;
    if (!timer_)
        throw_printf("Pin %d is not connected to any timer", pin_);
    
    for (timer_idx_ = 0; timer_idx_ < num_timer_configs; timer_idx_++)
        if (timer_config[timer_idx_].timer == timer_)
            break;
    
    assert(timer_idx_ < num_timer_configs);

    // Get the channel id for the pin. Convert from TIM_Channel_N -> 0..3
    channel_idx_ = (pin_info.timer_ch - TIM_Channel_1) / (TIM_Channel_2 - TIM_Channel_1);

    // Check that the channel pair for current pin is still available.
    if (channel_pair_used_by_pin[timer_idx_][channel_idx_/2])
        throw_printf("Pin %d conflicts with pin %d (same channel pair).", pin_, channel_pair_used_by_pin[timer_idx_][channel_idx_/2]-1);
    
    channel_pair_used_by_pin[timer_idx_][channel_idx_/2] = pin_+1;  // Store pin+1 to keep 0 as an 'available' flag.
}

InputNode::CreatorRegistrar InputTimNode::creator_([](uint32_t input_idx, const InputDef &input_def) -> std::unique_ptr<InputNode> {
    if (input_def.input_type == InputType::kTimer)
        return std::make_unique<InputTimNode>(input_idx, input_def);
    return nullptr;
});

InputTimNode::~InputTimNode() {
    // Deallocate channel pair.
    channel_pair_used_by_pin[timer_idx_][channel_idx_/2] = 0;

    // Remove our interrupt handler.
    HAL_Set_System_Interrupt_Handler(timer_config[timer_idx_].irq_by_channel[channel_idx_], NULL, NULL, NULL);
}


void InputTimNode::start() {
    InputNode::start();

    // See also HAL_Tone_Start for example of how timers are initialized.
    const TimerConfig &conf = timer_config[timer_idx_];
    const TimerChannelConfig &channel_conf = timer_channel_config[channel_idx_];

    // Enable clock for our timer
    switch (conf.rcc_apb) {
        case 1: RCC_APB1PeriphClockCmd(conf.rcc_apb_periph, ENABLE); break;
        case 2: RCC_APB2PeriphClockCmd(conf.rcc_apb_periph, ENABLE); break;
        default: assert(false);
    }

    // Enable Pin Alternative Configuration
    HAL_Pin_Mode(pin_, AF_OUTPUT_PUSHPULL);  // It's actually INPUT, but we do care about AF - alternative function.
    const STM32_Pin_Info &pin_info = HAL_Pin_Map()[pin_];
    GPIO_PinAFConfig(pin_info.gpio_peripheral, pin_info.gpio_pin_source, conf.gpio_af);

    // Enable IRQ Channel
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  // Highest priority.
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannel = conf.cc_irq;
    NVIC_Init(&NVIC_InitStructure);

    // Attach System Interrupt callback
    HAL_InterruptCallback callback = {
        [](void *data) { ((InputTimNode *)data)->_irq_handler(); }, this
    };
    HAL_Set_System_Interrupt_Handler(conf.irq_by_channel[channel_idx_], &callback, NULL, NULL);

    // Initialize Timer to use our Timestamp resolution via prescaler.
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    TIM_TimeBaseInitStruct.TIM_Prescaler = ((SystemCoreClock / conf.clock_divider) / sec) - 1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(timer_, &TIM_TimeBaseInitStruct);

    // Init Input Capture mode to capture both rising and falling edges, using pair of channels.
    TIM_ICInitTypeDef TIM_ICInitStruct;
    TIM_ICInitStruct.TIM_Channel = channel_conf.channel;
    TIM_ICInitStruct.TIM_ICPolarity = pulse_polarity_ ? TIM_ICPolarity_Falling : TIM_ICPolarity_Rising;
    TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStruct.TIM_ICFilter = 0x0;
    TIM_ICInit(timer_, &TIM_ICInitStruct);

    TIM_ICInitStruct.TIM_Channel = channel_conf.paired_channel;
    TIM_ICInitStruct.TIM_ICPolarity = pulse_polarity_ ? TIM_ICPolarity_Rising : TIM_ICPolarity_Falling;
    TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_IndirectTI;
    TIM_ICInit(timer_, &TIM_ICInitStruct);
    
    // Enable interrupt and timer.
    TIM_ITConfig(timer_, channel_conf.it_flag, ENABLE);
    TIM_Cmd(timer_, ENABLE);

    // Synchronize timer counter and current Timestamp
    TIM_SetCounter(timer_, Timestamp::cur_time().get_raw_value() & 0xFFFF);
}

void InputTimNode::_irq_handler() {
    Timestamp cur_time = Timestamp::cur_time();
    uint16_t pulse_stop  = (&timer_->CCR1)[channel_idx_];
    uint16_t pulse_start = (&timer_->CCR1)[channel_idx_ ^ 1];  // paired channel

    // We assume that the pulse length is within one timer period.
    uint16_t pulse_len = pulse_stop - pulse_start;
    TimeDelta pulse_len_ts(pulse_len, (TimeUnit)1);
    
    // We also assume that the time between end of pulse and interrupt is also within one period.
    // Plus, the timer counter is synchronized with the Timestamp's lower 16 bits.
    uint16_t time_from_pulse_stop = (uint16_t)(cur_time.get_raw_value() & 0xFFFF) - pulse_stop;
    if (time_from_pulse_stop > 0xFF00) time_from_pulse_stop = 0;   // Overflow precaution.
    TimeDelta time_from_pulse_stop_ts(time_from_pulse_stop, (TimeUnit)1);

    Timestamp pulse_start_ts(cur_time - time_from_pulse_stop_ts - pulse_len_ts);
    InputNode::enqueue_pulse(pulse_start_ts, pulse_len_ts);
}


bool InputTimNode::debug_cmd(HashedWord *input_words) {
    return InputNode::debug_cmd(input_words);
}

void InputTimNode::debug_print(PrintStream& stream) {
    InputNode::debug_print(stream);
}

