#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_globals.h"
#include "app_helpers.h"

#define LIB_PUNCHER

#define PUNCHER_TRIS TRISD        // Port tris
#define PUNCHER_LAT  LATD         // Port latch
#define PUNCHER_BIT_ADVANCE  0b10000000 // Bitmask for advancing the paper
#define PUNCHER_BIT_GUIDE    0b01000000 // Bitmask for the guide hole
#define PUNCHER_MASK_DATA    0b00011111 // Data mask
#define PUNCHER_MASK_SHIFT   0b00100000 // Shift mask (used for mode 1))
#define PUNCHER_MASK_NONZERO 0b10000000 // Used as flag in conversions

#define PUNCHER_ITA_LTRS     0b00011111 // 
#define PUNCHER_ITA_FIGS     0b00011011 // 
#define PUNCHER_ITA_NULL     0b00000000 // 

#define PUNCHER_BUFFER_SIZE 16

//                                  00 000000 0000000011111111111   1111   1   22 222 222 222 222223 3333333333   3333   3
//                                  01 234567 89abcdef0123456789a   bcde   f   01 234 567 89a bcdef0 123456789a   bcde   f  
const unsigned char txtITA2[] = "\x00E\nA SIU\rDRJNFCKTZLWHYPQOBG\x0fMXV\x0e\x003\n- \'87\r$4\a,!:(5\")2#6019?&\x0f./;\x0e";


unsigned char Puncher_Buffer[PUNCHER_BUFFER_SIZE];


typedef struct {
    unsigned char enabled;     // 0 = Off / 1 = On
    unsigned char mode;        // Mode Bitfield Bit0 = 0:Direct / 1:shifted | Bit1 = 1:ASCII | Bit2 = 4:HEX
    unsigned char shift;       // Mode 1 Shift: 0 = LTRS / 1 = FIGS
    unsigned char state;       // State Machine: 0 = Off / 1 = 
    unsigned char time_punch;  // Time in ms to punch the holes
    unsigned char time_gap1;   // Time in ms between punch and advance
    unsigned char time_advance;// Time in ms to advance the paper
    unsigned char time_gap2;   // Time in ms to rest before next command
    
    unsigned char tick;        // Time ticker in ms
    unsigned char *output;     // Output buffer (must be binary safe))
    unsigned char len;         // Output buffer counter n..0
} puncher_t;

puncher_t MasterPuncher = {0,0,0,0,0,0,0,0,0,0,0};

void Puncher_init(unsigned char bEnabled, unsigned char nMode){
    MasterPuncher.tick         = 0;
    MasterPuncher.time_punch   = 30;
    MasterPuncher.time_gap1    = 15;
    MasterPuncher.time_advance = 60;
    MasterPuncher.time_gap2    = 5;
    MasterPuncher.mode         = nMode;
    MasterPuncher.shift        = 0;
    MasterPuncher.state        = 0;
    MasterPuncher.enabled      = bEnabled;
}

inline void Puncher_tick(void){
    if (MasterPuncher.tick){
        MasterPuncher.tick--;
    }
}

void Puncher_service(void){
    unsigned char cIn;
    unsigned char cOut;
    unsigned char bAdv;
    
    if (MasterPuncher.state && !MasterPuncher.tick){
        if (MasterPuncher.state == 1){
            if (!MasterPuncher.len){
                // Error: Trying to punch an empty buffer
                MasterPuncher.state = 254;
                return;
            }
            MasterPuncher.state = 2;
        }

        if (MasterPuncher.state == 2){
            bAdv = 1;
            
            // Mode4 = HEX
            if (MasterPuncher.mode & 0x04){
                // With HEX conversion
                cOut = hexstr2byte(MasterPuncher.output);
                // In this mode we need 2 advances instead of just one
                MasterPuncher.len--;
                MasterPuncher.output++;
            }
            else{
                // Direct
                cOut = *MasterPuncher.output;
            }
            
            // Mode2/3 = ASCII
            if (MasterPuncher.mode & 2){
                // With ASCII -> ITA2+shift conversion
                cIn = toupper(cOut);
                // Find the corresponding ITA2 for the given ASCII
                for (cOut = 0; cOut < 66; cOut++){
                    if (cOut == 65){
                        // Not found
                        cOut = MasterPuncher.shift ? PUNCHER_MASK_SHIFT : 0;
                        break;
                    }
                    // The search order is determined by the current shift
                    if (MasterPuncher.shift){
                        if (txtITA2[64 - cOut] == cIn){
                            cOut = 64 - cOut;
                            break;
                        }
                    }
                    else{
                        if (txtITA2[cOut] == cIn){
                            break;
                        }
                    }
                }
                cOut |= PUNCHER_MASK_NONZERO; // Add flag to avoid Mode0
            }
            
            // Mode1 = ITA2+shift -> ITA2
            // Shift calc (used for both Mode1 and Mode2))
            if (MasterPuncher.mode & 1){
                // 5 bit + Shift flag
                if (cOut & PUNCHER_MASK_SHIFT){
                    // Got FIGS
                    if (!MasterPuncher.shift){
                        // Must switch to figs first
                        cOut = PUNCHER_ITA_FIGS;
                        MasterPuncher.shift = 1;
                        bAdv = 0;
                    }
                }
                else{
                    // Got LTRS
                    if (MasterPuncher.shift){
                        // Must switch to ltrs first
                        cOut = PUNCHER_ITA_LTRS;
                        MasterPuncher.shift = 0;
                        bAdv = 0;
                    }
                }
            }
            
            // If there was a shift conversion then we have to stay without updating the pointer
            if (bAdv){
                MasterPuncher.len--;
                MasterPuncher.output++;
            }

            // Deal with physical outputs now, limit data bits and add guide            
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = (cOut & PUNCHER_MASK_DATA) | PUNCHER_BIT_GUIDE;
            
            MasterPuncher.state = 3;
            MasterPuncher.tick  = MasterPuncher.time_punch;
        }
        else if (MasterPuncher.state == 3){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = 0;
            MasterPuncher.state = 4;
            MasterPuncher.tick  = MasterPuncher.time_gap1;
        }
        else if (MasterPuncher.state == 4){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = PUNCHER_BIT_ADVANCE;
            MasterPuncher.state = 5;
            MasterPuncher.tick  = MasterPuncher.time_advance;
        }
        else if (MasterPuncher.state == 5){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = 0;
            MasterPuncher.state = 6;
            MasterPuncher.tick  = MasterPuncher.time_gap2;
        }
        else if (MasterPuncher.state == 6){
            if (MasterPuncher.len){
                MasterPuncher.state = 2;
            }
            else{
                MasterPuncher.state = 0;
            }
        }
    }
}


unsigned char Puncher_write(unsigned char *buff, unsigned char len){
    if (!MasterPuncher.enabled){
        // Error: puncher disabled
        return 255;
    }
    if (MasterPuncher.state){
        // Error: Puncher busy
        return MasterPuncher.state;
    }
    MasterPuncher.len    = len ? len : strlen(buff);
    if (!MasterPuncher.len){
        // Error: Trying to punch an empty buffer
        return 254;
    }
    
    MasterPuncher.output = buff;
    MasterPuncher.state  = 1;
    return 0;
}


void Puncher_cmd(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pVal  = NULL;
    unsigned char n = 0;
    unsigned char m = 0;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    
    if (strequal(pArg1, "send")){
        if (MasterPuncher.state){
            bOK = false;
            strcat(sReply, txtErrorBusy);
        }
        else {
            n = strlen(pArg1 + 5);
            if (n < PUNCHER_BUFFER_SIZE){
                strcpy(Puncher_Buffer, pArg1 + 5);
                MasterPuncher.enabled = 1;
                m = Puncher_write(Puncher_Buffer, n);
                if (m){
                    bOK = false;
                    strcat(sReply, txtErrorBusy);
                }
                else{
                    sprintf(sReply, "%u + %u + %u + %u = %u (%u)\r\n",
                        MasterPuncher.time_punch,
                        MasterPuncher.time_gap1,
                        MasterPuncher.time_advance,
                        MasterPuncher.time_gap2,
                        (MasterPuncher.time_punch + MasterPuncher.time_gap1 + MasterPuncher.time_advance + MasterPuncher.time_gap2),
                        n
                    );
                }
            }
            else {
                bOK = false;
                strcat(sReply, txtErrorTooBig);
            }
        }
    }
    else{
        if (strequal(pArg1, "mode")){
            pVal = &MasterPuncher.mode;
        }
        else if (strequal(pArg1, "time_punch")){
            pVal = &MasterPuncher.time_punch;
        }
        else if (strequal(pArg1, "time_gap1")){
            pVal = &MasterPuncher.time_gap1;
        }
        else if (strequal(pArg1, "time_gap2")){
            pVal = &MasterPuncher.time_gap2;
        }
        else if (strequal(pArg1, "time_advance")){
            pVal = &MasterPuncher.time_advance;
        }

        if (pVal){
            pArg2 = strtok(NULL,  txtWhitespace);
            if (pArg2){
                *pVal = (unsigned char) atoi(pArg2);
            }
            sprintf("%s = %u", sReply, pArg1, *pVal);
        }
        else{
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);
        }
    }
    
    printReply(bOK, "PUNCH", sReply);
}