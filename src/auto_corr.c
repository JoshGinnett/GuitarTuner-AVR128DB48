#include "auto_corr.h"
#include <avr/interrupt.h>

// Initializes a buffer
void init_buffer(SampleBuffer *b) {
	b->size = 0;
	for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
		b->buffer[i] = 0;
	}
}

// Inserts a raw sample into the buffer
void insert_sample(SampleBuffer *b, uint16_t sample) {
	b->buffer[b->size] = sample;
	b->size++;
}

// returns true if the buffer is full, otherwise returns false
bool full_buffer(SampleBuffer *b) {
	return (b->size == BUFFER_SIZE);
}

// returns the estimated frequency of the signal in buffer
// using auto correlation
float estimated_frequency(SampleBuffer *b) {

	// Find DC Offset
	int32_t dc_sum = 0;
	for (uint16_t i = 0; i < BUFFER_SIZE; i++)
	dc_sum += b->buffer[i];
	int16_t dc_offset = (int16_t)(dc_sum >> 9);    // equivalent to dividing by BUFFER_SIZE 512
	
	// remove DC offset in-place
	int16_t *sig = (int16_t *)b->buffer;  // cast buffer as signed
	for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
		sig[i] = (int16_t)b->buffer[i] - dc_offset;
	}

	// find lag with maximum autocorrelation
	// R(k) = sum of sig[n] * sig[n + k] for all valid n
	// for a periodic signal, R(k) peaks when k equals one full period in samples
	uint16_t best_lag = MIN_LAG;
	int32_t  max_corr = 0;
	int32_t  prev_corr = 0;      // R[k-1], saved when best_lag updates
	int32_t  prev_sum = 0;       // tracks the sum from the previous iteration
	for (uint16_t k = MIN_LAG; k <= MAX_LAG; k++) {
		int32_t sum = 0;
		uint16_t limit = BUFFER_SIZE - k;
		for (uint16_t n = 0; n < limit; n++) {
			sum += (int32_t)sig[n] * sig[n + k];
		}
		// track what lag that produces strongest correlation
		if (sum > max_corr) {
			max_corr = sum;
			best_lag = k;
			prev_corr = prev_sum;    // R[best_lag - 1]
		}
		prev_sum = sum;
	}

	// parabolic interpolation for sub-sample accuracy
	// fits a parabola through R[best_lag-1], R[best_lag], R[best_lag+1]
	float interp_lag = (float)best_lag;
	if (best_lag > MIN_LAG && best_lag < MAX_LAG) {
		// recompute R[best_lag + 1] — only costs one extra correlation pass
		int32_t next_corr = 0;
		uint16_t limit = BUFFER_SIZE - (best_lag + 1);
		for (uint16_t n = 0; n < limit; n++) {
			next_corr += (int32_t)sig[n] * sig[n + best_lag + 1];
		}

		int32_t denom = prev_corr - 2 * max_corr + next_corr;
		if (denom != 0) {
			float delta = 0.5f * (float)(prev_corr - next_corr) / (float)denom;
			interp_lag = (float)best_lag + delta;
		}
	}

	// fundamental frequency of buffer is approx. Fs / interp_lag
	return (float)SAMPLING_F / interp_lag;
}
