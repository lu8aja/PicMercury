/* 
 * File:   app_leds.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:08 PM
 */

#define LIB_LEDS

#define Leds_Debug 0

typedef struct {
    // Configs
    union {
        unsigned char Configs;
        struct {
            unsigned Enabled:1;      // 0 = Off / 1 = On
            unsigned Debug:1;        // 0 = Off / 1 = On
            unsigned AlarmOverflow:1;// 
            unsigned AlarmUsb:1;     // 
            unsigned StepEnabled:1;  // 0 = Off / 1 = On
            unsigned StepRestart:1;  // 0 = Disable when Time is reached / 1 = Restart 
        };
    };

    unsigned char Time;         // Ticks to count while flashing led
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
inline unsigned char Leds_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);
void        Leds_cmd(unsigned char idBuffer, unsigned char *pArgs);



