
#include "flags.h"

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
const char txtSpc[]       = " ";
const char txtSep[]       = " / ";

const char txtErrorMissingCommand[]  = "Missing command";
const char txtErrorUnknownCommand[]  = "Unknown command";
const char txtErrorUnknownArgument[] = "Unknown argument";
const char txtErrorUnknownPin[]      = "Unknown pin";
const char txtErrorInvalidArgument[] = "Invalid argument";
const char txtErrorMissingArgument[] = "Missing argument";
const char txtErrorTooBig[]          = "Argument too big";
const char txtErrorBusy[]            = "Busy";
const char txtErrorCorrupt[]         = "Corrupt"; 


/** VARIABLES ******************************************************/
/*** MASTER DEBUG ***/
unsigned char MasterDebug         = 0;     // 
//unsigned char MasterDebugMsg[16]  = "";

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

Clock_t MasterClock;



Clock_t MasterClock;

/*** Buffer sizes ***/
#define sizeChunk      4
#define sizeOutput   300
#define sizeOutUsb    80
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

#define I2C_sizeInput 32

#ifdef DEVICE_PUNCHER
    #define HEAP_SIZE             100    // Remember that the ring structs themselves waste about 6 bytes
    #define SOFTSERIAL_sizeOutput 32
    #define SOFTSERIAL_sizeInput  32
#endif

#ifdef DEVICE_CONSOLE
    #define HEAP_SIZE             100    // Remember that the ring structs themselves waste about 6 bytes
#endif

#ifndef HEAP_SIZE
    #define HEAP_SIZE             100
#endif


/*** Buffers ***/
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

/*** Cursors ***/
unsigned int   posOutput       = 0;
unsigned char  posCommand      = 0;



