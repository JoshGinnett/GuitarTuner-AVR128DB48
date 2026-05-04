#include "tune_indicator.h"
#include <stdint.h>
#define F_CPU 16000000UL
#include <util/delay.h>

// in-tune levels in frequency for each standard tuning note, so in-tune LED can be determined
const float in_tune_table[6][2] = {
	// E2
	{82.17f, 82.65f},
	
	// A2
	{109.68f, 110.32f},
	
	// D3
	{146.41f, 147.25f},
	
	// G3
	{195.43f, 196.57f},

	// B3
	{246.23f, 247.65f},
	
	// E4
	{328.68f, 330.58f}
};

// level table provides 8 frequency levels to check for the 6 standard tuning notes, so the respective out-of-tune LED can be determined
const float level_table[6][8] = {
	// E2
	{80.06f, 80.53f, 80.99f, 81.70f, 83.13f, 83.85f, 84.34f, 84.82f},

	// A2
	{106.87f, 107.49f, 108.11f, 109.05f, 110.96f, 111.92f, 112.57f, 113.22f},

	// D3
	{142.65f, 143.48f, 144.31f, 145.56f, 148.11f, 149.40f, 150.26f, 151.13f},

	// G3
	{190.42f, 191.52f, 192.63f, 194.31f, 197.71f, 199.43f, 200.58f, 201.74f},

	// B3
	{239.91f, 241.30f, 242.70f, 244.81f, 249.09f, 251.26f, 252.71f, 254.18f},

	// E4
	{320.25f, 322.10f, 323.97f, 326.79f, 332.50f, 335.39f, 337.33f, 339.29f}
};

// initialize the tune_indicator
void init_tune_indicator(void) {
	OTUNE_PORT.DIRSET = 0xFF;
	ITUNE_PORT.DIRSET = ITUNE_PIN;
	OTUNE_PORT.OUTCLR = 0xFF;
	ITUNE_PORT.OUTCLR = 0xFF;
	
	// indicator for successful init
	// forward sweep
	OTUNE_PORT.OUTSET = PIN0_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN1_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN2_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN3_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	ITUNE_PORT.OUTSET = ITUNE_PIN; _delay_ms(80); ITUNE_PORT.OUTCLR = ITUNE_PIN;
	OTUNE_PORT.OUTSET = PIN4_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN5_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN6_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN7_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	// reverse sweep
	OTUNE_PORT.OUTSET = PIN6_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN5_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN4_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	ITUNE_PORT.OUTSET = ITUNE_PIN; _delay_ms(80); ITUNE_PORT.OUTCLR = ITUNE_PIN;
	OTUNE_PORT.OUTSET = PIN3_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN2_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN1_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
	OTUNE_PORT.OUTSET = PIN0_bm; _delay_ms(80); OTUNE_PORT.OUTCLR = 0xFF;
}

// reset (clear) the indicator
void reset_indicator(void) {
	OTUNE_PORT.OUTCLR = 0xFF;
	ITUNE_PORT.OUTCLR = ITUNE_PIN;
}

// given a desired note to tune too, light up the corresponding LED
// returns 1 if in tune, -1 otherwise
char update_indicator(GuitarTuningNote_t note, float current_frequency) {
	// check if it's in-tune, if so indicate and return
	if ((current_frequency >= in_tune_table[note][0]) && (current_frequency <= in_tune_table[note][1])) {
		reset_indicator();
		ITUNE_PORT.OUTSET = ITUNE_PIN;
		return 1;
	}
	
	// if its not in tune, set respective out-of tune LED
	else {
		// reset led indicator
		reset_indicator();
		
		uint8_t i = 0;  // current level being checked, (respective pin)
		// loop through frequency levels for respective note, stop if found
		while ((i < 7) && (current_frequency >= level_table[note][i+1])) {
			i++;
		}

		// turn on respective LED
		OTUNE_PORT.OUTSET = (1<<i);
	}
	return -1;
}