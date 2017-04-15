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

// These helpers should be available everywhere
#include "lib_helpers.h"

#include "app_io.h"

/********************************************************************
 MAIN GLOBALS USED BY APP
 *******************************************************************/

/** CONSTANTS ******************************************************/
extern const char txtVersion[];
extern const char txtCrLf[];
extern const char txtNum[];
extern const char txtWhitespace[];

extern const char txtOk[];
extern const char txtOn[];
extern const char txtOff[];
extern const char txtSpc[];
extern const char txtSep[];

extern const char txtCmdConfig[];

extern const char txtErrorMissingCommand[];
extern const char txtErrorUnknownCommand[];
extern const char txtErrorUnknownArgument[];
extern const char txtErrorUnknownPin[];
extern const char txtErrorInvalidArgument[];
extern const char txtErrorMissingArgument[];
extern const char txtErrorTooBig[];
extern const char txtErrorBusy[];
extern const char txtErrorCorrupt[];


/*** SYSTEM CLOCK */
// Timer0 setup (Clock 1:2 => once every 1/23 ms aprox)
#define       System_ClockTimer     0b01000000  // [0]Off, [1]8bit, [0]CLKO, [0]L2H, [0]PreOn, [000]1:2
#define       System_ClockTickCount 23          // Number of ticks per ms

typedef struct {
    unsigned char Tick     ;     // Tick counter 0,1,2
    unsigned long MS       ;     // MS counter, good for up to over a month
    unsigned int  NotifyCounter ;// Notification timer counter    
    unsigned int  NotifyTime;    // When to notify (1 minute))
    unsigned long NotifyNow;     // When not 0, the main loop will notify
    
} Clock_t;

typedef struct {
    union {
        unsigned char Configs;
        struct {
            unsigned Debug:1;    
            unsigned Reset: 1;
        } Config;
    };
    union {
        unsigned char Errors;
        struct {
            unsigned Heap:1;    
            unsigned UsbInput:1;
            unsigned UsbOutput:1;
            unsigned I2cInput:1;
            unsigned I2cOutput:1;
            unsigned SoftSerialInput:1;
            unsigned SoftSerialOutput:1;
            unsigned Tape:1;
        } Error;
    };
    Clock_t Clock;
    Ring_t * Buffers[8];
} System_t;


extern System_t System;

/*** Buffer sizes ***/
#define sizeChunk      2
#define sizeCommand   73
#define sizeReply    100
#define sizeStr       17

#define I2C_sizeInput   sizeCommand
#define I2C_sizeOutput  sizeCommand

#ifdef DEVICE_PUNCHER
    #define HEAP_Size             410    // Remember that the ring structs themselves waste about 6 bytes
    #define SoftSerial_sizeInput  sizeCommand
    #define SoftSerial_sizeOutput sizeCommand
    #define Puncher_sizeOutput    sizeCommand
    #define sizeOutput            150
    #define sizeOutUsb            60
#endif

#ifdef DEVICE_CONSOLE
    #define HEAP_Size             210    // Remember that the ring structs themselves waste about 6 bytes
    #define sizeOutput            300
    #define sizeOutUsb            150
#endif


#ifdef DEVICE_READER
    #define HEAP_Size             210    // Remember that the ring structs themselves waste about 6 bytes
    #define sizeOutput            300
    #define sizeOutUsb            150
#endif


/*** Buffers ***/
extern unsigned char  bufChunk[sizeChunk];
extern unsigned char  bufUsbOutput[sizeOutput + 1];
extern unsigned char  bufUsbCommand[sizeCommand];
extern unsigned char  bufCommand[sizeCommand];
extern unsigned char  bufUsbTmp[sizeOutUsb + 1];
extern unsigned char  sReply[sizeReply];
extern unsigned char  sStr1[sizeStr];
extern unsigned char  sStr2[sizeStr];
extern unsigned char  sStr3[sizeStr];
extern unsigned char  sStr4[sizeStr];
extern unsigned char  sStr5[sizeStr * 4];

/*** Cursors ***/
extern unsigned int   posOutput;
extern unsigned char  posUsbCommand;
