
/* 
 * File:   app_leds.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:08 PM
 */

#define LIB_LEDS

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

typedef struct {
    // Configs
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Alarms;       // 0 = Off / 1 = On
    unsigned char Time;         // Ticks in ms to count while flashing led
    unsigned char StepEnabled;  // 0 = Off / 1 = On
    unsigned char StepRestart;  // 0 = Disable when Time is reached / 1 = Restart 
    unsigned int  StepTime;     // Time in ms the step should be help in place
    
    // Runtime
    unsigned int  Status;       // Bitfield of 16 leds statuses
    unsigned char Step;         // Current step in sequence
    unsigned char Tick;         // Flashing tick period counter Time..0
    unsigned int  StepTick;     // Tick counter for each step StepTime..0
} leds_t;


leds_t MasterLeds;

const unsigned int MasterLedSteps[] = {
   //B4555544433332222
   //A0321032132103210
    0b0000000000000001,
    0b0000000000000010,
    0b0000000000000100,
    0b0000000000001000,
    0b0000000000010000,
    0b0000000000100000,
    0b0000000001000000,
    0b0000000010000000,
    0b0000000100000000,
    0b0000001000000000,
    0b0000010000000000,
    0b0000100000000000,
    0b0001000000000000,
    0b0010000000000000,
    0b0100000000000000,
    0b0000000000001111,
    0b0000000011110000,
    0b0000011100000000,
    0b0111100000000000,
    0b0101010101010101,
    0b0010101010101010,
    0b0111111111111111,
    0b0000000000000000,
};
const unsigned char MasterLedStepsLen = sizeof(MasterLedSteps) / sizeof(MasterLedSteps[0]);


inline void Leds_init(void);
inline void Leds_tick(void);
inline void Leds_service(void);
void Leds_updateLeds(void);
void Leds_updateUsb(void);
void Leds_cmd(unsigned char *pArgs);

inline void Leds_init(void){
    // Configs
    MasterLeds.Enabled    = 1;
    MasterLeds.Time       = 10;    // Ticks in ms to count while flashing led
    MasterLeds.StepEnabled= 0;     // 0 = Off / 1 = On
    MasterLeds.StepRestart= 1;     // 0 = Disable when Time is reached / 1 = Restart 
    MasterLeds.StepTime   = 1500;  // Time in ms for each step
    // Runtime
    MasterLeds.Status     = 0;     // Bitfield of 16 leds statuses
    MasterLeds.Tick       = 0;     // Flashing tick period counter Time..0
    MasterLeds.StepTick   = 0;     // Tick counter for each step StepTime..0
}

inline void Leds_tick(void){
    // MASTER LED STEPS
    if (MasterLeds.Tick){
        MasterLeds.Tick--;
    }
    if (MasterLeds.StepTick){
        MasterLeds.StepTick--;
    }
    
    if (MasterLeds.Alarms){
        Leds_updateUsb();
    }
}

inline void Leds_service(void){
    // MASTER LED
    if (!MasterLeds.Tick){
        Leds_updateLeds();
        MasterLeds.Tick = MasterLeds.Time;
    }
    
    // MASTER LED STEPS
    if (!MasterLeds.StepTick && MasterLeds.StepEnabled){
        if (MasterLeds.StepEnabled == 2){
            MasterLeds.Step        = 0;
            MasterLeds.StepEnabled = 1;
        }
        else{
            MasterLeds.Step++;
            if (MasterLeds.Step >= MasterLedStepsLen){
                if (MasterLeds.StepRestart){
                    MasterLeds.Step = 0; 
                }
                else {
                    MasterLeds.StepEnabled = 0;
                    printReply(3, "LEDSTEP", txtOff);
                }
            }
        }

        if (MasterLeds.StepEnabled){
            MasterLeds.Status   = MasterLedSteps[MasterLeds.Step];
            MasterLeds.StepTick = MasterLeds.StepTime;
            int2binstr(sStr1, MasterLeds.Status);               
            sprintf(sReply, "%02d %s", MasterLeds.Step, sStr1);
            printReply(3, "LEDSTEP", sReply);
        }
    }
}

void Leds_updateLeds(void){
    // This function turns on at most 4 leds at a time
    // This implies that worst case leds are turned on on a 1/4 duty cycle
    static unsigned char nCurrentAnode = 8;
    static unsigned char nCurrentShift = 12;
    
    if (!MasterLeds.Status){
        // No leds on, then switch all pins to 3rd state
        LEDS_CATHODES_TRIS |= LEDS_CATHODES;
        LEDS_ANODES_TRIS   |= LEDS_ANODES;
        return;
    }

    // Note: Leds normally change upon commands, but occasionally might form 
    // interrupts that happen between the first nonzero check and the loop.
    // Variable n ensures that at most 4 (one entire led loop) is tried, and no more.
    // It could have been avoided if we ensured from interrupt logic that a zero MasterLeds.Status
    // would be impossible to arrive to this point (however that was a major potential point of failure))
    unsigned char nCathodes;
    unsigned char n = 4;
    do {
        n--;
        nCurrentAnode  = nCurrentAnode >> 1;
        if (!nCurrentAnode){
            nCurrentAnode = 8;
            nCurrentShift = 12;
        }
        else{
            nCurrentShift -= 4;
        }
        nCathodes = (MasterLeds.Status >> nCurrentShift) & 0x0f;
    }
    while (!nCathodes && !n);
    
    // Make sure we do not disturb other A or B pins
    LEDS_CATHODES_TRIS = (LEDS_CATHODES_TRIS & ~LEDS_CATHODES) | ~nCathodes;
    LEDS_ANODES_TRIS   = (LEDS_ANODES_TRIS   & ~LEDS_ANODES)   | ~(nCurrentAnode << LEDS_ANODES_SHIFT);

    LEDS_CATHODES_LAT &= ~LEDS_CATHODES; // We always force a 0 on A (Cathode)
    LEDS_ANODES_LAT   |= LEDS_ANODES;    // We always force a 1 on B (Anode)

    /*
    byte2binstr(sStr1, nCathodes);
    byte2binstr(sStr2, LEDS_CATHODES_TRIS);
    byte2binstr(sStr3, LEDS_ANODES_TRIS);
    
    printf("\r\n!OK LED A=%u K=%s TK=%s TA=%s\r\n", nCurrentAnode, sStr1, sStr2, sStr3);
    */
}

/*
void Leds_updateLeds_original(void){
    // This function turns on at most 1 led at a time
    // This implies that worst case leds are turned on on a 1/16 duty cycle
    // This resulted in flickering and high update rates, so it was deprectaed

    static unsigned char nCurrentLed = 15;
                                       //FEDCBA9876543210 
    static unsigned int  iCurrentBit = 0b0000000100000000;
     
    unsigned char nInitialLed;
    //unsigned char nTrisB;
    //unsigned char nTrisA;
     
    if (!MasterLeds.Status){
        // No leds on, then switch all pins to 3rd state
        LEDS_CATHODES_TRIS |= LEDS_CATHODES;
        LEDS_ANODES_TRIS   |= LEDS_ANODES;
        return;
    }
    // Find the next led to be on
    nInitialLed = nCurrentLed;
    do {
        if (nCurrentLed == 0){
            nCurrentLed = 15;
            iCurrentBit = 0b1000000000000000;
        }
        else{
            nCurrentLed--;
            iCurrentBit = iCurrentBit >> 1;
        }
    } while (!(MasterLeds.Status & iCurrentBit));
    
    if (nInitialLed != nCurrentLed){
        // Not the full loop, it is then another led
        // Make sure we do not disturb other A or B pins
        LEDS_CATHODES_TRIS = (LEDS_CATHODES_TRIS & ~LEDS_CATHODES) | ((MasterLedMap[nCurrentLed] >> LEDS_CATHODES_SHIFT) & LEDS_CATHODES);
        LEDS_ANODES_TRIS   = (LEDS_ANODES_TRIS   & ~LEDS_ANODES)   | ((MasterLedMap[nCurrentLed] >> LEDS_ANODES_SHIFT  ) & LEDS_ANODES);

        LEDS_CATHODES_LAT &= ~LEDS_CATHODES; // We always force a 0 on A (Cathode)
        LEDS_ANODES_LAT   |= LEDS_ANODES;    // We always force a 1 on B (Anode)
    }

    //if (MasterDebug > 2) printf("\r\n!OK LED %d on\r\n", nCurrentLed);

    // If the led is the same one, we do not do anything as everything should already be set
}
 * */


void Leds_cmd(unsigned char *pArgs){
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
        sprintf(sReply, "%d %s", nLed, bit_is_set(MasterLeds.Status, nLed) ? txtOn : txtOff);
    }
    else if(strlen(pArg1) == 1) {
        nLed = (unsigned char) strtol(pArg1, NULL, 16);
        iBit = 0;
        if (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0){
            iBit = 1;
        }
        bit_write(MasterLeds.Status, nLed, iBit);
        sprintf(sReply, "%d %s", nLed, iBit ? txtOn : txtOff);
    }
    else if (strlen(pArg1) == 0){
        // Do nothing, just print info
    }
    else if (strcmp(pArg1, "steps") == 0){
        MasterLeds.StepTick    = 0;
        MasterLeds.StepEnabled = (strcmp(pArg2, "on") == 0 || strcmp(pArg2, "1") == 0) ? 2 : 0;
        sprintf(sReply, "Steps: %s", MasterLeds.StepEnabled ? txtOn : txtOff);
    }
    else{
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
    }
    
    strcat(sReply, " Status: ");
    int2binstr(sStr1, MasterLeds.Status);
    strcat(sReply, sStr1);
    
    printReply(bOK, "LED", sReply);
}

void Leds_updateUsb(void){
    static uint16_t ledCount = 0;

    if(USBIsDeviceSuspended() == true){
        bit_clear(MasterLeds.Status, LED_USB);
        return;
    }

    switch(USBGetDeviceState()){
        case CONFIGURED_STATE:
            /* We are configured.  Blink fast. On for 75ms, off for 75ms, then reset/repeat. */
            if(ledCount == 1){
                bit_set(MasterLeds.Status, LED_USB);
            }
            else if(ledCount == 75){
                bit_clear(MasterLeds.Status, LED_USB);
            }
            else if(ledCount > 150){
                ledCount = 0;
            }
            break;
        default:
            /* We aren't configured yet, but we aren't suspended so let's blink with
             * a slow pulse. On for 50ms, then off for 950ms, then reset/repeat. */
            if(ledCount == 1){
                bit_set(MasterLeds.Status, LED_USB);
            }
            else if(ledCount == 50){
                bit_clear(MasterLeds.Status, LED_USB);
            }
            else if(ledCount > 950){
                ledCount = 0;
            }
            break;
    }

    /* Increment the millisecond counter. */
    ledCount++;
}
