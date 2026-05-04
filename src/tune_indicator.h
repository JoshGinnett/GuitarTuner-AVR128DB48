#ifndef TUNE_INDICATOR
#define TUNE_INDICATOR
#include <avr/io.h>
#include "guitar_notes.h"


// port for out of tune LEDs
#define OTUNE_PORT PORTD
// in Tune port and pin
#define ITUNE_PORT PORTE
#define ITUNE_PIN PIN3_bm

// functions for interfacing with tune indicator
void init_tune_indicator(void);
void reset_indicator(void);
char update_indicator(GuitarTuningNote_t desired_freq, float current_frequency);


#endif