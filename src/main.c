/*
 * final_project.c
 *
 * Created: 3/16/2026 2:56:38 PM
 * Author : joshg
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>
#include "uart.h"
#include "auto_corr.h"
#include "guitar_notes.h"
#include "gc9a01a.h"
#include "tune_indicator.h"

/* circular buffer to store audio signal - see auto_corr.h */
SampleBuffer buffer;

/* State Machine */
typedef enum {
	STATE_FILLING,
	STATE_ESTIMATING,
	STATE_OUTPUTTING,
	STATE_NEWTARGET
} AppState;
volatile AppState state = STATE_FILLING;

// external clock setup
void init_clock() {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc |
	CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));  // wait for external clock to stabilize
}

/* ----- ADC setup ----- */
void init_ADC0(void) {
	// setup default channel as the potentiometer AIN120 (PORTF4)
	ADC0.MUXPOS = ADC_MUXPOS_AIN20_gc;
	// Set CLK_ADC to 1MHz (valid range from 125KHz to 2MHz)
	ADC0.CTRLC = ADC_PRESC_DIV16_gc;
	// VDD as reference
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	// enables the ADC circuitry - default is 12bit resolution
	ADC0.CTRLA = ADC_ENABLE_bm;
}

uint16_t read_ADC0() {
	ADC0.COMMAND = ADC_STCONV_bm;    // start the reading
	loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);  // wait until read is complete
	uint16_t result = ADC0.RES;  // read value (12 bit number)
	return result;
}

/* ----- ISR Button Handler ----- */
#define DEBOUNCE_TIME 10	// check buttons every 10ms
volatile int debounce_timer_cnt = DEBOUNCE_TIME;
volatile bool btn_pushed = false;
#define BTN1 (!(PORTC.IN & PIN4_bm))    // button on PC4

ISR(PORTC_PORT_vect)
{
	// Button at PORTC4
	if (PORTC.INTFLAGS & PIN4_bm) {
		PORTC.INTFLAGS |= PIN4_bm; // clear flag
		if (debounce_timer_cnt == 0) {
			debounce_timer_cnt = DEBOUNCE_TIME;
		}
	}
}



/* ----- Timers ----- */
// timer for sampling at 8kHz (64us period)
void init_timer_TCA0(void) {
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // Normal mode
	TCA0.SINGLE.PER = 499;  // period is 125us (8KHz)
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	// Set Prescalar to N = 4 & enable timer. Each tick is 4/16MHz = 0.25us
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm);
}

// TCA0 ISR - audio sampling handler
ISR(TCA0_OVF_vect) {
	//  read ADC and add sample to buffer, guarded by state machine
    if (state == STATE_FILLING) {
	    insert_sample(&buffer, read_ADC0());
    }
	// clear interrupt
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

// disable sampling helper
static inline void disable_sampling(void) {
	TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;
}

// enable sampling helper
static inline void enable_sampling(void) {
	TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
}

// 1ms timer TCB0
void init_timer_TCB0(void) {
	TCB0.INTCTRL = TCB_CAPT_bm; // Enable TCB Overflow ISR
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;
	TCB0.CCMP = 7999;    // overflow at 1ms
	// Set pre-scaler to N = 2 & enable timer
	TCB0.CTRLA |= (TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm);
}

// timer ISR, interrupts every 1ms
#define ESTIMATE_FREQ_TIMER 25    // update LCD every 25ms
volatile int estimate_freq_timer_cnt = ESTIMATE_FREQ_TIMER;
ISR(TCB0_INT_vect)
{
	// output timer counter
	if (estimate_freq_timer_cnt > 0) {
		estimate_freq_timer_cnt--;
	}
	
	// button1 handling/ debouncing
	if (debounce_timer_cnt > 0) {
		debounce_timer_cnt--;
		// debounce with external interrupts
		if ((debounce_timer_cnt == 0) & BTN1) {
			btn_pushed = true;
		}
	}
	
	TCB0.INTFLAGS |= TCB_CAPT_bm;  // must clear the interrupt
}

/* ----- Init Helper ----- */
void init_all(void) {
	init_clock();
	init_timer_TCB0();
	init_timer_TCA0();
	init_ADC0();
	uart_init(3, 115200, NULL);
	
	// LCD init
	tft_lcd_init();
	tft_draw_tuning_note(NOTE_E2, COLOR_WHITE);
	
	// tuning indicator init
	init_tune_indicator();
	
	// enable interrupts for button press on PORTC4
	PORTC.PIN4CTRL |= PORT_ISC_FALLING_gc;
	PORTC.DIRCLR = PIN4_bm;
	
	sei();
}

int main(void)
{
	// init
   init_buffer(&buffer);
   init_all();
   float frequency = 0.0;    // current estimated frequency
   GuitarTuningNote_t target_note = NOTE_E2;    // target tuning note 
   
    while (1)
    {
		// button handling, forces to new target state
		if (btn_pushed) {
			btn_pushed = false;
			// update target note and set state to new target
			target_note++;
			if (target_note > 5) target_note = NOTE_E2;
			state = STATE_NEWTARGET;
		}
		
		// state machine
		switch (state) {
			// filling the buffer
			case STATE_FILLING: {
				// proceed to estimating once buffer is full and timer is reached 
				if (full_buffer(&buffer) && estimate_freq_timer_cnt == 0) {
					state = STATE_ESTIMATING;
				}
				break;
			}
			
			// estimating frequency of buffer
			case STATE_ESTIMATING: {
				disable_sampling();   // disable the sampling interrupt
				frequency = estimated_frequency(&buffer);
				state = STATE_OUTPUTTING;
				break;
			}
			
			// update indicator / output
			case STATE_OUTPUTTING: {
				// update the indicator
				char result = update_indicator(target_note, frequency);
				
				// if in tune, update LCD
				if (result == 1) {
					tft_draw_tuning_note(target_note, COLOR_GREEN);
					_delay_ms(1500);   // delay for a second
					tft_draw_tuning_note(target_note, COLOR_WHITE);
				}
				
				char freq_str_buf[8];
				dtostrf(frequency, 3, 2, freq_str_buf);
				printf("DETECTED FREQ: %s\r\n", freq_str_buf);
				
				// reset to sampling state
				buffer.size = 0;    // reset the buffer size, marking as needing refill
				estimate_freq_timer_cnt = ESTIMATE_FREQ_TIMER;
				enable_sampling();   // re-enable sampling
				state = STATE_FILLING;
				break;
			}
			
			// new target state- update LCD/Indicator and reset buffer
			case STATE_NEWTARGET: {
				disable_sampling();  // disable sampling, rendering LCD screen may create gaps in buffer
				
				// update LCD screen
				tft_draw_tuning_note(target_note, COLOR_WHITE);
				
				// reset indicator
				reset_indicator();
				
				// reset to sampling state
				buffer.size = 0;    // reset the buffer size, marking as needing refill
				estimate_freq_timer_cnt = ESTIMATE_FREQ_TIMER;
				enable_sampling();   // re-enable sampling
				state = STATE_FILLING;
				break;
			}
		}
	}

}

