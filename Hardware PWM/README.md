# Hardware PWM
## Details
- Created by David Sheppard
- 5 October 2018
## Purpose
Uses PWM to gradually increase the brightness of an LED
## Implementation
- Uses the OUTMOD_7 (Reset/Set) to perfrom the PWM
- WHen the user presses the button, the duty cycle of the PWN increases by 10%
## Dependencies
Relies on the MSP430.h header file to function
