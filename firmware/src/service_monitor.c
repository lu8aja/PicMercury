#define LIB_MONITOR

#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>


#include "service_usb.h"

#include "app_globals.h"
#include "app_main.h"
#include "lib_helpers.h"
#include "app_io.h"


typedef struct {
    union {
        unsigned char Configs;
        struct {
            unsigned char Enabled:1;
            unsigned char A:1;
            unsigned char B:1;
            unsigned char C:1;
            unsigned char D:1;
            unsigned char E:1;
            unsigned char :1;
            unsigned char :1;
        } Config;
    };
    unsigned char Port[5];
    unsigned char Tris[5];
} monitor_t;

/*** Status of IO ***/

monitor_t MasterMonitor;

void Monitor_service(void);
void Monitor_checkPins(unsigned char cPortName);
void Monitor_cmd(Ring_t * pBuffer, unsigned char *pArgs);





void Monitor_service(void){
    // Notifications handling
    if (MasterMonitor.Config.Enabled
        && posCommand  == 0 
        && posOutput   == 0 
        && USB_isOutputAvailable()
    ){
        Monitor_checkPins(1);
        Monitor_checkPins(2);
        Monitor_checkPins(3);
        Monitor_checkPins(4);
        Monitor_checkPins(5);
    }
}


void Monitor_checkPins(unsigned char cPortName){
    unsigned char nPort;
    
    if (cPortName < 6){
        nPort = cPortName;
        cPortName += 'A' - 1;
    }
    else if (cPortName >= 'A' && cPortName <= 'E'){
        nPort = cPortName - 'B';  // c - A + 1 to be compatible with 1-based portnums
    }
    if (!nPort || nPort > 5 || bit_is_clear(MasterMonitor.Configs, nPort)){
        return;
    }
    
    nPort--; // Pointers are 0-based
    unsigned char *pPort = &PORTA + nPort;
    unsigned char *pTris = &TRISA + nPort;
    unsigned cPort       = MasterMonitor.Port[nPort];
    unsigned cTris       = MasterMonitor.Tris[nPort];
    
    if (*pPort != cPort || *pTris != cTris){
        Clock_getStr(sStr1, 0);
        byte2binstr(sStr2, cTris);
        byte2binstr(sStr3, *pTris);
        byte2binstr(sStr4, cPort);
        byte2binstr(sStr5, *pPort);
        
        sprintf(sReply, "%c %s TRIS %s > %s PORT %s > %s", cPortName, sStr1, sStr2, sStr3, sStr4, sStr5);

        printReply(0, 3, "MONITOR", sReply);
        
        MasterMonitor.Port[nPort] = *pPort;
        MasterMonitor.Tris[nPort] = *pTris;
    }
}

inline unsigned char Monitor_checkCmd(Ring_t * pBuffer, unsigned char pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "monitor")){
        Monitor_cmd(pBuffer, pArgs);
        return 1;
    }
    return 0;
}

void Monitor_cmd(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pS = sReply;
    unsigned char nPort;
    
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    sReply[0] = 0x00;
    
    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        nPort = 'A';
        
        *pS++ = nPort;
        *pS++ = ':';
        *pS++ = MasterMonitor.Config.A + 48;
        *pS++ = ' ';
        *pS++ = '/';
        
        nPort++;
        *pS++ = ' ';
        *pS++ = nPort;
        *pS++ = ':';
        *pS++ = MasterMonitor.Config.B + 48;
        *pS++ = ' ';
        *pS++ = '/';

        nPort++;
        *pS++ = ' ';
        *pS++ = nPort;
        *pS++ = ':';
        *pS++ = MasterMonitor.Config.C + 48;
        *pS++ = ' ';
        *pS++ = '/';

        nPort++;
        *pS++ = ' ';
        *pS++ = nPort;
        *pS++ = ':';
        *pS++ = MasterMonitor.Config.D + 48;
        *pS++ = ' ';
        *pS++ = '/';

        nPort++;
        *pS++ = ' ';
        *pS++ = nPort;
        *pS++ = ':';
        *pS++ = MasterMonitor.Config.E + 48;
        *pS++ = 0;
    }
    else if (strlen(pArg1) == 1){
        nPort = pArg1[0];
        if (nPort >= 'A' && nPort <= 'E'){
            strcpy(sReply, pArg1);
            strcat(sReply, txtSpc);

            nPort = nPort - 'B';  // c - A + 1 to be compatible with 1-based portnums
        
            if (!pArg2){
                bit_flip(MasterMonitor.Configs, nPort);
            }
            else {
                bit_write(MasterMonitor.Configs, nPort, strequal(pArg2, "on"));
            }
            
            MasterMonitor.Config.Enabled = 1;
            strcat(sReply, bit_is_set(MasterMonitor.Configs, nPort) ? txtOn : txtOff);
        }
        else {
            bOK = false;
            strcpy(sReply, txtErrorUnknownArgument);
        }
    }
    else {
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
    }

    printReply(pBuffer, bOK, "MONITOR", sReply);
}