#ifndef AUTO_CORR
#define AUTO_CORR
#include <stdint.h>
#include <stdbool.h>

/* 
Defines the core buffer data structure and supported operations.
Defines autocorrelation methods to be performed on the sample buffer.
*/

#define BUFFER_SIZE 512  // size of the buffer (N)
#define SAMPLING_F 8000   // sampling frequency (8 kHz provides better accuracy for autocorrelation)
#define MIN_LAG (int)(SAMPLING_F / 350)  // 350Hz highest frequency detectable
#define MAX_LAG (int)(SAMPLING_F / 65)   //  80Hz lowest frequency detectable

typedef struct {
    uint16_t buffer[BUFFER_SIZE];
    uint16_t size;			// number of items in the buffer
} SampleBuffer;

/* BUFFER OPERATIONS */
void init_buffer(SampleBuffer *b);
void insert_sample(SampleBuffer *b, uint16_t sample);
bool full_buffer(SampleBuffer *b);

/* AUTO CORRELATION */
float estimated_frequency(SampleBuffer *b);

#endif /* AUTO_CORR*/