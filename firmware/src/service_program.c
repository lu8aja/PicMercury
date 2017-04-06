
#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "service_program.h"
#include "app_programs.h"

program_t MasterProgram;


inline void Program_init(void){
    // Configs
    MasterProgram.Enabled = 0;    // 0 = Off / 1 = On / 2 = Start Program
    MasterProgram.Run     = 0;    // Program number being run
    MasterProgram.Time    = 1000; // Default program step time (it can be changed via the wait cmd)
    // Runtime
    MasterProgram.Tick    = 0;    // Tick counter in ms Time..0
    MasterProgram.Step    = 0;    // Current step in sequence
}


inline void Program_tick(void){
    if (MasterProgram.Tick){
        MasterProgram.Tick--;
    }
}


inline void Program_service(void){
    if (!MasterProgram.Enabled || MasterProgram.Tick){
        return;
    }
    if (MasterProgram.Type == 0){
        const unsigned char *pProgram = MasterPrograms[MasterProgram.Run];
        pProgram += MasterProgram.Step;

        if (!strlen(pProgram)){
            MasterProgram.Enabled = 0;
            printReply(0, 3, "RUN", "Done!");
        }
        else {
            MasterProgram.Step += strlen(pProgram) + 1;
        }

        // Next tone
        if (MasterProgram.Enabled){
            MasterProgram.Tick = MasterProgram.Time;

            sprintf(sReply, "#%02u s:%02u T:%02lu", 
                MasterProgram.Run,
                MasterProgram.Step,
                MasterProgram.Tick);

            printReply(0, 3, "RUN", sReply);

            strcpy(bufUsbCommand, pProgram);
            APP_executeCommand(0, bufUsbCommand);
            bufUsbCommand[0] = 0;
        }
    }
    else{
        if (MasterProgram.Run == 0x80){
            MasterProgram.Step = Program_custom_isPrime(MasterProgram.Step);
        }
        else if (MasterProgram.Run == 0x40){
            MasterProgram.Step = Program_custom_calcPrimes(MasterProgram.Step);
        }
        else if (MasterProgram.Run == 0x20){
            MasterProgram.Step = Program_custom_doMusic(MasterProgram.Step);
        }

        if (!MasterProgram.Step){
            MasterProgram.Enabled = 0;
        }
        else{
            MasterProgram.Tick = MasterProgram.Time;
        }

    }
}

inline unsigned char Program_run(unsigned char nPrg, unsigned char nConsole){
    
    if (nPrg != 0x80 && nPrg != 0x40 && nPrg != 0x20 && nPrg >= MasterProgramsLen){
        return 0;
    }
    MasterProgram.Console  = nConsole;
    MasterProgram.Run      = nPrg;
    MasterProgram.Type     = 0;
    if (nPrg == 0x80 || nPrg == 0x40 || nPrg == 0x20){
        MasterProgram.Type = 1;
    }
    MasterProgram.Tick     = 0;
    MasterProgram.Step     = 0;
    MasterProgram.Enabled  = 1;
    return 1;
}



inline unsigned char Program_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "run")){
        Program_cmd(idBuffer, pArgs);
        return 1;
    }
    else if (strequal(pCommand, "delay") || strequal(pCommand, "d")){
        MasterProgram.Tick = atol(pArgs);
        return 1;
    }
    return 0;
}



void Program_cmd(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg = NULL;
    unsigned char nArg1 = 0;
    unsigned int  iArg2 = 0;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg = strtok(pArgs, txtWhitespace);
    if (!pArg){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        nArg1 = atoi(pArg);
        pArg = strtok(NULL, txtWhitespace);
        if (pArg){
            MasterProgram.Input = atoi(pArg);
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                MasterProgram.Address = atoi(pArg);
                pArg = strtok(NULL, txtWhitespace);
                if (pArg){
                    MasterProgram.Switches = atoi(pArg);
                }
            }
        }

        if (!Program_run(nArg1, 0)){
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);
        }
    }

    printReply(idBuffer, bOK, "RUN", sReply);
}