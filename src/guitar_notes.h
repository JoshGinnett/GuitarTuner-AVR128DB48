#ifndef GUITAR_NOTES_H
#define GUITAR_NOTES_H

typedef enum {
	NOTE_E2 = 0,   /* String 6 low E  82.41  Hz */
	NOTE_A2,       /* String 5 A      110.00 Hz */
	NOTE_D3,       /* String 4 D      146.83 Hz */
	NOTE_G3,       /* String 3 G      196.00 Hz */
	NOTE_B3,       /* String 2 B      246.94 Hz */
	NOTE_E4        /* String 1 high E 329.63 Hz */
} GuitarTuningNote_t;

#endif /* GUITAR_NOTES_H */