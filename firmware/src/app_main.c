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

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "usb.h"

#include "app_main.h"
#include "usb_config.h"




/** CONSTANTS ******************************************************/
char txtCrLf[]       = "\r\n";
char txtAlphaLC[]    = "abcdefghijklmnopqrstuvwxyz";
char txtAlphaUC[]    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char txtAlpha[]      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char txtNum[]        = "0123456789";
char txtAlphaNum[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
char txtWhitespace[] = " \t\r\n";

char txtOk[]        = "+OK";
char txtOkSp[]      = "+OK ";
char txtOkCrLf[]    = "+OK\r\n";
char txtOkOn[]      = "+OK On\r\n";
char txtOkOff[]     = "+OK Off\r\n";

char txtError[]     = "-ERROR";
char txtErrorSp[]   = "-ERROR ";
char txtErrorCrLf[] = "-ERROR\r\n";

char txtErrorUnknownCommand[] = "Unknown command";
char txtErrorUnknownArgument[] = "Unknown command argument";
char txtErrorUnknownPin[] = "Unknown pin";



/** VARIABLES ******************************************************/
//static uint8_t sReadBuffer[CDC_DATA_OUT_EP_SIZE];
//static uint8_t sWriteBuffer[CDC_DATA_IN_EP_SIZE];

// MASTER CLOCK (9 bytes)
#define nMasterClockTickCount  3                 /* Number of ticks per ms */
unsigned char nMasterClockTick          = 0;     // Tick counter 0,1,2
unsigned long nMasterClockMS            = 0;     // MS counter, good for up to over a month
unsigned int  nMasterClockNotifyCounter = 0;     // Notification timer counter    
unsigned int  nMasterClockNotify        = 60000; // When to notify (1 minute))

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


// USB buffers
#define sizeChunk  255
#define sizeInput  255
#define sizeOutput 255

#define sizeCommand   64
#define sizeReply     100

char           bufChunk[sizeChunk];
char           bufInput[sizeInput];
char           bufOutput[sizeOutput];
char           bufCommand[sizeCommand];
char           sReply[sizeReply];

unsigned char  posInReadChecked = 0;
unsigned char  posInRead        = 0;
unsigned char  posInWrite       = 0;
unsigned char  posOutRead       = 0;
unsigned char  posOutWrite      = 0;

unsigned char  lenCommand    = 0;
unsigned char  posCommand    = 0;
unsigned char  posReply      = 0;
unsigned char  lenChunk      = 0;
unsigned char  posInput      = 0;
unsigned char  lenInput      = 0;
unsigned char  lenOutput     = 0;
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

// Helper functions
unsigned char putsOutput(const char *pStr, unsigned char nLen);

unsigned int isStrEqualConst(const char *sStr1, const char *sStr2, unsigned int nLen);
unsigned int isStrEqualVar(const char *sStr1, const char *sStr2, unsigned int nLen);
unsigned int isStrLeftEqualConst(const char *sStr1, const char *sStr2);

unsigned long str2long(const char *sStr);
void byte2hex(const char cChar, char *sStr1);
void int2hex(unsigned int iNum, char *sStr1);
void str2hex(const char *sStr2, char *sStr1);
void byte2binstr(const unsigned char iNum, char *sStr1);
void clock2string(char *sStr);

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
    
	// Timer0 setup (Clock once every 1/3 ms)
	T0CON               = 0b01000011; // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [011]1:16
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

void APP_checkPins(const unsigned char cPortName){
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
        
        
        
        strcpy(sReply, "!OK CHANGE x \0");
        sReply[11] = cPortName;
        putsOutput(sReply, 0);

        clock2string(sReply);
        strcat(sReply, " TRIS ");
        putsOutput(sReply, 0);

        byte2binstr(*pStatusTris, sReply);
        strcat(sReply, " > ");
        putsOutput(sReply, 0);

        byte2binstr(*pTris, sReply);
        strcat(sReply, " / PORT ");
        putsOutput(sReply, 0);

        byte2binstr(*pStatusPort, sReply);
        strcat(sReply, " > ");
        putsOutput(sReply, 0);

        byte2binstr(*pPort, sReply);
        strcat(sReply, txtCrLf);
        putsOutput(sReply, 0);

        *pStatusPort = *pPort;
        *pStatusTris = *pTris;

    }
}

void APP_main(){
    
    if (posCommand  == 0 
        && posReply == 0 
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
    
    if(USBUSARTIsTxTrfReady() == true){
        uint8_t i;
        uint8_t n;
        uint8_t numBytesRead;

        numBytesRead = getsUSBUSART(bufChunk, 1);
        if (numBytesRead > 0) {
            /* For every byte that was read... */
            if (bufChunk[0] == 0x0A || bufChunk[0] == 0x0D){
                bufCommand[posCommand] = 0x00;
                bufCommand[posCommand + 1] = 0x00;
                APP_executeCommand();
            }
            else{
                bufCommand[posCommand] = bufChunk[0];
                posCommand++;
                bufCommand[posCommand] = 0x00;
            }
        }
    }

    if(USBUSARTIsTxTrfReady() == true){
        if(posReply > 0){
            putUSBUSART(bufOutput, posReply);
            posReply = 0;
        }
    }

    CDCTxService();
}

void APP_main_2(void){   
	/*************************************************************************************************/
	// TASKS EXECUTED ALWAYS (EVEN WITHOUT USB)
	/*************************************************************************************************/

    // USB Stuff
    if ( USBGetDeviceState() < CONFIGURED_STATE || USBIsDeviceSuspended()== true){
        return;
    }

	// Process commands from USB
	APP_checkCommands();
    
	/*************************************************************************************************/
	// USB DEPENDENT TASKS

	// AUTOMATIC NOTIFY
    /*
	if (StatusConsole.reportOnce && StatusConsole.connected && StatusConsole.notify){
		cmdReportStatus();
		StatusConsole.reportOnce = 0;
		nDebugData = 0;
	}*/

	if (posReply > 0 && mUSBUSARTIsTxTrfReady()){
		putsUSBUSART(bufOutput);
		posReply = 0;
	}
    
    CDCTxService();
} // End APP_main

void APP_executeCommand(void){
    unsigned char n;
    unsigned char val;
    unsigned bOK = true;
    unsigned char *ptrCommand;
    unsigned char *ptrArgs;
    
    if (bufCommand[0] == 0x00){
        //return;
    }

    sReply[0] = 0x00;
    
    // Get the command
    ptrCommand = strtok(bufCommand, " ,.-\t\0\r\n");
    ptrArgs = NULL;
    // Get the arguments
    if (ptrCommand != NULL){
        ptrArgs = &bufCommand[strlen(ptrCommand) + 1];
    }
    
    // Debug
    putsOutput("CMD=<", 0);
    putsOutput(ptrCommand, 0);
    putsOutput(">\r\nARGS=<", 0);
    putsOutput(ptrArgs, 0);
    putsOutput(">\r\n", 0);

    if (ptrCommand == NULL){    
        putsOutput(txtErrorSp, 0);
        putsOutput("Missing command", 0);
        putsOutput(txtCrLf, 0);
    }
    // PING
    else if (strcmp(ptrCommand, "ping") == 0){
        putsOutput(txtOkSp, 0);
        putsOutput("PONG: ", 0);
        putsOutput(ptrArgs, 0);
        putsOutput(txtCrLf, 0);
    }
    // MONITOR
    else if (strcmp(ptrCommand, "monitor") == 0){
        putsOutput(txtOkSp, 0);
        putsOutput("MONITOR: ", 0);
        
        switch (ptrArgs[0]){
            case 'A': nStatus_MonitorA ^= 1; nStatus_MonitorA > 0 ? putsOutput("A On", 0) : putsOutput("A Off", 0); break;
            case 'B': nStatus_MonitorB ^= 1; nStatus_MonitorB > 0 ? putsOutput("B On", 0) : putsOutput("B Off", 0); break;
            case 'C': nStatus_MonitorC ^= 1; nStatus_MonitorC > 0 ? putsOutput("C On", 0) : putsOutput("C Off", 0); break;
            case 'D': nStatus_MonitorD ^= 1; nStatus_MonitorD > 0 ? putsOutput("D On", 0) : putsOutput("D Off", 0); break;
            case 'E': nStatus_MonitorE ^= 1; nStatus_MonitorE > 0 ? putsOutput("E On", 0) : putsOutput("E Off", 0); break;
            default: ;
        }
        putsOutput(txtCrLf, 0);
    }
    
    // READ
    else if (strcmp(ptrCommand, "r") == 0){ //bufCommand[0] == 114 /* r */
        switch (ptrArgs[0]){
            case 97: //a 
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISAbits.TRISA0 = 1; sReply[0] = PORTAbits.RA0 + 48; sReply[1] = 0x00; break;
                    case 49: /*1*/ TRISAbits.TRISA1 = 1; sReply[0] = PORTAbits.RA1 + 48; sReply[1] = 0x00; break;
                    case 50: /*2*/ TRISAbits.TRISA2 = 1; sReply[0] = PORTAbits.RA2 + 48; sReply[1] = 0x00; break;
                    case 51: /*3*/ TRISAbits.TRISA3 = 1; sReply[0] = PORTAbits.RA3 + 48; sReply[1] = 0x00; break;
                    case 52: /*4*/ TRISAbits.TRISA4 = 1; sReply[0] = PORTAbits.RA4 + 48; sReply[1] = 0x00; break;
                    case 53: /*5*/ TRISAbits.TRISA5 = 1; sReply[0] = PORTAbits.RA5 + 48; sReply[1] = 0x00; break;
                    //case 54: /*6*/ // OSC2
                    //case 55: /*7*/ // Does not exist
                    case 0: 
                        TRISA = 0xff;
                        byte2binstr(PORTA, sReply);
                        break;
                    default: 
                        bOK = false;
                        strcat(sReply, txtErrorUnknownPin);
                        break;
                }
                break;
            case 98: //b
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISBbits.TRISB0 = 1; sReply[0] = PORTBbits.RB0 + 48; sReply[1] = 0x00; break;
                    case 49: /*1*/ TRISBbits.TRISB1 = 1; sReply[0] = PORTBbits.RB1 + 48; sReply[1] = 0x00; break;
                    case 50: /*2*/ TRISBbits.TRISB2 = 1; sReply[0] = PORTBbits.RB2 + 48; sReply[1] = 0x00; break;
                    case 51: /*3*/ TRISBbits.TRISB3 = 1; sReply[0] = PORTBbits.RB3 + 48; sReply[1] = 0x00; break;
                    case 52: /*4*/ TRISBbits.TRISB4 = 1; sReply[0] = PORTBbits.RB4 + 48; sReply[1] = 0x00; break;
                    case 53: /*5*/ TRISBbits.TRISB5 = 1; sReply[0] = PORTBbits.RB5 + 48; sReply[1] = 0x00; break;
                    case 54: /*6*/ TRISBbits.TRISB6 = 1; sReply[0] = PORTBbits.RB6 + 48; sReply[1] = 0x00; break;
                    case 55: /*7*/ TRISBbits.TRISB7 = 1; sReply[0] = PORTBbits.RB7 + 48; sReply[1] = 0x00; break;
                    case 0: 
                        TRISB = 0xff;
                        byte2binstr(PORTB, sReply);
                        break;
                    default: 
                        bOK = false;
                        strcat(sReply, txtErrorUnknownPin);
                        break;
                }
                break;
            case 99: //c
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISCbits.TRISC0 = 1; sReply[0] = PORTCbits.RC0 + 48; sReply[1] = 0x00; break;
                    case 49: /*1*/ TRISCbits.TRISC1 = 1; sReply[0] = PORTCbits.RC1 + 48; sReply[1] = 0x00; break;
                    case 50: /*2*/ TRISCbits.TRISC2 = 1; sReply[0] = PORTCbits.RC2 + 48; sReply[1] = 0x00; break;
                    //case 51: /*3*/ // Does not exist
                    //case 52: /*4*/ // D- ReadOnly, Tris not needed (DISABLED as we use USB)
                    //case 53: /*5*/ // D+ ReadOnly, Tris not needed (DISABLED as we use USB)
                    //case 54: /*6*/ // TX (DISABLED as we use UART)
                    //case 55: /*7*/ // RX (DISABLED as we use UART)
                    case 0: 
                        TRISC = 0xff;
                        byte2binstr(PORTC, sReply);
                        break;
                    default: 
                        bOK = false;
                        strcat(sReply, txtErrorUnknownPin);
                        break;
                }
                break;
            case 100: //d
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISDbits.TRISD0 = 1; sReply[0] = PORTDbits.RD0 + 48; sReply[1] = 0x00; break;
                    case 49: /*1*/ TRISDbits.TRISD1 = 1; sReply[0] = PORTDbits.RD1 + 48; sReply[1] = 0x00; break;
                    case 50: /*2*/ TRISDbits.TRISD2 = 1; sReply[0] = PORTDbits.RD2 + 48; sReply[1] = 0x00; break;
                    case 51: /*3*/ TRISDbits.TRISD3 = 1; sReply[0] = PORTDbits.RD3 + 48; sReply[1] = 0x00; break;
                    case 52: /*4*/ TRISDbits.TRISD4 = 1; sReply[0] = PORTDbits.RD4 + 48; sReply[1] = 0x00; break;
                    case 53: /*5*/ TRISDbits.TRISD5 = 1; sReply[0] = PORTDbits.RD5 + 48; sReply[1] = 0x00; break;
                    case 54: /*6*/ TRISDbits.TRISD6 = 1; sReply[0] = PORTDbits.RD6 + 48; sReply[1] = 0x00; break;
                    case 55: /*7*/ TRISDbits.TRISD7 = 1; sReply[0] = PORTDbits.RD7 + 48; sReply[1] = 0x00; break;
                    case 0: 
                        TRISD = 0xff;
                        byte2binstr(PORTD, sReply);
                        break;
                    default: 
                        bOK = false;
                        strcat(sReply, txtErrorUnknownPin);
                        break;
                }
                break;
            case 101: //e
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISEbits.TRISE0 = 1; sReply[0] = PORTEbits.RE0 + 48; sReply[1] = 0x00; break;
                    case 49: /*1*/ TRISEbits.TRISE1 = 1; sReply[0] = PORTEbits.RE1 + 48; sReply[1] = 0x00; break;
                    case 50: /*2*/ TRISEbits.TRISE2 = 1; sReply[0] = PORTEbits.RE2 + 48; sReply[1] = 0x00; break;
                    case 0: 
                        TRISE = 0xff;
                        byte2binstr(PORTE, sReply);
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
        if (bOK) putsOutput(txtOkSp, 0);  else  putsOutput(txtErrorSp, 0); 
        putsOutput("READ: ", 0);
        putsOutput(sReply, 0);
        putsOutput(txtCrLf, 0);
    }
    // WRITE
    else if (strcmp(ptrCommand, "w") == 0){ //bufCommand[0] == 119 /* w */
        switch (ptrArgs[0]){
            case 97: //a 
                switch (ptrArgs[1]){
                    case 48: /*0*/ TRISAbits.TRISA0 = 0; val = LATAbits.LATA0 = ptrArgs[2] - 48; break;
                    case 49: /*1*/ TRISAbits.TRISA1 = 0; val = LATAbits.LATA1 = ptrArgs[2] - 48; break;
                    case 50: /*2*/ TRISAbits.TRISA2 = 0; val = LATAbits.LATA2 = ptrArgs[2] - 48; break;
                    case 51: /*3*/ TRISAbits.TRISA3 = 0; val = LATAbits.LATA3 = ptrArgs[2] - 48; break;
                    case 52: /*4*/ TRISAbits.TRISA4 = 0; val = LATAbits.LATA4 = ptrArgs[2] - 48; break;
                    case 53: /*5*/ TRISAbits.TRISA5 = 0; val = LATAbits.LATA5 = ptrArgs[2] - 48; break;
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
                    case 48: /*0*/ TRISBbits.TRISB0 = 0; val = LATBbits.LATB0 = ptrArgs[2] - 48; break;
                    case 49: /*1*/ TRISBbits.TRISB1 = 0; val = LATBbits.LATB1 = ptrArgs[2] - 48; break;
                    case 50: /*2*/ TRISBbits.TRISB2 = 0; val = LATBbits.LATB2 = ptrArgs[2] - 48; break;
                    case 51: /*3*/ TRISBbits.TRISB3 = 0; val = LATBbits.LATB3 = ptrArgs[2] - 48; break;
                    case 52: /*4*/ TRISBbits.TRISB4 = 0; val = LATBbits.LATB4 = ptrArgs[2] - 48; break;
                    case 53: /*5*/ TRISBbits.TRISB5 = 0; val = LATBbits.LATB5 = ptrArgs[2] - 48; break;
                    case 54: /*6*/ TRISBbits.TRISB6 = 0; val = LATBbits.LATB6 = ptrArgs[2] - 48; break; // PGC
                    case 55: /*7*/ TRISBbits.TRISB7 = 0; val = LATBbits.LATB7 = ptrArgs[2] - 48; break; // PGD
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
                    case 48: /*0*/ TRISCbits.TRISC0 = 0; val = LATCbits.LATC0 = ptrArgs[2] - 48; break;
                    case 49: /*1*/ TRISCbits.TRISC1 = 0; val = LATCbits.LATC1 = ptrArgs[2] - 48; break;
                    case 50: /*2*/ TRISCbits.TRISC2 = 0; val = LATCbits.LATC2 = ptrArgs[2] - 48; break;
                    //case 51: /*3*/ TRISCbits.TRISC3 = 0; val = LATCbits.LATC3 = ptrArgs[2] - 48; break; // Does not exist
                    //case 52: /*4*/ TRISCbits.TRISC4 = 0; val = LATCbits.LATC4 = ptrArgs[2] - 48; break; // D- ReadOnly (DISABLED as we use USB)
                    //case 53: /*5*/ TRISCbits.TRISC5 = 0; val = LATCbits.LATC5 = ptrArgs[2] - 48; break; // D+ ReadOnly (DISABLED as we use USB)
                    //case 54: /*6*/ TRISCbits.TRISC6 = 0; val = LATCbits.LATC6 = ptrArgs[2] - 48; break; // TX (DISABLED as we use UART)
                    //case 55: /*7*/ TRISCbits.TRISC7 = 0; val = LATCbits.LATC7 = ptrArgs[2] - 48; break; // RX (DISABLED as we use UART)
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
                    case 48: /*0*/ TRISDbits.TRISD0 = 0; val = LATDbits.LATD0 = ptrArgs[2] - 48; break;
                    case 49: /*1*/ TRISDbits.TRISD1 = 0; val = LATDbits.LATD1 = ptrArgs[2] - 48; break;
                    case 50: /*2*/ TRISDbits.TRISD2 = 0; val = LATDbits.LATD2 = ptrArgs[2] - 48; break;
                    case 51: /*3*/ TRISDbits.TRISD3 = 0; val = LATDbits.LATD3 = ptrArgs[2] - 48; break;
                    case 52: /*4*/ TRISDbits.TRISD4 = 0; val = LATDbits.LATD4 = ptrArgs[2] - 48; break;
                    case 53: /*5*/ TRISDbits.TRISD5 = 0; val = LATDbits.LATD5 = ptrArgs[2] - 48; break;
                    case 54: /*6*/ TRISDbits.TRISD6 = 0; val = LATDbits.LATD6 = ptrArgs[2] - 48; break;
                    case 55: /*7*/ TRISDbits.TRISD7 = 0; val = LATDbits.LATD7 = ptrArgs[2] - 48; break;
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
                    case 48: /*0*/ TRISEbits.TRISE0 = 0; val = LATEbits.LATE0 = ptrArgs[2] - 48; break;
                    case 49: /*1*/ TRISEbits.TRISE1 = 0; val = LATEbits.LATE1 = ptrArgs[2] - 48; break;
                    case 50: /*2*/ TRISEbits.TRISE2 = 0; val = LATEbits.LATE2 = ptrArgs[2] - 48; break;
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
        if (bOK) putsOutput(txtOkSp, 0);  else  putsOutput(txtErrorSp, 0); 
        putsOutput("WRITE: ", 0);
        putsOutput(sReply, 0);
        putsOutput(txtCrLf, 0);
    }
    else if (isStrLeftEqualConst(bufCommand, "notify")){
        putsOutput("Notify!", 0);
    }
	else if (isStrEqualConst(bufCommand, "uptime", 0)){
        putsOutput(txtOkSp, 0);
        putsOutput("UPTIME ", 0);
		clock2string(sReply);
		putsOutput(sReply, 0);
        putsOutput(txtCrLf, 0);
	}
    else if (strcmp(bufCommand, "notify") == 0){
        /*
          int i;
  char strtext[] = "129th";
  char cset[] = "1234567890";

  i = strspn (strtext,cset);
  printf ("The initial number has %d digits.\n",i);
  return 0;
         */
    }
    else {
        putsOutput(txtErrorSp, 0);
        putsOutput(txtErrorUnknownCommand, 0);
        putsOutput(txtCrLf, 0);
    }
    posCommand = 0;
    bufCommand[0] = 0x00;
}

void APP_checkCommands(void){
    unsigned char  lenCommand = 0;
    unsigned char  pLastRead  = 0;
    unsigned char  nRead      = 0;
    unsigned char  nMaxRead   = 0;
    
    if (posInWrite < posInRead){
        // Rp is ahead of Wp, read only up to it
        nMaxRead = posInRead - posInWrite - 1;
        if (nMaxRead == 0){
            // We have a write overflow
            return;
        }
    }
    else {
        nMaxRead = sizeInput - posInWrite;
        // We reached the end of the buffer
        if (nMaxRead == 0){
            posInWrite = 0;
            nMaxRead = posInRead - posInWrite - 1;
            if (nMaxRead == 0){
                // We have a write overflow
                return;
            }
        }
    }
    
    if( USBGetDeviceState() < CONFIGURED_STATE || USBIsDeviceSuspended()== true){
        return;
    }

    nRead = getsUSBUSART(&bufInput[posInWrite], nMaxRead);
    if (nRead == 0){
        return;    
    }
    
    posInWrite += nRead;
    if (posInWrite >= sizeInput){
        posInWrite = 0;
    }

    lenCommand = 0;    
    while (posInReadChecked != posInWrite){
        if (bufInput[posInReadChecked] == 0x0D || bufInput[posInReadChecked] == 0x0A){
            while (posInRead != posInReadChecked){
                bufCommand[lenCommand] = bufInput[posInRead];
                lenCommand++;
                if (lenCommand >= sizeCommand){
                    // Error: Command overflow
                    break;
                }
                posInRead++;
                if (posInRead >= sizeInput){
                    posInRead = 0;
                }
            }
            lenCommand--;
            bufCommand[lenCommand] = 0x00;
        }
        
        posInReadChecked++;
        if (posInReadChecked >= sizeInput){
            posInReadChecked = 0;
        }
    }
    
    if (lenCommand > 0){
        APP_executeCommand();
    }
} // end APP_checkCommands

/* APP FUNCTIONS */

void interrupt APP_interrupt_high(void){             // High priority interrupt
	// CLOCK: Timer0 overflow int
	if (INTCONbits.TMR0IF){
        nMasterClockTick++;
        if (nMasterClockTick == nMasterClockTickCount){
            nMasterClockTick = 0;
            nMasterClockMS++;
            nMasterClockNotifyCounter++;
            APP_updateUsbLed();
        }
        
        if (nMasterClockNotifyCounter >= nMasterClockNotify){
            nMasterClockNotifyCounter = 0;
            //APP_notifyUsb();
            if (nMasterClockNotify > 0 
                && posCommand == 0 
                && posReply == 0 
                && USBGetDeviceState() == CONFIGURED_STATE
                && USBIsDeviceSuspended() == false
            ){
                strcat(sReply, "!OK UPTIME ");
                clock2string(sReply);
                strcat(sReply, txtCrLf);
                putsOutput(sReply, 0);
            }
        }
		INTCONbits.TMR0IF = 0;
	}
    
    // USB
    if (USBInterruptFlag){
        USBDeviceTasks();
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

void APP_notifyUsb(void){
}

/* HELPER FUNCTIONS */
unsigned char putsOutput(const char *pStr, unsigned char nLen){
	if (nLen == 0){
        // Get length, break loop if max len is reached.
    	do{ nLen++; if (nLen == sizeOutput) break; } while(*pStr++);
        // Re-adjust pointer to its initial location
        pStr -= nLen;
	}
	

	// If we have the buffer empty and the status ready then just send it to usb directly
//	if (ptrReply = 0 && mUSBUSARTIsTxTrfReady()){
//		putsUSBUSART(pStr);
//		return 0;
//	}
	// Otherwise use the output buffer
	if (nLen > sizeOutput - posReply - 1){
		StatusConsole.bufferOverrun = 1;
		return 1;
	}
	while (nLen > 0){
		bufOutput[posReply] = *pStr;
		pStr++;
		posReply++;
		nLen--;
	}
	bufOutput[posReply] = 0x00;
	return 0;
}


unsigned int isStrEqualConst(const char *sStr1, const char *sStr2, unsigned int nLen){
    unsigned int len1, len2, i, max;
    len1 = len2 = i = max = 0;

    // Get length, break loop if max len is reached.
    do{	len1++; if (len1 == 255) break; } while(*sStr1++);
	// Re-adjust pointer to its initial location
    sStr1 -= len1;                    
    // Break loop once max len is reached.
    do{ len2++; if (len2 == 255) break; } while(*sStr2++); 
    // Re-adjust pointer to its initial location
    sStr2 -= len2;                    
	if (nLen > 0){
		max = nLen;
	}
	else if (len1 != len2){
		return 0;
	}
	else{
		max = len1;
	}
	for (i = 0; i < max; i++){
		if (sStr1[i] != sStr2[i]) return 0;
	}
	return 1;
} //end isStrEqualConst

unsigned int isStrEqualVar(const char *sStr1, const char *sStr2, unsigned int nLen){
    unsigned int len1, len2, i, max;
    len1 = len2 = i = max = 0;

    // Get length, break loop if max len is reached.
    do{	len1++; if (len1 == 255) break; } while(*sStr1++); 
	// Re-adjust pointer to its initial location
    sStr1 -= len1;                    
    // Break loop once max len is reached.
    do{ len2++; if (len2 == 255) break; } while(*sStr2++); 
    // Re-adjust pointer to its initial location
    sStr2 -= len2;                    
	if (nLen > 0){
		max = nLen;
	}
	else if (len1 != len2){
		return 0;
	}
	else{
		max = len1;
	}
	for (i = 0; i < max; i++){
		if (sStr1[i] != sStr2[i]) return 0;
	}
	return 1;
} //end isStrEqualConst

unsigned int isStrLeftEqualConst(const char *sStr1, const char *sStr2){
    unsigned char len1, len2;
    len1 = len2 = 0;

    // Break loop once max len is reached.
    do{ len2++; if (len2 == 255) break; } while (*sStr2++);  
    // Re-adjust pointer to its initial location
    sStr2 -= len2;

    // Get length
    do{	len1++; if (len1 >= len2) break; } while (*sStr1++);   
    // Re-adjust pointer to its initial location
	sStr1 -= len1;      
	if (len1 < len2) return 0;
	if (len2 == 0)   return 0;
	do {if (sStr1[len2] != sStr2[len2]) return 0; len2--;} while (len2 > 0);
	return 1;
} //end isStrLeftEqualConst

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

void byte2binstr(const unsigned char iNum, char *sStr1){
	sStr1[8] = 0x00;
	sStr1[7] = (iNum & 1)   ? '1' : '0';
	sStr1[6] = (iNum & 2)   ? '1' : '0';
	sStr1[5] = (iNum & 4)   ? '1' : '0';
	sStr1[4] = (iNum & 8)   ? '1' : '0';
	sStr1[3] = (iNum & 16)  ? '1' : '0';
	sStr1[2] = (iNum & 32)  ? '1' : '0';
	sStr1[1] = (iNum & 64)  ? '1' : '0';
	sStr1[0] = (iNum & 128) ? '1' : '0';
} //end byte2binstr

void clock2string (char *sStr){
	unsigned long ms;
	unsigned char tick, len, i, h;

	tick = nMasterClockTick;
	ms   = nMasterClockMS;
	if (tick != nMasterClockTick){
		// This is to avoid interim changes as operations with "long" are not atomic
		ms   = nMasterClockMS;
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