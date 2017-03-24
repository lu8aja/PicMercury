/* 
 * File:   app_leds.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:08 PM
 */

#define LIB_LEDS


typedef struct {
    // Configs
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Time;         // Ticks to count while flashing led
    unsigned char StepEnabled;  // 0 = Off / 1 = On
    unsigned char StepRestart;  // 0 = Disable when Time is reached / 1 = Restart 
    unsigned int  StepTime;     // Time in ms the step should be help in place
    
    // Runtime
    unsigned int  Status;       // Bitfield of 16 leds statuses
    unsigned char Step;         // Current step in sequence
    unsigned char Tick;         // Flashing tick period counter
    unsigned int  StepTick;     // Tick counter for each step 0..Time
} leds_t;


extern leds_t MasterLeds;

inline void Leds_init(void);
inline void Leds_tick(void);
inline void Leds_service(void);
void        Leds_updateLeds(void);
void        Leds_updateUsb(void);
inline unsigned char Leds_checkCmd(Ring_t * pBuffer, unsigned char pCommand, unsigned char *pArgs);
void        Leds_cmd(Ring_t * pBuffer, unsigned char *pArgs);



