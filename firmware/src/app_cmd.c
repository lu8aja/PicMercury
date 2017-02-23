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
#include "app_io.h"
#include "app_helpers.h"
#include "app_music.h"
#include "app_midi.h"
#include "app_programs.h"

#include "system_config.h"

// APP Commands
void APP_CMD_ping(unsigned char *pArgs);
void APP_CMD_uptime(unsigned char *pArgs);
void APP_CMD_debug(unsigned char *pArgs);
void APP_CMD_tone(unsigned char *pArgs);
void APP_CMD_led(unsigned char *pArgs);
void APP_CMD_monitor(unsigned char *pArgs);
void APP_CMD_read(unsigned char *pArgs);
void APP_CMD_write(unsigned char *pArgs);
void APP_CMD_keys(unsigned char *pArgs);
void APP_CMD_run(unsigned char *pArgs);
void APP_CMD_var(unsigned char *pArgs);
void APP_CMD_mem(unsigned char *pArgs);
void APP_CMD_dump(unsigned char *pArgs);


void APP_CMD_ping(unsigned char *pArgs){
    printReply(1, "PONG", pArgs);
}

void APP_CMD_uptime(unsigned char *pArgs){
    clock2str(sStr1, 0);
    printReply(1, "UPTIME", sStr1);
}
        
void APP_CMD_debug(unsigned char *pArgs){
    bool bOK = true;
    sReply[0] = 0x00;
    
    str2lower(pArgs);   
        
    if (strequal(pArgs, "on") || strequal(pArgs, "1")){
        MasterDebug = 1;
    }
    else if (strequal(pArgs, "off") || strequal(pArgs, "0")){
        MasterDebug = 0;
    }
    else if (strlen(pArgs)){
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
        strcat(sReply, ". Now: ");
    }

    strcat(sReply, MasterDebug > 0 ? txtOn : txtOff);

    printReply(1, "DEBUG", sReply);
}

void APP_CMD_tone(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned int nNote   = 0;
    unsigned int iFreq   = 0;
    unsigned int iPeriod = 0;
    unsigned int iArg2   = 0;
    
    bool bShowStatus = false;

    sReply[0] = 0x00;
    
    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);
    
    if (pArg2){
        iArg2 = atoi(pArg2);
    }

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
            MasterToneMode = (strequal(pArg2, "on") || strequal(pArg2, "music")) ? 1 : 0;
        }
        sprintf(sReply, "%s %s", pArg1, MasterToneMode ? "Music" : "Single");
    }
    else if (strequal(pArg1, "time")){
        if (pArg2){
            MasterToneTime = iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterToneTime);
    }
    else if (strequal(pArg1, "tempo")){
        if (pArg2){
            MasterToneTempo = iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterToneTempo);
    }
    else if (strequal(pArg1, "pitch")){
        if (pArg2){
            MasterTonePitch = (signed char) iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterTonePitch);
    }
   else if (strequal(pArg1, "period")){
        if (pArg2){
            MasterTonePeriod = (unsigned char) iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterTonePeriod);
    }
    else if (strspn(pArg1, txtNum) == strlen(pArg1)){
        iFreq = atoi(pArg1);
    }
    else if (strequal(pArg1, "freq")){
        if (pArg2){
            iFreq = iArg2;
        }
        else{
            bOK = false;
            strcat(sReply, txtErrorMissingArgument);    
        }
    }
    else if (strequal(pArg1, "midi")){
        // Get frequency from MIDI note number
        if (pArg2){
            nNote = iArg2;
            if (nNote < 24 || nNote > 95){
                bOK = false;
                strcat(sReply, txtErrorInvalidArgument);    
            }
            else{
                iPeriod = MasterToneMidiTable[nNote - 24];
            }
        }
        else{
            bOK = false;
            strcat(sReply, txtErrorMissingArgument);    
        }
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

    if (bShowStatus){
        if (MasterToneMode){
            sprintf(sStr5, " - Music %s #%u n: %d t: %d", 
                MasterToneEnabled ? txtOn : txtOff,
                MasterToneStep,
                MasterToneMusic[MasterToneStep].note,
                MasterToneMusic[MasterToneStep].time
            );
        }
        else{
            sprintf(sStr5, " - Single %s T: %u t: %u",
                MasterToneEnabled ? txtOn : txtOff,
                MasterTonePeriod,
                MasterToneTime
            );
        }
    }
    strcat(sReply, sStr5);
    printReply(bOK, "TONE", sReply);
}

void APP_CMD_led(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;

    unsigned char nLed    = 0;
    unsigned int  iBit    = 0;
    
    sReply[0] = 0x00;
    
    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (strlen(pArg1) == 1 && !pArg2){
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        sprintf(sReply, "%d %s", nLed, bit_is_set(MasterLedStatus, nLed) ? txtOn : txtOff);
    }
    else if(strlen(pArg1) == 1) {
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        if (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0){
            setbit(MasterLedStatus, nLed);
            iBit = 1;
        }
        else{
            clearbit(MasterLedStatus, nLed);
            iBit = 0;
        }
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
    
    strcat(sReply, " Status: ");
    int2binstr(sStr1, MasterLedStatus);
    strcat(sReply, sStr1);
    
    printReply(bOK, "LED", sReply);
}

void APP_CMD_monitor(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pMonitor = NULL;
    
    sReply[0] = 0x00;
    
    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
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

    printReply(bOK, "MONITOR", sReply);
}

void APP_CMD_read(unsigned char *pArgs){
    bool bOK = true;
    unsigned char nBit = 255; // Double purpose, bit number for single pin or TRIS value for whole port
    unsigned char nMax = '7';
    volatile unsigned char *pPort;
    volatile unsigned char *pTris;
    
    sReply[0] = 0x00;
    
    str2upper(pArgs);
    
    switch (pArgs[0]){
        case 'A':
            // RA6 OSC2 / RA7 n/a
            pPort = &PORTA; pTris = &TRISA; nMax = '5'; nBit = 0b00111111; break;
        case 'B':
            pPort = &PORTB; pTris = &TRISB; break;
        case 'C':
            // RC3 n/a / RC4 D- ReadOnly / RC5 D+ ReadOnly / RC6 TX UART / RC7 RX UART
            pPort = &PORTC; pTris = &TRISC; nMax = '2'; nBit = 0b10000111; break;
        case 'D':
            pPort = &PORTD; pTris = &TRISD; break;
        case 'E':
            // RE3..7 Does not exist
            pPort = &PORTE; pTris = &TRISE; nMax = '2'; nBit = 0b00000111;  break;
        default: 
            bOK = false;
            strcat(sReply, txtErrorUnknownArgument);
            break;
    }
    
    if (bOK){
        if (pArgs[1]){
            // Single pin
            if (pArgs[1] >= '0' && pArgs[1] <= nMax){
                nBit = pArgs[1] - 48;
                setbit(*pTris, nBit);
                
                sReply[0] = pArgs[0];
                sReply[1] = pArgs[1];
                sReply[2] = '=';
                sReply[3] = readbit(*pPort, nBit) + 48;
                sReply[4] = 0;
            }
            else {
                bOK = false;
                strcat(sReply, txtErrorUnknownPin);
            }
        }
        else{
            // Whole port
            *pTris = nBit;
            
            sReply[0] = pArgs[0];
            sReply[1] = '=';
            byte2binstr(&sReply[2], *pPort);

        }
    }
    
    printReply(bOK, "READ", sReply);
}

void APP_CMD_write(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;

    unsigned char nBit = 255; // Double purpose, bit number for single pin or TRIS value for whole port
    unsigned char nMax = '7';
    volatile unsigned char *pPort;
    volatile unsigned char *pTris;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1 || !pArg2){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }

    if (bOK){
        switch (pArg1[0]){
            case 'A':
                // RA6 OSC2 / RA7 n/a
                pPort = &LATA; pTris = &TRISA; nMax = '5'; break;
            case 'B':
                pPort = &LATB; pTris = &TRISB; break;
            case 'C':
                // RC3 n/a / RC4 D- ReadOnly / RC5 D+ ReadOnly / RC6 TX UART / RC7 RX UART
                pPort = &LATC; pTris = &TRISC; nMax = '2'; break;
            case 'D':
                pPort = &LATD; pTris = &TRISD; break;
            case 'E':
                // RE3..7 Does not exist
                pPort = &LATE; pTris = &TRISE; nMax = '2'; break;
            default: 
                bOK = false;
                strcat(sReply, txtErrorUnknownArgument);
                break;
        }
    }

    if (bOK){
        if (pArg1 && pArg1[1]){
            // Single pin
            if (pArgs[1] >= '0' && pArgs[1] <= nMax){
                nBit = pArgs[1] - 48;
                clearbit(*pTris, nBit);
                
                sReply[0] = pArg1[0];
                sReply[1] = pArg1[1];
                sReply[2] = '=';
                sReply[4] = 0;

                if (strequal(pArg2, "ON") || strequal(pArg2, "1")){
                    setbit(*pPort, nBit);
                    sReply[3] = '1';
                }
                else{
                    clearbit(*pPort, nBit);
                    sReply[3] = '0';
                }
            }
            else {
                bOK = false;
                strcat(sReply, txtErrorUnknownPin);
            }
        }
        else{
            // Whole port
            *pTris = 0x00;
            *pPort = (unsigned char) atoi(pArg2);

            
            sReply[0] = pArg1[0];
            sReply[1] = '=';
            byte2binstr(&sReply[2], *pPort);
        }
    }
    
    printReply(bOK, "WRITE", sReply);
}

void APP_CMD_keys(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char n = 0;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    APP_getKeys();
    
    /*
    int2binstr(sStr1, MasterKeysFunction);
    int2binstr(sStr2, MasterKeysAddress);
    int2binstr(sStr3, MasterKeysInput);
    int2binstr(sStr4, MasterKeysSwitches);
    
    sStr5[0] = MasterButtons[0].status + 48;
    sStr5[1] = MasterButtons[1].status + 48;
    sStr5[2] = MasterButtons[2].status + 48;
    sStr5[3] = 0;

    sprintf(sReply, "FUNC: %s ADDR: %s INPUT: %s SWITCHES: %s BTN: %s",
        sStr1, sStr2, sStr3, sStr4, sStr5
    );
     */
    APP_getStatusReply();    

    printReply(bOK, "KEYS", sReply);
}

void APP_CMD_run(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char n = 0;
    unsigned char s = 0;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        n = atoi(pArg1);
        if (n >= MasterProgramsLen){
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);
        }
        else{
            if (bOK){
                MasterProgramTick    = 0;
                MasterProgramStep    = 0;
                MasterProgramRun     = n;
                MasterProgramEnabled = 2;
            }
        }
    }

    printReply(bOK, "RUN", sReply);
}

void APP_CMD_var(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned char *pChar = NULL;
    unsigned int  *pInt  = NULL;
    unsigned long *pLong = NULL;

    unsigned int num = 0;
    unsigned long val = 0;
    unsigned char var = 0;

    sReply[0] = 0x00;

    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        num = atoi(pArg1);
        switch (num){
                 /*** MASTER DEBUG ***/
            case 0: 
                pChar = &MasterDebug; break;
            case 1: 
                /*** MASTER CLOCK */
                var    = MasterClockTickCount; break;
            case 2: 
                pChar  = &MasterClockTick; break;
            case 3: 
                pLong  = &MasterClockMS; break;
                /*** MASTER NOTIFY ***/
            case 10: 
                pInt   = &MasterNotifyCounter; break;
            case 11: 
                pInt   = &MasterNotify; break;
            case 12: 
                pLong  = &MasterNotifyNow; break;

                /*** MASTER LEDS ***/
                // Configs
            case 20: 
                var    = MasterLedTime; break;
            case 21: 
                pChar  = &MasterLedStepEnabled; break;
            case 22: 
                pChar  = &MasterLedStepRestart; break;
            case 23: 
                pInt   = &MasterLedStepTime; break;
                // Runtime
            case 30: 
                pInt   = &MasterLedStatus; break;
            case 31: 
                pInt   = &MasterLedCounter; break;
            case 32: 
                pInt   = &MasterLedStepTick; break;
            case 33: 
                pChar  = &MasterLedStep; break;
                /*** MASTER TONES ***/
                // Configs
            case 40: 
                pChar  = &MasterToneEnabled; break;
            case 41: 
                pChar  = &MasterToneMode; break;
            case 42: 
                pInt   = &MasterToneTempo; break;
            case 43: 
                pChar  = &MasterTonePitch; break;
            case 44: 
                pChar  = &MasterToneRestart; break;
            case 45: 
                pChar  = &MasterTonePeriod; break;
            case 46: 
                pInt   = &MasterToneTime; break;
                // Runtime
            case 60: 
                pChar  = &MasterToneTick; break;
            case 61: 
                pInt   = &MasterToneCounter; break;
            case 62: 
                pChar  = &MasterToneStep; break;
                /*** MASTER KEYS ***/
            case 70: 
                pInt   = &MasterKeysFunction; break;
            case 71: 
                pInt   = &MasterKeysAddress; break;
            case 72: 
                pInt   = &MasterKeysInput; break;
            case 73: 
                pInt   = &MasterKeysSwitches; break;
                /*** MASTER BUTTONS ***/
            case 80: 
                pInt   = &MasterButtonsTick; break;
            case 81: 
                pInt   = &MasterButtonsBit; break;
            case 82: 
                var    = MasterButtonsTime; break;
            case 83: 
                pInt   = &MasterButtonsRunEnabled; break;

                /*** MASTER PROGRAM ***/
                // Configs
            case 90: 
                pLong  = &MasterProgramTime; break;
            case 91: 
                pInt   = &MasterProgramRun; break;
            case 92: 
                pInt   = &MasterProgramEnabled; break;
                // Runtime
            case 93: 
                pLong  = &MasterProgramTick; break;
            case 94: 
                pInt   = &MasterProgramStep; break;
                
                /*** PORTS ***/  
            case 100: 
                pChar  = &PORTA; break;
            case 101: 
                pChar  = &PORTB; break;
            case 102: 
                pChar  = &PORTC; break;
            case 103: 
                pChar  = &PORTD; break;
            case 104: 
                pChar  = &PORTE; break;
            case 110: 
                pChar  = &TRISA; break;
            case 111: 
                pChar  = &TRISB; break;
            case 112: 
                pChar  = &TRISC; break;
            case 113: 
                pChar  = &TRISD; break;
            case 114: 
                pChar  = &TRISE; break;
            case 120: 
                pChar  = &LATA; break;
            case 121: 
                pChar  = &LATB; break;
            case 122: 
                pChar  = &LATC; break;
            case 123: 
                pChar  = &LATD; break;
            case 124: 
                pChar  = &LATE; break;
                
                
            case 200: 
                pChar  = &ADCON0; break;
            case 201: 
                pChar  = &ADCON1; break;
            case 202: 
                pChar  = &ADCON2; break;
            case 210: 
                pChar  = &TXSTA; break;
            case 211: 
                pChar  = &RCSTA; break;
            case 212: 
                pChar  = &TXREG; break;
            case 213: 
                pChar  = &RCREG; break;
            case 214: 
                pChar  = &BAUDCON; break;
            case 215: 
                pChar  = &SPBRGH; break;
            case 216: 
                pChar  = &SPBRG; break;
            default:
                if (num >= 0x0F60 && num <= 0x0FFF){
                    // Special registers
                    pChar = num;
                }
                else{
                    bOK = false;
                    strcat(sReply, txtErrorInvalidArgument);

                }
        }
        
        if (bOK){
            if (pArg2){
                val = atoi(pArg2);
                if (pChar){
                    *pChar = val;
                }
                else if (pInt){
                    *pInt = val;
                }
                else if (pLong){
                    *pLong = val;
                }
                else {
                    bOK = false;
                    // No need for explanation, as the reply will show that it is a constant
                }
            }
            

            if (pChar){
                byte2binstr(sStr1, *pChar);
                sprintf(sReply, "%u Char %3u %2x %s", num, *pChar, *pChar, sStr1);
            }
            else if (pInt){
                int2binstr(sStr1, *pInt);
                sprintf(sReply, "%u Int %5u %4u %s", num, *pInt, *pInt, sStr1);
            }
            else if (pLong){
                any2binstr(sStr5, *pLong, 32);
                *pLong = val;
                sprintf(sReply, "%u Long %lu %s", num, *pLong, sStr5);
            }
            else {
                sprintf(sReply, "%u Const %u (readonly)", num, var);
            }
        }
    }
    printReply(bOK, "VAR", sReply);
}


void APP_CMD_mem(unsigned char *pArgs){
    /*
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned char *pChar = NULL;

    sReply[0] = 0x00;

    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        pChar = atoi(pArg1);
        if (pArg2){
            *pChar = atoi(pArg2);
        }
        byte2binstr(sStr1, *pChar);
        sprintf(sReply, "%4x = %2x %3u %s", pChar, *pChar, *pChar, sStr1);
    }
        
    printReply(bOK, "MEM", sReply);
     * */
}

/*
void APP_CMD_dumptest(unsigned char *pArgs){
    unsigned char n = 0;
    unsigned char m = 0;
    for (m = 'A'; m < 'Z'; m++){
        putch('\r');
        putch('\n');
        putch(m);
        putch(' ');
        for (n = 'a'; n <= 'z'; n++){
            putch(n);
        }
    }
    putch('\r');
    putch('\n');
    printf(" %u\r\n", posOutput);

}
*/

void APP_CMD_dump(unsigned char *pArgs){
    /*
    bool bOK = true;
    unsigned char *pChar = 0x0f60;
    unsigned char *pMax  = 0x1000;
    
    sReply[0] = 0x00;

    while(pChar < pMax){
        printf("\t%04x =", pChar);
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        print(txtCrLf);
        
        // Flush often to avoid buffer overrun
        APP_outputUsb();
    }
        
    printReply(bOK, "DUMP", 0);
    */
}