/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "usb.h"
#include "usb_device_cdc.h"

#include "app_io.h"
#include "app_helpers.h"
#include "app_leds.h"
#include "app_music.h"
#include "app_midi.h"
#include "app_cmd.h"

#include "system_config.h"

/** MACROS **/
/* Bit Operation macros */
#define setbit(b,n)   ( b |=   (1 << n))        /* Set bit number n in byte b   */
#define clearbit(b,n) ( b &= (~(1 << n)))       /* Clear bit number n in byte b */
#define readbit(b,n)  ((b  &   (1 << n)) >> n)  /* Read bit number n in byte b  */
#define flipbit(b,n)  ( b ^=   (1 << n))        /* Flip bit number n in byte b  */

#define reciprocal(a, fp)  ( (( 1 << fp) + a - 1) / a )  /* Reciprocal 1/x without using floats */

#define bit_is_set(b,n)   (b   & (1 << n))     /* Test if bit number n in byte b is set   */
#define bit_is_clear(b,n) (!(b & (1 << n)))    /* Test if bit number n in byte b is clear */

/** CONSTANTS ******************************************************/
const char txtCrLf[]       = "\r\n";
//const char txtAlphaLC[]    = "abcdefghijklmnopqrstuvwxyz";
//const char txtAlphaUC[]    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//const char txtAlpha[]      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char txtNum[]        = "0123456789";
//const char txtAlphaNum[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char txtWhitespace[] = " \t\r\n";

//const char txtOk[]        = "+OK";
const char txtOkSp[]      = "+OK ";
//const char txtOkCrLf[]    = "+OK\r\n";
//const char txtOkOn[]      = "+OK On\r\n";
//const char txtOkOff[]     = "+OK Off\r\n";
const char txtOn[]        = "On";
const char txtOff[]       = "Off";

//const char txtError[]     = "-ERROR";
const char txtErrorSp[]   = "-ERROR ";
//const char txtErrorCrLf[] = "-ERROR\r\n";

const char txtErrorUnknownCommand[]  = "Unknown command";
const char txtErrorUnknownArgument[] = "Unknown argument";
const char txtErrorUnknownPin[]      = "Unknown pin";
const char txtErrorInvalidArgument[] = "Invalid argument";



/** VARIABLES ******************************************************/

unsigned char MasterDebug         = 0;     // 

// MASTER CLOCK
#define       MasterClockTickCount  23     /* Number of ticks per ms */
unsigned char MasterClockTick     = 23;    // Tick counter 0,1,2
unsigned long MasterClockMS       = 0;     // MS counter, good for up to over a month
// MASTER NOTIFY
unsigned int  MasterNotifyCounter = 0;     // Notification timer counter    
unsigned int  MasterNotify        = 60000; // When to notify (1 minute))
unsigned long MasterNotifyNow     = 0;     // When not 0, the main loop will notify
// MASTER LEDS
unsigned int  MasterLedStatus     = 0;     // Bitfield of 16 leds statuses
unsigned int  MasterLedCounter    = 0;     // Tick Counter
#define       MasterLedTime         1500  // Ticks to count between
unsigned char MasterLedStep       = 0;
unsigned int  MasterLedStepTime   = 30000;
unsigned int  MasterLedStepTick   = 0;
unsigned char MasterLedStepEnabled= 0;
unsigned char MasterLedStepRestart= 0;

// MASTER TONES
unsigned char MasterToneEnabled   = 0;     //
unsigned char MasterTonePeriod    = 0;     //
unsigned char MasterToneTick      = 0;     //
unsigned long MasterToneTime      = 0;     //
unsigned long MasterToneCounter   = 0;     //
unsigned long MasterToneTempo     = 600;   //
unsigned char MasterTonePitch     = 1;     //
unsigned char MasterToneRestart   = 0;     //
unsigned char MasterToneStep      = 0;     //
unsigned char MasterToneMode      = 1;

// USB buffers
#define sizeChunk      4
#define sizeOutput   255
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

unsigned char  bufChunk[sizeChunk];
unsigned char  bufOutput[sizeOutput + 1];
unsigned char  bufCommand[sizeCommand];
unsigned char  sReply[sizeReply];
unsigned char  sStr1[sizeStr];
unsigned char  sStr2[sizeStr];
unsigned char  sStr3[sizeStr];
unsigned char  sStr4[sizeStr];
unsigned char  sStr5[sizeStr];

unsigned char  posOutput       = 0;
unsigned char  posCommand      = 0;

// Status of IO
unsigned char nStatus_PortA    = 0;
unsigned char nStatus_PortB    = 0;
unsigned char nStatus_PortC    = 0;
unsigned char nStatus_PortD    = 0;
unsigned char nStatus_PortE    = 0;

unsigned char nStatus_TrisA    = 0;
unsigned char nStatus_TrisB    = 0;
unsigned char nStatus_TrisC    = 0;
unsigned char nStatus_TrisD    = 0;
unsigned char nStatus_TrisE    = 0;

unsigned char nStatus_MonitorA = 0;
unsigned char nStatus_MonitorB = 0;
unsigned char nStatus_MonitorC = 0;
unsigned char nStatus_MonitorD = 0;
unsigned char nStatus_MonitorE = 0;

// 6 bytes
// Console
typedef struct {
	unsigned usb:1;
	unsigned connected:1;
	unsigned notify:1;
	unsigned reportOnce:1;
	unsigned bufferOverrun:1;
} console_t;

console_t MasterConsoleStatus;

/** PUBLIC PROTOTYPES ***************************************/
// Main procs
void         APP_init(void);
void         APP_main(void);

/** PRIVATE PROTOTYPES ***************************************/
// USB
void         APP_usbConfigured(void);
void         APP_checkCommands(void);
void         APP_executeCommand(void);
void         APP_updateUsbLed(void);

// APP helper functions
void APP_checkPins(const unsigned char cPortName);
void APP_updateLeds(void);


/** FUNCTIONS *******************************************************/
void APP_init(void){
    // Keyboard Inputs
    TRISD = 0xff;

	MasterConsoleStatus.notify        = 0;
	MasterConsoleStatus.reportOnce    = 0;
	MasterConsoleStatus.bufferOverrun = 0;
    
	// Timer0 setup (Clock 1:16 => once every 1/3 ms)
	T0CON               = 0b01000000; // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [011]1:16
	INTCONbits.TMR0IE   = 1;
	T0CONbits.TMR0ON    = 1;          // Enable
/*
	// Timer2 setup
	T2CON               = 0b01111011; // 1:16 Off 1:16
	PR2                 = 0x80;       // Period register
	T2CONbits.TMR2ON    = 1;          // Enable timer
	IPR1bits.TMR2IP     = 1;          // High priority interrupt
	PIE1bits.TMR2IE     = 1;          // Int enable

	// INT2 setup
    INTCON3bits.INT2IP  = 1;  // High priority
    INTCON3bits.INT2IF  = 0;  // Int Flag
    INTCON2bits.INTEDG2 = 0;  // Falling Edge
	INTCON3bits.INT2IE  = 1;  // Int enable
 */
} // End APP_init

void APP_main(){
    
    
    if (MasterToneEnabled){
        if (MasterToneEnabled == 2){
            MasterToneCounter = 0;
        }

        if (!MasterToneCounter){
            if (!MasterToneMode){
                // Single tone mode
                if (MasterToneRestart){
                    MasterToneCounter = MasterToneTime;
                }
                else{
                    MasterToneEnabled = 0;
                }
            }
            else{
                // Music mode
                if (MasterToneEnabled == 2){
                    MasterToneStep    = 0;
                    MasterToneEnabled = 1;
                }
                else{
                    MasterToneStep++;
                }

                // End of music
                if (MasterToneStep >= MasterToneMusicLen){
                    if (MasterToneRestart){
                        MasterToneStep = 0;
                    }
                    else{
                        MasterToneEnabled = 0;
                    }
                }
                // Next tone
                if (MasterToneEnabled){
                    if (MasterToneMusicType){
                        // MIDI BASED
                        if (MasterToneMusic[MasterToneStep].note){
                            MasterTonePeriod  = MasterToneMidiTable[MasterToneMusic[MasterToneStep].note + MasterTonePitch - 24];
                        }
                        else{
                            MasterTonePeriod  = 0;
                        }
                    }
                    else{
                        // PERIOD BASED
                        MasterTonePeriod  = MasterToneMusic[MasterToneStep].note * MasterTonePitch;
                    }

                    MasterToneCounter = MasterToneMusic[MasterToneStep].time * MasterToneTempo;

                    printf("!OK TONE #%2u n:%2u t:%2u\r\n", MasterToneStep, MasterToneMusic[MasterToneStep].note, MasterToneMusic[MasterToneStep].time);
                    MasterToneTick    = 0;
                    if (!MasterToneCounter){
                        // Autostop
                        MasterToneEnabled = 0;
                        MasterTonePeriod  = 0;
                    }
                }
            }
        }
    }
    
    // Notifications handling
    if (posCommand  == 0 
        && posOutput == 0 
        && USBGetDeviceState()     == CONFIGURED_STATE
        && USBIsDeviceSuspended()  == false
        && mUSBUSARTIsTxTrfReady() == true
    ){
        APP_checkPins('A');
        APP_checkPins('B');
        APP_checkPins('C');
        APP_checkPins('D');
        APP_checkPins('E');
    }
    
    if (MasterNotifyNow){
        clock2str(sStr1, MasterNotifyNow);
        printf("!OK UPTIME: %s\r\n", sStr1);
        MasterNotifyNow = 0;
        //printf("DEBUG on:%u st:%u T:%u t:%u k:%u n:%u\r\n", MasterToneEnabled, MasterToneStep, MasterTonePeriod, MasterToneTime, MasterToneTick, MasterToneCounter);
    }
    
    // MASTER LED STEPS
    if (!MasterLedStepTick && MasterLedStepEnabled){
        if (MasterLedStepEnabled == 2){
            MasterLedStep        = 0;
            MasterLedStepEnabled = 1;
        }
        else{
            MasterLedStep++;
            if (MasterLedStep >= MasterLedStepsLen){
                if (MasterLedStepRestart){
                    MasterLedStep = 0; 
                }
                else {
                    MasterLedStepEnabled = 0;
                    printf("!OK LEDSTEP:%s\r\n", txtOff);
                }
            }
        }

        if (MasterLedStepEnabled){
            MasterLedStatus   = MasterLedSteps[MasterLedStep];
            MasterLedStepTick = MasterLedStepTime;
            int2binstr(sStr1, MasterLedStatus);               
            printf("!OK LEDSTEP: %02d %s\r\n", MasterLedStep, sStr1);
        }
    }
    
    // MASTER LED
    if (!MasterLedCounter){
        APP_updateLeds();
        MasterLedCounter = MasterLedTime;
    }


                
    // INPUT handling
    if(USBUSARTIsTxTrfReady() == true){
        uint8_t i;
        uint8_t n;
        uint8_t nBytes;

        do {
            nBytes = getsUSBUSART(bufChunk, 1);
            if (nBytes > 0) {
                /* For every byte that was read... */
                if (bufChunk[0] == 0x0D || bufChunk[0] == 0x0A){
                    bufCommand[posCommand] = 0x00;
                    bufCommand[posCommand + 1] = 0x00; // Workaround for args parsing
                    APP_executeCommand();
                    break;
                }
                else{
                    bufCommand[posCommand] = bufChunk[0];
                    posCommand++;
                    bufCommand[posCommand] = 0x00;
                }
            }
        }
        while(nBytes > 0);
    }

    if(USBUSARTIsTxTrfReady() == true){
        if(posOutput > 0){
            putUSBUSART(bufOutput, posOutput);
            posOutput = 0;
        }
    }

    CDCTxService();
}

void APP_executeCommand(void){
    unsigned char n;
    unsigned char val;
    unsigned bOK = true;
    unsigned char *ptrCommand;
    unsigned char *ptrArgs;
    
    if (bufCommand[0] == 0x00){
        //return;
    }
   
    // Get the command
    ptrCommand = strtok(bufCommand, txtWhitespace);
    ptrArgs = NULL;
    // Get the arguments
    if (ptrCommand != NULL){
        ptrArgs = &bufCommand[strlen(ptrCommand) + 1];
    }
    
    str2lower(ptrCommand);    
    
    print(txtCrLf);

    // Debug
    if (MasterDebug){
        printf("CMD=<%s>\r\nARGS=<%s>\r\n", ptrCommand, ptrArgs);
    }
    
    if (ptrCommand == NULL){    
        print(txtErrorSp);
        print("Missing command");
        print(txtCrLf);
    }
    // PING
    else if (strcmp(ptrCommand, "ping") == 0){
        APP_CMD_ping(ptrArgs);
    }
    // UPTIME
	else if (strcmp(ptrCommand, "uptime") == 0){
        APP_CMD_uptime(ptrArgs);
	}
    // DEBUG
    else if (strcmp(ptrCommand, "debug") == 0){
        APP_CMD_debug(ptrArgs);
    }
    // TONE
    else if (strcmp(ptrCommand, "tone") == 0){
        APP_CMD_tone(ptrArgs);
    }
    // MONITOR
    else if (strcmp(ptrCommand, "monitor") == 0){
        APP_CMD_monitor(ptrArgs);
    }
    // READ
    else if (strcmp(ptrCommand, "read") == 0 || strcmp(ptrCommand, "r") == 0){
        APP_CMD_read(ptrArgs);
    }
    // WRITE
    else if (strcmp(ptrCommand, "write") == 0 || strcmp(ptrCommand, "w") == 0){
        APP_CMD_write(ptrArgs);
    }
    // LED
    else if (strcmp(ptrCommand, "led") == 0){
        APP_CMD_led(ptrArgs);
    }
    else {
        print(txtErrorSp);
        print(txtErrorUnknownCommand);
        print(txtCrLf);
    }
    posCommand = 0;
    bufCommand[0] = 0x00;
}


/* APP FUNCTIONS */
void interrupt APP_interrupt_high(void){             // High priority interrupt
	// CLOCK: Timer0 overflow int
	if (INTCONbits.TMR0IF){
        
        // MASTER TONES
        if (MasterToneEnabled){
            if (MasterToneCounter){
                MasterToneCounter--;
            }
            
            if (MasterToneTick){
                MasterToneTick--;
            }
            else{
                MasterToneTick = MasterTonePeriod;
                if (MasterTonePeriod){
                    // Hoot/Speaker output
                    PIN_HOOT_TRIS  = OUTPUT;
                    PIN_HOOT_OUT  ^= 1;
                }
            }
        }
        
        // MASTER CLOCK
        if (MasterClockTick){
            MasterClockTick--;
        }
        else {
            MasterClockTick = MasterClockTickCount;
            // All counters that need ms updates
            MasterClockMS++;
            if (MasterNotifyCounter){
                MasterNotifyCounter--;
            }
            // Must run once a ms
            APP_updateUsbLed();
        }
        
        // MASTER NOTIFY
        if (MasterNotifyCounter == 0 && MasterNotify != 0){
            MasterNotifyCounter = MasterNotify;
            if (   posCommand == 0 
                && posOutput  == 0 
                && USBGetDeviceState()    == CONFIGURED_STATE
                && USBIsDeviceSuspended() == false
            ){
                unsigned char tick = MasterClockTick;
                MasterNotifyNow = MasterClockMS;
                if (tick != MasterClockTick){
                    // This is to avoid interim changes as operations with "long" are not atomic
                    MasterNotifyNow = MasterClockMS;
                }
    
            }
        }

        
        // MASTER LEDS
        if (MasterLedCounter){
            MasterLedCounter--;
        }
        
        // MASTER LED STEPS
        if (MasterLedStepTick){
            MasterLedStepTick--;
        }
        
		INTCONbits.TMR0IF = 0;
	}
    
    // USB
    if (USBInterruptFlag){
        USBDeviceTasks();
    }
}

void APP_updateLeds(void){
    // Note: We start with 8 just because that led is the only one which does not exist
    static unsigned char nCurrentLed = 15;
                                      //FEDCBA9876543210 
    static unsigned int  iCurrentBit = 0b0000000100000000;
     
    unsigned char nInitialLed;
    unsigned char nTrisB;
    unsigned char nTrisA;
     
    if (!MasterLedStatus){
        // No leds on, then switch all pins to 3rd state
        TRISA |= 0b00001111;
        TRISB |= 0b00111100;
        return;
    }
    // Find the next led to be on
    nInitialLed = nCurrentLed;
    do {
        if (nCurrentLed == 0){
            nCurrentLed = 15;
            iCurrentBit = 0b1000000000000000;
        }
        else{
            nCurrentLed--;
            iCurrentBit = iCurrentBit >> 1;
        }
    } while (!(MasterLedStatus & iCurrentBit));
    
    if (nInitialLed != nCurrentLed){
        // Not the full loop, it is then another led
        // Make sure we do not disturb other A or B pins
        TRISA = (TRISA & 0b11110000) | (MasterLedMap[nCurrentLed]        & 0b00001111);
        TRISB = (TRISB & 0b11000011) | ((MasterLedMap[nCurrentLed] >> 2 )& 0b00111100);

        LATA &= 0b11110000; // We always force a 0 on A (Cathode)
        LATB |= 0b00111100; // We always force a 1 on B (Anode)
    }

    if (MasterDebug > 2) printf("\r\n!OK LED %d on\r\n", nCurrentLed);

    // If the led is the same one, we do not do anything as everything should already be set
}

void APP_checkPins(unsigned char cPortName){
    unsigned char *pMonitor;
    volatile unsigned char *pPort;
    volatile unsigned char *pTris;
    unsigned char *pStatusTris;
    unsigned char *pStatusPort;
    
    switch (cPortName){
        case 'A': 
            pPort       = &PORTA; 
            pTris       = &TRISA; 
            pMonitor    = &nStatus_MonitorA; 
            pStatusTris = &nStatus_TrisA; 
            pStatusPort = &nStatus_PortA; 
            break;
        case 'B': pMonitor = &nStatus_MonitorB; pPort = &PORTB; pTris = &TRISB; pStatusTris = &nStatus_TrisB; pStatusPort = &nStatus_PortB; break;
        case 'C': pMonitor = &nStatus_MonitorC; pPort = &PORTC; pTris = &TRISC; pStatusTris = &nStatus_TrisC; pStatusPort = &nStatus_PortC; break;
        case 'D': pMonitor = &nStatus_MonitorD; pPort = &PORTD; pTris = &TRISD; pStatusTris = &nStatus_TrisD; pStatusPort = &nStatus_PortD; break;
        case 'E': pMonitor = &nStatus_MonitorE; pPort = &PORTE; pTris = &TRISE; pStatusTris = &nStatus_TrisE; pStatusPort = &nStatus_PortE; break;
        default: return;
    }
    
    if (*pMonitor == 0){
        return;
    }
    
    if (*pPort != *pStatusPort || *pTris != *pStatusTris){
        sReply[0] = 0x00;
        
        clock2str(sStr1, 0);
        byte2binstr(sStr2, *pStatusTris);
        byte2binstr(sStr3, *pTris);
        byte2binstr(sStr4, *pStatusPort);
        byte2binstr(sStr5, *pPort);
        printf("!OK CHANGE %c %s TRIS %s > %s PORT %s > %s\r\n", cPortName, sStr1, sStr2, sStr3, sStr4, sStr5);

        *pStatusPort = *pPort;
        *pStatusTris = *pTris;

    }
}

void APP_usbConfigured(void){
    CDCInitEP();
    line_coding.bCharFormat = 0;
    line_coding.bDataBits   = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate   = 9600;
}

void APP_updateUsbLed(void){
    static uint16_t ledCount = 0;

    if(USBIsDeviceSuspended() == true){
        clearbit(MasterLedStatus, LED_USB);
        //LED_Off(LED_USB_DEVICE_STATE);
        return;
    }

    switch(USBGetDeviceState()){
        case CONFIGURED_STATE:
            /* We are configured.  Blink fast. On for 75ms, off for 75ms, then reset/repeat. */
            if(ledCount == 1){
                setbit(MasterLedStatus, LED_USB);
                //LED_On(LED_USB_DEVICE_STATE);
            }
            else if(ledCount == 75){
                clearbit(MasterLedStatus, LED_USB);
                //LED_Off(LED_USB_DEVICE_STATE);
            }
            else if(ledCount > 150){
                ledCount = 0;
            }
            break;
        default:
            /* We aren't configured yet, but we aren't suspended so let's blink with
             * a slow pulse. On for 50ms, then off for 950ms, then reset/repeat. */
            if(ledCount == 1){
                setbit(MasterLedStatus, LED_USB);
                //LED_On(LED_USB_DEVICE_STATE);
            }
            else if(ledCount == 50){
                clearbit(MasterLedStatus, LED_USB);
                //LED_Off(LED_USB_DEVICE_STATE);
            }
            else if(ledCount > 950){
                ledCount = 0;
            }
            break;
    }

    /* Increment the millisecond counter. */
    ledCount++;
}
