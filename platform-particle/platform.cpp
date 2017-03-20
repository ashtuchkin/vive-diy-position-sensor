#include "debug_node.h"
#include "led_state.h"
#include "primitives/timestamp.h"
#include "application.h"


// ====  Timestamp helpers  ===================================================

extern volatile system_tick_t system_millis, system_millis_clock;
constexpr uint32_t F_CPU = 120*1000000;

static_assert(F_CPU % sec == 0, "Timestamp tick take whole number of CPU ticks");

Timestamp Timestamp::cur_time() {
    // Reimplementation of GetSystem1UsTick() to get better precision.
    int is = __get_PRIMASK();
    __disable_irq();
    system_tick_t base_millis = system_millis;
    system_tick_t base_clock = system_millis_clock;
    system_tick_t cur_clock = DWT->CYCCNT;
    if ((is & 1) == 0)
        __enable_irq();
    
    return base_millis * msec + (cur_clock - base_clock) / (F_CPU / sec);
}

uint32_t Timestamp::cur_time_millis() {
    return GetSystem1MsTick();
}


// ====  LED helpers  =========================================================

LedState cur_led_state = LedState::kNotInitialized;
LEDStatus led_statuses[] = {
    [(int)LedState::kNotInitialized] = LEDStatus(RGB_COLOR_ORANGE, LED_PATTERN_SOLID),
    [(int)LedState::kConfigMode]     = LEDStatus(RGB_COLOR_ORANGE, LED_PATTERN_BLINK, LED_SPEED_SLOW),
    [(int)LedState::kNoFix]          = LEDStatus(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_SLOW),
    [(int)LedState::kFixFound]       = LEDStatus(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_FAST),
};

void set_led_state(LedState state) {
    if (state == cur_led_state) 
        return;
    if ((uint32_t)state >= sizeof(led_statuses) / sizeof(led_statuses[0]))
        return;
    led_statuses[(uint32_t)cur_led_state].setActive(false);
    cur_led_state = state;
    led_statuses[(uint32_t)cur_led_state].setActive(true);
}

void update_led_pattern(Timestamp cur_time) {
    // Do nothing.
}


// ====  Debug node helpers  ==================================================

void print_platform_memory_info(PrintStream &stream) {
    uint32_t freemem = System.freeMemory();
    stream.printf("RAM: free %d\n", freemem);
}


// ====  Configuration helpers  ===============================================

void restart_system() {
    System.reset();
}

void _eeprom_init_if_needed() {
    static bool initialized = false;
    if (!initialized) {
        HAL_EEPROM_Init();
        initialized = true;
    }
}

void eeprom_read(uint32_t eeprom_addr, void *dest, uint32_t len) {
    _eeprom_init_if_needed();
    HAL_EEPROM_Get(eeprom_addr, dest, len);
}

void eeprom_write(uint32_t eeprom_addr, const void *src, uint32_t len) {
    _eeprom_init_if_needed();
    HAL_EEPROM_Put(eeprom_addr, src, len);
}
