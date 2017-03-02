

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
#include "app_io.h"
#include "app_cmd.h"

#if defined(LIB_PROGRAM)
    #include "service_program.h"
#endif

#define LIB_KEYS

typedef struct {
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Time;         // Buttons check interval (ms) 0  is disabled (def:30)
    unsigned char Run;          // Allows running programs from keyboard using the prepulse button and the address row
    // Runtime counters
    unsigned char Tick;         // Tick counter (runtime)
    unsigned char Bit;          // Bit pointer 0..7    
    // Runtime status
    unsigned int  Function;      // Runtime value for Function keys
    unsigned int  Address;       // Runtime value for Address keys
    unsigned int  Input;         // Runtime value for Input keys
    unsigned int  Switches;      // Runtime value for misc console switches
} keys_t;


keys_t MasterKeys;

void Keys_init(void);
inline void Keys_tick(void);
inline void Keys_service(void);
void Keys_checkButtons(void);
void Keys_getKeys(void);
void Keys_getStatusReply(void);
void Keys_cmd(unsigned char *pArgs);



void Keys_init(void){
    MasterKeys.Enabled  = 1;
    MasterKeys.Time     = 30;
    MasterKeys.Run      = 0;
    
    MasterKeys.Tick     = 0;
    MasterKeys.Bit      = 0;
    
    MasterKeys.Function = 0;
    MasterKeys.Address  = 0;
    MasterKeys.Input    = 0;
    MasterKeys.Switches = 0;
    
    
    // Keyboard Inputs
    PIN_KEYS_IN_TRIS = 0xff;
    
    // Keyboard Outputs
    PIN_KEYS_OUT_0 = 0;
    PIN_KEYS_OUT_1 = 0;
    PIN_KEYS_OUT_2 = 0;
    PIN_KEYS_OUT_3 = 0;
    PIN_KEYS_OUT_4 = 0;
    PIN_KEYS_OUT_0_TRIS = OUTPUT;
    PIN_KEYS_OUT_1_TRIS = OUTPUT;
    PIN_KEYS_OUT_2_TRIS = OUTPUT;
    PIN_KEYS_OUT_3_TRIS = OUTPUT;
    PIN_KEYS_OUT_4_TRIS = OUTPUT;

}

inline void Keys_tick(void){
    // MASTER BUTTONS
    if (MasterKeys.Tick){
        MasterKeys.Tick--;
    }
}

inline void Keys_service(void){
    if (!MasterKeys.Enabled){
        return;
    }
    if (MasterKeys.Tick){
        return;
    }
    if (!MasterKeys.Time){
        return;
    }
    
    Keys_checkButtons();
    MasterKeys.Tick = MasterKeys.Time;
}

void Keys_checkButtons(void){
    unsigned char nBtn;
    unsigned char nOnes = 0;
    unsigned char n = 0;
    bool bChanged = 0;
    
    PIN_KEYS_OUT_0 = 0;
    PIN_KEYS_OUT_1 = 0;
    PIN_KEYS_OUT_2 = 0;
    PIN_KEYS_OUT_3 = 0;
    PIN_KEYS_OUT_4 = 0;
    
    if  (MasterKeys.Bit){
        MasterKeys.Bit--;
    }
    else {
        // We only use 7 bits as history
        MasterKeys.Bit = MasterButtonsHistoryBits - 1;
    }
    
    for (nBtn = 0; nBtn < MasterButtonsMapLen; nBtn++) {
        setbit(*MasterButtonsMap[nBtn].out_port, MasterButtonsMap[nBtn].out_bit);
        
        n = readbit(*MasterButtonsMap[nBtn].in_port, MasterButtonsMap[nBtn].in_bit);
        if (n){
            setbit(MasterButtons[nBtn].history, MasterKeys.Bit);
        }
        else{
            clearbit(MasterButtons[nBtn].history, MasterKeys.Bit);
        };
        
        clearbit(*MasterButtonsMap[nBtn].out_port, MasterButtonsMap[nBtn].out_bit);

        // byte2binstr(sStr5, MasterButtons[nBtn].history);
        // printf("!BTN%u %u %u %s\r\n", nBtn, MasterKeys.Bit, n, sStr5);
        
        // Count bits
        nOnes = 0;
        for (n = 0b01000000; n; n = n >> 1){
            if (MasterButtons[nBtn].history & n){
                nOnes++;
            }
        }
        // Determine new status
        n = 0; // Should flip?
        if (MasterButtons[nBtn].status){
            if (nOnes < 3){
                n = 1;
            }
        }
        else{
            if (nOnes > 4){
                n = 1;
            }
        }
        // Flip status and notify
        if (n){
            MasterButtons[nBtn].status ^= 1;
            sReply[0] = nBtn + 48;
            sReply[1] = '=';
            sReply[2] = MasterButtons[nBtn].status + 48;
            sReply[3] = 0x00;
            printReply(3, "BUTTON", sReply);
            
            bChanged = 1;
        }
    }
    if (bChanged){
        Keys_getKeys();
        Keys_getStatusReply();
        printReply(3, "STATUS", sReply);

        #if defined(LIB_PROGRAM)
        if (MasterKeys.Run && !MasterProgram.Enabled){
            unsigned char s[5];
            sprintf(s, "%u", MasterKeys.Address & 0x07);
            Program_cmd(s);
        }
        #endif
    }
}

void Keys_getKeys(void){
    volatile unsigned char nLsb = 0;

    // Keyboard Inputs
    PIN_KEYS_IN_TRIS = 0xff;
    
    // Keyboard Outputs
    PIN_KEYS_OUT_0 = 0;
    PIN_KEYS_OUT_1 = 0;
    PIN_KEYS_OUT_2 = 0;
    PIN_KEYS_OUT_3 = 0;
    PIN_KEYS_OUT_4 = 0;
    PIN_KEYS_OUT_0_TRIS = OUTPUT;
    PIN_KEYS_OUT_1_TRIS = OUTPUT;
    PIN_KEYS_OUT_2_TRIS = OUTPUT;
    PIN_KEYS_OUT_3_TRIS = OUTPUT;
    PIN_KEYS_OUT_4_TRIS = OUTPUT;
    
    // Get the shared LSB from C0
    PIN_KEYS_OUT_3 = 1;
    nLsb = PIN_KEYS_IN;
    PIN_KEYS_OUT_3 = 0;
    
    PIN_KEYS_OUT_0 = 1;
    MasterKeys.Input    = ((unsigned int) PIN_KEYS_IN << 2) | ((nLsb & 0b00001100) >> 2);
    PIN_KEYS_OUT_0 = 0;

    PIN_KEYS_OUT_1 = 1;
    MasterKeys.Address  = ((unsigned int) PIN_KEYS_IN << 2) | ((nLsb & 0b00110000) >> 4);
    PIN_KEYS_OUT_1 = 0;

    PIN_KEYS_OUT_2 = 1;
    MasterKeys.Function = ((unsigned int) PIN_KEYS_IN << 2) | ((nLsb & 0b11000000) >> 6);
    PIN_KEYS_OUT_2 = 0;
    
    PIN_KEYS_OUT_4 = 1;
    MasterKeys.Switches = (unsigned int) (((unsigned int) PIN_KEYS_IN << 8) | (unsigned int) nLsb);
    PIN_KEYS_OUT_4 = 0;

}

void Keys_getStatusReply(void){
    unsigned char n = 0;
    unsigned char *s;
    
    s = sReply + strlen(sReply);
    
    n |= MasterButtons[2].status;
    n = n << 1;
    n |= MasterButtons[1].status;
    n = n << 1;
    n |= MasterButtons[0].status;
    
    sprintf(s, "%3s F: %04x / A: %04x / I: %04x / S: %04x / B: %01x",
        MasterKeys.Enabled ? txtOn : txtOff,
        MasterKeys.Function, 
        MasterKeys.Address,
        MasterKeys.Input,
        MasterKeys.Switches,
        n
    );
}


void Keys_cmd(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    //unsigned char *pArg2 = NULL;

    sReply[0] = 0x00;

    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    //pArg2 = strtok(NULL,  txtWhitespace);

    if (pArg1 == NULL){
        
    }
    else if (strequal(pArg1, "on") || strequal(pArg1, "1")){
        MasterKeys.Enabled = 1;
    }
    else if (strequal(pArg1, "off") || strequal(pArg1, "0")){
        MasterKeys.Enabled = 0;
    }
    else{
        bOK = false;
        strcat(sReply, txtErrorUnknownArgument);
    }

    if (bOK){
        Keys_getKeys();
        Keys_getStatusReply();    
    }

    printReply(bOK, "KEYS", sReply);
}