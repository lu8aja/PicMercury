/* 
 * File:   service_program.h
 * Author: Javier
 *
 * Created on March 1, 2017, 4:46 PM
 */

#define LIB_PROGRAM

typedef struct {
    // Configs
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Run;          // Program number being run
    unsigned long Time;         // Default program step time (it can be changed via the wait cmd)
    // Runtime
    unsigned char Step;         // Current step in sequence
    unsigned long Tick;         // Tick counter in ms Time..0
} program_t;


extern program_t MasterProgram;


inline void Program_init(void);

inline void Program_tick(void);

inline void Program_service(void);

void Program_cmd(unsigned char *pArgs);