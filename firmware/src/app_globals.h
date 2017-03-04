/* 
 * File:   app_globals.h
 * Author: Javier
 *
 * Created on February 24, 2017, 12:10 AM
 */


/********************************************************************
 MAIN GLOBALS USED BY APP
 *******************************************************************/

/** MACROS ******************************************************/
/* Bit Operation macros */
#define setbit(b,n)        ( b |=   (1 << n))        /* Set bit number n in byte b   */
#define clearbit(b,n)      ( b &= (~(1 << n)))       /* Clear bit number n in byte b */
#define readbit(b,n)       ((b  &   (1 << n)) >> n)  /* Read bit number n in byte b  */
#define flipbit(b,n)       ( b ^=   (1 << n))        /* Flip bit number n in byte b  */

#define reciprocal(a, fp)  ( (( 1 << fp) + a - 1) / a )  /* Reciprocal 1/x without using floats */

#define bit_is_set(b,n)    (b   & (1 << n))     /* Test if bit number n in byte b is set   */
#define bit_is_clear(b,n)  (!(b & (1 << n)))    /* Test if bit number n in byte b is clear */


/** CONSTANTS ******************************************************/
extern const char txtVersion[];
extern const char txtCrLf[];
extern const char txtNum[];
extern const char txtWhitespace[];

extern const char txtOn[];
extern const char txtOff[];

extern const char txtErrorMissingCommand[];
extern const char txtErrorUnknownCommand[];
extern const char txtErrorUnknownArgument[];
extern const char txtErrorUnknownPin[];
extern const char txtErrorInvalidArgument[];
extern const char txtErrorMissingArgument[];
extern const char txtErrorTooBig[];
extern const char txtErrorBusy[];

/*** MASTER DEBUG ***/
extern unsigned char MasterDebug         ;     // 

/*** MASTER CLOCK */
// Timer0 setup (Clock 1:2 => once every 1/23 ms aprox)
#define       MasterClockTimer 0b01000000  // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [000]1:2
#define       MasterClockTickCount  23         // Number of ticks per ms
extern unsigned char MasterClockTick     ;     // Tick counter 0,1,2
extern unsigned long MasterClockMS       ;     // MS counter, good for up to over a month

/*** MASTER NOTIFY ***/
extern unsigned int  MasterNotifyCounter ;     // Notification timer counter    
extern unsigned int  MasterNotify        ;     // When to notify (1 minute))
extern unsigned long MasterNotifyNow     ;     // When not 0, the main loop will notify

// USB buffers
#define sizeChunk      4
#define sizeOutput   300
#define sizeOutUsb   120
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

#define I2C_sizeInput 32

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

extern unsigned int   posOutput;
extern unsigned char  posCommand;

// Console
typedef struct {
	unsigned usb:1;
	unsigned connected:1;
	unsigned notify:1;
	unsigned reportOnce:1;
	unsigned bufferOverrun:1;
} console_t;

extern console_t MasterConsoleStatus;
