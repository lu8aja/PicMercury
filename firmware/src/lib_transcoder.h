/* 
 * File:   lib_transcoder.h
 * Author: Javier
 *
 * Created on March 16, 2017, 10:40 AM
 */


#define LIB_TRANSCODER

#include "lib_ring.h"
#include "app_globals.h"
#include "app_helpers.h"

#define TRANSCODER_MASK_DATA      0b00011111 // Data mask
#define TRANSCODER_MASK_SHIFT     0b00100000 // Shift mask (used for mode 1))

#define TRANSCODER_ITA_LTRS       0b00011111 // 
#define TRANSCODER_ITA_FIGS       0b00011011 // 
#define TRANSCODER_ITA_NULL       0b00000000 //
#define TRANSCODER_ITA_UNKNOWN    0b00000100 //
#define TRANSCODER_ASCII_UNKNOWN  0xff //

typedef struct {
    union {
        unsigned char Configs;    // Status Register
        struct {
            unsigned ModeAscii:1;     // [01] ASCII -> ITA2 w/bit6
            unsigned ModeBit6:1;      // [02] ITA2 w/bit6 to ITA2 + ltrs/figs
            unsigned AvoidNull:1;     // [04] Do not add ASCII 0 null when writing
            unsigned LinkedShifts:1;  // [08] Shift gets synced to another buffer at Linked
            unsigned :3;              // 
            unsigned Shift:1;         // [80] Current Shift Status 0: LTRS / 1: FIGS
        };
    };
    Ring_t *Ring;
    unsigned char *LinkedConfigs;
} Transcoder_t;



Transcoder_t * Transcoder_new(const unsigned char nSize);
unsigned char Transcoder_read(Transcoder_t *pTranscoder, unsigned char *pChar);
unsigned char Transcoder_write(Transcoder_t *pTranscoder, unsigned char cIn);

extern const unsigned char txtTranscoderTable[];


