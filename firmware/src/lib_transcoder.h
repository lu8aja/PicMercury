/* 
 * File:   lib_transcoder.h
 * Author: Javier
 *
 * Created on March 16, 2017, 10:40 AM
 */


#define LIB_TRANSCODER

#include "lib_ring.h"
#include "app_globals.h"
#include "lib_helpers.h"

#define TRANSCODER_MASK_DATA      0b00011111 // Data mask
#define TRANSCODER_MASK_SHIFT     0b00100000 // Shift mask (used for mode 1))

#define TRANSCODER_ITA_LTRS       0b00011111 // 
#define TRANSCODER_ITA_FIGS       0b00011011 // 
#define TRANSCODER_ITA_NULL       0b00000000 //
#define TRANSCODER_ITA_CR         0b00001000 //
#define TRANSCODER_ITA_LF         0b00000010 //
#define TRANSCODER_ITA_UNKNOWN    0b00000100 //
#define TRANSCODER_ASCII_UNKNOWN  0x1A       // Substitute character

#define TRANSCODER_CODE_ITA2      0
#define TRANSCODER_CODE_ITA2_ES   1
#define TRANSCODER_CODE_USTTY     2

#ifndef CFG_TRANSCODER_COLUMNS
    #define CFG_TRANSCODER_COLUMNS   70
#endif


#ifndef CFG_TRANSCODER_CODE
    #define CFG_TRANSCODER_CODE   TRANSCODER_CODE_ITA2
#endif


typedef struct {
    union {
        unsigned char Configs;        // Configs Register
        struct {
            unsigned char ModeIta2:1;      // [01] ASCII -> ITA2 w/bit6
            unsigned char ModeBit6:1;      // [02] ITA2 w/bit6 to ITA2 + ltrs/figs
            unsigned char AvoidNull:1;     // [04] Do not add ASCII 0 null when writing
            unsigned char AutoCrLf:1;      // [08] Automatically insert CrCrLf at column 70
            unsigned char Linked:1;        // [10] Shift gets synced to another buffer at Linked
        };
    };
    union {
        unsigned char Status;         // Status Register
        struct {
            // IMPORTANT: If you change shift position you will have to update masks at functions write and read
            unsigned char Shift:1;         // [01] Current Shift Status 0: LTRS / 1: FIGS
            unsigned char LastPrintable:1; // [02] 1: Last character written was printable (not LTRS, NULL, FIGS)
            unsigned char CrLfCtrl:2;      // [04] 
        };
    };
    unsigned char Char;               // Last Character being processed
    unsigned char Column;             // Column tracking
    Ring_t *Ring;                     // Ring buffer
    unsigned char *LinkedStatus;      // Configs of the other paired transcoder
    unsigned char *LinkedColumn;      // Column of the other paired transcoder
} Transcoder_t;



Transcoder_t * Transcoder_new(const unsigned char nSize);
unsigned char  Transcoder_read(Transcoder_t *pTranscoder, unsigned char *pChar);
unsigned char  Transcoder_write(Transcoder_t *pTranscoder, unsigned char cIn, unsigned char bTest);

extern const unsigned char txtTranscoderTable[];


