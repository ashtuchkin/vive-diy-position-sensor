//
// Definition of 'Comparator' type input (InputType::kCMP).
//
// Teensy 3.x has up to 4 comparator modules: CMP0, CMP1, CMP2 and CMP3. Each of them can issue an interrupt when
// input pin voltage is crossing a threshold, both up and down. The threshold can be adjusted dynamically 
// in 64 steps from 0V to 3.3V. This allows for more flexibility when dealing with analog sensors like the one
// described in this repo, but not really needed for commercial sensors.
//
// PROs:
//   * Threshold can be dynamically adjusted.
// CONs:
//   * Up to 4 (3 on Teensy 3.2) sensors at the same time; Only particular pins are supported (see comparator_defs below).
//   * Timing is calculated in ISR, potentially increasing jitter.
//
#include "input_cmp.h"
#include "settings.h"
#include <Arduino.h>

// Teensy comparator port address block. Order is significant.
struct ComparatorPorts {
    volatile uint8_t cr0, cr1, fpr, scr, daccr, muxcr;
};
static_assert(sizeof(ComparatorPorts) == 6, "ComparatorPorts struct must be packed.");

constexpr int NA = -1; // Pin Not Available

// Static configuration of a comparator: control ports base address, irq number and pin numbers
struct ComparatorDef {
    union {
        volatile uint8_t *port_cr0;
        ComparatorPorts *ports;
    };
    int irq;
    int input_pins[6];  // CMPx_INy to Teensy digital pin # mapping. DAC0=100, DAC1=101.
};

// Comparator pin definitions.
const static ComparatorDef comparator_defs[] = {
#if defined(__MK20DX128__)  // Teensy 3.0. Chip 64 LQFP pin numbers in comments. Teensy digital pins as numbers.
    {&CMP0_CR0, IRQ_CMP0, {/*51 */11, /*52 */12, /*53 */28, /*54 */ 27,  /*-- */NA,  /*17*/NA}},
    {&CMP1_CR0, IRQ_CMP1, {/*45 */23, /*46 */ 9,        NA,         NA,         NA,  /*17*/NA}},
#elif defined(__MK20DX256__)  // Teensy 3.1, 3.2. Chip 64 LQFP pin numbers in comments. Teensy digital pins as numbers, DAC0=100
    {&CMP0_CR0, IRQ_CMP0, {/*51 */11, /*52 */12, /*53 */28, /*54 */ 27,  /*55 */29,  /*17*/NA}},
    {&CMP1_CR0, IRQ_CMP1, {/*45 */23, /*46 */ 9,        NA, /*18 */100,         NA,  /*17*/NA}},
    {&CMP2_CR0, IRQ_CMP2, {/*28 */ 3, /*29 */ 4,        NA,         NA,         NA,        NA}},
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)  // Teensy 3.5, 3.6. Chip 144 MAPBGA pin id-s in comments. Teensy digitial pins as numbers, DAC0=100, DAC1=101
    {&CMP0_CR0, IRQ_CMP0, {/*C8 */11, /*B8 */12, /*A8 */35, /*D7 */ 36, /*L4 */101, /*M3 */NA}},
    {&CMP1_CR0, IRQ_CMP1, {/*A12*/23, /*A11*/ 9, /*J3 */NA, /*L3 */100,         NA, /*M3 */NA}},
    {&CMP2_CR0, IRQ_CMP2, {/*K9 */ 3, /*J9 */ 4, /*K3 */NA, /*L4 */101,         NA,        NA}},
  #if defined(__MK66FX1M0__)
    {&CMP3_CR0, IRQ_CMP3, {       NA, /*L11*/27, /*K10*/28,         NA, /*K12*/ NA, /*J12*/NA}},
  #endif
#endif
};

constexpr int num_comparators = sizeof(comparator_defs) / sizeof(comparator_defs[0]);
constexpr int num_inputs = sizeof(comparator_defs[0].input_pins) / sizeof(comparator_defs[0].input_pins[0]);

// Registered InputCmpNode-s, by comparator num. Used by interrupt handlers.
static InputCmpNode *input_cmps[num_comparators];  


void InputCmpNode::setCmpThreshold(uint32_t level) { // level = 0..63, from 0V to 3V3.
    if (!pulse_polarity_)
        level = 63 - level;
    
    // DAC Control: Enable; Reference=3v3 (Vin1=VREF_OUT=1V2; Vin2=VDD=3V3); Output select=0
    ports_->daccr = CMP_DACCR_DACEN | CMP_DACCR_VRSEL | CMP_DACCR_VOSEL(level);
}
/*
inline ComparatorData *cmpDataFromInputIdx(uint32_t input_idx) {
    ComparatorData *pCmp = comparator_data_by_input_idx[input_idx];
    if (!pCmp || !pCmp->is_active)
        return NULL;
    return pCmp;
}

void changeCmpThreshold(uint32_t input_idx, int delta) {
    if (ComparatorData *pCmp = cmpDataFromInputIdx(input_idx)) {
        pCmp->cmp_threshold = (uint32_t)max(0, min(63, (int)pCmp->cmp_threshold + delta));
        setCmpThreshold(pCmp, pCmp->cmp_threshold);
    }
}

int getCmpLevel(uint32_t input_idx) {  // Current signal level: 0 or 1 (basically, below or above threshold)
    if (ComparatorData *pCmp = cmpDataFromInputIdx(input_idx))
        return *pCmp->ports->daccr & CMP_SCR_COUT;
    else
        return 0;
}

int getCmpThreshold(uint32_t input_idx) {
    if (ComparatorData *pCmp = cmpDataFromInputIdx(input_idx))
        return pCmp->cmp_threshold;
    else
        return 0;
}
*/
/*
void dynamicThresholdAdjustment() {
    // TODO.
    // 1. Comparator level dynamic adjustment.
    __disable_irq();
    uint32_t crossings = d.crossings; d.crossings = 0;
    uint32_t big_pulses = d.big_pulses; d.big_pulses = 0;
    uint32_t small_pulses = d.small_pulses; d.small_pulses = 0;
    //uint32_t fake_big_pulses = d.fake_big_pulses; d.fake_big_pulses = 0;
    __enable_irq();

    if (crossings > 100) { // Comparator level is at the background noise level. Try to increase it.
        changeCmpThreshold(d, +3);

    } else if (crossings == 0) { // No crossings of comparator level => too high or too low.
        if (getCmpLevel(d) == 1) { // Level is too low
            changeCmpThreshold(d, +16);
        } else { // Level is too high
            changeCmpThreshold(d, -9);
        }
    } else {

        if (big_pulses <= 6) {
            changeCmpThreshold(d, -4);
        } else if (big_pulses > 10) {
            changeCmpThreshold(d, +4);
        } else if (small_pulses < 4) {

            // Fine tune - we need 4 small pulses.
            changeCmpThreshold(d, -1);
        }
    }
}
*/


InputCmpNode::InputCmpNode(uint32_t input_idx, const InputDef &input_def)
    : InputNode(input_idx)
    , rise_time_()
    , rise_valid_(false)
    , cmp_threshold_(input_def.initial_cmp_threshold)
    , pulse_polarity_(input_def.pulse_polarity) {

    // Find comparator and input num for given pin.
    int cmp_idx, cmp_input_idx;
    bool pin_found = false;
    for (int i = 0; i < num_comparators; i++)
        for (int j = 0; j < num_inputs; j++)
            if (comparator_defs[i].input_pins[j] == (int)input_def.pin) {
                cmp_idx = i;
                cmp_input_idx = j;
                pin_found = true;
            }
    
    if (!pin_found)
        throw_printf("Pin %lu is not supported for 'cmp' input type.\n", input_def.pin);

    InputCmpNode *otherInput = input_cmps[cmp_idx];
    if (otherInput)
        throw_printf("Can't use pin %lu for a 'cmp' input type: CMP%lu is already in use by pin %lu.\n", 
                                input_def.pin, cmp_idx, comparator_defs[cmp_idx].input_pins[otherInput->cmp_input_idx_]);
    
    if (input_def.initial_cmp_threshold >= 64)
        throw_printf("Invalid threshold value for 'cmp' input type on pin %lu. Supported values: 0-63\n", input_def.pin);

    cmp_idx_ = cmp_idx;
    cmp_input_idx_ = cmp_input_idx;
    cmp_def_ = &comparator_defs[cmp_idx_];
    ports_ = cmp_def_->ports;
    input_cmps[cmp_idx_] = this;
}

InputNode::CreatorRegistrar InputCmpNode::creator_([](uint32_t input_idx, const InputDef &input_def) -> std::unique_ptr<InputNode> {
    if (input_def.input_type == InputType::kCMP)
        return std::make_unique<InputCmpNode>(input_idx, input_def);
    return nullptr;
});

InputCmpNode::~InputCmpNode() {
    input_cmps[cmp_idx_] = nullptr;
}

void InputCmpNode::start() {
    InputNode::start();

    NVIC_SET_PRIORITY(cmp_def_->irq, 64); // very high prio (0 = highest priority, 128 = medium, 255 = lowest)
    NVIC_ENABLE_IRQ(cmp_def_->irq);

    SIM_SCGC4 |= SIM_SCGC4_CMP; // Enable clock for comparator

    // Filter disabled; Hysteresis level 0 (0=5mV; 1=10mV; 2=20mV; 3=30mV)
    ports_->cr0 = CMP_CR0_FILTER_CNT(0) | CMP_CR0_HYSTCTR(0);

    // Filter period - disabled
    ports_->fpr = 0;

    // Input/MUX Control
    pinMode(cmp_def_->input_pins[cmp_input_idx_], INPUT);
    const static uint32_t ref_input = 7; // CMPn_IN7 (DAC Reference Voltage, which we control in setCmpThreshold())
    ports_->muxcr = pulse_polarity_
        ? CMP_MUXCR_PSEL(cmp_input_idx_) | CMP_MUXCR_MSEL(ref_input)
        : CMP_MUXCR_PSEL(ref_input) | CMP_MUXCR_MSEL(cmp_input_idx_);

    // Comparator ON; Sampling disabled; Windowing disabled; Power mode: High speed; Output Pin disabled;
    ports_->cr1 = CMP_CR1_PMODE | CMP_CR1_EN;
    setCmpThreshold(cmp_threshold_);

    delay(5);

    // Status & Control: DMA Off; Interrupt: both rising & falling; Reset current state.
    ports_->scr = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}

bool InputCmpNode::debug_cmd(HashedWord *input_words) {
    // TODO: Threshold manipulation (changeCmpThreshold).
    return InputNode::debug_cmd(input_words);
}

void InputCmpNode::debug_print(PrintStream& stream) {
    InputNode::debug_print(stream);    
}

inline void __attribute__((always_inline)) InputCmpNode::_isr_handler(volatile uint8_t *scr) {
    const uint32_t cmpState = *scr;
    const Timestamp timestamp = Timestamp::cur_time();

    if (rise_valid_ && (cmpState & CMP_SCR_CFF)) { // Falling edge registered
        InputNode::enqueue_pulse(rise_time_, timestamp - rise_time_);
        rise_valid_ = false;
    }

    const static uint32_t mask = CMP_SCR_CFR | CMP_SCR_COUT;
    if ((cmpState & mask) == mask) { // Rising edge registered and state is now high
        rise_time_ = timestamp;
        rise_valid_ = true;
    }

    // Clear flags, re-enable interrupts.
    *scr = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}

void cmp0_isr() { if (input_cmps[0]) input_cmps[0]->_isr_handler(&CMP0_SCR); }
void cmp1_isr() { if (input_cmps[1]) input_cmps[1]->_isr_handler(&CMP1_SCR); }
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
void cmp2_isr() { if (input_cmps[2]) input_cmps[2]->_isr_handler(&CMP2_SCR); }
#endif
#if defined(__MK66FX1M0__)
void cmp3_isr() { if (input_cmps[3]) input_cmps[3]->_isr_handler(&CMP3_SCR); }
#endif