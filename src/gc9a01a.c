/*
 * gc9a01a.c - 240×240 round TFT driver
 
 Credit to Adafruit for code snippets (especially initialization sequence): https://github.com/adafruit/Adafruit_GC9A01A/tree/main
 
 */

#define F_CPU 16000000UL
#include "gc9a01a.h"
#include <util/delay.h>

// GC901A - register addresses - provided by Adafruit
#define GC9A01A_SLPOUT      0x11
#define GC9A01A_INVON       0x21
#define GC9A01A_DISPON      0x29
#define GC9A01A_CASET       0x2A
#define GC9A01A_RASET       0x2B
#define GC9A01A_RAMWR       0x2C
#define GC9A01A_TEON        0x35
#define GC9A01A_MADCTL      0x36
#define GC9A01A_VSCRSADD    0x37
#define GC9A01A_COLMOD      0x3A
#define GC9A01A_INREGEN1    0xFE
#define GC9A01A_INREGEN2    0xEF
#define GC9A01A_PWRCTL2     0xC3
#define GC9A01A_PWRCTL3     0xC4
#define GC9A01A_PWRCTL4     0xC9
#define GC9A01A_FRAMERATE   0xE8
#define GC9A01A_SET_GAMMA1  0xF0
#define GC9A01A_SET_GAMMA2  0xF1
#define GC9A01A_SET_GAMMA3  0xF2
#define GC9A01A_SET_GAMMA4  0xF3
#define MADCTL_MX           0x40
#define MADCTL_BGR          0x08
#define TFT_WIDTH           240
#define TFT_HEIGHT          240

// init command sequence, provided by Adafruit
static const uint8_t PROGMEM init_240x240[] = {
    GC9A01A_INREGEN2, 0,
    0xEB, 1, 0x14,
    GC9A01A_INREGEN1, 0,
    GC9A01A_INREGEN2, 0,
    0xEB, 1, 0x14,
    0x84, 1, 0x40,
    0x85, 1, 0xFF,
    0x86, 1, 0xFF,
    0x87, 1, 0xFF,
    0x88, 1, 0x0A,
    0x89, 1, 0x21,
    0x8A, 1, 0x00,
    0x8B, 1, 0x80,
    0x8C, 1, 0x01,
    0x8D, 1, 0x01,
    0x8E, 1, 0xFF,
    0x8F, 1, 0xFF,
    0xB6, 2, 0x00, 0x00,
    GC9A01A_MADCTL, 1, MADCTL_MX | MADCTL_BGR,
    GC9A01A_COLMOD, 1, 0x05,
    0x90, 4, 0x08, 0x08, 0x08, 0x08,
    0xBD, 1, 0x06,
    0xBC, 1, 0x00,
    0xFF, 3, 0x60, 0x01, 0x04,
    GC9A01A_PWRCTL2, 1, 0x13,
    GC9A01A_PWRCTL3, 1, 0x13,
    GC9A01A_PWRCTL4, 1, 0x22,
    0xBE, 1, 0x11,
    0xE1, 2, 0x10, 0x0E,
    0xDF, 3, 0x21, 0x0c, 0x02,
    GC9A01A_SET_GAMMA1, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    GC9A01A_SET_GAMMA2, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
    GC9A01A_SET_GAMMA3, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    GC9A01A_SET_GAMMA4, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
    0xED, 2, 0x1B, 0x0B,
    0xAE, 1, 0x77,
    0xCD, 1, 0x63,
    GC9A01A_FRAMERATE, 1, 0x34,
    0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
               0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
    0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
               0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
    0x64,  7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
    0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
    0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
    0x74,  7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    0x98,  2, 0x3e, 0x07,
    GC9A01A_VSCRSADD, 2, 0, 0,
    GC9A01A_TEON, 0,
    GC9A01A_INVON, 0,
    GC9A01A_SLPOUT, 0x80,
    GC9A01A_DISPON, 0x80,
    0x00 
};

// font tables from Adafruit - 7 columns wide by 12 rows high (12pt)
#define FONT_W     7
#define FONT_H     12
#define CELL_SIZE  90   /* each character cell is CELL_SIZE × CELL_SIZE px */

// drawing area coordinates
#define AREA_X0    30   /* left of 180×180 drawing area */
#define AREA_Y0    30   /* top  of 180×180 drawing area */
#define AREA_W     180
#define AREA_H     180

// total rendered width of both glyphs
#define GLYPH_W     (FONT_W * SCALE_X)
#define GLYPH_GAP   -10
#define GROUP_W     (GLYPH_W * 2 + GLYPH_GAP)

// center the group horizontally in the 240px display
#define LETTER_X0   ((TFT_WIDTH - GROUP_W) / 2)
#define DIGIT_X0    (LETTER_X0 + GLYPH_W + GLYPH_GAP)

// center vertically too (FONT_H * SCALE_Y = 84px tall)
#define GLYPH_H     (FONT_H * SCALE_Y)
#define CELL_Y0     ((TFT_HEIGHT - GLYPH_H) / 2)

// scale factor for glyphs from 12pt table to fill approx 90x90 area
#define SCALE_X 13  // 7 cols x 13 = 91 pixels wide
#define SCALE_Y  7  // 12 rows x 7 = 84 pixels tall

static const uint8_t PROGMEM font_E[FONT_H] = {
	0x00, //
	0xFC, // ######
	0x44, //  #   #
	0x50, //  # #
	0x70, //  ###
	0x50, //  # #
	0x40, //  #
	0x44, //  #   #
	0xFC, // ######
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_B[FONT_H] = {
	0x00, //
	0xF8, // #####
	0x44, //  #   #
	0x44, //  #   #
	0x78, //  ####
	0x44, //  #   #
	0x44, //  #   #
	0x44, //  #   #
	0xF8, // #####
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_G[FONT_H] = {
	0x00, //
	0x3C, //   ####
	0x44, //  #   #
	0x40, //  #
	0x40, //  #
	0x4E, //  #  ###
	0x44, //  #   #
	0x44, //  #   #
	0x38, //   ###
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_D[FONT_H] = {
	0x00, //
	0xF0, // ####
	0x48, //  #  #
	0x44, //  #   #
	0x44, //  #   #
	0x44, //  #   #
	0x44, //  #   #
	0x48, //  #  #
	0xF0, // ####
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_A[FONT_H] = {
	0x00, //
	0x30, //   ##
	0x10, //    #
	0x28, //   # #
	0x28, //   # #
	0x28, //   # #
	0x7C, //  #####
	0x44, //  #   #
	0xEE, // ### ###
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_2[FONT_H] = {
	0x00, //
	0x38, //   ###
	0x44, //  #   #
	0x04, //      #
	0x08, //     #
	0x10, //    #
	0x20, //   #
	0x44, //  #   #
	0x7C, //  #####
	0x00, //
	0x00, //
	0x00  //
};

static const uint8_t PROGMEM font_3[FONT_H] = {
	0x00, //
	0x38, //   ###
	0x44, //  #   #
	0x04, //      #
	0x18, //    ##
	0x04, //      #
	0x04, //      #
	0x44, //  #   #
	0x38, //   ###
	0x00, //
	0x00, //
	0x00  //
};

/* @240 '4' (7 pixels wide) – your supplied bitmap, unchanged */
static const uint8_t PROGMEM font_4[FONT_H] = {
	0x00, //
	0x0C, //     ##
	0x14, //    # #
	0x14, //    # #
	0x24, //   #  #
	0x44, //  #   #
	0x7E, //  ######
	0x04, //      #
	0x0E, //     ###
	0x00, //
	0x00, //
	0x00  //
};

static inline void _cs_low(void)     { TFT_SPI_PORT.OUTCLR = TFT_CS; }    // drive CS line low  (start of transmission)
static inline void _cs_high(void)    { TFT_SPI_PORT.OUTSET = TFT_CS; }    // drive CS line high (end of transmission)
static inline void _dc_data(void)    { TFT_DC_PORT.OUTSET  = TFT_DC; }    // drive DC line high (sending data)
static inline void _dc_command(void) { TFT_DC_PORT.OUTCLR  = TFT_DC; }	  // drive DC line low  (sending command)

// write one byte over SPI, block until complete
static void _spi_write(uint8_t byte) {
    SPI0.DATA = byte;
    loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp);
}

// send a command byte over SPI, caller handles CS toggle
static inline void _cmd(uint8_t c) { _dc_command(); _spi_write(c); }

// set drawing window / area
static void _set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    _cs_low();

	// column address set
    _cmd(GC9A01A_CASET); _dc_data();
    _spi_write(x0 >> 8); _spi_write(x0 & 0xFF);    // starting column
    _spi_write(x1 >> 8); _spi_write(x1 & 0xFF);    // ending column

	// row address set
    _cmd(GC9A01A_RASET); _dc_data();
    _spi_write(y0 >> 8); _spi_write(y0 & 0xFF);    // starting row
    _spi_write(y1 >> 8); _spi_write(y1 & 0xFF);    // ending row

	// force update by sending RAMWR command
    _cmd(GC9A01A_RAMWR); _dc_data();
}

// fill a rectangle with a solid RGB-565 color
static void _fill_rect(uint16_t x0, uint16_t y0, uint16_t w,  uint16_t h, uint16_t color) {
	// check rectangle dimensions are valid
    if (!w || !h) return;
	
	// set drawing window  as size of rectangle
    _set_window(x0, y0, x0 + w - 1, y0 + h - 1);  // cs pulled low in _set_window
    uint8_t  hi = color >> 8, lo = color & 0xFF;  // high and low bytes of color
    uint32_t n  = (uint32_t)w * h;  // number of pixels to write
    while (n--) { _spi_write(hi); _spi_write(lo); }  // write 16 bit color
	// end transmission
    _cs_high();
}

// renders a glyph in progmem (pointed to by bmp) to fill a 90x90 cell 
// x: top-left x coordinate of 90x90 cell
// y: top-left y coordinate of 90x90 cell
// color: color of glyph rendered
static void _draw_glyph(uint16_t cell_x, uint16_t cell_y, const uint8_t *bmp, uint16_t color) {
	
	// iterate over each row (FONT_H = 12 total)
    for (uint8_t row = 0; row < FONT_H; row++) {
		// // read byte from progmem skip if 0
        uint8_t bits = pgm_read_byte(bmp + row);
        if (!bits) continue;

		// iterate over the (FONT_W = 7) columns
        for (uint8_t col = 0; col < FONT_W; col++) {
			// check if this columns pixel is lit, mask starts at bit 6 (bit 7 is padding)
            if (bits & (0x80 >> col)) {
				_fill_rect(cell_x + col * SCALE_X, cell_y + row * SCALE_Y, SCALE_X, SCALE_Y, color);
            }
        }
    }
}

// --------------
/* PUBLIC API */
// --------------
void tft_lcd_init(void)
{
    /* --- SPI INIT --- */
    TFT_SPI_PORT.DIR |= TFT_MOSI | TFT_SCK | TFT_CS | TFT_RST;
    TFT_DC_PORT.DIR  |= TFT_DC;
    _cs_high();
    TFT_SPI_PORT.OUTSET = TFT_RST;
	// use 8MHz SPI
    SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV4_gc | SPI_CLK2X_bm;
	// manual SS setting
    SPI0.CTRLB = SPI_SSD_bm | SPI_MODE_0_gc;

    /* Use Adafruit Init Sequence */
    const uint8_t *p = init_240x240;   // pointer to init table
    uint8_t cmd, x, nargs;   // send the command register, then the number of arguments a 1-byte frames
    while ((cmd = pgm_read_byte(p++)) > 0) {
        _cs_low();   // pull cs low
        _cmd(cmd);   // send command register
        x = pgm_read_byte(p++);  // read next byte from sequence
        nargs = x & 0x7F;  // read number of arguments to send to register
        _dc_data();  // signal data transmission
        while (nargs--) _spi_write(pgm_read_byte(p++));
        _cs_high();  // end transmission
        if (x & 0x80) _delay_ms(150);  // give 150ms for LCD to setup
    }

    /* Clear display by setting all black */
    _fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_BLACK);
}

void tft_draw_tuning_note(GuitarTuningNote_t note, uint16_t color) {
    /* Erase the 180×180 drawing area (margins stay black from init). */
    _fill_rect(AREA_X0, AREA_Y0, AREA_W, AREA_H, COLOR_BLACK);

    // Find letter glyph and digit glyph for the note
    const uint8_t *l_bmp;    // letter bitmap pointer
	const uint8_t *d_bmp;    // digit bitmap pointer
    switch (note) {
        case NOTE_E2: {
			l_bmp = font_E;
			d_bmp = font_2;
			break;
		}
        case NOTE_A2: {
			l_bmp = font_A;
			d_bmp = font_2;
			break;
		}
        case NOTE_D3: {
			l_bmp = font_D;
			d_bmp = font_3;
			break;
		}
		case NOTE_G3: {
			l_bmp = font_G;
			d_bmp = font_3;
			break;
		}
		case NOTE_B3: {
			l_bmp = font_B;
			d_bmp = font_3;
			break;
		}
		case NOTE_E4: {
			l_bmp = font_E;
			d_bmp = font_4;
			break;
		}
		default: {
			l_bmp = 0; 
			d_bmp = 0;
			break;
		}
    }

	// render both glyphs side by side
    if (l_bmp) _draw_glyph(LETTER_X0, CELL_Y0, l_bmp, color);
    if (d_bmp) _draw_glyph(DIGIT_X0,  CELL_Y0, d_bmp, color);
}