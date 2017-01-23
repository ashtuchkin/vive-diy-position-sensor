//
// Definition of 'Comparator' type input (kCMP).
//
// Teensy 3.2 has 3 comparator modules: CMP0, CMP1, and CMP2. Each of them can issue an interrupt when
// input pin voltage is crossing a threshold, both up and down. The threshold, called `cmp_threshold` in code, can be adjusted 
// dynamically in 64 steps from 0V to 3.3V. This allows for more flexibility when dealing with analog sensors like the one
// described in this repo, but not really needed for commercial sensors.
//
// PROs:
//   * Threshold can be dynamically adjusted.
// CONs:
//   * Up to 3 sensors at the same time; Only particular pins are supported (see input_pin_variants below).
//   * Timing is calculated in ISR, leading to potential jitter.
//
#include "main.h"
#include "settings.h"

// Mapping of comparator inputs to teensy pins
struct ComparatorInputPin {
    uint32_t cmp_num;  // Comparator index (CMP0/CMP1/CMP2)
    uint32_t cmp_input;// Input # for comparator (ie. 5 => CMP0_IN5)
    uint32_t pin;      // Teensy PIN
};

const static int num_input_pin_variants = 9;
const static ComparatorInputPin input_pin_variants[num_input_pin_variants] = {
    {0, 0, 11}, // CMP0_IN0 = Pin 11
    {0, 1, 12}, // CMP0_IN1 = Pin 12
    {0, 2, 28}, // CMP0_IN2 = Pin 28
    {0, 3, 27}, // CMP0_IN3 = Pin 27
    {0, 4, 29}, // CMP0_IN4 = Pin 29
    {1, 0, 23}, // CMP1_IN0 = Pin 23
    {1, 1,  9}, // CMP1_IN1 = Pin 9
    {2, 0,  3}, // CMP2_IN0 = Pin 3
    {2, 1,  4}, // CMP2_IN1 = Pin 4
};

// Port definitions for comparators
struct ComparatorPorts {
    volatile uint8_t *cr0, *cr1, *fpr, *scr, *daccr, *muxcr;
    int irq;
};
const static ComparatorPorts comparator_port_defs[num_comparators] = {
    {&CMP0_CR0, &CMP0_CR1, &CMP0_FPR, &CMP0_SCR, &CMP0_DACCR, &CMP0_MUXCR, IRQ_CMP0},
    {&CMP1_CR0, &CMP1_CR1, &CMP1_FPR, &CMP1_SCR, &CMP1_DACCR, &CMP1_MUXCR, IRQ_CMP1},
    {&CMP2_CR0, &CMP2_CR1, &CMP2_FPR, &CMP2_SCR, &CMP2_DACCR, &CMP2_MUXCR, IRQ_CMP2},
};

// Dynamic data for each comparator.
struct ComparatorData {
    bool is_active;
    uint32_t rise_time;
    const ComparatorPorts *ports;
    int input_idx;
    int cmp_threshold;
    bool pulse_polarity;
    ComparatorInputPin pin;
};

static ComparatorData comparator_data[num_comparators] = {};
static ComparatorData *comparator_data_by_input_idx[max_num_inputs] = {};


void setCmpThreshold(const ComparatorData *pCmp, uint32_t level) { // level = 0..63, from 0V to 3V3.
    if (!pCmp->pulse_polarity)
        level = 63 - level;
    
    // DAC Control: Enable; Reference=3v3 (Vin1=VREF_OUT=1V2; Vin2=VDD=3V3); Output select=0
    *pCmp->ports->daccr = CMP_DACCR_DACEN | CMP_DACCR_VRSEL | CMP_DACCR_VOSEL(level);
}

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

void dynamicThresholdAdjustment() {
    // TODO.
    /*
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
    */
}

void resetCmpAfterValidation() {
    for (int i = 0; i < num_comparators; i++)
        comparator_data[i] = {};
}

bool setupCmpInput(uint32_t input_idx, const InputDefinition &input_def, char *error_message, bool validation_mode) {
    // Find comparator and input num for given pin.
    uint32_t cmp_num, cmp_input;
    int i = 0;
    for (; i < num_input_pin_variants; i++)
        if (input_pin_variants[i].pin == input_def.pin) {
            cmp_num = input_pin_variants[i].cmp_num;
            cmp_input = input_pin_variants[i].cmp_input;
            break;
        }
    
    if (i == num_input_pin_variants) {
        sprintf(error_message, "Pin %lu is not supported for 'cmp' input type.\n", input_def.pin);
        return false;
    }
    ComparatorData *pCmp = &comparator_data[cmp_num];
    if (pCmp->is_active) {
        sprintf(error_message, "Can't use pin %lu for a 'cmp' input type: CMP%lu is already in use by pin %lu.\n", 
                input_def.pin, pCmp->pin.cmp_num, pCmp->pin.pin);
        return false;
    }
    if (input_def.initial_cmp_threshold >= 64) {
        sprintf(error_message, "Invalid threshold value for 'cmp' input type on pin %lu. Supported values: 0-63\n", input_def.pin);
        return false;
    } 
    pCmp->is_active = true;
    pCmp->input_idx = input_idx;
    pCmp->cmp_threshold = input_def.initial_cmp_threshold;
    pCmp->pulse_polarity = input_def.pulse_polarity;
    pCmp->ports = &comparator_port_defs[cmp_num];
    pCmp->rise_time = 0;
    pCmp->pin = input_pin_variants[i];

    if (validation_mode)
        return true;
    
    comparator_data_by_input_idx[input_idx] = pCmp;

    NVIC_SET_PRIORITY(pCmp->ports->irq, 64); // very high prio (0 = highest priority, 128 = medium, 255 = lowest)
    NVIC_ENABLE_IRQ(pCmp->ports->irq);

    SIM_SCGC4 |= SIM_SCGC4_CMP; // Enable clock for comparator

    // Filter disabled; Hysteresis level 0 (0=5mV; 1=10mV; 2=20mV; 3=30mV)
    *pCmp->ports->cr0 = CMP_CR0_FILTER_CNT(0) | CMP_CR0_HYSTCTR(0);

    // Filter period - disabled
    *pCmp->ports->fpr = 0;

    // Input/MUX Control
    pinMode(input_def.pin, INPUT);
    const static int ref_input = 7; // CMPn_IN7 (DAC Reference Voltage, which we control in setCmpThreshold())
    *pCmp->ports->muxcr = input_def.pulse_polarity
        ? CMP_MUXCR_PSEL(cmp_input) | CMP_MUXCR_MSEL(ref_input)
        : CMP_MUXCR_PSEL(ref_input) | CMP_MUXCR_MSEL(cmp_input);

    // Comparator ON; Sampling disabled; Windowing disabled; Power mode: High speed; Output Pin disabled;
    *pCmp->ports->cr1 = CMP_CR1_PMODE | CMP_CR1_EN;
    setCmpThreshold(pCmp, pCmp->cmp_threshold);

    delay(5);

    // Status & Control: DMA Off; Interrupt: both rising & falling; Reset current state.
    *pCmp->ports->scr = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
    return true;
}


inline void __attribute__((always_inline)) cmp_isr(int cmp_num, volatile uint8_t *scr) {
    const uint32_t cmpState = *scr;
    const uint32_t timestamp = micros();

    ComparatorData *pCmp = &comparator_data[cmp_num];
    if (!pCmp->is_active)
        return;

    if (pCmp->rise_time && (cmpState & CMP_SCR_CFF)) { // Falling edge registered
        add_pulse(pCmp->input_idx, pCmp->rise_time, timestamp);
        pCmp->rise_time = 0;
    }

    const static uint32_t mask = CMP_SCR_CFR | CMP_SCR_COUT;
    if ((cmpState & mask) == mask) { // Rising edge registered and state is now high
        pCmp->rise_time = timestamp;
    }

    // Clear flags, re-enable interrupts.
    *scr = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}

void cmp0_isr() { cmp_isr(0, &CMP0_SCR); }
void cmp1_isr() { cmp_isr(1, &CMP1_SCR); }
void cmp2_isr() { cmp_isr(2, &CMP2_SCR); }
