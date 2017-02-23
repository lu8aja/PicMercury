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
void APP_CMD_ping(unsigned char *pArgs);
void APP_CMD_uptime(unsigned char *pArgs);
void APP_CMD_debug(unsigned char *pArgs);
void APP_CMD_tone(unsigned char *pArgs);
void APP_CMD_led(unsigned char *pArgs);
void APP_CMD_monitor(unsigned char *pArgs);
void APP_CMD_read(unsigned char *pArgs);
void APP_CMD_write(unsigned char *pArgs);




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


