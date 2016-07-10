/* Example for triggering the ADC with PDB
*   Valid for Teensy 3.0 and 3.1, not LC
*/


#include <ADC.h>

const int readPin = A9; // ADC0
const int readPin2 = A2; // ADC1

ADC *adc = new ADC(); // adc object;

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(readPin, INPUT);
    pinMode(readPin2, INPUT);

    Serial.begin(9600);

    Serial.println("Begin setup");

    ///// ADC0 ////
    // reference can be ADC_REF_3V3, ADC_REF_1V2 (not for Teensy LC) or ADC_REF_EXT.
    //adc->setReference(ADC_REF_1V2, ADC_0); // change all 3.3 to 1.2 if you change the reference to 1V2

    adc->setAveraging(1); // set number of averages
    adc->setResolution(8); // set bits of resolution

    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    // see the documentation for more information
    adc->setConversionSpeed(ADC_VERY_HIGH_SPEED); // change the conversion speed
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    adc->setSamplingSpeed(ADC_VERY_HIGH_SPEED); // change the sampling speed

    adc->enableInterrupts(ADC_0); // it's necessary to enable interrupts for PDB to work (why?)

    adc->analogRead(readPin, ADC_0); // call this to setup everything before the pdb starts


    ////// ADC1 /////
    #if defined(ADC_TEENSY_3_1)
    adc->setAveraging(32, ADC_1); // set number of averages
    adc->setResolution(16, ADC_1); // set bits of resolution
    adc->setConversionSpeed(ADC_VERY_LOW_SPEED, ADC_1); // change the conversion speed
    adc->setSamplingSpeed(ADC_VERY_LOW_SPEED, ADC_1); // change the sampling speed

    adc->enableInterrupts(ADC_1);

    adc->analogRead(readPin2, ADC_1); // call this to setup everything before the pdb starts
    #endif

    Serial.println("End setup");

}

char c=0;
int value;
int value2;

void loop() {

    if (Serial.available()) {
        c = Serial.read();
        if(c=='v') { // value
            Serial.print("Value ADC0: ");
            value = (uint16_t)adc->analogReadContinuous(ADC_0); // the unsigned is necessary for 16 bits, otherwise values larger than 3.3/2 V are negative!
            Serial.println(value*3.3/adc->getMaxValue(ADC_0), DEC);
            #if defined(ADC_TEENSY_3_1)
            Serial.print("Value ADC1: ");
            value2 = (uint16_t)adc->analogReadContinuous(ADC_1); // the unsigned is necessary for 16 bits, otherwise values larger than 3.3/2 V are negative!
            Serial.println(value2*3.3/adc->getMaxValue(ADC_1), DEC);
            #endif
        } else if(c=='s') { // start pdb, before pressing enter write the frequency in Hz
            uint32_t freq = Serial.parseInt();
            Serial.print("Start pdb with frequency ");
            Serial.print(freq);
            Serial.println(" Hz.");
            adc->adc0->stopPDB();
            adc->adc0->startPDB(freq); //frequency in Hz
            #if defined(ADC_TEENSY_3_1)
            adc->adc1->stopPDB();
            adc->adc1->startPDB(freq); //frequency in Hz
            #endif
        } else if(c=='p') { // pbd stats
            Serial.print("Prescaler:");
            Serial.println( (PDB0_SC&0x7000)>>12 , HEX);
            Serial.print("Mult:");
            Serial.println( (PDB0_SC&0xC)>>2, HEX);
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
        if(adc->adc0->fail_flag == ADC_ERROR_COMPARISON) {
            adc->adc0->fail_flag &= ~ADC_ERROR_COMPARISON; // clear that error
            Serial.println("Comparison error in ADC0");
        }
    }
    #if defined(ADC_TEENSY_3_1)
    if(adc->adc1->fail_flag) {
        Serial.print("ADC1 error flags: 0x");
        Serial.println(adc->adc1->fail_flag, HEX);
        if(adc->adc1->fail_flag == ADC_ERROR_COMPARISON) {
            adc->adc1->fail_flag &= ~ADC_ERROR_COMPARISON; // clear that error
            Serial.println("Comparison error in ADC1");
        }
    }
    #endif

    //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

    //delay(100);
}


// If you enable interrupts make sure to call readSingle() to clear the interrupt.
void adc0_isr() {
        adc->adc0->readSingle();
        //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN) );
}

#if defined(ADC_TEENSY_3_1)
void adc1_isr() {
        adc->adc1->readSingle();
        digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN) );
}
#endif

// pdb interrupt is enabled in case you need it.
void pdb_isr(void) {
        PDB0_SC &=~PDB_SC_PDBIF; // clear interrupt
        //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN) );
}
