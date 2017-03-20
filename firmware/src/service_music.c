/* 
 * File:   app_midi.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:07 PM
 */

#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>


#include "data_eeprom.h"

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"
#include "app_main.h"
#ifdef LIB_KEYS
    #include "service_keys.h"
#endif

#define LIB_MUSIC

typedef struct {
    unsigned char address;
    unsigned char length;
    unsigned char first_note;
} midi_t;

typedef struct {
    unsigned char enabled;      // 0 = Off / 1 = On
    unsigned char start;        // 0 = Off / 1 = Start
    unsigned char mode;         // 0 = Single tone / 1 = Music
    unsigned char restart;      // 0 = Disable when Time is reached / 1 = Restart either music from step 0 or steady tone
    unsigned char notify;       // Notify events
    unsigned char debug;        // Debug
    unsigned char song;         // Song number
    unsigned int  tempo;        // For music: Time per beat in ms
    signed   char pitch;        // For music: Number of notes to shift the MIDI note
    unsigned char address;      // Song address in eeprom
    unsigned char length;       // Song length in eeprom, number of notes
    unsigned char step;         // Current note/step
    unsigned char note;         // Current note
    unsigned char period;       // Current tone semi-period
    unsigned char beats;        // Current note duration beats
    unsigned int  time;         // Current note duration in ms
    // Runtime counters
    unsigned char tick;         // Current tick counter (between wave inversions) 0 .. Period
    unsigned int  counter;      // Current note time counter 0 .. Time
} music_t;


music_t MasterMusic;
midi_t  MasterMidi;

void          Music_init(void);
inline void   Music_tick(void);
inline void   Music_beat(void);
void          Music_service(void);
unsigned char Music_getMidiPeriod(unsigned char nNote);
void          Music_setSingleTone(const unsigned char nPeriod, const unsigned char nTime);
void          Music_getStatus(unsigned char *pStatus);
void          Music_cmd(Ring_t * pBuffer, unsigned char *pArgs);

void Music_init(void){
    MasterMidi.address    = EEPROM_read(EEDATA_MIDI_ADDRESS);
    MasterMidi.length     = EEPROM_read(EEDATA_MIDI_LENGTH);
    MasterMidi.first_note = EEPROM_read(EEDATA_MIDI_FIRST_NOTE);
    
    MasterMusic.address   = EEPROM_read(EEDATA_MUSIC_0_ADDRESS);
    MasterMusic.length    = EEPROM_read(EEDATA_MUSIC_0_LENGTH)  >> 1;
    
    MasterMusic.period    = 50;
    MasterMusic.time      = 10000;
    MasterMusic.mode      = 1;
    MasterMusic.tempo     = 30;
    MasterMusic.notify    = 1;
}

inline void Music_tick(void){
    // MASTER TONES
    if (MasterMusic.enabled){
        if (MasterMusic.tick){
            MasterMusic.tick--;
        }
        else if (MasterMusic.period){
            MasterMusic.tick = MasterMusic.period;
            // Hoot/Speaker output
            PIN_HOOT_TRIS  = OUTPUT;
            PIN_HOOT_OUT  ^= 1;
        }
        else {
            PIN_HOOT_OUT  = 0;
        }
    }
}

inline void Music_beat(void){
    // MASTER TONE ms Counter
    if (MasterMusic.counter){
        MasterMusic.counter--;
    }
}


void Music_service(void){
    unsigned char nAddress;
    
    if (!MasterMusic.enabled){
        return;
    }
    
    if (MasterMusic.start){
        MasterMusic.counter = 0;
    }

    if (MasterMusic.counter){
        return;
    }
    
    if (!MasterMusic.mode){
        // Single tone mode
        if (MasterMusic.start){
            MasterMusic.start = 0;
            MasterMusic.counter = MasterMusic.time;
        }
        else if (MasterMusic.restart){
            MasterMusic.counter = MasterMusic.time;
        }
        else{
            MasterMusic.enabled = 0;
        }
        return;
    }

    // Music mode
    if (MasterMusic.start){
        MasterMusic.step  = 0;
        MasterMusic.start = 0;
    }
    else if (MasterMusic.period){
        MasterMusic.period = 0;
        MasterMusic.counter = MasterMusic.tempo;
        return;
    }
    else{
        MasterMusic.step++;
    }

    // End of music
    if (MasterMusic.step >= MasterMusic.length){
        MasterMusic.step = 0;
        if (!MasterMusic.restart){
            MasterMusic.enabled = 0;
        }
    }
    // Next tone
    if (MasterMusic.enabled){
        // Get midi note and beats (note time)
        nAddress = MasterMusic.address + (MasterMusic.step << 1);
        MasterMusic.note  = EEPROM_read(nAddress);
        MasterMusic.beats = EEPROM_read(nAddress + 1);

        if (MasterMusic.beats){

            // Calc time tone duration
            MasterMusic.time = MasterMusic.beats * MasterMusic.tempo;

            // Apply pitch
            MasterMusic.note  += MasterMusic.pitch;
        
            // Calc period for the tone
            MasterMusic.period = Music_getMidiPeriod(MasterMusic.note);
        }
        else {
            // Autostop on note beat time zero
            MasterMusic.enabled = 0;
            MasterMusic.period  = 0;
            MasterMusic.time    = 0;
        }
        
        // Set runtime counters so interrupt can work immediately
        MasterMusic.counter = MasterMusic.time;
        MasterMusic.tick    = 0; // ?? confirmar
        
        if (MasterMusic.notify){
            sprintf(sReply, "#%02u n:%02u t:%02u", 
                MasterMusic.step,
                MasterMusic.note,
                MasterMusic.time);

            printReply(0, 3, "MUSIC", sReply);
        }
    }
    if (!MasterMusic.enabled){
        printReply(0, 3, "MUSIC", txtOff);
    }
}

unsigned char Music_getMidiPeriod(unsigned char nNote){
    // Calc period for the tone
    if (nNote < MasterMidi.first_note){
        return 0;
    }
    else {
        nNote = nNote - MasterMidi.first_note;
        if (nNote < MasterMidi.length){
            return EEPROM_read(MasterMidi.address + nNote);
        }
        else{
            return 0;
        }
    }
}
   
void Music_setSingleTone(const unsigned char nPeriod, const unsigned char nTime){
    // Time 0 means continuous
    MasterMusic.period  = nPeriod;
    MasterMusic.time    = nTime ? (unsigned int) nTime << 8 : 10000;
    MasterMusic.restart = nTime ? 0     : 1;
    MasterMusic.mode    = 0;
    MasterMusic.step    = 0;
    MasterMusic.start   = 1;
    MasterMusic.enabled = 1;
}

void Music_getStatus(unsigned char *pStatus){
    if (MasterMusic.mode){
        sprintf(pStatus, " - Music %s #%u n: %d t: %d", 
            MasterMusic.enabled ? txtOn : txtOff,
            MasterMusic.step,
            MasterMusic.note,
            MasterMusic.beats
        );
    }
    else{
        sprintf(pStatus, " - Single %s T: %u t: %u",
            MasterMusic.enabled ? txtOn : txtOff,
            MasterMusic.period,
            MasterMusic.time
        );
    }
}

void Music_cmd(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned int nNote   = 0;
    unsigned int iFreq   = 0;
    unsigned char nTime  = 0;
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
    else if (strequal(pArg1, "on") || strequal(pArg1, "1")){
        MasterMusic.enabled = 1;
        MasterMusic.start   = 1;
        strcpy(sReply, txtOn);
    }
    else if (strequal(pArg1, "off") || strequal(pArg1, "0")){
        MasterMusic.enabled = 0;
        strcpy(sReply, txtOff);
    }
    else if (strequal(pArg1, "restart")){
        if (pArg2){
            MasterMusic.restart = strequal(pArg2, "on") ? 1 : 0;
        }
        sprintf(sReply, "%s %s", pArg1, MasterMusic.restart ? txtOn : txtOff);
    }
    else if (strequal(pArg1, "mode")){
        if (pArg2){
            MasterMusic.mode = (strequal(pArg2, "on") || strequal(pArg2, "music") || strequal(pArg2, "1")) ? 1 : 0;
        }
        sprintf(sReply, "%s %s", pArg1, MasterMusic.mode ? "Music" : "Single");
    }
    else if (strequal(pArg1, "play")){
        MasterMusic.enabled = 0;
        MasterMusic.mode    = 1;
        MasterMusic.restart = 0;
        MasterMusic.start   = 1;
        MasterMusic.enabled = 1;
    }
    else if (strequal(pArg1, "time")){
        if (pArg2){
            MasterMusic.time = iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterMusic.time);
    }
    else if (strequal(pArg1, "tempo")){
        if (pArg2){
            MasterMusic.tempo = iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterMusic.tempo);
    }
    else if (strequal(pArg1, "pitch")){
        if (pArg2){
            MasterMusic.pitch = (signed char) iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterMusic.pitch);
    }
    else if (strequal(pArg1, "period")){
        if (pArg2){
            MasterMusic.period = (unsigned char) iArg2;
        }
        sprintf(sReply, "%s %u", pArg1, MasterMusic.period);
    }
    else if (strspn(pArg1, txtNum) == strlen(pArg1)){
        // Frequency directly with optional time in secs
        iFreq = atoi(pArg1);
        if (pArg2 && strspn(pArg2, txtNum) == strlen(pArg2)){
            nTime = (unsigned char) atoi(pArg2);
        }
    }
    #ifdef LIB_KEYS
    else if (strequal(pArg1, "keys")){
        Keys_getKeys();
        iFreq = MasterKeys.Input << 1;
        nTime = 60; // 1 Minute max
    }
    #endif
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
            iPeriod = Music_getMidiPeriod(iArg2);
            if (!iPeriod){
                bOK = false;
                strcat(sReply, txtErrorInvalidArgument);    
            }
        }
        else{
            bOK = false;
            strcat(sReply, txtErrorMissingArgument);    
        }
    }
    else{
        bOK = false;
        sprintf(sReply, "%s. Now: %s", txtErrorUnknownArgument, MasterMusic.enabled > 0 ? txtOn : txtOff);
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
        bShowStatus = true;
        Music_setSingleTone((unsigned char) iPeriod, nTime);
    }

    if (bShowStatus){
        Music_getStatus(sStr5);
    }
    strcat(sReply, sStr5);
    printReply(pBuffer, bOK, "MUSIC", sReply);
}