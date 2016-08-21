#include "main.h"

void setupComparator() {
    NVIC_SET_PRIORITY(IRQ_CMP0, 64); // very high prio (0 = highest priority, 128 = medium, 255 = lowest)
    NVIC_ENABLE_IRQ(IRQ_CMP0);

    SIM_SCGC4 |= SIM_SCGC4_CMP; // Enable clock for comparator

    // Filter disabled; Hysteresis level 0 (0=5mV; 1=10mV; 2=20mV; 3=30mV)
    CMP0_CR0 = CMP_CR0_FILTER_CNT(0) | CMP_CR0_HYSTCTR(0);

    // Filter period - disabled
    CMP0_FPR = 0;

    // MUX Control:
    pinMode(12, INPUT);
    const int plusInput = 1;  // CMP0_IN1 (Pin 12)
    const int minusInput = 7; // CMP0_IN7 (DAC Reference)
    CMP0_MUXCR = (plusInput << 3) | minusInput;

    // Comparator ON; Sampling disabled; Windowing disabled; Power mode: High speed; Output Pin disabled;
    CMP0_CR1 = CMP_CR1_PMODE | CMP_CR1_EN;

    setCmpDacLevel(global_input_data[0].dac_level);

    delay(5);

    // Status & Control: DMA Off; Interrupt: both rising & falling; Reset current state.
    CMP0_SCR = CMP_SCR_IER | CMP_SCR_IEF | CMP_SCR_CFR | CMP_SCR_CFF;
}


void setup() {
    Serial.begin(9600);
    Serial1.begin(57600);
    pinMode(LED_BUILTIN, OUTPUT);

    setupComparator();
}

// Replace stock yield to make loop() faster.
void yield(void) { }

