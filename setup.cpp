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

// Setup flex timer for pulse width measurement
// TODO: Currently only set up for pin 3, FTM1, channel 0 & 1, need to enable for any FTM ch
bool setupFlexTimer(uint32_t pin = 3) {
    // Derived from https://github.com/tni/teensy-samples/blob/master/input_capture_dma.ino
    // Derived from https://github.com/PaulStoffregen/FreqMeasureMulti

    uint8_t channel = 0;

    // Turn off IRQs for FTM1 during setup
    NVIC_DISABLE_IRQ(IRQ_FTM1);

    switch (pin) {
        // FTM0, channels 0-7
        case 22: channel = 0; CORE_PIN22_CONFIG = PORT_PCR_MUX(4); break;
        case 23: channel = 1; CORE_PIN23_CONFIG = PORT_PCR_MUX(4); break;
        case  9: channel = 2; CORE_PIN9_CONFIG  = PORT_PCR_MUX(4); break;
        case 10: channel = 3; CORE_PIN10_CONFIG = PORT_PCR_MUX(4); break;
        case  6: channel = 4; CORE_PIN6_CONFIG  = PORT_PCR_MUX(4); break;
        case 20: channel = 5; CORE_PIN20_CONFIG = PORT_PCR_MUX(4); break;
#if defined(KINETISK)
        case 21: channel = 6; CORE_PIN21_CONFIG = PORT_PCR_MUX(4); break;
        case  5: channel = 7; CORE_PIN5_CONFIG  = PORT_PCR_MUX(4); break;

        // FTM1, channels 0-1
        case  3: channel = 0; CORE_PIN3_CONFIG  = PORT_PCR_MUX(3); break;
#endif

        default:
            channel = 8;
            return false;
	}

    // Disable write protect on FTM1
    FTM1_MODE = FTM_MODE_WPDIS | FTM_MODE_FTMEN;

    // setup FTM1 in free running mode, counting from 0 - 0xFFFF
    FTM1_SC = 0;
    FTM1_CNT = 0;
    FTM1_CNTIN = 0;
    FTM1_MOD = 0xFFFF;

    // No filtering
    FTM1_FILTER = 0;

    // set FTM1 clock source to system clock; FTM_SC_PS(0): divide clock by 1
    FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(0));

    // set FTM1 CH0 to dual edge capture enable, paired channels
    FTM1_COMBINE = FTM_COMBINE_DECAPEN0;

    // channel 0, capture falling edge; FTM_CSC_MSA --> dual-edge, continous capture mode
    FTM1_C0SC = FTM_CSC_ELSB | FTM_CSC_MSA;

    // channel 1, capture rising edge; FTM_CSC_MSA --> continous capture mode
    // channel 1 interrupt enable
    FTM1_C1SC = FTM_CSC_ELSA | FTM_CSC_MSA | FTM_CSC_CHIE;

    // Enable interrupts for FTM1
    // TODO: choose IRQ prioritiy for FTM1_SC
    NVIC_SET_PRIORITY(IRQ_FTM1, 128);
    NVIC_ENABLE_IRQ(IRQ_FTM1);

    // Clear channel flags and enable dual edge captures
    FTM1_C0SC &= ~FTM_CSC_CHF;
    FTM1_C1SC &= ~FTM_CSC_CHF;
    FTM1_COMBINE |= FTM_COMBINE_DECAP0;
    return true;
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(57600);
    pinMode(LED_BUILTIN, OUTPUT);

    setupComparator();
    setupFlexTimer(3);
}

// Replace stock yield to make loop() faster.
void yield(void) { }

