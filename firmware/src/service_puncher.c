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
#define PUNCHER_BIT_ADVANCE  0b01000000 // Bitmask for advancing the paper
#define PUNCHER_BIT_GUIDE    0b00100000 // Bitmask for the guide hole
#define PUNCHER_MASK_DATA    0b00011111 // Data mask
#define PUNCHER_MASK_SHIFT   0b00100000 // Shift mask (used for mode 1))
#define PUNCHER_MASK_NONZERO 0b10000000 // Used as flag in conversions

#define PUNCHER_ITA_LTRS     0b00011111 // 
#define PUNCHER_ITA_FIGS     0b00011011 // 
#define PUNCHER_ITA_NULL     0b00000000 // 

#define PUNCHER_BUFFER_SIZE 16

//                                  00   000000   0000000011111111111   1111   1     22 222 222 222 222223 3333333333   3333   3
//                                  01   234567   89abcdef0123456789a   bcde   f     01 234 567 89a bcdef0 123456789a   bcde   f  
//const unsigned char txtITA2[] = "\x00E\x0aA SIU\x0dDRJNFCKTZLWHYPQOBG\x0fMXV\x0e" "\x003\n- \'87\r$4\a,!:(5\")2#6019?&\x0f./;\x0e";

const unsigned char txtITA2[] = {0,'E',0x0a,'A',' ','S','I','U',0x0d,'D','R','J','N','F','C','K','T','Z','L','W','H','Y','P','Q','O','B','G',0x0f,'M','X','V',0x0e,0x00,'3',0x0a,'-',' ','\'','8','7','\r','$','4','\a',',','!',':','(','5','\"',')','2','#','6','0','1','9','?','&',0x0f,'.','/',';',0x0e};


unsigned char Puncher_Buffer[PUNCHER_BUFFER_SIZE];


typedef struct {
    unsigned char Enabled;     // 0 = Off / 1 = On
    unsigned char Mode;        // Mode Bitfield Bit0 = 0:Direct / 1:shifted | Bit1 = 1:ASCII | Bit2 = 4:HEX
    unsigned char Shift;       // Mode 1 Shift: 0 = LTRS / 1 = FIGS
    unsigned char State;       // State Machine: 0 = Off / 1 = 
    unsigned char TimePunch;   // Time in ms to punch the holes
    unsigned char TimeGap1;    // Time in ms between punch and advance
    unsigned char TimeAdvance; // Time in ms to advance the paper
    unsigned char TimeGap2;    // Time in ms to rest before next command
    
    unsigned char Tick;        // Time ticker in ms
    unsigned char *Output;     // Output buffer (must be binary safe))
    unsigned char Len;         // Output buffer counter n..0
} puncher_t;

puncher_t MasterPuncher = {0,0,0,0,0,0,0,0,0,0,0};

void Puncher_init(unsigned char bEnabled, unsigned char nMode){
    MasterPuncher.Tick         = 0;
    MasterPuncher.TimePunch   = 30;
    MasterPuncher.TimeGap1    = 15;
    MasterPuncher.TimeAdvance = 60;
    MasterPuncher.TimeGap2    = 5;
    MasterPuncher.Mode         = nMode;
    MasterPuncher.Shift        = 0;
    MasterPuncher.State        = 0;
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
    
    if (MasterPuncher.State && !MasterPuncher.Tick){
        if (MasterPuncher.State == 1){
            if (!MasterPuncher.Len){
                // Error: Trying to punch an empty buffer
                MasterPuncher.State = 254;
                return;
            }
            MasterPuncher.State = 2;
        }
        
        if (MasterPuncher.State == 2){
putch('%');
            bAdv = 1;
            // Mode4 = HEX
            if (MasterPuncher.Mode & 0x04){
                // With HEX conversion
                cOut = hexstr2byte(MasterPuncher.Output);
                // In this mode we need 2 advances instead of just one
                if (MasterPuncher.Len > 1){
                    MasterPuncher.Len--;
                    MasterPuncher.Output++;
                }
putch('x');
putch('=');
byte2binstr(sStr1, cOut);
print(sStr1);
putch(' ');
            }
            else{
                // Direct
                cOut = *MasterPuncher.Output;
            }
            
            // Mode2/3 = ASCII
            if (MasterPuncher.Mode & 2){
                // With ASCII -> ITA2+shift conversion
                cIn = toupper(cOut);
putch(cIn);
                // Find the corresponding ITA2 for the given ASCII
                for (cOut = 0; cOut < 65; cOut++){
                    if (cOut == 64){
                        // Not found
                        cOut = MasterPuncher.Shift ? PUNCHER_MASK_SHIFT : 0;
                        break;
                    }
                    // The search order is determined by the current shift
                    if (MasterPuncher.Shift){
                        if (txtITA2[63 - cOut] == cIn){
                            cOut = 63 - cOut;
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
putch('i');
putch('=');
byte2binstr(sStr1, cOut);
print(sStr1);
putch(' ');
            }
            
            // Mode1 = ITA2+shift -> ITA2
            // Shift calc (used for both Mode1 and Mode2))
            if (MasterPuncher.Mode & 1){
                // 5 bit + Shift flag
                if (cOut & PUNCHER_MASK_SHIFT){
                    // Got FIGS
                    if (!MasterPuncher.Shift){
                        // Must switch to figs first
                        cOut = PUNCHER_ITA_FIGS;
                        MasterPuncher.Shift = 1;
                        bAdv = 0;
                    }
                }
                else{
                    // Got LTRS
                    if (MasterPuncher.Shift){
                        // Must switch to ltrs first
                        cOut = PUNCHER_ITA_LTRS;
                        MasterPuncher.Shift = 0;
                        bAdv = 0;
                    }
                }
            }
            
            // If there was a shift conversion then we have to stay without updating the pointer
            if (bAdv){
                MasterPuncher.Len--;
                MasterPuncher.Output++;
            }

            // Deal with physical outputs now, limit data bits and add guide            
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = (cOut & PUNCHER_MASK_DATA) | PUNCHER_BIT_GUIDE;
putch('>');
putch('=');
byte2binstr(sStr1, PUNCHER_LAT);
print(sStr1);
putch('\r');
putch('\n');
            MasterPuncher.State = 3;
            MasterPuncher.Tick  = MasterPuncher.TimePunch;
        }
        else if (MasterPuncher.State == 3){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = 0;
            MasterPuncher.State = 4;
            MasterPuncher.Tick  = MasterPuncher.TimeGap1;
        }
        else if (MasterPuncher.State == 4){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = PUNCHER_BIT_ADVANCE;
            MasterPuncher.State = 5;
            MasterPuncher.Tick  = MasterPuncher.TimeAdvance;
        }
        else if (MasterPuncher.State == 5){
            PUNCHER_TRIS = 0;
            PUNCHER_LAT  = 0;
            MasterPuncher.State = 6;
            MasterPuncher.Tick  = MasterPuncher.TimeGap2;
        }
        else if (MasterPuncher.State == 6){
            if (MasterPuncher.Len){
                MasterPuncher.State = 2;
            }
            else{
                MasterPuncher.State = 0;
            }
        }
    }
}


unsigned char Puncher_write(unsigned char *buff, unsigned char len){
    if (!MasterPuncher.Enabled){
        // Error: puncher disabled
        return 255;
    }
    if (MasterPuncher.State){
        // Error: Puncher busy
        return MasterPuncher.State;
    }
    MasterPuncher.Len    = len ? len : strlen(buff);
    if (!MasterPuncher.Len){
        // Error: Trying to punch an empty buffer
        return 254;
    }
    
    MasterPuncher.Output = buff;
    MasterPuncher.State  = 1;
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

    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    
    if (strequal(pArg1, "send")){
        if (MasterPuncher.State){
            bOK = false;
            strcat(sReply, txtErrorBusy);
        }
        else {
            n = strlen(pArg1 + 5);
            if (n < PUNCHER_BUFFER_SIZE){
                strcpy(Puncher_Buffer, pArg1 + 5);
                MasterPuncher.Enabled = 1;
                m = Puncher_write(Puncher_Buffer, n);
                if (m){
                    bOK = false;
                    strcat(sReply, txtErrorBusy);
                }
                else{
                    sprintf(sReply, "%u + %u + %u + %u = %u (%u)\r\n",
                        MasterPuncher.TimePunch,
                        MasterPuncher.TimeGap1,
                        MasterPuncher.TimeAdvance,
                        MasterPuncher.TimeGap2,
                        (MasterPuncher.TimePunch + MasterPuncher.TimeGap1 + MasterPuncher.TimeAdvance + MasterPuncher.TimeGap2),
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
            pVal = &MasterPuncher.Mode;
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
            strcat(sReply, txtErrorInvalidArgument);
        }
    }
    
    printReply(bOK, "PUNCH", sReply);
}