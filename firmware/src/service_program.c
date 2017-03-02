
#define LIB_PROGRAM

#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_programs.h"

#include "app_globals.h"
#include "app_helpers.h"
#include "app_main.h"


typedef struct {
    // Configs
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Run;          // Program number being run
    unsigned long Time;         // Default program step time (it can be changed via the wait cmd)
    // Runtime
    unsigned char Step;         // Current step in sequence
    unsigned long Tick;         // Tick counter in ms Time..0
} program_t;


program_t MasterProgram;


inline void Program_init(void){
    // Configs
    MasterProgram.Enabled = 0;    // 0 = Off / 1 = On / 2 = Start Program
    MasterProgram.Run     = 0;    // Program number being run
    MasterProgram.Time    = 1000; // Default program step time (it can be changed via the wait cmd)
    // Runtime
    MasterProgram.Tick    = 0;    // Current step in sequence
    MasterProgram.Step    = 0;    // Tick counter in ms Time..0
}


inline void Program_tick(void){
    if (MasterProgram.Tick){
        MasterProgram.Tick--;
    }
}


inline void Program_service(void){
    if (MasterProgram.Enabled && !posCommand){
        if (MasterProgram.Enabled == 2){
            MasterProgram.Tick = 0;
        }

        if (!MasterProgram.Tick){
            const unsigned char *pProgram = MasterPrograms[MasterProgram.Run];
            pProgram += MasterProgram.Step;
            
            if (MasterProgram.Enabled == 2){
                MasterProgram.Step    = 0;
                MasterProgram.Enabled = 1;
            }
            
            if (!strlen(pProgram)){
                MasterProgram.Enabled = 0;
                printReply(3, "RUN", "Done!");
            }
            else {
                MasterProgram.Step += strlen(pProgram) + 1;
            }
            
            // Next tone
            if (MasterProgram.Enabled){
                MasterProgram.Tick = MasterProgram.Time;

                sprintf(sReply, "#%02u s:%02u T:%02u", 
                    MasterProgram.Run,
                    MasterProgram.Step,
                    MasterProgram.Tick);

                printReply(3, "RUN", sReply);

                strcpy(bufCommand, pProgram);
                APP_executeCommand();

            }
        }
    }
}



void Program_cmd(unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char n = 0;
    unsigned char s = 0;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        n = atoi(pArg1);
        if (n >= MasterProgramsLen){
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);
        }
        else{
            if (bOK){
                MasterProgram.Tick    = 0;
                MasterProgram.Step    = 0;
                MasterProgram.Run     = n;
                MasterProgram.Enabled = 2;
            }
        }
    }

    printReply(bOK, "RUN", sReply);
}