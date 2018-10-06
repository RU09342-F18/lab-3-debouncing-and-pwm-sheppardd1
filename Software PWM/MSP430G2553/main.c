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


//Software PWM

#include <msp430G2553.h>

//definitions
#define LED0 BIT0   //defining LED0 as BIT0
#define LED1 BIT6   //defining LED1 as BIT6
#define BUTTON BIT3 //defining BUTTON as BIT3

//function prototypes
void TimerA0Setup();
void TimerA1Setup();
unsigned int Hz_to_timer(unsigned int, int);


//global variables:
unsigned int onTime = 1000;
unsigned int offTime = 1000;
unsigned int time = 1000;

void main(void)

{
    /* Plan:
     * TimerA0 is used for debouncing only
     * TimeA1 is used with 2 CCRs to interrupt to turn button on and off at correct times
     * The values of CCRs for TimeA1 are changed every time button is pressed in order to change duty cycle
     */


    //want 1kHz signal

    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    BCSCTL3 = LFXT1S_2;                       //interfaces with crystal (needed for clock to work)
 /*   TimerA1Setup();
    TimerA0Setup();*/
    //TA0CCTL0 = CCIE;           // CCR0 interrupt enabled
    //TA1CCTL0 = CCIE;
    TA0CCR0 = 13107;
    TA1CCR0 = 60000;
    TA1CCR1 = 50000;
    TA1CTL = TASSEL_2 + MC_1 + ID_3 + TAIE;
    P1DIR |= (LED0 + LED1);                   // Set LEDs to be outputs
    P1OUT &= ~(LED0 + LED1);                  // shut off LEDs
    P1IFG &= ~BUTTON;                         // clear the P1.3 interrupt flag
    P1IE |= BUTTON;                           //enable interrupts from the button

    __bis_SR_register(LPM0_bits + GIE);       // Enter low power mode w/ interrupt

}

/****************************Interrupts****************************************/


// Timer A0 interrupt service routine - used for debouncing
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
    TA0CTL = MC_0;           //stop timer
    TA0R = 0;               //reset timer reg contents
    P1IE |= BUTTON;         //reenable port 1 interrupts, so button can be read again
}


//Timer A1 interrupt service routine - used for toggling LED
/*#pragma vector=TIMER1_A1_VECTOR
__interrupt void Timer1_A(void)
{
    TA1R = 0;                         //reset timer reg contents
    P1OUT ^= LED0;                      //toggle LED0
}*/

#pragma vector=TIMER1_A1_VECTOR // This drives the PWM
__interrupt void Timer1_A(void)
{
    int Timer0Int = TA0IV;  // since we were having issues with the switch statement
    switch(Timer0Int)       // Based on the interrupt source
    {
    case 2:                 // If CCR1 is triggered, we need to turn LED output to zero for the rest of the cycle
    {
        P1OUT &= ~BIT0;     // Turn LED off
        TA0IV &= ~TA0IV_TACCR1; // Clear the Timer0 interrupt Flag
    }
    break;
    case 10:                // Timer has overflowed, meaning we need to reset our cycle
    {
        P1OUT |= BIT0;      // Turn LED on
        TA0IV &= ~TA0IV_TAIFG; // Clear the Timer0 interrupt Flag
    }
    break;
    }
}

//Button interrupt service routine - used for increasing brightness (duty cycle)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)   //take care of interrupt coming from port 1
{

    P1IES ^= BUTTON;                    // toggle the interrupt edge,
    P1OUT ^= LED1;                      //toggle LED1

    TA0CTL = TASSEL_2 + MC_1 + ID_3 + TAIE;     // SMCLK, up mode, input divider = 8
    P1IE &= BUTTON;                     //disable port 1 interrupts (button won't cause interrupt)
    P1IFG &= ~BUTTON;                   // clear the P1.3 interrupt flag

}


/***************************Functions*******************************************/
/*
void TimerA0Setup(){
    TA0CTL = CCIE;                              // CCR0 interrupt enabled
    TA0CCR0 = 13107;                            //set capture compare register to about 1/10 of SMCLK, so button interrupt is disabled for 0.1 s
    //Don't want to start timer until button is pressed
    //TA0CTL = TASSEL_2 + MC_1 + ID_3;          // SMCLK, up mode, input divider = 0
}


void TimerA1Setup(){
    TA1CTL = CCIE;                          // CCR0 interrupt enabled
    TA1CCR0 = Hz_to_timer(500, 0);          //set capture compare register (input of function needs to be desired frequency / 2 since LED toggles each time CCR is reached)
    TA1CTL = TASSEL_2 + MC_1 + ID_0;        // SMCLK, up mode, input divider = 0
}
*/

unsigned int Hz_to_timer(unsigned int Hz, int ID)
{//ID is divider
   if(Hz <= (1048576 << ID))            //if desired freq is <= max freq of clock divided by the set divider
       return ((1048576 << ID) / Hz);   //return the clock freq (accounting for divider) divided by desired freq
   else
       return 1;                        //if desired freq is higher than highest possible freq, just use highest possible freq
}


/***********************************Notes******************************************/
//SMCLK: 1048576 Hz
//ACLK: 32768 Hz




