
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
    MasterProgram.Enabled = 1;    // 0 = Off / 1 = On / 2 = Start Program
    MasterProgram.Run     = 4;    // Program number being run
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
    if (MasterProgram.Enabled && !posCommand){
        if (MasterProgram.Enabled == 2){
            MasterProgram.Tick = 0;
        }

        if (!MasterProgram.Tick){
            if (MasterProgram.Type == 0){
                const unsigned char *pProgram = MasterPrograms[MasterProgram.Run];
                pProgram += MasterProgram.Step;

                if (MasterProgram.Enabled == 2){
                    MasterProgram.Step    = 0;
                    MasterProgram.Enabled = 1;
                }

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

                    strcpy(bufCommand, pProgram);
                    APP_executeCommand(0, bufCommand);
                    bufCommand[0] = 0;
                }
            }
            else{
                if (MasterProgram.Enabled == 2){
                    MasterProgram.Step    = 0;
                    MasterProgram.Enabled = 1;
                }
                
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
    }
}





inline unsigned char Program_checkCmd(Ring_t * pBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "run")){
        Program_cmd(pBuffer, pArgs);
        return 1;
    }
    else if (strequal(pCommand, "delay") || strequal(pCommand, "d")){
        MasterProgram.Tick = atol(pArgs);
        return 1;
    }
    return 0;
}



void Program_cmd(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char n = 0;

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
            if (n == 0x80 || n == 0x40 || n == 0x20){
                MasterProgram.Tick    = 0;
                MasterProgram.Step    = 0;
                MasterProgram.Run     = n;
                MasterProgram.Type    = 1;
                MasterProgram.Enabled = 2;
            }
            else{
                bOK = false;
                strcat(sReply, txtErrorInvalidArgument);
            }
        }
        else{
            if (bOK){
                MasterProgram.Tick    = 0;
                MasterProgram.Step    = 0;
                MasterProgram.Run     = n;
                MasterProgram.Type    = 0;
                MasterProgram.Enabled = 2;
            }
        }
    }

    printReply(pBuffer, bOK, "RUN", sReply);
}