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


//Hardware PWM MSP430G2553
//Created: 5 October 2018
//By: David Sheppard
// based on: https://www.kompulsa.com/example-code-msp430-pwm/

#include <msp430G2553.h>

#define LED0 BIT0   //defining LED0 as BIT0
#define LED1 BIT6   //defining LED1 as BIT6
#define BUTTON BIT3 //defining BUTTON as BIT3

unsigned int DC = 50; //duty cycle

int main(void) {

    WDTCTL = WDTPW + WDTHOLD; //Disable the Watchdog timer for our convenience.
    P1SEL |= LED0; //Select pin LED as our PWM output.
    P1DIR |= (LED0 + LED1);                   // Set LEDs to be outputs
    P1OUT &= ~(LED0 + LED1);                  // shut off LEDs
    P1IFG &= ~BUTTON;                         // clear the P1.3 interrupt flag
    P1IE |= BUTTON;                           //enable interrupts from the button
    BCSCTL3 = LFXT1S_2;                     //interfaces with crystal (needed for clock to work)
    TA0CCTL0 = CCIE;                        // CCR0 interrupt enabled for TA0
    TA1CCTL1 = CCIE;
    TA1CCR0 = 262; //Set the period in the Timer A Capture/Compare 0 register to 1000 us.
    TA1CCTL1 = OUTMOD_7;
    TA1CCR1 = 131; //The period in microseconds that the power is ON. It's half the time, which translates to a 50% duty cycle.
    TA0CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TASSEL_2 selects SMCLK as the clock source, and MC_1 tells it to count up to the value in TA0CCR0.
    __bis_SR_register(LPM0_bits); //Switch to low power mode 0.
}

// Timer A0 interrupt service routine - used for debouncing
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
    TA0CTL = MC_0;           //stop timer
    TA0R = 0;               //reset timer reg contents
    P1IE |= BUTTON;         //reenable port 1 interrupts, so button can be read again
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)   //take care of interrupt coming from port 1
{

    P1OUT ^= LED1;                      //toggle LED1
    P1IFG &= ~BUTTON;                   // clear the P1.3 interrupt flag
    TA0CTL = TASSEL_2 + MC_1 + ID_3 + TAIE;     // SMCLK, up mode, input divider = 8
    P1IE &= BUTTON;                     //disable port 1 interrupts (button won't cause interrupt)

    switch(DC) //determines the brightness of the LED based on the PWN duty cycle
    //the Hz_to_time() function was acting weird, so I'm stuck with this switch statement layout
    {
    case 0:
    {
        TACTL = TACLR;  //clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 = 0;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 10:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 20:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 30:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 27;  //since we should technically increment by 26.2, I use 27 here to account for lost decimal place
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 40:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 50:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 60:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 70:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;;
    case 80:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 27;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 90:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    case 100:
    {
        TACTL = TACLR;//clear timer
        TA0CCTL1 = OUTMOD_7;
        TA1CCR1 += 26;  //increment CCR1 (increase duty cycle by 10%)
        TA1CTL = TASSEL_2 + MC_1 + ID_2 + TAIE; //TA1 initialized with interrupt enabled
    }
        break;
    }

    if(DC != 100)
        DC += 10;
    else
        DC = 0;

}







