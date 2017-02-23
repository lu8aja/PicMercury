
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

/** VARIABLES ******************************************************/

extern unsigned char MasterDebug;

// MASTER CLOCK
#define       MasterClockTickCount  23     /* Number of ticks per ms */
extern unsigned char MasterClockTick;      // Tick counter 0,1,2
extern unsigned long MasterClockMS;        // MS counter, good for up to over a month
// MASTER NOTIFY
extern unsigned int  MasterNotifyCounter;  // Notification timer counter    
extern unsigned int  MasterNotify;         // When to notify (1 minute))
extern unsigned long MasterNotifyNow;      // When not 0, the main loop will notify
// MASTER LEDS
extern unsigned int  MasterLedStatus;      // Bitfield of 16 leds statuses
extern unsigned int  MasterLedCounter;     // Tick Counter
#define       MasterLedTime         500    // Ticks to count between
extern unsigned char MasterLedStep;
extern unsigned int  MasterLedStepTime;
extern unsigned int  MasterLedStepTick;
extern unsigned char MasterLedStepEnabled;
extern unsigned char MasterLedStepRestart;

// MASTER TONES
// Configs
extern unsigned char MasterToneEnabled;  // 0 = Off / 1 = On / 2 = Start Music
extern unsigned char MasterToneMode;     // 0 = Single tone / 1 = Music
extern unsigned int  MasterToneTempo;    // For music: Time per beat in ms
extern signed   char MasterTonePitch;    // For music: Number of notes to shift the MIDI note
extern unsigned char MasterToneRestart;  // 0 = Disable tones when Time is reached / 1 = Restart either music from step 1 or steady tone
extern unsigned char MasterTonePeriod;   // Current tone semi-period
extern unsigned int  MasterToneTime;     // Current note duration in ms
// Runtime
extern unsigned char MasterToneTick;     // Current tick counter (between wave inversions) 0 .. Period
extern unsigned int  MasterToneCounter;  // Current note time counter 0 .. Time
extern unsigned char MasterToneStep;     // Music Step (current note)

// USB buffers
#define sizeChunk      4
#define sizeOutput   255
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

extern unsigned char  bufChunk[sizeChunk];
extern unsigned char  bufOutput[sizeOutput + 1];
extern unsigned char  bufCommand[sizeCommand];
extern unsigned char  sReply[sizeReply];
extern unsigned char  sStr1[sizeStr];
extern unsigned char  sStr2[sizeStr];
extern unsigned char  sStr3[sizeStr];
extern unsigned char  sStr4[sizeStr];
extern unsigned char  sStr5[sizeStr * 4];

extern unsigned char  posOutput;
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
