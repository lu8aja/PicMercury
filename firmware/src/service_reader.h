/* 
 * File:   service_reader.h
 * Author: Javier
 *
 * Created on April 11, 2017, 10:41 PM
 */


#include "lib_transcoder.h"
#include "app_globals.h"

#define LIB_READER

#define READER_MOTOR_TRIS TRISC          // Port tris
#define READER_MOTOR_LAT  LATC           // Port latch
#define READER_MOTOR_MASK 0b00000111
#define READER_Pin_EnabledCoils LATCbits.LC2

#define READER_INPUT_TRIS   TRISD        // Port tris
#define READER_INPUT_PORT   PORTD      
#define READER_INPUT_MASK   0b10000000



#define READER_INPUT_InGuide   PORTDbits.RD5 // 

#define READER_Pin_EnableLeds  LATDbits.LD7 // 

#define READER_Button          !PORTDbits.RD6 // 
#define READER_SwitchPaper     !PORTEbits.RE2 // 


#ifndef Reader_sizeOutput
    #define Reader_sizeInput 40
#endif

#define Reader_sizeBlock 16  // When the buffer gets this full, the 

typedef struct {
    union {
        unsigned char Configs;
        struct {
            unsigned char Enabled:1;      // Service enabled
            unsigned char Debug:1;        // Debug enabled
            unsigned char Continuous:1;   // Read continuously until the end of the tape
            unsigned char Store:1;        // Store into buffer
            unsigned char AutoStop:1;     // Automatically stop when buffer is full
            unsigned char EchoToUsb:1;    // Echo to USB even if requestor is not USB
            unsigned char Direction:1;    // Steps direction: 0 = + / 1 = -
            unsigned char NotifyMaster:1; // Notify events to Master I2C
        };
    };
    unsigned char Guide;
    unsigned char Time;        // Time in ms for a step (4 steps = 1 character distance in paper)
    unsigned char Requestor;   // I2C Address, ID Buffer
    unsigned int  Steps;       // Number of pending steps
    unsigned char Alignment;   // Stepper sprocket alignment 0..3
    unsigned char State;       // Stepper sequence state 0..3
    unsigned char Sequence[4]; // Sequence of values for the stepper
    // Runtime
    unsigned char Tick;        // Time ticker in ms
    Transcoder_t *Input;       // Input buffer (must be binary safe)
    // For debug
    unsigned long Start;       // Clock start time
    unsigned int  Chars;       // Number of characters to be read in total
} Reader_t;

extern Reader_t Puncher;

void Reader_init(void);
inline void Reader_tick(void);
void Reader_service(void);

inline unsigned char Reader_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);

void Reader_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs);
void Reader_cmd(unsigned char idBuffer, unsigned char *pArgs);
