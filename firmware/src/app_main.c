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

#include "app_main.h"
#include "usb_config.h"

/** MACROS **/
/* Bit Operation macros */
#define setbit(b,n)   (b |=   (1 << n))        /* Set bit number n in byte b   */
#define clearbit(b,n) (b &= (~(1 << n)))       /* Clear bit number n in byte b */
#define readbit(b,n)  ((b &    (1 << n)) >> n) /* Read bit number n in byte b  */
#define flipbit(b,n)  (b ^=   (1 << n))        /* Flip bit number n in byte b  */

#define bit_is_set(b,n)   (b   & (1 << n))     /* Test if bit number n in byte b is set   */
#define bit_is_clear(b,n) (!(b & (1 << n)))    /* Test if bit number n in byte b is clear */

/** CONSTANTS ******************************************************/
const char txtCrLf[]       = "\r\n";
const char txtAlphaLC[]    = "abcdefghijklmnopqrstuvwxyz";
const char txtAlphaUC[]    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char txtAlpha[]      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char txtNum[]        = "0123456789";
const char txtAlphaNum[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char txtWhitespace[] = " \t\r\n";

const char txtOk[]        = "+OK";
const char txtOkSp[]      = "+OK ";
const char txtOkCrLf[]    = "+OK\r\n";
const char txtOkOn[]      = "+OK On\r\n";
const char txtOkOff[]     = "+OK Off\r\n";
const char txtOn[]        = "On";
const char txtOff[]       = "Off";

const char txtError[]     = "-ERROR";
const char txtErrorSp[]   = "-ERROR ";
const char txtErrorCrLf[] = "-ERROR\r\n";

const char txtErrorUnknownCommand[]  = "Unknown command";
const char txtErrorUnknownArgument[] = "Unknown argument";
const char txtErrorUnknownPin[]      = "Unknown pin";

// First nibble goes to RB5~2 as 1, second nibble goes to RA3~0
const unsigned char mapLeds[] = {
    0b11101110, // 0001 0001,
    0b11101101, // 0001 0010,
    0b11101011, // 0001 0100,
    0b11100111, // 0001 1000,
    0b11011110, // 0010 0001,
    0b11011101, // 0010 0010,
    0b11011011, // 0010 0100,
    0b11010111, // 0010 1000,
    0b10111101, // 0100 0010,
    0b10111011, // 0100 0100,
    0b10110111, // 0100 1000,
    0b01111110, // 1000 0001,
    0b01111101, // 1000 0010,
    0b01111011, // 1000 0100,
    0b01110111, // 1000 1000
    0b11111111  // 0100 0001 DOES NOT EXIST IN HARDWARE
};

const unsigned char MasterToneNotes[] = {
   69	, //	0	F5
   0	, //	1	-
   69	, //	2	F5
   0	, //	3	-
   69	, //	4	F5
   0	, //	5	-
   92	, //	6	C5
   0	, //	7	-
   55	, //	8	A5
   0	, //	9	-
   55	, //	10	A5
   0	, //	11	-
   55	, //	12	A5
   0	, //	13	-
   69	, //	14	F5
   0	, //	15	-
   69	, //	16	F5
   0	, //	17	-
   55	, //	18	A5
   0	, //	19	-
   46	, //	20	C6
   0	, //	21	-
   46	, //	22	C6
   0	, //	23	-
   58	, //	24	A#5
   0	, //	25	-
   55	, //	26	A5
   0	, //	27	-
   61	, //	28	G5
   0	, //	29	-
   61	, //	30	G5
   0	, //	31	-
   55	, //	32	A5
   0	, //	33	-
   58	, //	34	A#5
   0	, //	35	-
   58	, //	36	A#5
   0	, //	37	-
   55	, //	38	A5
   0	, //	39	-
   61	, //	40	G5
   0	, //	41	-
   55	, //	42	A5
   0	, //	43	-
   69	, //	44	F5
   0	, //	45	-
   69	, //	46	F5
   0	, //	47	-
   55	, //	48	A5
   0	, //	49	-
   61	, //	50	G5
   0	, //	51	-
   92	, //	52	C5
   0	, //	53	-
   73	, //	54	E5
   0	, //	55	-
   61	  //	56	G5
};
const unsigned int MasterToneBeats[] = {
    12	, //	0	F5
    1	, //	1	-
    4	, //	2	F5
    1	, //	3	-
    16	, //	4	F5
    1	, //	5	-
    16	, //	6	C5
    1	, //	7	-
    12	, //	8	A5
    1	, //	9	-
    4	, //	10	A5
    1	, //	11	-
    16	, //	12	A5
    1	, //	13	-
    16	, //	14	F5
    1	, //	15	-
    12	, //	16	F5
    1	, //	17	-
    4	, //	18	A5
    1	, //	19	-
    24	, //	20	C6
    1	, //	21	-
    8	, //	22	C6
    1	, //	23	-
    8	, //	24	A#5
    1	, //	25	-
    8	, //	26	A5
    1	, //	27	-
    32	, //	28	G5
    1	, //	29	-
    12	, //	30	G5
    1	, //	31	-
    4	, //	32	A5
    1	, //	33	-
    16	, //	34	A#5
    1	, //	35	-
    16	, //	36	A#5
    1	, //	37	-
    12	, //	38	A5
    1	, //	39	-
    4	, //	40	G5
    1	, //	41	-
    16	, //	42	A5
    1	, //	43	-
    16	, //	44	F5
    1	, //	45	-
    12	, //	46	F5
    1	, //	47	-
    4	, //	48	A5
    1	, //	49	-
    24	, //	50	G5
    1	, //	51	-
    8	, //	52	C5
    1	, //	53	-
    8	, //	54	E5
    1	, //	55	-
    8	  //	56	G5
};

/** VARIABLES ******************************************************/

unsigned char nMasterDebug        = 0;     // 

// MASTER CLOCK
#define       MasterClockTickCount  24     /* Number of ticks per ms */
unsigned char MasterClockTick     = 0;     // Tick counter 0,1,2
unsigned long MasterClockMS       = 0;     // MS counter, good for up to over a month
// MASTER NOTIFY
unsigned int  MasterNotifyCounter = 0;     // Notification timer counter    
unsigned int  MasterNotify        = 60000; // When to notify (1 minute))
unsigned long MasterNotifyNow     = 0;     // When not 0, the main loop will notify
// MASTER LEDS
unsigned int  MasterLedStatus     = 0;     // Bitfield of 16 leds statuses
unsigned int  MasterLedCounter    = 0;     // Tick Counter
#define       MasterLedTime         30000  // Ticks to count between
// MASTER TONES
unsigned char MasterToneEnabled   = 0;     //
unsigned char MasterTonePeriod    = 0;     //
unsigned char MasterToneTick      = 0;     //
unsigned long MasterToneTime      = 0;     //
unsigned long MasterToneCounter   = 0;     //
unsigned long MasterToneTempo     = 7200;  //
unsigned char MasterToneRestart   = 0;     //
unsigned char MasterToneStep      = 0;     //
unsigned char MasterToneMode      = 1;



// USB buffers
#define sizeChunk     16
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
struct {
	unsigned usb:1;
	unsigned connected:1;
	unsigned notify:1;
	unsigned reportOnce:1;
	unsigned bufferOverrun:1;
} StatusConsole;

/** PRIVATE PROTOTYPES ***************************************/
// Main procs
void         APP_init(void);
void         APP_main(void);

// USB
void         APP_usbConfigured(void);
void         APP_checkCommands(void);
void         APP_executeCommand(void);
void         APP_updateUsbLed(void);

// APP Commands
void APP_CMD_ping(unsigned char *ptrArgs);
void APP_CMD_uptime(unsigned char *ptrArgs);
void APP_CMD_debug(unsigned char *ptrArgs);
void APP_CMD_tone(unsigned char *ptrArgs);
void APP_CMD_monitor(unsigned char *ptrArgs);
void APP_CMD_read(unsigned char *ptrArgs);
void APP_CMD_write(unsigned char *ptrArgs);
void APP_CMD_led(unsigned char *ptrArgs);

// APP functions
void APP_splitArgs4(unsigned char *pArgs, unsigned char *pArg1, unsigned char *pArg2, unsigned char *pArg3, unsigned char *pArg4);
void APP_checkPins(const unsigned char cPortName);
void APP_updateLeds(void);

// Helper functions
void putch(unsigned char byte);
void print(unsigned char *pStr);

// Convertions and string handling
unsigned long str2long(const char *sStr);
void          byte2hex(const char cChar, char *sStr1);
void          int2hex(unsigned int iNum, char *sStr1);
void          str2hex(const char *sStr2, char *sStr1);
inline void   str2lower(unsigned char *pStr);
inline void   str2upper(unsigned char *pStr);
void          byte2binstr(char *sStr, unsigned char iNum);
void          int2binstr(char *sStr, unsigned int iNum);
void          clock2string(char *sStr);


/** FUNCTIONS *******************************************************/
void APP_init(void){
    /*7
    LED_Enable(LED_D1);
    LED_Enable(LED_D2);
    LED_Enable(LED_D3);
      */

    //output
    TRISDbits.TRISD0 = 1;
    TRISDbits.TRISD1 = 1;
    TRISDbits.TRISD2 = 1;
    TRISDbits.TRISD3 = 1;
    TRISDbits.TRISD4 = 1;
    TRISDbits.TRISD5 = 1;
    TRISDbits.TRISD6 = 1;
    TRISDbits.TRISD7 = 1;

    
	StatusConsole.notify        = 0;
	StatusConsole.reportOnce    = 0;
	StatusConsole.bufferOverrun = 0;
    
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
    
	RCONbits.IPEN       = 1;  // Interrupts priority enable
	enableInterrupts();
 */
} // End APP_init

void APP_main(){
    
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
        // TODO: change it to use MasterNotifyNow instead of MasterClockMS
        clock2string(sStr1);
        printf("!OK UPTIME: %s\r\n", sStr1);
        MasterNotifyNow = 0;
    }
                
    // INPUT handling
    if(USBUSARTIsTxTrfReady() == true){
        uint8_t i;
        uint8_t n;
        uint8_t nBytes;

        if (true) {
            nBytes = getsUSBUSART(bufChunk, 1);
            if (nBytes > 0) {
                /* For every byte that was read... */
                if (bufChunk[0] == 0x0D || bufChunk[0] == 0x0A){
                    bufCommand[posCommand] = 0x00;
                    bufCommand[posCommand + 1] = 0x00; // Workaround for args parsing
                    APP_executeCommand();
                }
                else{
                    bufCommand[posCommand] = bufChunk[0];
                    posCommand++;
                    bufCommand[posCommand] = 0x00;
                }
            }
        }
        else {
            nBytes = getsLineUSBUSART(&bufCommand[posCommand], sizeof(bufCommand) - posCommand - 1);
            if (nBytes > 0) {
                //strcat(bufCommand, bufChunk);
                posCommand += nBytes;
                /* For every byte that was read... */
                if (bufCommand[posCommand - 1] == 0x0D || bufCommand[posCommand - 1] == 0x0A){
                    bufCommand[posCommand - 1] = 0x00;
                    bufCommand[posCommand]     = 0x00;
                    bufCommand[posCommand + 1] = 0x00; // Workaround for args parsing
                    APP_executeCommand();
                }
            }
        }
        
        
    }

    if(USBUSARTIsTxTrfReady() == true){
        if(posOutput > 0){
            putUSBUSART(bufOutput, posOutput);
            posOutput = 0;
        }
    }

    CDCTxService();
}


void APP_CMD_ping(unsigned char *ptrArgs){
    print(txtOkSp);
    print("PONG: ");
    print(ptrArgs);
    print(txtCrLf);
}

void APP_CMD_uptime(unsigned char *ptrArgs){
    print(txtOkSp);
    print("UPTIME: ");
    clock2string(sReply);
    print(sReply);
    print(txtCrLf);
}
        
void APP_CMD_debug(unsigned char *ptrArgs){
    str2lower(ptrArgs);   
        
    print(txtOkSp);
    print("DEBUG: ");
    if (strcmp(ptrArgs, "on") == 0 || strcmp(ptrArgs, "1") == 0){
        nMasterDebug = 1;
        print(txtOn);
    }
    else if (strcmp(ptrArgs, "off") == 0 || strcmp(ptrArgs, "0") == 0){
        nMasterDebug = 0;
        print(txtOff);
    }
    else if (strlen(ptrArgs) == 0){
        print(nMasterDebug > 0 ? txtOn : txtOff);
    }
    else{
        print(txtErrorUnknownArgument);
        print(". Currently: ");
        print(nMasterDebug > 0 ? txtOn : txtOff);
    }
    print(txtCrLf);
}

void APP_CMD_tone(unsigned char *ptrArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    sReply[0] = 0x00;
    
    str2lower(ptrArgs);
    
    pArg1 = strtok(ptrArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (pArg1 == NULL){
        if (MasterToneMode){
            sprintf(sReply, "%s %s Step: %d Period: %d Time: %d", 
                "Music",
                MasterToneEnabled > 0 ? txtOn : txtOff,
                MasterToneStep,
                MasterToneNotes[MasterToneStep],
                MasterToneBeats[MasterToneStep]
            );
        }
        else{
            sprintf(sReply, "%s %s Period: %d Time: %d",
                "Single",
                MasterToneEnabled > 0 ? txtOn : txtOff,
                MasterTonePeriod,
                MasterToneTime
            );
        }
    }
    else if (strcmp(pArg1, "on") == 0 || strcmp(pArg1, "1") == 0){
        MasterToneEnabled = 1;
        strcpy(sReply, txtOn);
    }
    else if (strcmp(pArg1, "off") == 0 || strcmp(pArg1, "0") == 0){
        MasterToneEnabled = 0;
        strcpy(sReply, txtOff);
    }
    else if (strcmp(pArg1, "restart") == 0){
        MasterToneRestart ^= 1;
        sprintf(sReply, "Restart %s", MasterToneRestart ? txtOn : txtOff);
    }
    else if (strcmp(pArg1, "mode") == 0){
        MasterToneMode ^= 1;
        sprintf(sReply, "Mode %s", MasterToneMode ? txtOn : txtOff);
    }
    else if (strcmp(pArg1, "period") == 0){
        MasterTonePeriod = (unsigned char) atoi(pArg2);
        sprintf(sReply, "Period %u", MasterTonePeriod);
    }
    else if (strcmp(pArg1, "time") == 0){
        MasterToneTime = (unsigned int) atoi(pArg2);
        sprintf(sReply, "Period %u", MasterToneTime);
    }
    else if (strcmp(pArg1, "tempo") == 0){
        MasterToneTempo = (unsigned int) atoi(pArg2);
        sprintf(sReply, "Tempo %u", MasterToneTempo);
    }
    else{
        sprintf(sReply, "%s. Currently: %s", txtErrorUnknownArgument, MasterToneEnabled > 0 ? txtOn : txtOff);
    }

    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("TONE: ");
    print(sReply);
    print(txtCrLf);
}


// TODO: remover
void APP_splitArgs4(unsigned char *pArgs, unsigned char *pArg1, unsigned char *pArg2, unsigned char *pArg3, unsigned char *pArg4){
    pArg1 = NULL;
    pArg2 = NULL;
    pArg3 = NULL;
    pArg4 = NULL;
    
    // Get the command
    printf("TESTING\r\nARGS=<%s>\r\n", pArgs);

    pArg1 = strtok(pArgs, " \t\0\r\n");
    if (pArg1 != NULL){
        pArg2 = strtok(NULL, " \t\0\r\n");
        if (pArg2 != NULL){
            pArg3 = strtok(NULL, " \t\0\r\n");
            if (pArg3 != NULL){
                pArg4 = strtok(NULL, " \t\0\r\n");
            }
        }
    }

    printf("1=<%s>\r\n2=<%s>\r\n3=<%s>\r\n4=<%s>\r\n", pArgs, pArg1, pArg2, pArg3, pArg4);
}

void APP_CMD_monitor(unsigned char *ptrArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pMonitor = NULL;
    
    sReply[0] = 0x00;
    
    str2lower(ptrArgs);
    
    pArg1 = strtok(ptrArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    str2upper(pArg1);
    if (strlen(pArg1) == 1){
        switch (pArg1[0]){
            case 'A': pMonitor = &nStatus_MonitorA; break;
            case 'B': pMonitor = &nStatus_MonitorB; break;
            case 'C': pMonitor = &nStatus_MonitorC; break;
            case 'D': pMonitor = &nStatus_MonitorD; break;
            case 'E': pMonitor = &nStatus_MonitorE; break;
        }
    }

    if (!pArg1){
        strcat(sReply, "A ");
        strcat(sReply, nStatus_MonitorA > 0 ? txtOn : txtOff);
        strcat(sReply, " / B ");
        strcat(sReply, nStatus_MonitorB > 0 ? txtOn : txtOff);
        strcat(sReply, " / C ");
        strcat(sReply, nStatus_MonitorC > 0 ? txtOn : txtOff);
        strcat(sReply, " / D ");
        strcat(sReply, nStatus_MonitorD > 0 ? txtOn : txtOff);
        strcat(sReply, " / E ");
        strcat(sReply, nStatus_MonitorE > 0 ? txtOn : txtOff);
    }
    else {
        if (!pMonitor){
            bOK = false;
            strcpy(sReply, txtErrorUnknownArgument);
        }
        else if (!pArg2){
            *pMonitor ^= 1;
            strcpy(sReply, pArg1);
            strcat(sReply, *pMonitor ? " On" : " Off");
        }
        else {
            *pMonitor = strcmp(pArg2, "on") == 0 ? 1 : 0;
            strcpy(sReply, pArg1);
            strcat(sReply, *pMonitor ? " On" : " Off");
        }
    }

    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("MONITOR: ");
    print(sReply);
    print(txtCrLf);

}

void APP_CMD_read(unsigned char *ptrArgs){
    bool bOK = true;
    
    sReply[0] = 0x00;
    sReply[1] = 0x00;
    
    switch (ptrArgs[0]){
        case 'a': //97 a 
        case 'A': //97 a 
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISAbits.TRISA0 = 1; sReply[0] = PORTAbits.RA0 + 48; break;
                case 49: /*1*/ TRISAbits.TRISA1 = 1; sReply[0] = PORTAbits.RA1 + 48; break;
                case 50: /*2*/ TRISAbits.TRISA2 = 1; sReply[0] = PORTAbits.RA2 + 48; break;
                case 51: /*3*/ TRISAbits.TRISA3 = 1; sReply[0] = PORTAbits.RA3 + 48; break;
                case 52: /*4*/ TRISAbits.TRISA4 = 1; sReply[0] = PORTAbits.RA4 + 48; break;
                case 53: /*5*/ TRISAbits.TRISA5 = 1; sReply[0] = PORTAbits.RA5 + 48; break;
                //case 54: /*6*/ // OSC2
                //case 55: /*7*/ // Does not exist
                case 0: 
                    TRISA = 0xff;
                    byte2binstr(sReply, PORTA);
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 98: //b
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISBbits.TRISB0 = 1; sReply[0] = PORTBbits.RB0 + 48; break;
                case 49: /*1*/ TRISBbits.TRISB1 = 1; sReply[0] = PORTBbits.RB1 + 48; break;
                case 50: /*2*/ TRISBbits.TRISB2 = 1; sReply[0] = PORTBbits.RB2 + 48; break;
                case 51: /*3*/ TRISBbits.TRISB3 = 1; sReply[0] = PORTBbits.RB3 + 48; break;
                case 52: /*4*/ TRISBbits.TRISB4 = 1; sReply[0] = PORTBbits.RB4 + 48; break;
                case 53: /*5*/ TRISBbits.TRISB5 = 1; sReply[0] = PORTBbits.RB5 + 48; break;
                case 54: /*6*/ TRISBbits.TRISB6 = 1; sReply[0] = PORTBbits.RB6 + 48; break;
                case 55: /*7*/ TRISBbits.TRISB7 = 1; sReply[0] = PORTBbits.RB7 + 48; break;
                case 0: 
                    TRISB = 0xff;
                    byte2binstr(sReply, PORTB);
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 99: //c
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISCbits.TRISC0 = 1; sReply[0] = PORTCbits.RC0 + 48; break;
                case 49: /*1*/ TRISCbits.TRISC1 = 1; sReply[0] = PORTCbits.RC1 + 48; break;
                case 50: /*2*/ TRISCbits.TRISC2 = 1; sReply[0] = PORTCbits.RC2 + 48; break;
                //case 51: /*3*/ // Does not exist
                //case 52: /*4*/ // D- ReadOnly, Tris not needed (DISABLED as we use USB)
                //case 53: /*5*/ // D+ ReadOnly, Tris not needed (DISABLED as we use USB)
                //case 54: /*6*/ // TX (DISABLED as we use UART)
                //case 55: /*7*/ // RX (DISABLED as we use UART)
                case 0: 
                    TRISC = 0xff;
                    byte2binstr(sReply, PORTC);
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 100: //d
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISDbits.TRISD0 = 1; sReply[0] = PORTDbits.RD0 + 48; break;
                case 49: /*1*/ TRISDbits.TRISD1 = 1; sReply[0] = PORTDbits.RD1 + 48; break;
                case 50: /*2*/ TRISDbits.TRISD2 = 1; sReply[0] = PORTDbits.RD2 + 48; break;
                case 51: /*3*/ TRISDbits.TRISD3 = 1; sReply[0] = PORTDbits.RD3 + 48; break;
                case 52: /*4*/ TRISDbits.TRISD4 = 1; sReply[0] = PORTDbits.RD4 + 48; break;
                case 53: /*5*/ TRISDbits.TRISD5 = 1; sReply[0] = PORTDbits.RD5 + 48; break;
                case 54: /*6*/ TRISDbits.TRISD6 = 1; sReply[0] = PORTDbits.RD6 + 48; break;
                case 55: /*7*/ TRISDbits.TRISD7 = 1; sReply[0] = PORTDbits.RD7 + 48; break;
                case 0: 
                    TRISD = 0xff;
                    byte2binstr(sReply, PORTD);
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 101: //e
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISEbits.TRISE0 = 1; sReply[0] = PORTEbits.RE0 + 48; break;
                case 49: /*1*/ TRISEbits.TRISE1 = 1; sReply[0] = PORTEbits.RE1 + 48; break;
                case 50: /*2*/ TRISEbits.TRISE2 = 1; sReply[0] = PORTEbits.RE2 + 48; break;
                case 0: 
                    TRISE = 0xff;
                    byte2binstr(sReply, PORTE);
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        default: 
            bOK = false;
            strcat(sReply, txtErrorUnknownArgument);
            break;
    }
    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("READ: ");
    print(sReply);
    print(txtCrLf);
}

void APP_CMD_write(unsigned char *ptrArgs){
    bool bOK = true;
    
    sReply[0] = 0x00;
    sReply[1] = 0x00;

    switch (ptrArgs[0]){
        case 97: //a 
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISAbits.TRISA0 = 0; LATAbits.LATA0 = ptrArgs[2] - 48; break;
                case 49: /*1*/ TRISAbits.TRISA1 = 0; LATAbits.LATA1 = ptrArgs[2] - 48; break;
                case 50: /*2*/ TRISAbits.TRISA2 = 0; LATAbits.LATA2 = ptrArgs[2] - 48; break;
                case 51: /*3*/ TRISAbits.TRISA3 = 0; LATAbits.LATA3 = ptrArgs[2] - 48; break;
                case 52: /*4*/ TRISAbits.TRISA4 = 0; LATAbits.LATA4 = ptrArgs[2] - 48; break;
                case 53: /*5*/ TRISAbits.TRISA5 = 0; LATAbits.LATA5 = ptrArgs[2] - 48; break;
                //case 54: /*6*/ TRISAbits.TRISA6 = 0; LATAbits.LATA6 = ptrArgs[2] - 48; break;
                //case 55: /*7*/ TRISAbits.TRISA7 = 0; LATAbits.LATA7 = ptrArgs[2] - 48; break;
                case 0: 
                    TRISA = 0x00;
                    LATA = ptrArgs[2];
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 98: //b
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISBbits.TRISB0 = 0; LATBbits.LATB0 = ptrArgs[2] - 48; break;
                case 49: /*1*/ TRISBbits.TRISB1 = 0; LATBbits.LATB1 = ptrArgs[2] - 48; break;
                case 50: /*2*/ TRISBbits.TRISB2 = 0; LATBbits.LATB2 = ptrArgs[2] - 48; break;
                case 51: /*3*/ TRISBbits.TRISB3 = 0; LATBbits.LATB3 = ptrArgs[2] - 48; break;
                case 52: /*4*/ TRISBbits.TRISB4 = 0; LATBbits.LATB4 = ptrArgs[2] - 48; break;
                case 53: /*5*/ TRISBbits.TRISB5 = 0; LATBbits.LATB5 = ptrArgs[2] - 48; break;
                case 54: /*6*/ TRISBbits.TRISB6 = 0; LATBbits.LATB6 = ptrArgs[2] - 48; break; // PGC
                case 55: /*7*/ TRISBbits.TRISB7 = 0; LATBbits.LATB7 = ptrArgs[2] - 48; break; // PGD
                case 0: 
                    TRISB = 0x00;
                    LATB = ptrArgs[2];
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 99: //c
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISCbits.TRISC0 = 0; LATCbits.LATC0 = ptrArgs[2] - 48; break;
                case 49: /*1*/ TRISCbits.TRISC1 = 0; LATCbits.LATC1 = ptrArgs[2] - 48; break;
                case 50: /*2*/ TRISCbits.TRISC2 = 0; LATCbits.LATC2 = ptrArgs[2] - 48; break;
                //case 51: /*3*/ // Does not exist
                //case 52: /*4*/ // D- ReadOnly (DISABLED as we use USB)
                //case 53: /*5*/ // D+ ReadOnly (DISABLED as we use USB)
                //case 54: /*6*/ // TX (DISABLED as we use UART)
                //case 55: /*7*/ // RX (DISABLED as we use UART)
                case 0: 
                    TRISC = 0x00;
                    LATC = ptrArgs[2];
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 100: //d
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISDbits.TRISD0 = 0; LATDbits.LATD0 = ptrArgs[2] - 48; break;
                case 49: /*1*/ TRISDbits.TRISD1 = 0; LATDbits.LATD1 = ptrArgs[2] - 48; break;
                case 50: /*2*/ TRISDbits.TRISD2 = 0; LATDbits.LATD2 = ptrArgs[2] - 48; break;
                case 51: /*3*/ TRISDbits.TRISD3 = 0; LATDbits.LATD3 = ptrArgs[2] - 48; break;
                case 52: /*4*/ TRISDbits.TRISD4 = 0; LATDbits.LATD4 = ptrArgs[2] - 48; break;
                case 53: /*5*/ TRISDbits.TRISD5 = 0; LATDbits.LATD5 = ptrArgs[2] - 48; break;
                case 54: /*6*/ TRISDbits.TRISD6 = 0; LATDbits.LATD6 = ptrArgs[2] - 48; break;
                case 55: /*7*/ TRISDbits.TRISD7 = 0; LATDbits.LATD7 = ptrArgs[2] - 48; break;
                case 0: 
                    TRISD = 0x00;
                    LATD = ptrArgs[2];
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        case 101: //e
            switch (ptrArgs[1]){
                case 48: /*0*/ TRISEbits.TRISE0 = 0; LATEbits.LATE0 = ptrArgs[2] - 48; break;
                case 49: /*1*/ TRISEbits.TRISE1 = 0; LATEbits.LATE1 = ptrArgs[2] - 48; break;
                case 50: /*2*/ TRISEbits.TRISE2 = 0; LATEbits.LATE2 = ptrArgs[2] - 48; break;
                case 0: 
                    TRISE = 0x00;
                    LATE = ptrArgs[2];
                    break;
                default: 
                    bOK = false;
                    strcat(sReply, txtErrorUnknownPin);
                    break;
            }
            break;
        default: 
            break;
    }
    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("WRITE: ");
    print(sReply);
    print(txtCrLf);
}

void APP_CMD_led(unsigned char *ptrArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char nLed   = 0;
    unsigned int  iBit   = 1;
    unsigned int  iMask  = 1; // It is later inverted
    
    sReply[0] = 0x00;
    
    str2lower(ptrArgs);
    
    pArg1 = strtok(ptrArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (strlen(pArg1) == 1){
        iBit = (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0) ? 1 : 0;
        
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        
        printf("N=%x Bit=%x Mask=%x\r\n", nLed, iBit, iMask);
        if (nLed){
            iBit  = iBit  << nLed;
            iMask = iMask << nLed;
        }
        iMask = ~iMask;
        printf("N=%x Bit=%x Mask=%x\r\n", nLed, iBit, iMask);
        
        MasterLedStatus = (MasterLedStatus & iMask) | iBit;
        sprintf(sReply, "%d %s", nLed, iBit ? txtOn : txtOff);
    }
    else{
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
    }
    
    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("LED: ");
    print(sReply);
    print(" Status: ");
    int2binstr(sStr1, MasterLedStatus);
    print(sStr1);
    print(txtCrLf);
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
    if (nMasterDebug){
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
            if (MasterToneCounter == 0){
                if (MasterToneMode == 0){
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
                    MasterToneStep++;
                    if (MasterToneStep >= sizeof(MasterToneNotes)){
                        if (MasterToneRestart){
                            MasterToneStep = 0;
                        }
                        else{
                            MasterToneEnabled = 0;
                        }
                    }
                    if (MasterToneEnabled){
                        MasterTonePeriod  = MasterToneNotes[MasterToneStep];
                        MasterToneCounter = MasterToneBeats[MasterToneStep] * MasterToneTempo;
                        MasterToneTick    = 0;
                        if (!MasterToneCounter){
                            // Autostop
                            MasterToneEnabled = 0;
                            MasterTonePeriod  = 0;
                        }
                    }
                }
            }
            else{
                MasterToneCounter--;
            }
            
            if (MasterToneTick == 0){
                MasterToneTick = MasterTonePeriod;
                if (MasterTonePeriod){
                    // Invert output
                    TRISCbits.RC2 = 0;
                    LATCbits.LATC2 ^= 1;
                }
            }
            else{
                MasterToneTick--;
            }
        }
        
        // MASTER CLOCK
        MasterClockTick++;
        if (MasterClockTick == MasterClockTickCount){
            MasterClockTick = 0;
            // All counters that need ms updates
            MasterClockMS++;
            if (MasterNotifyCounter){
                MasterNotifyCounter--;
            }
            //MasterNotifyCounter++;
            // Must run once a ms
            // TODO! reveer que hacer con el led usb
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
                MasterNotifyNow = MasterClockMS;
            }
        }

        
        // MASTER LEDS
        if (MasterLedCounter == 0){
            MasterLedCounter = MasterLedTime;
            APP_updateLeds();
        }
        else {
            MasterLedCounter--;
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
        TRISA = (TRISA & 0b11110000) | (mapLeds[nCurrentLed]        & 0b00001111);
        TRISB = (TRISB & 0b11000011) | ((mapLeds[nCurrentLed] >> 2 )& 0b00111100);

        LATA &= 0b11110000; // We always force a 0 on A (Cathode)
        LATB |= 0b00111100; // We always force a 1 on B (Anode)
    }

    printf("\r\n!OK LED %d on\r\n", nCurrentLed);

    // If the led is the same one, we do not do anything as everything should already be set
}

void APP_checkPins(unsigned char cPortName){
    unsigned char *pMonitor;
    unsigned char *pPort;
    unsigned char *pTris;
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
        
        clock2string(sStr1);
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
        LED_Off(LED_USB_DEVICE_STATE);
        return;
    }

    switch(USBGetDeviceState()){
        case CONFIGURED_STATE:
            /* We are configured.  Blink fast. On for 75ms, off for 75ms, then reset/repeat. */
            if(ledCount == 1){
                LED_On(LED_USB_DEVICE_STATE);
            }
            else if(ledCount == 75){
                LED_Off(LED_USB_DEVICE_STATE);
            }
            else if(ledCount > 150){
                ledCount = 0;
            }
            break;
        default:
            /* We aren't configured yet, but we aren't suspended so let's blink with
             * a slow pulse. On for 50ms, then off for 950ms, then reset/repeat. */
            if(ledCount == 1){
                LED_On(LED_USB_DEVICE_STATE);
            }
            else if(ledCount == 50){
                LED_Off(LED_USB_DEVICE_STATE);
            }
            else if(ledCount > 950){
                ledCount = 0;
            }
            break;
    }

    /* Increment the millisecond counter. */
    ledCount++;
}

/* HELPER FUNCTIONS */

// Function used by stdio (printf, etc)
void putch(unsigned char byte){
    if (posOutput >= sizeOutput){
        if (mUSBUSARTIsTxTrfReady()){
            putsUSBUSART(bufOutput);
            posOutput = 0;
        }
        else{
            // TODO: TBD what to do
            putsUSBUSART("\r\nOUTPUT OVERFLOW!\r\n");
            posOutput = 0;
            bufOutput[posOutput] = 0x00;
        }
    }

    bufOutput[posOutput] = byte;
    posOutput++;
    bufOutput[posOutput] = 0x00;
    if (posOutput >= sizeOutput && mUSBUSARTIsTxTrfReady()){
        putsUSBUSART(bufOutput);
        posOutput = 0;
    }
}


void print(unsigned char *pStr){
    while (*pStr > 0){
        putch(*pStr);
        pStr++;
    }
}

void byte2hex(const char cChar, char *sStr1){
    unsigned char x;
	x = (cChar & 0xF0) >> 4;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[0] = x;
	x = cChar & 0x0F;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[1] = x;
	sStr1[2] = 0x00;
} //end byte2hex

void int2hex(unsigned int iNum, char *sStr1){
    unsigned char x;
	x = (iNum & 0xF000) >> 12;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[0] = x;
	x = (iNum & 0x0F00) >> 8;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[1] = x;
	x = (iNum & 0x00F0) >> 4;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[2] = x;
	x = iNum & 0x000F;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[3] = x;
	sStr1[4] = 0x00;
} //end int2hex

void str2hex(const char *sStr2, char *sStr1){
    unsigned char len2, i, j, x;
	len2 = i = j = 0;
    do{ len2++; if (len2 == 255) break; } while (*sStr2++); // Break loop once max len is reached.
    sStr2 -= len2;                    // Re-adjust pointer to its initial location
	for (i = 0; i < len2; i++){
		x = (sStr2[i] & 0xF0) >> 4;
		x = (x < 10) ? x | 0x30 : x + 0x37;		
		sStr1[j] = x;
		j ++;
		x = sStr2[i] & 0x0F;
		x = (x < 10) ? x | 0x30 : x + 0x37;		
		sStr1[j] = x;
		j++;
	}
	sStr1[j] = 0x00;
} //end str2hex

inline void str2lower(unsigned char *pStr){
    while (*pStr){
        // if (*pStr >= 'A' && *pStr <= 'Z') *pStr =  *pStr | 0x60;
        *pStr =  tolower(*pStr);
        pStr++;
    }
}

inline void str2upper(unsigned char *pStr){
    while (*pStr){
        // if (*pStr >= 'a' && *pStr <= 'z') *pStr =  *pStr & 0x9f;
        *pStr =  toupper(*pStr);
        pStr++;
    }
}


void byte2binstr(char *sStr, unsigned char iNum){
	sStr[8] = 0x00;
	sStr[7] = (iNum & 1)   ? '1' : '0';
	sStr[6] = (iNum & 2)   ? '1' : '0';
	sStr[5] = (iNum & 4)   ? '1' : '0';
	sStr[4] = (iNum & 8)   ? '1' : '0';
	sStr[3] = (iNum & 16)  ? '1' : '0';
	sStr[2] = (iNum & 32)  ? '1' : '0';
	sStr[1] = (iNum & 64)  ? '1' : '0';
	sStr[0] = (iNum & 128) ? '1' : '0';
} //end byte2binstr

void int2binstr(char *sStr, unsigned int iNum){
    unsigned int  n = 1;
    unsigned char m = 15;
	sStr1[16] = 0x00;
    
    do {
        sStr[m] = (iNum & n)   ? '1' : '0';
        if (m){
            m--;
            n = n << 1;
        }
    }
    while (m);
} //end int2binstr

void clock2string (char *sStr){
	unsigned long ms;
	unsigned char tick, len, i, h;

	tick = MasterClockTick;
	ms   = MasterClockMS;
	if (tick != MasterClockTick){
		// This is to avoid interim changes as operations with "long" are not atomic
		ms   = MasterClockMS;
	}
	len = 15;
	                                          sStr[len] = 0x00;
	len--;                      i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = '.';
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 6;  sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = ':';
	len--; ms  -= i; ms /= 6;   i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 6;  sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = ':';
		   // 24 is not divisible by 10, so this means extra care for the hours presentation
	       ms  -= i; ms /= 6;   h = ms  % 24; ms  -= h; ms /= 24;  
	       // now we have h hours and ms days
	len--;                      i = h   % 10; sStr[len] = i   | 0x30;
	len--; h   -= i; h  /= 10;                sStr[len] = h   | 0x30;
	len--;                                    sStr[len] = 'd';
	len--;                      i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;                sStr[len] = ms  | 0x30;
	       // long 4 byte unsigned, expressing ms, will wrap at arround day 49, so no more than 2 digits for the day are needed
}