
/********************************************************************
 MAIN GLOBALS USED BY APP
 *******************************************************************/


/** CONSTANTS ******************************************************/
extern const char txtCrLf[];
extern const char txtNum[];
extern const char txtWhitespace[];

extern const char txtOkSp[];
extern const char txtOn[];
extern const char txtOff[];

extern const char txtErrorSp[];


extern const char txtErrorUnknownCommand[];
extern const char txtErrorUnknownArgument[];
extern const char txtErrorUnknownPin[];
extern const char txtErrorInvalidArgument[];

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
#define       MasterLedTime         1500  // Ticks to count between
extern unsigned char MasterLedStep;
extern unsigned int  MasterLedStepTime;
extern unsigned int  MasterLedStepTick;
extern unsigned char MasterLedStepEnabled;
extern unsigned char MasterLedStepRestart;

// MASTER TONES
extern unsigned char MasterToneEnabled;     //
extern unsigned char MasterTonePeriod;     //
extern unsigned char MasterToneTick;     //
extern unsigned long MasterToneTime;     //
extern unsigned long MasterToneCounter;     //
extern unsigned long MasterToneTempo;   //
extern unsigned char MasterTonePitch;     //
extern unsigned char MasterToneRestart;     //
extern unsigned char MasterToneStep;     //
extern unsigned char MasterToneMode;

// USB buffers
#define sizeChunk      4
#define sizeOutput   255
#define sizeCommand   64
#define sizeReply    100
#define sizeStr       17

extern unsigned char  bufChunk[];
extern unsigned char  bufOutput[];
extern unsigned char  bufCommand[];
extern unsigned char  sReply[];
extern unsigned char  sStr1[];
extern unsigned char  sStr2[];
extern unsigned char  sStr3[];
extern unsigned char  sStr4[];
extern unsigned char  sStr5[];

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
