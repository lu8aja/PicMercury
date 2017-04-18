/* 
 * File:   service_reader.h
 * Author: Javier
 *
 * Created on April 11, 2017, 10:41 PM
 */


#include "lib_transcoder.h"
#include "app_globals.h"

#define LIB_READER

// I/O Configs

#define READER_MOTOR_TRIS TRISC          // Port tris
#define READER_MOTOR_LAT  LATC           // Port latch
#define READER_MOTOR_MASK 0b00000111
#define READER_Pin_EnabledCoils LATCbits.LC2

#define READER_INPUT_TRIS   TRISD        // Port tris
#define READER_INPUT_PORT   PORTD      
#define READER_INPUT_MASK   0b10000000

#define READER_INPUT_InGuide   (!PORTDbits.RD5) // 

#define READER_Pin_EnableLeds  LATDbits.LD7 // 

#define READER_Button          (!PORTDbits.RD6) // 
#define READER_SwitchPaper     (!PORTEbits.RE2) // 

#define READER_Debug         0 // 


#ifndef Reader_sizeOutput
    #define Reader_sizeInput 40
#endif


// After the guide is lost, this number of characters should be read
// This is due to how the reader optotransistors are placed, as the guide is
// not inline with the bits
#define Reader_ReadPostGuide 7

// When this number of bytes were read, notify Master
#define Reader_sizeBlock 16

#define Reader_Button_TicksStep  5
#define Reader_Button_TicksStart 30




typedef struct {
    union {
        unsigned char Configs;
        struct {
            unsigned char Enabled:1;      // Service enabled
            unsigned char Debug:1;        // Debug enabled
            unsigned char Continuous:1;   // Read continuously until the end of the tape
            unsigned char Store:1;        // Store into buffer
            unsigned char UserStart:1;    // User can start reading from button
            unsigned char EchoToUsb:1;    // Echo to USB even if requestor is not USB
            unsigned char Direction:1;    // Steps direction: 0 = + / 1 = -
            unsigned char NotifyMaster:1; // Notify button events to Master I2C
        };
    };
    union {
        unsigned char Status;
        struct {
            unsigned char GuideLast:1;       //
            unsigned char Aligned:1;         //
            unsigned char BtnStatus: 1;
            unsigned char BufferFull:1;      //
        };
    };
    unsigned char BtnHistory;
    unsigned char BtnTime;
    unsigned char MustStop;    // Flag to stop and know why it did so
    unsigned char Time;        // Time in ms for a step (4 steps = 1 character distance in paper)
    unsigned char Sequence[4]; // Sequence of values for the stepper
    // Runtime
    unsigned int  CharsPending;// Number of pending steps
    unsigned char Align;       // Stepper sprocket alignment 0..3
    unsigned char GuideExtra;  // Counter to control how many steps we try to align the guide (0..4)
    unsigned char ReadPost;    // Counter to control how many chars to read after loosing alignment (0..Reader_ReadPostGuide)
    unsigned char Requestor;   // I2C Address, ID Buffer
    unsigned char Step;        // Stepper sequence current state 0..3
    unsigned char Tick;        // Time ticker in ms
    Transcoder_t *Input;       // Input buffer (must be binary safe)
    // For debug
    unsigned long Start;       // Clock start time
    unsigned int  CharsRead;   // Number of characters actually read in total

} Reader_t;

extern Reader_t Puncher;

void Reader_init(void);
inline void Reader_tick(void);
void Reader_service(void);
inline unsigned char Reader_next(unsigned char nStep);
inline void Reader_start(signed char nCount, unsigned char nContinuous);
void Reader_checkButton(void);

inline unsigned char Reader_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);

void Reader_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs);
void Reader_cmd(unsigned char idBuffer, unsigned char *pArgs);
