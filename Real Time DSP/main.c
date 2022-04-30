/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
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
 * --/COPYRIGHT--*/
//******************************************************************************
//!  main.c
//!
//!  Description: Record/playback demo application on MSP-EXP430FR5994
//!               using BOOSTXL-AUDIO.
//!
//!               Press the S1 switch to begin recording (3 seconds),
//!               Red LED1 indicating active recording.
//!
//!               Press the S2 switch to playback what was just recorded.
//!               Green LED2 indicates active playback.
//!
//!               NOTE: This demo requires the BOOSTXL-AUDIO boosterpack
//!
//!                  MSP430FR5994               Audio Boosterpack
//!               -----------------  		    -----------------
//!              |                 |           |                 |
//!              |    P5.0/UCB1SIMO|---------> |SPI_SI           |
//!              |                 |           |                 |
//!          /|\ |     P5.2/UCB1CLK|---------> |SPI_CLK          |
//!           |  |                 |           |                 |
//!            --|RST          P8.2|---------> |SYNC             |
//!              |                 |           |                 |
//!              |             P1.3|<--------- | MIC OUT         |
//!              |                 |           |                 |
//!              |             P6.2|---------> | MIC PWR         |
//!              |-----------------|           |-----------------|
//!
//******************************************************************************

#include <driverlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "audio_collect.h"
#include "audio_playback.h"
#include "application.h"
#include "dac8311.h"
#include <msp430.h>
#include <stdint.h>


#define MAX_NUM_NOTES   50
#define SLAVE_ADDR  0x48

typedef enum
{
  NOTE,
  OCTAVE,
  LENGTH
} current_reading_t;

current_reading_t status = NOTE;
uint8_t rx_val = 0;


uint8_t beats_per_meas = 0;
uint8_t beats_per_min = 0;
uint8_t mode = 3;
uint8_t total_length = 0;
uint8_t num_harmonies = 0;
uint8_t num_notes = 0;
uint8_t notes_h1[MAX_NUM_NOTES] = {0};
uint8_t octaves_h1[MAX_NUM_NOTES] = {0};
uint8_t lengths_doubled_h1[MAX_NUM_NOTES] = {0}; // so if the length is 1.5 notes, it is represented as 3
uint8_t notes_h2[MAX_NUM_NOTES] = {0};
uint8_t octaves_h2[MAX_NUM_NOTES] = {0};
uint8_t lengths_doubled_h2[MAX_NUM_NOTES] = {0}; // so if the length is 1.5 notes, it is represented as 3
uint8_t notes_h3[MAX_NUM_NOTES] = {0};
uint8_t octaves_h3[MAX_NUM_NOTES] = {0};
uint8_t lengths_doubled_h3[MAX_NUM_NOTES] = {0}; // so if the length is 1.5 notes, it is represented as 3
uint8_t count = 0;
uint8_t current_harmony = 1;
uint8_t start = 4;
uint8_t chord_count = 0;
uint8_t num_chords = 0;
uint8_t chords[4] = {0};
uint32_t delay_cyc = 0;

void initClock(void);
void initGpio(void);
void initI2C(void);

// Global audio config parameter
Audio_configParams gAudioConfig;

// Global audio playback config parameter
Playback_configParams gPlaybackConfig;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

    initClock();
    initGpio();
    initI2C();

    PM5CTL0 &= ~LOCKLPM5;           // Clear lock bit

    // Enable Switch interrupt
    GPIO_clearInterrupt(PUSHBUTTON1_PORT, PUSHBUTTON1_PIN);
    GPIO_enableInterrupt(PUSHBUTTON1_PORT, PUSHBUTTON1_PIN);
    GPIO_clearInterrupt(PUSHBUTTON2_PORT, PUSHBUTTON2_PIN);
    GPIO_enableInterrupt(PUSHBUTTON2_PORT, PUSHBUTTON2_PIN);

    // Initialize DAC
	DAC8311_init();

	// Set the DAC to low power mode with output high-Z
	DAC8311_setLowPowerMode(DAC8311_OUTPUT_HIGHZ);

	__bis_SR_register(GIE);

    // Starts the application. It is a function that never returns.

	while(start == 4)
	{

	}

	uint32_t del = 0;
	while(del <= delay_cyc)
	{
	    __delay_cycles(100);
	    del += 100;
	}


    runApplication();

    return 1;
}

// Initializes the 32kHz crystal and MCLK to 8MHz
void initClock(void)
{
    PJSEL0 |= BIT4 | BIT5;                  // For XT1

    // XT1 Setup
    CSCTL0_H = CSKEY >> 8;                  // Unlock CS registers
    CSCTL1 = DCOFSEL_6;                     // Set DCO to 8MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // set all dividers
    CSCTL4 &= ~LFXTOFF;

    do
    {
        CSCTL5 &= ~LFXTOFFG;                // Clear XT1 fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);              // Test oscillator fault flag
    CSCTL0_H = 0;                           // Lock CS registers
}

void initGpio(void)
{
    P1OUT = 0x00;
    P1DIR = 0xFF;

    P2OUT = 0x00;
    P2DIR = 0xFF;

    P3OUT = 0x00;
    P3DIR = 0xFF;

    P4OUT = 0x00;
    P4DIR = 0xFF;

    P5OUT = 0x00;
    P5DIR = 0xFF;

    P6OUT = 0x00;
    P6DIR = 0xFF;

    //P7OUT = 0x00;
    //P7DIR = 0xFF;

    // I2C pins
    P7SEL0 |= BIT0 | BIT1;
    P7SEL1 &= ~(BIT0 | BIT1);

    P8OUT = 0x04;
    P8DIR = 0xFF;

    PJOUT = 0x00;
    PJDIR = 0xFF;

    // Configure Push button switch with high to low transition
    GPIO_setAsInputPinWithPullUpResistor(PUSHBUTTON1_PORT,
                                         PUSHBUTTON1_PIN);

    GPIO_selectInterruptEdge(PUSHBUTTON1_PORT,
                             PUSHBUTTON1_PIN,
                             GPIO_HIGH_TO_LOW_TRANSITION);

    GPIO_setAsInputPinWithPullUpResistor(PUSHBUTTON2_PORT,
                                         PUSHBUTTON2_PIN);

    GPIO_selectInterruptEdge(PUSHBUTTON2_PORT,
                             PUSHBUTTON2_PIN,
                             GPIO_HIGH_TO_LOW_TRANSITION);
}


void initI2C(void)
{
    UCB2CTLW0 = UCSWRST;                      // Software reset enabled
    UCB2CTLW0 |= UCMODE_3 | UCSYNC;           // I2C mode, sync mode
    UCB2I2COA0 = SLAVE_ADDR | UCOAEN;        // Own Address and enable
    UCB2CTLW0 &= ~UCSWRST;                    // clear reset register
    UCB2IE |= UCRXIE + UCSTPIE;
}



//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B2_VECTOR
__interrupt void USCI_B2_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B2_VECTOR))) USCI_B2_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //Must read from UCB2RXBUF

  switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG))
  {

      case UCIV_8:
          rx_val = UCB2RXBUF;
          if(beats_per_meas == 0)
          {
              beats_per_meas = rx_val;

          }
          else if (beats_per_min == 0)
          {
              beats_per_min = rx_val;
              delay_cyc = beats_per_min/(beats_per_meas*60*__SYSTEM_FREQUENCY_MHZ__);
          }
          else if (mode == 3)
          {
              mode = rx_val;

          }
          else if (mode == 0) //manual mode chosen
          {
              if(total_length == 0)
              {
                  total_length = rx_val;
              }
              else if (num_harmonies == 0)
              {
                  num_harmonies = rx_val;
              }
              else if (num_notes == 0)
              {
                  num_notes = rx_val;
                  if(num_notes > MAX_NUM_NOTES)
                  {
                      num_notes = MAX_NUM_NOTES;
                  }
              }
              else if(current_harmony <= num_harmonies) // receiving note, octave, length over and over
              {
                  if(current_harmony==1)
                  {
                      if(count < num_notes)
                      {
                          if(status == NOTE) //giving us note
                          {
                              notes_h1[count] = rx_val;
                              status = OCTAVE;
                          }
                          else if(status == OCTAVE) //giving us octave
                          {
                              octaves_h1[count] = rx_val;
                              status = LENGTH;
                          }
                          else //giving us length
                          {
                              lengths_doubled_h1[count] = rx_val;
                              status = NOTE;
                              count++;
                              if(count == num_notes)
                              {
                                  count = 0;
                                  current_harmony++;
                                  status = NOTE;
                              }
                          }
                      }
                  }
                  else if (current_harmony == 2)
                  {
                      if(count < num_notes)
                      {
                          if(status == NOTE) //giving us note
                          {
                              notes_h2[count] = rx_val;
                              status = OCTAVE;
                          }
                          else if(status == OCTAVE) //giving us octave
                          {
                              octaves_h2[count] = rx_val;
                              status = LENGTH;
                          }
                          else //giving us length
                          {
                              lengths_doubled_h2[count] = rx_val;
                              status = NOTE;
                              count++;
                              if(count == num_notes)
                              {
                                  count = 0;
                                  current_harmony++;
                                  status = NOTE;
                              }
                          }
                      }
                  }
                  else
                  {
                      if(count < num_notes)
                      {
                          if(status == NOTE) //giving us note
                          {
                              notes_h3[count] = rx_val;
                              status = OCTAVE;
                          }
                          else if(status == OCTAVE) //giving us octave
                          {
                              octaves_h3[count] = rx_val;
                              status = LENGTH;
                          }
                          else //giving us length
                          {
                              lengths_doubled_h3[count] = rx_val;
                              status = NOTE;
                              count++;
                              if(count == num_notes)
                              {
                                  count = 0;
                                  current_harmony++;
                                  status = NOTE;
                              }
                          }
                      }
                  }
              }
              else
              {
                  start = rx_val;
              }
          }
          else if (mode == 1) //auto mode chosen
          {
              if(num_harmonies == 0)
              {
                  num_harmonies = rx_val;
              }
              else if(num_chords == 0)
              {
                  num_chords = rx_val;
              }
              else if(chord_count < num_chords)
              {
                  chords[chord_count] = rx_val;
                  chord_count++;
              }
              else
              {
                  start = rx_val;
              }

          }
          break;
  }
}
