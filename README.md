# GuitarTuner-AVR128DB48

A real-time guitar tuner built on the **AVR128DB48 Curiosity Nano** microcontroller. The device captures audio from a guitar string, detects the fundamental frequency using an autocorrelation algorithm, and provides instant visual feedback through a round TFT LCD display and a multi-LED tuning indicator.

---

## Demo
[![Guitar Tuner Demo](https://img.shields.io/badge/Watch-Demo%20Video-red)](https://youtu.be/YMSiEr61HQ0)

---

## Features

- Detects fundamental frequency of guitar strings in the range **65–350 Hz**
- Supports all 6 standard guitar string targets: **E2, A2, D3, G3, B3, E4**
- Displays the target note on a **240×240 round TFT LCD** (GC9A01A controller)
- 8-LED bar indicator shows how sharp or flat the string is
- Dedicated **in-tune LED** illuminates when within ±0.5 Hz of the target
- Cycle through strings with a single **button press**
- Audio sampled at **8 kHz** using the on-chip ADC with a 512-sample buffer

---

## Hardware

| Component | Part / Notes |
|---|---|
| Microcontroller | AVR128DB48 Curiosity Nano |
| Display | GC9A01A 240×240 round TFT LCD |
| Audio input | Analog signal on PORTF4 (AIN20) |
| Out-of-tune LEDs | 8× LEDs on PORTD0–PORTD7 |
| In-tune LED | 1× LED on PORTE3 |
| User button | Tactile switch on PORTC4 (active low) |
| Debug UART | USART3 TX on PORTB0, 115200 baud |

### Pin Connections

#### TFT LCD (SPI)

| LCD Pin | AVR128DB48 Pin |
|---|---|
| MOSI | PORTA4 |
| SCK | PORTA6 |
| CS | PORTA7 |
| RST | PORTA3 |
| DC | PORTC0 |

#### LEDs

| Signal | Pin |
|---|---|
| Tuning LED 0–7 (flat → sharp) | PORTD0–PORTD7 |
| In-tune LED | PORTE3 |

#### Audio Input

| Signal | Pin |
|---|---|
| Guitar audio | PORTF4 / AIN20 |

> **Note:** The audio signal must be biased to **~VDD/2** before reaching the ADC pin. The AVR's ADC only samples 0–VDD, so an AC guitar signal centered at 0 V would clip the negative half of the waveform. A simple resistor voltage divider (e.g., two equal resistors from VDD and GND) sets the midpoint bias, allowing the full audio waveform to be captured.

---

## Software Architecture

The firmware is structured around a 4-state machine driven by an 8 kHz ADC interrupt:

| State | Description |
|---|---|
| `STATE_FILLING` | Fills a 512-sample circular buffer with ADC readings |
| `STATE_ESTIMATING` | Runs the autocorrelation pitch detection algorithm |
| `STATE_OUTPUTTING` | Updates the LED indicator and triggers an LCD refresh |
| `STATE_NEWTARGET` | Handles button press and redraws the target note on the LCD |

**Pitch detection** uses autocorrelation with parabolic interpolation for sub-sample frequency accuracy. DC offset is removed before processing to improve accuracy on AC-coupled audio inputs.

**Timers:**
- `TCA0` — fires the ADC at 8 kHz (125 µs period)
- `TCB0` — 1 ms tick for button debouncing (10 ms threshold) and LCD update pacing

---

## Building & Flashing

This project was developed with **MPLAB X IDE** and the **AVR-GCC** toolchain.

1. Open `GuitarTuner-AVR128DB48` in MPLAB X IDE.
2. Select the **AVR128DB48 Curiosity Nano** as the target device.
3. Build the project.
4. Connect the Curiosity Nano via USB and flash.

Debug output is available over the on-board CDC serial port at **115200 baud** using any terminal emulator.

---

## Attributions

- **GC9A01A display driver** — initialization sequences and font tables adapted from [Adafruit's GC9A01A library](https://github.com/adafruit/Adafruit_GC9A01A).

---

## License

MIT License — see [LICENSE](LICENSE) for details.
