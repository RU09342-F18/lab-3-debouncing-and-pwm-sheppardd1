# Button Debouncing
## Details
- Created by David Sheppard 
- 5 October 2018
## Purpose
Prevents accidental button presses caused by button bounce in hardware.
## Implementation:
- Button is debounced by disenabling interrupts from the button for a very short time period
- When button pressed, start timer and disenable interrupts.
- When timer rolls over, reenable interrupts so we can check button again.
- LED0 starts out off (and LED1 starts out on in case of MSP430G2553). They toggle when button is pressed and released
- LED0: off when button not pressed, on when pressed.
## Dependencies
dependent on the MSP430.h header file
