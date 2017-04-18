
#include "app_globals.h"

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

const char txtOk[]        = "OK";
const char txtOn[]        = "On";
const char txtOff[]       = "Off";
const char txtSpc[]       = " ";
const char txtSep[]       = " / ";

const char txtCmdConfig[] = "cfg";

const char txtErrorMissingCommand[]  = "Missing command";
const char txtErrorUnknownCommand[]  = "Unknown command";
const char txtErrorUnknownArgument[] = "Unknown argument";
const char txtErrorInvalidArgument[] = "Invalid argument";
const char txtErrorMissingArgument[] = "Missing argument";
const char txtErrorTooBig[]          = "Too big";
const char txtErrorBufferFull[]      = "Buffer full";
const char txtErrorUnknownPin[]      = "Unknown pin";
const char txtErrorBusy[]            = "Busy";
const char txtErrorCorrupt[]         = "Corrupt"; 


/** VARIABLES ******************************************************/

System_t System;

/*** Buffers ***/
unsigned char  bufChunk[sizeChunk]          = "";

unsigned char  bufUsbOutput[sizeOutput + 1] = "";
unsigned char  bufUsbCommand[sizeCommand]   = "";
unsigned char  bufUsbTmp[sizeOutUsb + 1]    = "";

unsigned char  bufCommand[sizeCommand]      = "";

unsigned char  sReply[sizeReply]  = "";

unsigned char  sStr1[sizeStr]     = "";
unsigned char  sStr2[sizeStr]     = "";
unsigned char  sStr3[sizeStr]     = "";
unsigned char  sStr4[sizeStr]     = "";
unsigned char  sStr5[sizeStr * 4] = "";

/*** Cursors ***/
unsigned int   posOutput       = 0;
unsigned char  posUsbCommand   = 0;



