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

#include "app_main.h"
#include "app_helpers.h"
#include "app_music.h"
#include "app_midi.h"

#include "system_config.h"

// APP Commands
void APP_CMD_ping(unsigned char *ptrArgs);
void APP_CMD_uptime(unsigned char *ptrArgs);
void APP_CMD_debug(unsigned char *ptrArgs);
void APP_CMD_tone(unsigned char *ptrArgs);
void APP_CMD_led(unsigned char *ptrArgs);
void APP_CMD_monitor(unsigned char *ptrArgs);
void APP_CMD_read(unsigned char *ptrArgs);
void APP_CMD_write(unsigned char *ptrArgs);



void APP_CMD_ping(unsigned char *ptrArgs){
    print(txtOkSp);
    print("PONG: ");
    print(ptrArgs);
    print(txtCrLf);
}

void APP_CMD_uptime(unsigned char *ptrArgs){
    print(txtOkSp);
    print("UPTIME: ");
    clock2str(sReply, 0);
    print(sReply);
    print(txtCrLf);
}
        
void APP_CMD_debug(unsigned char *ptrArgs){
    str2lower(ptrArgs);   
        
    print(txtOkSp);
    print("DEBUG: ");
    if (strcmp(ptrArgs, "on") == 0 || strcmp(ptrArgs, "1") == 0){
        MasterDebug = 1;
        print(txtOn);
    }
    else if (strcmp(ptrArgs, "off") == 0 || strcmp(ptrArgs, "0") == 0){
        MasterDebug = 0;
        print(txtOff);
    }
    else if (strlen(ptrArgs) == 0){
        print(MasterDebug > 0 ? txtOn : txtOff);
    }
    else{
        print(txtErrorUnknownArgument);
        print(". Currently: ");
        print(MasterDebug > 0 ? txtOn : txtOff);
    }
    print(txtCrLf);
}

void APP_CMD_tone(unsigned char *ptrArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned int nNote   = 0;
    unsigned int iFreq   = 0;
    unsigned int iPeriod = 0;
    
    sReply[0] = 0x00;
    
    bool bShowStatus = false;
    
    str2lower(ptrArgs);
    
    pArg1 = strtok(ptrArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (pArg1 == NULL){
        bShowStatus = true;
    }
    else if (strequal(pArg1, "on")){
        MasterToneEnabled = MasterToneMode ? 2 : 1;
        strcpy(sReply, txtOn);
    }
    else if (strequal(pArg1, "off")){
        MasterToneEnabled = 0;
        strcpy(sReply, txtOff);
    }
    else if (strequal(pArg1, "restart")){
        if (pArg2){
            MasterToneRestart = strequal(pArg2, "on") ? 1 : 0;
        }
        sprintf(sReply, "%s %s", pArg1, MasterToneRestart ? txtOn : txtOff);
    }
    else if (strequal(pArg1, "mode")){
        if (pArg2){
            MasterToneMode = strequal(pArg2, "on") ? 1 : 0;
        }
        sprintf(sReply, "%s %s", pArg1, MasterToneMode ? txtOn : txtOff);
    }
    else if (strequal(pArg1, "period")){
        MasterTonePeriod = (unsigned char) atoi(pArg2);
        sprintf(sReply, "%s %u", pArg1, MasterTonePeriod);
    }
    else if (strspn(pArg1, txtNum) == strlen(pArg1)){
        iFreq = atoi(pArg1);
    }
    else if (strequal(pArg1, "freq")){
        iFreq = atoi(pArg2);
    }
    else if (strequal(pArg1, "midi")){
        // Get frequency from MIDI note number
        nNote = atoi(pArg2);
        if (nNote < 24 || nNote > 95){
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);    
        }
        else{
            iPeriod = MasterToneMidiTable[nNote - 24];
        }
    }
    else if (strequal(pArg1, "time")){
        MasterToneTime = (unsigned int) atoi(pArg2);
        sprintf(sReply, "%s %u", pArg1, MasterToneTime);
    }
    else if (strequal(pArg1, "tempo")){
        MasterToneTempo = (unsigned int) atoi(pArg2);
        sprintf(sReply, "%s %u", pArg1, MasterToneTempo);
    }
    else if (strequal(pArg1, "pitch")){
        MasterTonePitch = (unsigned int) atoi(pArg2);
        sprintf(sReply, "%s %u", pArg1, MasterTonePitch);
    }
    else{
        bOK = false;
        sprintf(sReply, "%s. Now: %s", txtErrorUnknownArgument, MasterToneEnabled > 0 ? txtOn : txtOff);
    }

    if (bOK && !iPeriod && iFreq){
        // Calculate period from frequency
        if (iFreq < 25 || iFreq > 4000){
            iPeriod = 0;
        }
        else {
            iPeriod = (unsigned int) MasterClockTickCount * 500 / iFreq;
        }
        if (iPeriod < 1 || iPeriod > 255){
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);    
        }
    }
    
    if (bOK && iPeriod){
        // Assign period
        MasterTonePeriod  = (unsigned char) iPeriod;
        MasterToneTime    = 10000;
        MasterToneRestart = 1;
        MasterToneMode    = 0;
        MasterToneEnabled = 1;
        bShowStatus       = true;
    }

    if (bOK) print(txtOkSp);  else  print(txtErrorSp); 
    print("TONE: ");
    print(sReply);
    if (bShowStatus){
        if (MasterToneMode){
            printf("%s %s Step: %d n: %d t: %d", 
                "Music",
                MasterToneEnabled ? txtOn : txtOff,
                MasterToneStep,
                MasterToneMusic[MasterToneStep].note,
                MasterToneMusic[MasterToneStep].time
            );
        }
        else{
            printf("%s %s T: %d t: %d",
                "Single",
                MasterToneEnabled ? txtOn : txtOff,
                MasterTonePeriod,
                MasterToneTime
            );
        }
    }
    print(txtCrLf);
}

void APP_CMD_led(unsigned char *ptrArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;

    unsigned char nLed    = 0;
    unsigned int  iBit    = 1;
    unsigned int  iMask   = 1; // It is later inverted
    
    sReply[0] = 0x00;
    
    str2lower(ptrArgs);
    
    pArg1 = strtok(ptrArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (strlen(pArg1) == 1 && !pArg2){
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        sprintf(sReply, "%d %s", nLed, bit_is_set(MasterLedStatus, nLed) ? txtOn : txtOff);
    }
    else if(strlen(pArg1) == 1) {
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        if (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0){
            setbit(MasterLedStatus, nLed);
        }
        else{
            clearbit(MasterLedStatus, nLed);
        }
        /*
        printf("N=%x Bit=%x Mask=%x\r\n", nLed, iBit, iMask);
        if (nLed){
            iBit  = iBit  << nLed;
            iMask = iMask << nLed;
        }
        iMask = ~iMask;
        printf("N=%x Bit=%x Mask=%x\r\n", nLed, iBit, iMask);
        
        MasterLedStatus = (MasterLedStatus & iMask) | iBit;
         * */
        sprintf(sReply, "%d %s", nLed, iBit ? txtOn : txtOff);
    }
    else if (strlen(pArg1) == 0){
        // Do nothing, just print info
    }
    else if (strcmp(pArg1, "steps") == 0){
        MasterLedStepTick    = 0;
        MasterLedStepEnabled = (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0) ? 2 : 0;
        sprintf(sReply, "Steps: %s", MasterLedStepEnabled ? txtOn : txtOff);
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


