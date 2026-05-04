/*
 * gc9a01a.h - 240×240 round TFT driver
 */

#ifndef GC9A01A_H
#define GC9A01A_H

#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include "guitar_notes.h"

// pin mapping for SPI
#define TFT_SPI_PORT  PORTA
#define TFT_MOSI      PIN4_bm   /* SDA  */
#define TFT_SCK       PIN6_bm   /* SCL  */
#define TFT_CS        PIN7_bm   /* CSX  */
#define TFT_RST       PIN3_bm   /* RST  */
#define TFT_DC_PORT   PORTC
#define TFT_DC        PIN0_bm   /* D/CX */

// screen uses 16-bit color codes
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_GREEN   0x07E0

// public API
// initializes SPI, resets the screen, and runs the full init sequence (provided by Adagfruit)
void tft_lcd_init(void);

// renders a guitar tuning note centered on the display with provided color (white or green)
void tft_draw_tuning_note(GuitarTuningNote_t note, uint16_t color);

#endif /* GC9A01A_H */