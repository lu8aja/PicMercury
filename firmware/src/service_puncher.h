/* 
 * File:   service_puncher.h
 * Author: Javier
 *
 * Created on March 1, 2017, 1:36 AM
 */

#include "lib_transcoder.h"
#include "app_globals.h"

#define LIB_PUNCHER

#define PUNCHER_TRIS TRISD        // Port tris
#define PUNCHER_LAT  LATD         // Port latch
#define PUNCHER_BIT_ADVANCE    0b01000000 // Bitmask for advancing the paper
#define PUNCHER_BIT_GUIDE      0b00100000 // Bitmask for the guide hole

#ifndef Puncher_sizeOutput
    #define Puncher_sizeOutput 40
#endif

typedef struct {
    unsigned char Enabled;     // 0 = Off / 1 = On
    unsigned char State;       // State Machine: 0 = Off / 1 = 
    unsigned char TimePunch;   // Time in ms to punch the holes
    unsigned char TimeGap1;    // Time in ms between punch and advance
    unsigned char TimeAdvance; // Time in ms to advance the paper
    unsigned char TimeGap2;    // Time in ms to rest before next command
    unsigned char Tick;        // Time ticker in ms
    Transcoder_t *Output;      // Output buffer (must be binary safe))
} Puncher_t;

extern Puncher_t Puncher;


void          Puncher_init(unsigned char bEnabled, unsigned char nMode);
inline void   Puncher_tick(void);
void          Puncher_service(void);
inline unsigned char Puncher_write(const unsigned char *pStr);

inline unsigned char Puncher_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);

void          Puncher_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs);
void          Puncher_cmd(unsigned char idBuffer, unsigned char *pArgs);