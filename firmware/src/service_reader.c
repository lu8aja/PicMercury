#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "service_reader.h"

Reader_t Reader;

void Reader_init(void){


    Reader.Input = Transcoder_new(Reader_sizeInput);
    if (!Reader.Input){
        System.Error.Tape = 1;
    }
    
    Reader.Input->ModeBit6 = 1;
    Reader.Input->ModeIta2 = 1;
    
    READER_MOTOR_LAT  &= ~READER_MOTOR_MASK; // Init low (xxxxx000)    
    READER_MOTOR_TRIS &= ~READER_MOTOR_MASK; // Init outputs (xxxxx000)

    READER_INPUT_TRIS &= ~READER_INPUT_MASK;
    READER_Pin_EnableLeds = 0;
    
    Reader.Sequence[0] = 0;
    Reader.Sequence[1] = 2;
    Reader.Sequence[2] = 3;
    Reader.Sequence[3] = 1;
    
    Reader.Enabled     = 0;
    Reader.Time        = 8;
    Reader.Tick        = 0;
    Reader.State       = 0;
    Reader.Alignment   = 0;
}

inline void Reader_tick(void){
    if (Reader.Tick){
        Reader.Tick--;
    }
}

void Reader_service(void){
    unsigned char nChar;
    
    if (Reader.Tick || !Reader.Enabled || !Reader.Steps){
        return;
    }

    if (Reader.Debug){
        byte2binstr(sStr1, READER_INPUT_PORT);
        printf("\r\n%s ", sStr1);
    }
    //putch(READER_INPUT_InGuide ? 'o' : 'x');    
    if (Reader.Alignment == Reader.State){
        if (!READER_INPUT_InGuide){
            Reader.Chars++;
            
            nChar = READER_INPUT_PORT & 0b00011111;
            
            Transcoder_write(Reader.Input, nChar, !Reader.Store);
            
            if (Reader.Debug){
                byte2binstr(sStr1, READER_INPUT_PORT);
                printf("\r\n\t\t\t%s %02x [%c] ", sStr1, nChar, Reader.Input->Char < 32 ? '.' : Reader.Input->Char);
            }
            else if (Reader.EchoToUsb){
                putch(Reader.Input->Char);
            }
            
            if (Reader.Store && Reader.AutoStop && !ring_available(Reader.Input)){
                Reader.Steps = 0;
                if (Reader.Debug){
                    print("\r\nBuffer full\r\n");
                }
            }
            else if (Reader.Continuous){
                Reader.Steps = 1;
            }
            else{
                Reader.Steps--;
            }
            
            if (!Reader.Steps){
                Reader.Enabled = 0;
                READER_Pin_EnabledCoils = 0;
                unsigned long now = Clock_getTime();
                unsigned long delta = now - Reader.Start;

                printf("\r\nDONE %u in %lums = %u\r\n", Reader.Chars, delta, (unsigned int) ((((unsigned long) Reader.Chars) * 1000) / delta));
                return;
            }

        }
        else{
            // Guide hole not found, move alignment to the next
            if (Reader.Direction){
                putch('-');
                // Backwards
                if (Reader.Alignment){
                    Reader.Alignment--;
                }
                else{
                    Reader.Alignment = 3;
                }
            }
            else{
                putch('+');
                // Forward
                if (Reader.Alignment < 3){
                    Reader.Alignment++;
                }
                else{
                    Reader.Alignment = 0;
                }
            }
        }
    }
    
    // Next state
    if (Reader.Direction){
        // Backwards
        if (Reader.State){
            Reader.State--;
        }
        else{
            Reader.State = 3;
        }
    }
    else{
        // Forward
        if (Reader.State < 3){
            Reader.State++;
        }
        else{
            Reader.State = 0;
        }
    }
    READER_MOTOR_LAT = (READER_MOTOR_LAT & ~READER_MOTOR_MASK) | Reader.Sequence[Reader.State];
    READER_Pin_EnabledCoils = 1;
    Reader.Tick = Reader.Time;
}



inline unsigned char Reader_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "cfg reader")){
        Reader_cmd_cfg(idBuffer, pArgs);
        return 1;
    }
    if (strequal(pCommand, "reader")){
        Reader_cmd(idBuffer, pArgs);
        return 1;
    }
    return 0;
}


void Reader_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pVal  = NULL;
    unsigned char n = 0;
    unsigned char m = 0;

    sReply[0] = 0x00;

    str2lower(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    if (!pArg1){

    }
    else if (strequal(pArg1, "debug")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Debug = (unsigned char) atoi(pArg2);
        }
    }
    else if (strequal(pArg1, "time")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Time = (unsigned char) atoi(pArg2);
        }
    }
    else if (strequal(pArg1, "align")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Alignment = (unsigned char) atoi(pArg2);
        }
    }
    else if (strequal(pArg1, "autostop")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.AutoStop = strequal(pArg2, "on");
        }
    }
    else if (strequal(pArg1, "store")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Store = strequal(pArg2, "on");
        }
    }
    else if (strequal(pArg1, "echo")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.EchoToUsb = strequal(pArg2, "on");
        }
    }
    else if (strequal(pArg1, "seq")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Sequence[3] = (unsigned char) atoi(pArg2);
            pArg2 = strtok(NULL,  txtWhitespace);
        }
        if (pArg2){
            Reader.Sequence[2] = (unsigned char) atoi(pArg2);
            pArg2 = strtok(NULL,  txtWhitespace);
        }
        if (pArg2){
            Reader.Sequence[1] = (unsigned char) atoi(pArg2);
            pArg2 = strtok(NULL,  txtWhitespace);
        }
        if (pArg2){
            Reader.Sequence[0] = (unsigned char) atoi(pArg2);
        }
    }
    else{
        bOK = false;
        strcpy(sReply, txtErrorInvalidArgument);
    }
    
    sprintf(sReply, 
        " (%u,%u,%u,%u) @ %ums on %u / St=%u AS=%u Cont=%u Echo=%u", 
        Reader.Sequence[3],
        Reader.Sequence[2],
        Reader.Sequence[1],
        Reader.Sequence[0],
        Reader.Time,
        Reader.Alignment,
        Reader.Store,
        Reader.AutoStop,
        Reader.Continuous,
        Reader.EchoToUsb
    );
    printReply(idBuffer, bOK, "READER", sReply);
}

void Reader_cmd(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char n = 0;
    signed int m = 0;

    sReply[0] = 0x00;

    n = strlen(pArgs);
    if (!n){
        bOK = false;
        strcpy(sReply, txtErrorMissingArgument);
    }
    else if (strequal(pArgs, "all")){
        Reader.Direction = 0;
        Reader.AutoStop  = 1;
        Reader.Continuous= 1;
        Reader.Steps     = 1;
        Reader.Start     = Clock_getTime();
        Reader.Chars     = 0;
        Reader.Enabled   = 1;
    }
    else{
        m = atoi(pArgs);
        Reader.Direction = m >= 0 ? 0 : 1;
        Reader.Continuous= 0;
        Reader.Steps     = abs(m) + 1;
        Reader.Start     = Clock_getTime();
        Reader.Chars     = 0;
        Reader.Enabled   = 1;
    }
    
    printReply(idBuffer, bOK, "READER", sReply);
}
/*
unsigned int abs(signed int v){
    int v;           // we want to find the absolute value of v
    unsigned int r;  // the result goes here 
    int const mask = v >> sizeof(int) * 7 - 1;
    r = (v + mask) ^ mask;
    return r;
}
 * */
