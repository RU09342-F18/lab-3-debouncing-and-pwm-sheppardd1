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
 * THE IMPLIED WARRANTIES OF MERCHANTBBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTBL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATB, OR PROFITS;
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
//  Description: Toggle P1.0 using software and TB_0 ISR. Toggles every
//  50000 SMCLK cycles. SMCLK provides clock source for TBCLK.
//  During the TB_0 ISR, P1.0 is toggled and 50000 clock cycles are added to
//  CCR0. TB_0 ISR is triggered every 50000 cycles. CPU is normally off and
//  used only during TB_ISR.
//  ACLK = n/a, MCLK = SMCLK = TBCLK = default DCO
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


//Software PWM MSP430FR2311
//Created: 5 October 2018
//By: David Sheppard
//Purpose: Increases brightness of LED using PWM

#include <msp430FR2311.h>

//definitions
#define LED0 BIT0   //defining LED0 as BIT0
#define LED1 BIT6   //defining LED1 as BIT6
#define BUTTON BIT1 //defining BUTTON as BIT3

//function prototypes
unsigned int Hz_to_timer(unsigned int, int);


//global variables:
unsigned int onTime = 1000;
unsigned int offTime = 1000;
unsigned int time = 1000;

unsigned int DC = 50;

void main(void)

{
    /* Plan:
     * TimerA0 is used for debouncing only
     * TimeA1 is used with 2 CCRs to interrupt to turn button on and off at correct times
     * The values of CCRs for TimeA1 are changed every time button is pressed in order to change duty cycle
     */

    //want 1kHz signal overall

    PM5CTL0 &= ~LOCKLPM5;           //NEEDED FOR ALL FR BOARDS - takes out of low power mode and ensures correct programming
    WDTCTL = WDTPW + WDTHOLD;               // Stop WDT
    TB0CTL = CCIE;                        // CCR0 interrupt enabled for TB0
    TB1CTL = CCIE;                        // CCR1 interrupt enabled for TB1
    TB0CCR0 = 13107;                        //set value for debouncing timer
    TB1CCR0 = 262;         //time that counter rolls over (gives us period of 1000 Hz
    TB1CCR1 = 131;                          //time that LED inverts at (starting off with 50% duty cycle)
    TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    P1DIR |= (LED0 + LED1);                   // Set LEDs to be outputs
    P1OUT &= ~(LED0 + LED1);                  // shut off LEDs
    P1IFG &= ~BUTTON;                         // clear the P1.3 interrupt flag
    P1IE |= BUTTON;                           //enable interrupts from the button

    __bis_SR_register(LPM0_bits + GIE);       // Enter low power mode w/ interrupt

}

/****************************Interrupts****************************************/


// Timer A0 interrupt service routine - used for debouncing
#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer_B(void)
{
    TB0CTL = MC_0;           //stop timer
    TB0R = 0;               //reset timer reg contents
    P1IE |= BUTTON;         //reenable port 1 interrupts, so button can be read again
}

//special thanks to Russell Trafford for much of this interrupt:
#pragma vector=TIMER1_B1_VECTOR // This drives the PWM
__interrupt void Timer1_B(void)
{
    int Timer0Int = TB1IV;  // since we were having issues with the switch statement
    switch(Timer0Int)       // Based on the interrupt source
    {
    case 2:                 // If CCR1 is triggered, we need to turn LED output to zero for the rest of the cycle
    {
        P1OUT &= ~BIT0;     // Turn LED off
        TB1IV &= ~TB1IV_TBCCR1; // Clear the Timer0 interrupt Flag
    }
    break;
    case 10:                // Timer has overflowed, meaning we need to reset our cycle
    {
        P1OUT |= BIT0;      // Turn LED on
        TB1IV &= ~TB1IV_TBIFG; // Clear the Timer0 interrupt Flag
    }
    break;
    }
}

//Button interrupt service routine - used for increasing brightness (duty cycle)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)   //take care of interrupt coming from port 1
{

    P1OUT ^= LED1;                      //toggle LED1
    P1IFG &= ~BUTTON;                   // clear the P1.3 interrupt flag
    TB0CTL = TBSSEL_2 + MC_1 + ID_3 + TBIE;     // SMCLK, up mode, input divider = 8
    P1IE &= BUTTON;                     //disable port 1 interrupts (button won't cause interrupt)

    switch(DC) //determines the brightness of the LED based on the PWN duty cycle
    //the Hz_to_time() function was acting weird, so I'm stuck with this switch statement layout
    {
    case 0:
    {
        TBCTL = TBCLR;  //clear timer
        TB1CCR1 = 0;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 10:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 20:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 30:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 27;  //since we should technically increment by 26.2, I use 27 here to account for lost decimal place
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 40:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 50:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 60:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 70:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;;
    case 80:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 27;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 90:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;//increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    case 100:
    {
        TBCTL = TBCLR;//clear timer
        TB1CCR1 += 26;  //increment CCR1 (increase duty cycle by 10%)
        TB1CTL = TBSSEL_2 + MC_1 + ID_2 + TBIE; //TB1 initialized with interrupt enabled
    }
        break;
    }

    if(DC != 100)
        DC += 10;
    else
        DC = 0;

}


/***************************Functions*******************************************/
/*
unsigned int Hz_to_timer(unsigned int Hz, int ID)
{//ID is divider
   if(Hz <= (1048576 << ID))            //if desired freq is <= max freq of clock divided by the set divider
       return ((1048576 << ID) / Hz);   //return the clock freq (accounting for divider) divided by desired freq
   else
       return 1;                        //if desired freq is higher than highest possible freq, just use highest possible freq
}
*/

/***********************************Notes******************************************/
//SMCLK: 1048576 Hz
//ACLK: 32768 Hz




