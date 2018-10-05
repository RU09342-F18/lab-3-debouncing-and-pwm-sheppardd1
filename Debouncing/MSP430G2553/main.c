/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430G2xx3 Demo - Timer_A, Toggle P1.0, CCR0 Cont. Mode ISR, DCO SMCLK
//
//  Description: Toggle P1.0 using software and TA_0 ISR. Toggles every
//  50000 SMCLK cycles. SMCLK provides clock source for TACLK.
//  During the TA_0 ISR, P1.0 is toggled and 50000 clock cycles are added to
//  CCR0. TA_0 ISR is triggered every 50000 cycles. CPU is normally off and
//  used only during TA_ISR.
//  ACLK = n/a, MCLK = SMCLK = TACLK = default DCO
//
//           MSP430G2xx3
//         ---------------
//     /|\|            XIN|-
//      | |               |
//      --|RST        XOUT|-
//        |               |
//        |           P1.0|-->LED
//
//  D. Dang
//  Texas Instruments Inc.
//  December 2010
//   Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 5.10
//******************************************************************************


//Debouncing MSP430G2553
//Created: 1 October 2018
//By: David Sheppard
//Last Updated: 5 October 2018
//Updates: It actually works now

#include <msp430G2553.h>

#define LED0 BIT0   //defining LED0 as BIT0
#define LED1 BIT6   //defining LED1 as BIT6
#define BUTTON BIT3 //defining BUTTON as BIT3

void main(void)

/*
 * Concept: Button is debounced by disenabling interrupts from the button for a very short time period
 * Implementation: When button pressed, start timer and disenable interrupts.
 * When timer rolls over, reenable interrupts so we can check button again.
 * LED0 starts out off and LED1 starts out on. They toggle when button is pressed and released
 * LED0: off when button not pressed, on when pressed. LED1 is opposite.
 * */

{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  BCSCTL3 = LFXT1S_2;                       //interfaces with crystal (needed for clock to work)
  P1DIR |= (LED0 + LED1);                   // Set LEDs to be outputs
  P1OUT &= ~(LED0 + LED1);                  // shut off LED
  CCTL0 = CCIE;                             // CCR0 interrupt enabled
  CCR0 = 3277;                              //set capture compare register to about 1/10 of aclk, so button interrupt (positive or negative edge) is disabled for 0.1 s
  P1IFG &= ~BUTTON;                         // clear the P1.3 interrupt flag
  P1IE |= BUTTON;                           //enable interrupts from the button

  P1OUT ^= LED1;                            //I want LED1 and LED0 to always be on at different times, so I'll toggle LED1 now

  __bis_SR_register(LPM0_bits + GIE);       // Enter low power mode w/ interrupt

}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{

    TACTL = MC_0;           //stop timer
    TA0R = 0;               //reset timer reg contents
    P1IE |= BUTTON;         //reenable port 1 interrupts, so button can be read again

}
// ACLK is 32768 Hz


// Port 1 interrupt service routine (for button)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)       //take care of interrupt coming from port 1
{
    P1IES ^= BUTTON;                // toggle the interrupt edge,
    P1OUT ^= (LED0 + LED1           //toggle LEDs

    TACTL = TASSEL_1 + MC_1 + ID_0;     // ACLK, up mode, input divider = 0
    P1IE &= BUTTON;                     //disable port 1 interrupts (button won't cause interrupt)
    P1IFG &= ~BUTTON;                   // clear the P1.3 interrupt flag
}

// Note: the interrupt vector will be called
// when P1.3 goes from Hi to Low as well as
// LowtoHigh



