
/********************************************************************
 MAIN GLOBALS USED BY APP
 *******************************************************************/


/** CONSTANTS ******************************************************/
extern const char txtVersion[];
extern const char txtCrLf[];
extern const char txtNum[];
extern const char txtWhitespace[];

extern const char txtOn[];
extern const char txtOff[];

extern const char txtErrorUnknownCommand[];
extern const char txtErrorUnknownArgument[];
extern const char txtErrorUnknownPin[];
extern const char txtErrorInvalidArgument[];
extern const char txtErrorMissingArgument[];

/*** MASTER DEBUG ***/
extern unsigned char MasterDebug         ;     // 

/*** MASTER CLOCK */
#define       MasterClockTickCount  23         // Number of ticks per ms
extern unsigned char MasterClockTick     ;     // Tick counter 0,1,2
extern unsigned long MasterClockMS       ;     // MS counter, good for up to over a month

/*** MASTER NOTIFY ***/
extern unsigned int  MasterNotifyCounter ;     // Notification timer counter    
extern unsigned int  MasterNotify        ;     // When to notify (1 minute))
extern unsigned long MasterNotifyNow     ;     // When not 0, the main loop will notify

/*** MASTER LEDS ***/
// Configs
#define       MasterLedTime         500        // Ticks to count while flashing led
extern unsigned char MasterLedStepEnabled;     // 0 = Off / 1 = On / 2 = Start Music
extern unsigned char MasterLedStepRestart;     // 0 = Disable when Time is reached / 1 = Restart 
extern unsigned int  MasterLedStepTime   ;     // Time for each step
// Runtime
extern unsigned int  MasterLedStatus     ;     // Bitfield of 16 leds statuses
extern unsigned int  MasterLedCounter    ;     // Flashing tick period counter
extern unsigned int  MasterLedStepTick   ;     // Tick counter for each step 0..Time
extern unsigned char MasterLedStep       ;     // Current step in sequence
/*** MASTER TONES ***/
// Configs
extern unsigned char MasterToneEnabled   ;     // 0 = Off / 1 = On / 2 = Start Music
extern unsigned char MasterToneMode      ;     // 0 = Single tone / 1 = Music
extern unsigned int  MasterToneTempo     ;     // For music: Time per beat in ms
extern signed   char MasterTonePitch     ;     // For music: Number of notes to shift the MIDI note
extern unsigned char MasterToneRestart   ;     // 0 = Disable when Time is reached / 1 = Restart either music from step 0 or steady tone
extern unsigned char MasterTonePeriod    ;     // Current tone semi-period
extern unsigned int  MasterToneTime      ;     // Current note duration in ms
// Runtime
extern unsigned char MasterToneTick      ;     // Current tick counter (between wave inversions) 0 .. Period
extern unsigned int  MasterToneCounter   ;     // Current note time counter 0 .. Time
extern unsigned char MasterToneStep      ;     // Music Step (current note)

/*** MASTER KEYS ***/
extern unsigned int  MasterKeysFunction  ;      // Runtime value for Function keys
extern unsigned int  MasterKeysAddress   ;      // Runtime value for Address keys
extern unsigned int  MasterKeysInput     ;      // Runtime value for Input keys
extern unsigned int  MasterKeysSwitches  ;      // Runtime value for misc console switches

/*** MASTER BUTTONS ***/
extern unsigned int  MasterButtonsTick   ;      // Tick counter (runtime)
extern unsigned int  MasterButtonsBit    ;      // Bit pointer 0..7
#define       MasterButtonsTime      30         // Buttons check interval (ms)
extern unsigned int  MasterButtonsRunEnabled;   // Allows running programs from keyboard using the prepulse button and the address row

/*** MASTER PROGRAM ***/
// Configs
extern unsigned long MasterProgramTime    ;     // Default program step time (it can be changed via the wait cmd))
extern unsigned int  MasterProgramRun     ;     // Program number being run
extern unsigned int  MasterProgramEnabled ;     // 0 = Off / 1 = On / 2 = Start Program
// Runtime
extern unsigned long MasterProgramTick    ;     // Tick counter (runtime)
extern unsigned int  MasterProgramStep    ;     // Step counter/pointer (runtime)



// USB buffers
#define sizeChunk      4
#define sizeOutput   512
#define sizeOutUsb   120
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

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

// Status of IO
extern unsigned char nStatus_PortA;
extern unsigned char nStatus_PortB;
extern unsigned char nStatus_PortC;
extern unsigned char nStatus_PortD;
extern unsigned char nStatus_PortE;

extern unsigned char nStatus_TrisA;
extern unsigned char nStatus_TrisB;
extern unsigned char nStatus_TrisC;
extern unsigned char nStatus_TrisD;
extern unsigned char nStatus_TrisE;

extern unsigned char nStatus_MonitorA;
extern unsigned char nStatus_MonitorB;
extern unsigned char nStatus_MonitorC;
extern unsigned char nStatus_MonitorD;
extern unsigned char nStatus_MonitorE;


// Console
typedef struct {
	unsigned usb:1;
	unsigned connected:1;
	unsigned notify:1;
	unsigned reportOnce:1;
	unsigned bufferOverrun:1;
} console_t;

extern console_t MasterConsoleStatus;

void APP_init();
void APP_main();
void APP_usbConfigured(void);
void APP_getStatusReply(void);
void APP_getKeys(void);
void APP_outputUsb(void);
