/* 
 * File:   service_program.h
 * Author: Javier
 *
 * Created on March 1, 2017, 4:46 PM
 */

#include <xc.h>
#include <string.h>

#include "app_globals.h"
#include "app_io.h"
#include "app_main.h"

#define LIB_PROGRAM

typedef struct {
    // Configs
    union {
        unsigned char Status;    // Status
        struct {
            unsigned Enabled:1;  // [01] Module Enabled
            unsigned Type:1;     // [02] 0: String / 1: PIC
            unsigned Console:1;  // [04] 0: Command line / 1: Console Keyboard
        };
    };
    unsigned char Run;           // Program number being run
    unsigned int  Function;      // Runtime value for Function keys, used when running programs from console
    unsigned int  Address;       // Runtime value for Address keys, used when running programs from console
    unsigned int  Input;         // Runtime value for Input keys
    unsigned int  Switches;      // Runtime value for misc console switches
    unsigned long Time;          // Default program step time (it can be changed via the wait cmd)
    // Runtime
    unsigned char Step;          // Current step in sequence
    unsigned long Tick;          // Tick counter in ms Time..0
} program_t;


extern program_t MasterProgram;


inline void          Program_init(void);

inline void          Program_tick(void);

inline void          Program_service(void);

inline unsigned char Program_run(unsigned char nPrg, unsigned char nConsole);

inline unsigned char Program_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);

void                 Program_cmd(unsigned char idBuffer, unsigned char *pArgs);