
#include "flags.h"

/** MACROS ******************************************************/
/* Bit Operation macros */
#define setbit(b,n)   ( b |=   (1 << n))        /* Set bit number n in byte b   */
#define clearbit(b,n) ( b &= (~(1 << n)))       /* Clear bit number n in byte b */
#define readbit(b,n)  ((b  &   (1 << n)) >> n)  /* Read bit number n in byte b  */
#define flipbit(b,n)  ( b ^=   (1 << n))        /* Flip bit number n in byte b  */

#define reciprocal(a, fp)  ( (( 1 << fp) + a - 1) / a )  /* Reciprocal 1/x without using floats */

#define bit_is_set(b,n)   (b   & (1 << n))     /* Test if bit number n in byte b is set   */
#define bit_is_clear(b,n) (!(b & (1 << n)))    /* Test if bit number n in byte b is clear */

/** CONSTANTS ******************************************************/

#if defined(DEVICE_CONSOLE)
    const char txtVersion[]    = "0.2.2 Console";
#elif defined(DEVICE_PUNCHER)
    const char txtVersion[]    = "0.2.2 Puncher";
#elif defined(DEVICE_READER)
    const char txtVersion[]    = "0.2.2 Reader";
#elif defined(DEVICE_CRTS)
    const char txtVersion[]    = "0.2.2 CRTs";
#else // Some other unknown device!?
    const char txtVersion[]    = "0.2.2 Unknown";
#endif


const char txtCrLf[]       = "\r\n";
//const char txtAlphaLC[]    = "abcdefghijklmnopqrstuvwxyz";
//const char txtAlphaUC[]    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//const char txtAlpha[]      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char txtNum[]        = "0123456789";
//const char txtAlphaNum[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
const char txtWhitespace[] = " \t\r\n";

const char txtOn[]        = "On";
const char txtOff[]       = "Off";

const char txtErrorMissingCommand[]  = "Missing command";
const char txtErrorUnknownCommand[]  = "Unknown command";
const char txtErrorUnknownArgument[] = "Unknown argument";
const char txtErrorUnknownPin[]      = "Unknown pin";
const char txtErrorInvalidArgument[] = "Invalid argument";
const char txtErrorMissingArgument[] = "Missing argument";
const char txtErrorTooBig[]          = "Argument too big";
const char txtErrorBusy[]            = "Busy";



/** VARIABLES ******************************************************/
/*** MASTER DEBUG ***/
unsigned char MasterDebug         = 0;     // 

/*** MASTER CLOCK */

// Timer0 setup (Clock 1:2 => once every 1/23 ms aprox)
#define       MasterClockTimer 0b01000000  // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [000]1:2
#define       MasterClockTickCount  23     // Number of ticks per ms
unsigned char MasterClockTick     = 23;    // Tick counter 0,1,2
unsigned long MasterClockMS       = 0;     // MS counter, good for up to over a month

/*** MASTER NOTIFY ***/
unsigned int  MasterNotifyCounter = 0;     // Notification timer counter    
unsigned int  MasterNotify        = 60000; // When to notify (1 minute))
unsigned long MasterNotifyNow     = 0;     // When not 0, the main loop will notify


/*** USB buffers ***/
#define sizeChunk      4
#define sizeOutput   300
#define sizeOutUsb   120
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

unsigned char  bufChunk[sizeChunk];
unsigned char  bufOutput[sizeOutput + 1];
unsigned char  bufCommand[sizeCommand]   = "";
unsigned char  bufTmp[sizeOutUsb + 1];
unsigned char  sReply[sizeReply];
unsigned char  sStr1[sizeStr];
unsigned char  sStr2[sizeStr];
unsigned char  sStr3[sizeStr];
unsigned char  sStr4[sizeStr];
unsigned char  sStr5[sizeStr * 4];

unsigned int   posOutput       = 0;
unsigned char  posCommand      = 0;

/*** Console ***/
typedef struct {
	unsigned usb:1;
	unsigned connected:1;
	unsigned notify:1;
	unsigned reportOnce:1;
	unsigned bufferOverrun:1;
} console_t;

console_t MasterConsoleStatus = {0, 0, 0, 0, 0};

