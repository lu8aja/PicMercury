#define LIB_MONITOR

#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "usb.h"
#include "usb_device_cdc.h"

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"


/*** Status of IO ***/
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



void Monitor_service(void);
void Monitor_checkPins(unsigned char cPortName);
void Monitor_cmd(unsigned char *pArgs);





void Monitor_service(void){
    // Notifications handling
    if (posCommand  == 0 
        && posOutput == 0 
        && USBGetDeviceState()     == CONFIGURED_STATE
        && USBIsDeviceSuspended()  == false
        && mUSBUSARTIsTxTrfReady() == true
    ){
        Monitor_checkPins('A');
        Monitor_checkPins('B');
        Monitor_checkPins('C');
        Monitor_checkPins('D');
        Monitor_checkPins('E');
    }
}


void Monitor_checkPins(unsigned char cPortName){
    unsigned char *pMonitor;
    volatile unsigned char *pPort;
    volatile unsigned char *pTris;
    unsigned char *pStatusTris;
    unsigned char *pStatusPort;
    
    switch (cPortName){
        case 'a': 
        case 'A': 
            pPort       = &PORTA; 
            pTris       = &TRISA; 
            pMonitor    = &nStatus_MonitorA; 
            pStatusTris = &nStatus_TrisA; 
            pStatusPort = &nStatus_PortA; 
            break;
        case 'b': 
        case 'B': 
            pMonitor    = &nStatus_MonitorB;
            pPort       = &PORTB;
            pTris       = &TRISB;
            pStatusTris = &nStatus_TrisB;
            pStatusPort = &nStatus_PortB;
            break;
        case 'c': 
        case 'C':
            pMonitor    = &nStatus_MonitorC;
            pPort       = &PORTC;
            pTris       = &TRISC;
            pStatusTris = &nStatus_TrisC;
            pStatusPort = &nStatus_PortC;
            break;
        case 'd': 
        case 'D':
            pMonitor    = &nStatus_MonitorD;
            pPort       = &PORTD;
            pTris       = &TRISD;
            pStatusTris = &nStatus_TrisD;
            pStatusPort = &nStatus_PortD;
            break;
        case 'e': 
        case 'E':
            pMonitor    = &nStatus_MonitorE;
            pPort       = &PORTE;
            pTris       = &TRISE;
            pStatusTris = &nStatus_TrisE;
            pStatusPort = &nStatus_PortE;
            break;
        default: return;
    }
    
    if (*pMonitor == 0){
        return;
    }
    
    if (*pPort != *pStatusPort || *pTris != *pStatusTris){
        clock2str(sStr1, 0);
        byte2binstr(sStr2, *pStatusTris);
        byte2binstr(sStr3, *pTris);
        byte2binstr(sStr4, *pStatusPort);
        byte2binstr(sStr5, *pPort);
        
        sprintf(sReply, "%c %s TRIS %s > %s PORT %s > %s", cPortName, sStr1, sStr2, sStr3, sStr4, sStr5);

        printReply(3, "MONITOR", sReply);
        
        *pStatusPort = *pPort;
        *pStatusTris = *pTris;
    }
}


void Monitor_cmd(unsigned char *pArgs){
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