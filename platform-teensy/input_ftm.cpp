//
// Definition of 'Hardware Timer' type input (InputType::kTimer) for Teensy.
//
// Teensy 3.x has up to 4 Flexible Timer Modules: FTM0, FTM1, FTM2, and FTM3. 
// We use Double Edge Capture mode, using 2 channels per pin to capture both rising and falling edges.
// See 36.4.24 Dual Edge Capture mode in K10 reference manual.
// 
// Teensy 3.0 usable pins (5 independent): (3|16), 6, (9|24), 21, 22.
// Teensy 3.1, 3.2 usable pins (6 independent): (3|16), 6, (9|24), 21, 22, 32.
// Teensy 3.5, 3.6 usable pins (10 independent): 2, (3|16), 6, 7, (9|13|25), 21, 22, 29, 35, (37|57)
//
// PROs:
//   * More stable timing, less jitter. Independent from other things happening on the microcontroller.
//   * More pins and independent channels available than in comparator.
// CONs:
//   * Voltage threshold is not adjustable.
//
#include "input_ftm.h"
#include "settings.h"
#include <Arduino.h>

constexpr int num_channels = 8;
constexpr int num_channel_alts = 3;  // Up to 2 pins per channel.

// FTM ports address block. Order is significant.
// See 36.3.1 Memory map in K10 Family Manual for reference.
struct FTMPorts {
    volatile uint32_t sc, cnt, mod;
    struct {
        volatile uint32_t sc, v;
    } ch[num_channels];
    volatile uint32_t cntin, status, mode, sync, outinit, outmask, combine, deadtime, 
        exttrig, pol, fms, filter, fltctrl, qdctrl, conf, fltpol, synconf, invctrl, swoctrl, pwmload;
};
static_assert(sizeof(FTMPorts) == 0x9C, "FTMPorts struct must be packed.");

// Static configuration of an FTM module: ports base address, irq number and pin numbers
struct FTMDef {
    union {
        volatile uint32_t *port_sc;
        FTMPorts *ports;
    };
    int irq;
    int channel_pins[num_channels][num_channel_alts];  // FTMx_CHy to Teensy digital pin # mapping.
    int pin_mux[num_channel_alts]; // Pin alternative function assignment, depending on mapping.
};

constexpr int NA = 0; // Pin Not Available

// FTM definitions: Ports, Interrupts and Input pins.
constexpr static FTMDef timer_defs[] = {
#if defined(__MK20DX128__) || defined(__MK20DX256__)  // Teensy 3.0, 3.1, 3.2. Chip 64 LQFP pin numbers in comments. Teensy digital pins as numbers.
    {&FTM0_SC, IRQ_FTM0, {{/*25*/NA, /*44*/22}, {/*26*/33, /*45*/23},  {/*27*/24, /*46*/ 9}, {      NA, /*49*/10},
                          {      NA, /*61*/ 6}, {/*22*/NA, /*62*/20},  {/*23*/NA, /*63*/21}, {/*24*/NA, /*64*/ 5}}, {3, 4}},
    {&FTM1_SC, IRQ_FTM1, {{/*28*/ 3, /*35*/16}, {/*29*/ 4, /*36*/17}}, {3, 3}},
  #if defined(__MK20DX256__)
    {&FTM2_SC, IRQ_FTM2, {{/*41*/32,       NA}, {/*42*/25,       NA}}, {3, 3}},
  #endif
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)  // Teensy 3.5, 3.6. Chip 144 MAPBGA pin id-s in comments. Teensy digital pins as numbers.
    {&FTM0_SC, IRQ_FTM0, {{/*K7*/NA, /*B11*/22}, {/*L7*/NA, /*A12*/23}, {/*M8*/25, /*A11*/ 9, /*D8*/13}, {/*J7*/NA, /*A9*/10}, 
                          {/*J8*/NA, /*A4 */ 6}, {/*J5*/NA, /*A3 */20}, {/*J6*/NA, /*A2 */21}, {/*K6*/NA, /*A1*/5}}, {3, 4, 7}},
    {&FTM1_SC, IRQ_FTM1, {{/*K8*/NA, /*K9 */ 3, /*H10*/16}, {/*L8*/NA,   /*J9*/ 4, /*H9 */17}}, {3, 3, 3}},
    {&FTM2_SC, IRQ_FTM2, {{/*M9*/NA, /*D12*/29}, {/*L9*/NA, /*D11*/30}}, {3, 3}},
    {&FTM3_SC, IRQ_FTM3, {{/*E2*/NA, NA, /*A5*/2}, {/*E1*/NA, NA, /*D4*/14}, {/*F4*/NA, NA, /*C4*/7}, {/*F3*/NA, NA, /*B4*/8}, 
                          {/*F2*/NA, /*A8 */35}, {/*F1*/56, /*D7*/36}, {/*G4*/57, /*C7*/37}, {/*G3*/NA, /*B7*/38}}, {6, 3, 4}},
#endif
};

constexpr int num_timers = sizeof(timer_defs) / sizeof(timer_defs[0]);

// Registered InputFTMNode-s, by FTM num. Used by interrupt handlers.
static InputFTMNode *ftms_used[num_timers][num_channels];  

InputFTMNode::InputFTMNode(uint32_t input_idx, const InputDef &input_def)
    : InputNode(input_idx)
    , pin_(input_def.pin)
    , pulse_polarity_(input_def.pulse_polarity) {

    // Find FTM and input num for given pin.
    bool pin_found = false;
    for (int i = 0; i < num_timers; i++)
        for (int j = 0; j < num_channels; j++)
            for (int k = 0; k < num_channel_alts; k++)
                if (timer_defs[i].channel_pins[j][k] == (int)input_def.pin) {
                    ftm_idx_ = i;
                    ftm_ch_idx_ = j;
                    ftm_ch_alt_ = k;
                    pin_found = true;
                }
    
    // Don't allow odd channel numbers.
    if (!pin_found || (ftm_ch_idx_ & 1) || input_def.pin == NA)
        throw_printf("Pin %lu is not supported for 'timer' input type.\n", pin_);

    InputFTMNode *otherInput = ftms_used[ftm_idx_][ftm_ch_idx_];
    if (otherInput)
        throw_printf("Can't use pin %lu for a 'timer' input type: FTM%lu channel %lu is already in use by pin %lu.\n", 
                                pin_, ftm_idx_, ftm_ch_idx_, otherInput->pin_);
    
    ftms_used[ftm_idx_][ftm_ch_idx_] = this;
}

InputNode::CreatorRegistrar InputFTMNode::creator_([](uint32_t input_idx, const InputDef &input_def) -> std::unique_ptr<InputNode> {
    if (input_def.input_type == InputType::kTimer)
        return std::make_unique<InputFTMNode>(input_idx, input_def);
    return nullptr;
});

InputFTMNode::~InputFTMNode() {
    ftms_used[ftm_idx_][ftm_ch_idx_] = nullptr;
}

constexpr int log2(int val) {
    int res = 0;
    while (val > 1) {
        if (val % 2) return -1;  // Not exact power-of-two.
        val /= 2; res++;
    }
    return res;
}

void InputFTMNode::start() {
    InputNode::start();

    // Enable interrupts
    auto ftm_def = &timer_defs[ftm_idx_];
    NVIC_SET_PRIORITY(ftm_def->irq, 64); // very high prio (0 = highest priority, 128 = medium, 255 = lowest)
    NVIC_ENABLE_IRQ(ftm_def->irq);

    // Clocks are already enabled (see SIM_SCGC6_FTM0, SIM_SCGC6_FTM1, SIM_SCGC3_FTM2, SIM_SCGC3_FTM3).

    // Set pin mode to input and set alternative routing to connect to FTM.
    pinMode(pin_, INPUT);
    *portConfigRegister(pin_) = PORT_PCR_MUX(ftm_def->pin_mux[ftm_ch_alt_]);

    // Calculate prescaler so that timer would be in the same units as our timestamp.
    constexpr int prescaler = log2(F_BUS / sec);
    static_assert(F_BUS % sec == 0, "Timestamp unit must be a divisor of system bus frequency.");
    static_assert(0 <= prescaler && prescaler < 8, "(F_BUS / Timestamp unit) must be power-of-2 and <= 128");

    // Common Timer settings: Free-running counter with 16bit resolution.
    auto ports = ftm_def->ports;
    ports->sc = FTM_SC_PS(prescaler); // Stop clock, set prescaler
    ports->cntin = 0; // Initial value of the counter.
    ports->mod = 0xFFFF; // Use full 16 bit counting.
    ports->mode |= FTM_MODE_FTMEN; // Enable extended registers

    // Configure channels for Double Edge Capture and enable it for given channel pair
    constexpr uint32_t rise_edge = FTM_CSC_ELSA, fall_edge = FTM_CSC_ELSB;
    ports->ch[ftm_ch_idx_+0].sc = (pulse_polarity_ ? rise_edge : fall_edge) | FTM_CSC_MSA;  // MSA -> Continuous mode
    ports->ch[ftm_ch_idx_+1].sc = (pulse_polarity_ ? fall_edge : rise_edge) | FTM_CSC_CHIE;  // Only generate IRQ at the end of the pulse
    ports->combine |= FTM_COMBINE_DECAPEN0 << (ftm_ch_idx_/2*8);

    // Enable module clock (F_BUS) and start double edge capture
    ports->combine |= FTM_COMBINE_DECAP0 << (ftm_ch_idx_/2*8);
    ports->sc |= FTM_SC_CLKS(1);
}

bool InputFTMNode::debug_cmd(HashedWord *input_words) {
    return InputNode::debug_cmd(input_words);
}

void InputFTMNode::debug_print(PrintStream& stream) {
    InputNode::debug_print(stream);    
}

inline void __attribute__((always_inline)) _ftm_isr_handler(int ftm_idx) {
    FTMPorts *ports = timer_defs[ftm_idx].ports;
    uint32_t status = ports->status;
    if (status) {
        // Get reference timer count and timestamp. We'll use them in time calculation later.
        // Note, these two statements must be next to each other.
        uint32_t cur_timer_cnt = ports->cnt;
        Timestamp cur_time = Timestamp::cur_time();

        // Calculate time and send pulses to all registered inputs.
        for (int i = 0; i < num_channels; i += 2, status >>= 2)
            if ((status & 2) && ftms_used[ftm_idx][i]) {
                // Read value of timers at start of pulse and end of pulse.
                uint32_t start = ports->ch[i].v;
                uint32_t end = ports->ch[i+1].v;

                // Calculate needed periods of time, using the fact that overflowing uint16
                // will give us correct results (assuming, of course, that periods are less
                // than full 16bit FTM period).
                TimeDelta pulse_len(uint16_t(end - start), (TimeUnit)1),
                          delay_from_pulse_end(uint16_t(cur_timer_cnt - end), (TimeUnit)1);
                
                Timestamp end_pulse = cur_time - delay_from_pulse_end;
                
                ftms_used[ftm_idx][i]->InputNode::enqueue_pulse(end_pulse-pulse_len, pulse_len);
            }
        ports->status = 0;
    }
}

void ftm0_isr() { _ftm_isr_handler(0); }
void ftm1_isr() { _ftm_isr_handler(1); }
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
void ftm2_isr() { _ftm_isr_handler(2); }
#endif
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
void ftm3_isr() { _ftm_isr_handler(3); }
#endif