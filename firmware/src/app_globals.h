/* 
 * File:   app_globals.h
 * Author: Javier
 *
 * Created on February 24, 2017, 12:10 AM
 */

// Device specific flags, coming from the board subfolders
#include "flags.h"

// Ring & Heap libraries
#include "lib_ring.h"

// Thes ehelpers should be available everywhere
#include "lib_helpers.h"

/********************************************************************
 MAIN GLOBALS USED BY APP
 *******************************************************************/

/** MACROS ******************************************************/
/* Bit Operation macros */
#define bit_set(A,n)           ( A |=   (1 << n))                            // Set bit number n in byte b 
#define bit_clear(A,n)         ( A &= (~(1 << n)))                           // Clear bit number n in byte b
#define bit_flip(A,n)          ( A ^=   (1 << n))                            // Flip bit number n in byte b 
#define bit_read(A,n)          ((A  &   (1 << n)) >> n)                      // Read bit number n in byte b 
#define bit_write(A,n,v)       (v ? (A |= (1 << n)) : (A &= (~((1 << n)))))  // Write value v to bit n of A 
#define bit_mask_write(A,m,v)  (v ? (A |= m) : (A &= (~m)))                  // According to v set or clear based upon a mask
#define bit_is_set(A,n)        (A   & (1 << n))                              // Test if bit number n in byte b is set
#define bit_is_clear(A,n)      (!(A & (1 << n)))                             // Test if bit number n in byte b is clear
#define reciprocal(a, fp)      ( (( 1 << fp) + a - 1) / a )                  // Reciprocal 1/x without using floats

/** CONSTANTS ******************************************************/
extern const char txtVersion[];
extern const char txtCrLf[];
extern const char txtNum[];
extern const char txtWhitespace[];

extern const char txtOn[];
extern const char txtOff[];
extern const char txtSpc[];
extern const char txtSep[];

extern const char txtErrorMissingCommand[];
extern const char txtErrorUnknownCommand[];
extern const char txtErrorUnknownArgument[];
extern const char txtErrorUnknownPin[];
extern const char txtErrorInvalidArgument[];
extern const char txtErrorMissingArgument[];
extern const char txtErrorTooBig[];
extern const char txtErrorBusy[];
extern const char txtErrorCorrupt[];

/*** MASTER DEBUG ***/
extern unsigned char MasterDebug;     // 
//unsigned char MasterDebugMsg[];

/*** MASTER CLOCK */
// Timer0 setup (Clock 1:2 => once every 1/23 ms aprox)
#define       MasterClockTimer 0b01000000  // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [000]1:2
#define       MasterClockTickCount  23         // Number of ticks per ms

typedef struct {
    unsigned char Tick     ;     // Tick counter 0,1,2
    unsigned long MS       ;     // MS counter, good for up to over a month
    /*** MASTER NOTIFY ***/
    unsigned int  NotifyCounter ;// Notification timer counter    
    unsigned int  NotifyTime;    // When to notify (1 minute))
    unsigned long NotifyNow;     // When not 0, the main loop will notify
    
} Clock_t;

extern Clock_t MasterClock;

/*** Buffer sizes ***/
#define sizeChunk      4
#define sizeOutput   200
#define sizeOutUsb   100
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

#define I2C_sizeInput   40
#define I2C_sizeOutput  40

#ifdef DEVICE_PUNCHER
    #define HEAP_Size             200    // Remember that the ring structs themselves waste about 6 bytes
    #define SOFTSERIAL_sizeOutput 45
    #define SOFTSERIAL_sizeInput  45
#endif


#ifdef DEVICE_CONSOLE
    #define HEAP_Size             210    // Remember that the ring structs themselves waste about 6 bytes
#endif


/*** Buffers ***/
extern unsigned char  bufChunk[sizeChunk];
extern unsigned char  bufOutput[sizeOutput + 1];
extern unsigned char  bufCommand[sizeCommand];
extern unsigned char  bufTmp[sizeOutUsb + 1];
extern unsigned char  sReply[sizeReply];
extern unsigned char  sStr1[sizeStr];
extern unsigned char  sStr2[sizeStr];
extern unsigned char  sStr3[sizeStr];
extern unsigned char  sStr4[sizeStr];
extern unsigned char  sStr5[sizeStr * 4];

/*** Cursors ***/
extern unsigned int   posOutput;
extern unsigned char  posCommand;
