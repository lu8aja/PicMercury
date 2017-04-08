#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_globals.h"
#include "lib_helpers.h"

#include "service_puncher.h"

Puncher_t Puncher = {0,0,0,0,0,0,0,0};

void Puncher_init(unsigned char bEnabled, unsigned char nMode){
    Puncher.State       = 0;
    Puncher.Tick        = 0;
    Puncher.TimePunch   = 30;
    Puncher.TimeGap1    = 15;
    Puncher.TimeAdvance = 60;
    Puncher.TimeGap2    = 5;
    
    Puncher.Output      = Transcoder_new(Puncher_sizeOutput);
    if (!Puncher.Output){
        System.Error.PuncherOutput = 1;
    }
    
    System.Buffers[2]   = Puncher.Output->Ring;
    
    Puncher.Output->Configs = nMode;
    
    PUNCHER_TRIS = 0; // All outputs
    PUNCHER_LAT  = 0;
    
    Puncher.Enabled      = bEnabled;
}

inline void Puncher_tick(void){
    if (Puncher.Tick){
        Puncher.Tick--;
    }
}

void Puncher_service(void){
    unsigned char cIn;
    unsigned char cOut;
    unsigned char bAdv;
    
    if (Puncher.Tick){
        return;
    }
    
    // 0 - Idle
    if (Puncher.State == 0){
        if (ring_is_empty(Puncher.Output->Ring)){
            return;
        }
        Puncher.State = 1;
    }
    
    PUNCHER_TRIS = 0;
    
    // 1 - Punch
    if (Puncher.State == 1){
         if (!Transcoder_read(Puncher.Output, &cOut)){
             // Nothing to punch, strange, this should not have happened!!
             // Back to idle
             Puncher.State = 0;
             return;
         }

        // Deal with physical outputs now, limit data bits and add guide            
        PUNCHER_LAT  = (cOut & TRANSCODER_MASK_DATA) | PUNCHER_BIT_GUIDE;

        // Debug
        //byte2binstr(sStr1, PUNCHER_LAT);
        //printf("> %s\r\n", sStr1);
        
        Puncher.State++;
        Puncher.Tick  = Puncher.TimePunch;
    }
    // 2 - Gap 1
    else if (Puncher.State == 2){
        PUNCHER_LAT  = 0;
        Puncher.State++;
        Puncher.Tick  = Puncher.TimeGap1;
    }
    // 3 - Advance
    else if (Puncher.State == 3){
        PUNCHER_LAT  = PUNCHER_BIT_ADVANCE;
        Puncher.State++;
        Puncher.Tick  = Puncher.TimeAdvance;
    }
    // 4 - Gap 2
    else if (Puncher.State == 4){
        PUNCHER_LAT  = 0;
        Puncher.State++;
        Puncher.Tick  = Puncher.TimeGap2;
    }
    // 5 - Done
    else if (Puncher.State == 5){
        Puncher.State = 0;
    }
    // Debug
    //byte2binstr(sStr1, PUNCHER_LAT);
    //printf("> %s\r\n", sStr1);

}



inline unsigned char Puncher_write(const unsigned char *pStr){
    if (!Puncher.Enabled){
        // Error: puncher disabled
        return 0;
    }
    
    return ring_appendEscaped(Puncher.Output->Ring, pStr);
}


inline unsigned char Puncher_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "cfg punch")){
        Puncher_cmd_cfg(idBuffer, pArgs);
        return 1;
    }
    if (strequal(pCommand, "punch")){
        Puncher_cmd(idBuffer, pArgs);
        return 1;
    }
    return 0;
}


void Puncher_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pVal  = NULL;
    unsigned char n = 0;
    unsigned char m = 0;

    sReply[0] = 0x00;

    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    
    if (strequal(pArg1, "mode")){
        pVal = &Puncher.Output->Configs;
    }
    else if (strequal(pArg1, "time.punch")){
        pVal = &Puncher.TimePunch;
    }
    else if (strequal(pArg1, "time.gap1")){
        pVal = &Puncher.TimeGap1;
    }
    else if (strequal(pArg1, "time.gap2")){
        pVal = &Puncher.TimeGap2;
    }
    else if (strequal(pArg1, "time.advance")){
        pVal = &Puncher.TimeAdvance;
    }

    if (pVal){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            *pVal = (unsigned char) atoi(pArg2);
        }
        sprintf(sReply, "%s = %u", pArg1, *pVal);
    }
    else{
        bOK = false;
        strcpy(sReply, txtErrorInvalidArgument);
    }
    
    printReply(idBuffer, bOK, "PUNCH", sReply);
}

void Puncher_cmd(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char n = 0;
    unsigned char m = 0;

    sReply[0] = 0x00;

    n = strlen(pArgs);
    if (!n){
        bOK = false;
        strcpy(sReply, txtErrorMissingArgument);
    }
    else if (n > ring_available(Puncher.Output->Ring)){
        bOK = false;
        strcpy(sReply, txtErrorTooBig);
    }
    else{
        Puncher.Enabled = 1;
        m = Puncher_write(pArgs);
        if (m){
            sprintf(sReply, "%u", m);
        }
        else{
            bOK = false;
            strcpy(sReply, txtErrorBusy);
        }
    }
    
    printReply(idBuffer, bOK, "PUNCH", sReply);
}