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

Puncher_t MasterPuncher = {0,0,0,0,0,0,0,0};

void Puncher_init(unsigned char bEnabled, unsigned char nMode){
    MasterPuncher.State       = 0;
    MasterPuncher.Tick        = 0;
    MasterPuncher.TimePunch   = 30;
    MasterPuncher.TimeGap1    = 15;
    MasterPuncher.TimeAdvance = 60;
    MasterPuncher.TimeGap2    = 5;
    
    MasterPuncher.Output       = Transcoder_new(PUNCHER_BUFFER_SIZE);
    MasterPuncher.Output->Configs = nMode;
    
    PUNCHER_TRIS = 0;
    
    MasterPuncher.Enabled      = bEnabled;
}

inline void Puncher_tick(void){
    if (MasterPuncher.Tick){
        MasterPuncher.Tick--;
    }
}

void Puncher_service(void){
    unsigned char cIn;
    unsigned char cOut;
    unsigned char bAdv;
    
    if (MasterPuncher.Tick){
        return;
    }
    
    // 0 - Idle
    if (MasterPuncher.State == 0){
        if (ring_is_empty(MasterPuncher.Output->Ring)){
            return;
        }
        MasterPuncher.State = 1;
    }
    
    PUNCHER_TRIS = 0;
    
    // 1 - Punch
    if (MasterPuncher.State == 1){
         if (!Transcoder_read(MasterPuncher.Output, &cOut)){
             // Nothing to punch, strange, this should not have happened!!
             // Back to idle
             MasterPuncher.State = 0;
             return;
         }

        // Deal with physical outputs now, limit data bits and add guide            
        PUNCHER_LAT  = (cOut & TRANSCODER_MASK_DATA) | PUNCHER_BIT_GUIDE;

        // Debug
        //byte2binstr(sStr1, PUNCHER_LAT);
        //printf("> %s\r\n", sStr1);
        
        MasterPuncher.State++;
        MasterPuncher.Tick  = MasterPuncher.TimePunch;
    }
    // 2 - Gap 1
    else if (MasterPuncher.State == 2){
        PUNCHER_LAT  = 0;
        MasterPuncher.State++;
        MasterPuncher.Tick  = MasterPuncher.TimeGap1;
    }
    // 3 - Advance
    else if (MasterPuncher.State == 3){
        PUNCHER_LAT  = PUNCHER_BIT_ADVANCE;
        MasterPuncher.State++;
        MasterPuncher.Tick  = MasterPuncher.TimeAdvance;
    }
    // 4 - Gap 2
    else if (MasterPuncher.State == 4){
        PUNCHER_LAT  = 0;
        MasterPuncher.State++;
        MasterPuncher.Tick  = MasterPuncher.TimeGap2;
    }
    // 5 - Done
    else if (MasterPuncher.State == 5){
        MasterPuncher.State = 0;
    }
}



inline unsigned char Puncher_write(unsigned char *pStr){
    if (!MasterPuncher.Enabled){
        // Error: puncher disabled
        return 0;
    }
    return ring_append(MasterPuncher.Output->Ring, pStr);
}


void Puncher_cmd(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pVal  = NULL;
    unsigned char n = 0;
    unsigned char m = 0;

    sReply[0] = 0x00;

    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    
    if (strequal(pArg1, "send")){
        n = strlen(pArg1 + 5);
        if (!n){
            bOK = false;
            strcpy(sReply, txtErrorInvalidArgument);
        }
        else if (n > ring_available(MasterPuncher.Output->Ring)){
            bOK = false;
            strcpy(sReply, txtErrorTooBig);
        }
        else{
            MasterPuncher.Enabled = 1;
            m = Puncher_write(pArg1 + 5);
            if (m){
                sprintf(sReply, "%u + %u + %u + %u = %u (%u)\r\n",
                    MasterPuncher.TimePunch,
                    MasterPuncher.TimeGap1,
                    MasterPuncher.TimeAdvance,
                    MasterPuncher.TimeGap2,
                    (MasterPuncher.TimePunch + MasterPuncher.TimeGap1 + MasterPuncher.TimeAdvance + MasterPuncher.TimeGap2),
                    n
                );
            }
            else{
                bOK = false;
                strcpy(sReply, txtErrorBusy);
            }
        }

    }
    else{
        if (strequal(pArg1, "mode")){
            pVal = &MasterPuncher.Output->Configs;
        }
        else if (strequal(pArg1, "time_punch")){
            pVal = &MasterPuncher.TimePunch;
        }
        else if (strequal(pArg1, "time_gap1")){
            pVal = &MasterPuncher.TimeGap1;
        }
        else if (strequal(pArg1, "time_gap2")){
            pVal = &MasterPuncher.TimeGap2;
        }
        else if (strequal(pArg1, "time_advance")){
            pVal = &MasterPuncher.TimeAdvance;
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
    }
    
    printReply(pBuffer, bOK, "PUNCH", sReply);
}