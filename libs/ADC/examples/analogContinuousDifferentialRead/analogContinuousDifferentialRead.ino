/* Example for analogContinuousDifferentialRead
*  It measures continuously the voltage difference on pins A10-A11,
*  Write c and press enter on the serial console to check that the conversion is taking place,
*  Write t to check if the voltage agrees with the comparison in the setup()
*  Write s to stop the conversion, you can restart it writing r.
*/

#include <ADC.h>

ADC *adc = new ADC(); // adc object

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(A10, INPUT); //Diff Channel 0 Positive
    pinMode(A11, INPUT); //Diff Channel 0 Negative

    #if !defined(ADC_TEENSY_LC)
    pinMode(A12, INPUT); //Diff Channel 3 Positive
    pinMode(A13, INPUT); //Diff Channel 3 Negative
    #endif

    Serial.begin(9600);

    ///// ADC0 ////
    // reference can be ADC_REF_3V3, ADC_REF_1V2 (not for Teensy LC) or ADC_REF_EXT.
    //adc->setReference(ADC_REF_1V2, ADC_0); // change all 3.3 to 1.2 if you change the reference to 1V2

    adc->setAveraging(1); // set number of averages
    adc->setResolution(9); // set bits of resolution

    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    // see the documentation for more information
    // additionally the conversion speed can also be ADC_ADACK_2_4, ADC_ADACK_4_0, ADC_ADACK_5_2 and ADC_ADACK_6_2,
    // where the numbers are the frequency of the ADC clock in MHz and are independent on the bus speed.
    adc->setConversionSpeed(ADC_MED_SPEED); // change the conversion speed
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    adc->setSamplingSpeed(ADC_MED_SPEED); // change the sampling speed

    // always call the compare functions after changing the resolution!
    // Compare values at 16 bits differential resolution are twice what you write!
    //adc->enableCompare(-1.0/3.3*adc->getMaxValue(ADC_0), 0, ADC_0); // measurement will be ready if value < -1.0V
    //adc->enableCompareRange(-1.0*adc->getMaxValue(ADC_0)/3.3, 2.0*adc->getMaxValue(ADC_0)/3.3, 0, 1, ADC_0); // ready if value lies out of [-1.0,2.0] V

    // If you enable interrupts, notice that the isr will read the result, so that isComplete() will return false (most of the time)
    //adc->enableInterrupts(ADC_0);

    adc->startContinuousDifferential(A10, A11, ADC_0);

    ////// ADC1 /////
    #if defined(ADC_TEENSY_3_1)

    adc->setAveraging(32, ADC_1); // set number of averages
    adc->setResolution(13, ADC_1); // set bits of resolution
    adc->setConversionSpeed(ADC_VERY_LOW_SPEED, ADC_1); // change the conversion speed
    adc->setSamplingSpeed(ADC_VERY_LOW_SPEED, ADC_1); // change the sampling speed

    // always call the compare functions after changing the resolution!
    //adc->enableCompare(1.0/3.3*adc->getMaxValue(ADC_1), 0, ADC_1); // measurement will be ready if value < 1.0V
    adc->enableCompareRange(-1.0*adc->getMaxValue(ADC_1)/3.3, 2.0*adc->getMaxValue(ADC_1)/3.3, 0, 1, ADC_1); // ready if value lies out of [-1.0,2.0] V

    //adc->enableInterrupts(ADC_1);

    adc->startContinuousDifferential(A12, A13, ADC_1);

    #endif

    delay(500);
}

int value, value2 = 0;
char c=0;

void loop() {

    if (Serial.available()) {
        c = Serial.read();
        if(c=='c') { // conversion active?
            Serial.print("Converting? ADC0: ");
            Serial.println(adc->isConverting(ADC_0));
            #if defined(ADC_TEENSY_3_1)
            Serial.print("Converting? ADC1: ");
            Serial.println(adc->isConverting(ADC_1));
            #endif
        } else if(c=='s') { // stop conversion
            adc->stopContinuous(ADC_0);
            Serial.println("Stopped");
        } else if(c=='t') { // conversion complete?
            Serial.print("Conversion successful? ADC0: ");
            Serial.println(adc->isComplete(ADC_0));
            #if defined(ADC_TEENSY_3_1)
            Serial.print("Conversion successful? ADC1: ");
            Serial.println(adc->isComplete(ADC_1));
            #endif
        } else if(c=='r') { // restart conversion
            Serial.println("Restarting conversions ");
            adc->startContinuousDifferential(A10, A11, ADC_0);
        } else if(c=='v') { // value
            Serial.print("Value ADC0: ");
            value = adc->analogReadContinuous(ADC_0);
            Serial.println(value*3.3/adc->getMaxValue(ADC_0), DEC);
            #if defined(ADC_TEENSY_3_1)
            Serial.print("Value ADC1: ");
            value2 = adc->analogReadContinuous(ADC_1);
            Serial.println(value2*3.3/adc->getMaxValue(ADC_1), DEC);
            #endif
        }
    }


    /* fail_flag contains all possible errors,
        They are defined in  ADC_Module.h as

        ADC_ERROR_OTHER
        ADC_ERROR_CALIB
        ADC_ERROR_WRONG_PIN
        ADC_ERROR_ANALOG_READ
        ADC_ERROR_COMPARISON
        ADC_ERROR_ANALOG_DIFF_READ
        ADC_ERROR_CONT
        ADC_ERROR_CONT_DIFF
        ADC_ERROR_WRONG_ADC
        ADC_ERROR_SYNCH

        You can compare the value of the flag with those masks to know what's the error.
    */
    if(adc->adc0->fail_flag) {
        Serial.print("ADC0 error flags: 0x");
        Serial.println(adc->adc0->fail_flag, HEX);
    }
    #if defined(ADC_TEENSY_3_1)
    if(adc->adc1->fail_flag) {
        Serial.print("ADC1 error flags: 0x");
        Serial.println(adc->adc1->fail_flag, HEX);
    }
    #endif

    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
    delay(100);

}

void adc0_isr(void) {
    adc->analogReadContinuous(ADC_0);
    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN)); // Toggle the led
}
#if defined(ADC_TEENSY_3_1)
void adc1_isr(void) {
    // Low-level code
    adc->analogReadContinuous(ADC_1);
    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

}
#endif


// RESULTS OF THE TEST Teesny 3.x
// Measure continuously a voltage divider.
// Measurement pin A9 (23). Clock speed 96 Mhz, bus speed 48 MHz.

//
//  Using ADC_LOW_SPEED (same as ADC_VERY_LOW_SPEED) for sampling and conversion speeds
// ADC resolution     Measurement frequency                 Num. averages
//     16  bits            64 kHz                               1
//     13  bits            71 kHz                               1
//     11  bits            71 kHz                               1
//      9  bits            77 kHz                               1

//     16  bits             2.0 kHz                              32
//     13  bits             2.2 kHz                              32
//     11  bits             2.2 kHz                              32
//      9  bits             2.4 kHz                              32

//
//  Using ADC_MED_SPEED for sampling and conversion speeds
// ADC resolution     Measurement frequency                 Num. averages
//     16  bits           150 kHz                               1
//     13  bits           167 kHz                               1
//     11  bits           167 kHz                               1
//      9  bits           182 kHz                               1

//     11  bits            42 kHz                               4 (default settings) corresponds to about 17.24 us

//
//  Using ADC_HIGH_SPEED (same as ADC_HIGH_SPEED_16BITS) for sampling and conversion speeds
// ADC resolution     Measurement frequency                 Num. averages
//     16  bits           316 kHz                               1
//     13  bits           353 kHz                               1
//     11  bits           353 kHz                               1
//      9  bits           387 kHz                               1
//
//      9  bits           245 kHz                               1 ADC_VERY_LOW_SPEED sampling
//      9  bits           293 kHz                               1 ADC_LOW_SPEED sampling
//      9  bits           343 kHz                               1 ADC_MED_SPEED sampling
//      9  bits           414 kHz                               1 ADC_VERY_HIGH_SPEED sampling

//
//  Using ADC_VERY_HIGH_SPEED for sampling and conversion speeds
//  This conversion speed is over the limit of the specs! (speed=24MHz, limit = 18 MHz for res<16 and 12 for res=16)
// ADC resolution     Measurement frequency                 Num. averages
//     16  bits           666 kHz                               1
//     13  bits           749 kHz                               1
//     11  bits           749 kHz                               1
//      9  bits           827 kHz                               1


// At 96 Mhz (bus at 48 MHz), 414 KHz is the fastest we can do within the specs, and only if the sample's impedance is low enough.

// RESULTS OF THE TEST Teensy LC
// Measure continuously a voltage divider.
// Measurement pin A9 (23). Clock speed 48 Mhz, bus speed 24 MHz.

//
//  Using ADC_VERY_LOW_SPEED for sampling and conversion speeds
// ADC resolution     Measurement frequency                 Num. averages
//     16  bits            27.8 kHz                             1
//     13  bits            30 kHz                               1
//     11  bits            30 kHz                               1
//      9  bits            31.9 kHz                             1

//     16  bits             0.87 kHz                            32
//     13  bits             0.94 kHz                            32
//     11  bits             0.94 kHz                            32
//      9  bits             0.99 kHz                            32

//
//  ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED and ADC_VERY_HIGH_SPEED are the same for Teensy 3.x and LC,
//  except for a very small ammount that depends on the bus speed and not on the ADC clock (which is the same for those speeds).
//  This difference corresponds to 5 bus clock cycles, which is about 0.1 us.
//
//  For 9 bits resolution, 1 average, ADC_MED_SPEED sampling speed the measurement frequencies for the different ADACK are:
//  ADC_ADACK_2_4        74.4 kHz
//  ADC_ADACK_4_0       116.1 kHz
//  ADC_ADACK_5_2       163.8 kHz
//  ADC_ADACK_6_2       188.2 kHz
//  For Teensy 3.x the results are similar but not identical for two reasons: the bus clock plays a small role in the total time and
//  the frequency of this ADACK clock is acually quite variable, the values are the typical ones, but in the electrical datasheet
//  it says that they can range from +-50% their values aproximately, so every Teensy can have different frequencies.
