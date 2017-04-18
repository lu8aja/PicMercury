#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "service_reader.h"

#include "service_i2c.h"
#include "app_main.h"

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
    READER_Pin_EnableLeds = 1;
    
    Reader.Sequence[0] = 0;
    Reader.Sequence[1] = 2;
    Reader.Sequence[2] = 3;
    Reader.Sequence[3] = 1;
    
    Reader.Enabled     = 0;
    Reader.Time        = 8;
    Reader.Tick        = 0;
    Reader.Step        = 0;
    Reader.Align       = 0;
    Reader.Aligned     = 0;
    Reader.Store       = 1;
    Reader.EchoToUsb   = 1;
    Reader.UserStart   = 1;
    Reader.Debug       = 0;
    Reader.BtnStatus   = 0;
    Reader.BtnHistory  = 0;
}

inline void Reader_tick(void){
    if (Reader.Tick){
        Reader.Tick--;
    }
}

void Reader_service(void){
    unsigned char nChar;
    unsigned char bMustRead = 0;
    
    if (Reader.Tick){
        return;
    }

    // BUTTON
    Reader_checkButton();
    if (Reader.BtnStatus){
        // ON
        if (Reader.BtnTime < 255){
            // Button is being pressed
            Reader.BtnTime++;
        }
    }
    else if (Reader.BtnTime){
        // OFF immediately after ON
        // Button was pressed and released
        if (Reader.UserStart && !Reader.Enabled){
            if (Reader.BtnTime > Reader_Button_TicksStart){
                // Released after a long press: Continuous Fwd
                Reader_start(1, 1);
            }
            else if (Reader.BtnTime > Reader_Button_TicksStep){
                // Released after a short press: One step Fwd
                Reader_start(1, 0);
            }
            // The press was too short, so it gets ignored
        }
        Reader.BtnTime = 0;
    }
    
    if (!Reader.Enabled){
        Reader.Tick = Reader.Time;
        return;
    }
   
    // STOP EVENT
    if (Reader.MustStop){
        Reader.Enabled = 0;
        READER_Pin_EnableLeds   = 1;
        READER_Pin_EnabledCoils = 0;
        unsigned long now = Clock_getTime();
        unsigned long delta = now - Reader.Start;
        sprintf(sStr5, "DONE (%c) %u in %lums = %u", Reader.MustStop, Reader.CharsRead, delta, (unsigned int) ((((unsigned long) Reader.CharsRead) * 1000) / delta));
        printReply(Reader.Requestor, 3, "READER", sStr5);
        return;
    }

    if (Reader.Debug){
        byte2binstr(sStr1, READER_INPUT_PORT);
        printf("%u %u %u %u %s %u %u ", Reader.CharsRead, Reader.Step, Reader.Align, Reader.Aligned, sStr1, Reader.GuideExtra, Reader.ReadPost);
    }
    
    /* The is disabled, because the current physical hardware we use
     * does not allow the paper to go through the security switch
    if (READER_SwitchPaper){
        // Tape security switch cancelled
        Reader.MustStop = 'J';
        if (Reader.Debug) putch('U');
        printReply(Reader.Requestor, 3, "READER", "Paper jammed");
    }
    */
    
    // STEP AND CONDITION HANDLING
    if (READER_Button){
        // User cancelled by pushing the reader button
        Reader.MustStop = 'U';
        if (Reader.Debug) putch('U');
    }
    else if (READER_INPUT_InGuide && !Reader.GuideLast){
        // Normal alignment
        Reader.Align   = Reader.Step;
        Reader.Aligned = 1;
        bMustRead      = 1;
        if (Reader.Debug) putch('A');
    }
    else if (Reader.Step != Reader.Align){
        // Moving between chars
        if (Reader.Debug) putch('B');
    }
    else if (Reader.Aligned){
        // End of tape, we lost guide, but we need to keep reading 8 more chars
        bMustRead    = 1;
        if (Reader.ReadPost){
            Reader.ReadPost--;
            if (Reader.Debug) putch('C');
        }
        else{
            Reader.MustStop = 'D';
            if (Reader.Debug) putch('D');
        }
    }
    else if (Reader.GuideExtra){
        // We never aligned, lets only try 4 steps
        Reader.GuideExtra--;
        Reader.Align = Reader_next(Reader.Align);
        if (Reader.Debug) putch('E');
    }
    else{
        // We couldn't even start, probably the cover is open or there is no tape
        Reader.MustStop = 'F';
        if (Reader.Debug) putch('F');
    }

    if (Reader.Debug) print(txtCrLf);
    
    Reader.GuideLast = READER_INPUT_InGuide;    
    
    // READ
    if (bMustRead){
        Reader.CharsRead++;

        nChar = READER_INPUT_PORT & 0b00011111;

        Transcoder_write(Reader.Input, nChar, !Reader.Store);

        #if READER_Debug
            if (Reader.Debug){
                byte2binstr(sStr1, READER_INPUT_PORT);
                printf("\r\n\t\t\t%s %02x [%c] ", sStr1, nChar, Reader.Input->Char < 32 ? '_' : Reader.Input->Char);
            }
            else if (Reader.EchoToUsb){
                putch(Reader.Input->Char);
            }
        #else
            if (Reader.EchoToUsb){
                putch(Reader.Input->Char);
            }
        #endif
        
        if (Reader.Store && !ring_available(Reader.Input->Ring)){
            Reader.CharsPending = 0;
            Reader.BufferFull   = 1;

            printReply(Reader.Requestor, 2, "READER", txtErrorBufferFull);
        }
        else if (Reader.Continuous){
            Reader.CharsPending = 1;
        }
        else{
            Reader.CharsPending--;
        }
        
        if (!Reader.CharsPending){
            Reader.MustStop = 1;
        }
    }

    // MOTOR
    Reader.Step = Reader_next(Reader.Step);
    READER_MOTOR_LAT = (READER_MOTOR_LAT & ~READER_MOTOR_MASK) | Reader.Sequence[Reader.Step];
    READER_Pin_EnabledCoils = 1;
    Reader.Tick = Reader.Time;
    
    // DUMP
    if (Reader.MustStop || ring_strlen(Reader.Input->Ring) >= Reader_sizeBlock){
        // Get whatever we have, out
        ring_str(Reader.Input->Ring, sStr5, sizeof(sStr5), 0);
        printReply(Reader.Requestor, 3, "READER DATA", sStr5);
    }
}

inline unsigned char Reader_next(unsigned char nStep){
    // Next state
    if (Reader.Direction){
        // Backwards
        if (nStep){
            nStep--;
        }
        else{
            nStep = sizeof(Reader.Sequence) - 1;
        }
    }
    else{
        // Forward
        if (nStep < sizeof(Reader.Sequence) - 1){
            nStep++;
        }
        else{
            nStep = 0;
        }
    }
    return nStep;
}

inline void Reader_start(signed char nCount, unsigned char nContinuous){
    Reader.Start          = Clock_getTime();
    Reader.Continuous     = nContinuous;
    Reader.CharsRead      = 0;
    Reader.ReadPost       = Reader_ReadPostGuide;
    Reader.MustStop       = 0;
    Reader.Aligned        = READER_INPUT_InGuide;        

    if (nContinuous){
        Reader.Direction      = 0;
        Reader.CharsPending   = 1;
    }
    else{
        Reader.Direction      = nCount >= 0 ? 0 : 1;
        Reader.CharsPending   = abs(nCount);
    }
    
    if (Reader.Direction){
        Reader.GuideExtra = (sizeof(Reader.Sequence) + 1) * Reader_ReadPostGuide;
    }
    else{
        Reader.GuideExtra = sizeof(Reader.Sequence);    
    }
    READER_Pin_EnableLeds = 0;
    Reader.Enabled = 1;
}


void Reader_checkButton(void){
    unsigned char n = 0;
    unsigned char nOnes;
    
    // Insert current bit, discarding oldest bit
    Reader.BtnHistory = (Reader.BtnHistory << 1) | READER_Button;
    
    // Count bits
    nOnes = 0;
    for (n = 0b10000000; n; n = n >> 1){
        if (Reader.BtnHistory & n) nOnes++;
    }
    
    // Determine new status: Should flip?
    n = 0;
    if (Reader.BtnStatus){
        if (nOnes < 3) n = 1;
    }
    else{
        if (nOnes > 4) n = 1;
    }

    // Flip status and notify
    if (n){
        Reader.BtnStatus ^= 1;
        if (Reader.BtnStatus){
            // ON
            Reader.BtnTime = 0;
            READER_Pin_EnableLeds   = 0;
            printReply(0, 3, "READER", "BTN ON");
            #ifdef LIB_I2C
            if (Reader.NotifyMaster){
                I2C_send(0, 0, I2C_Address_Master, 0, "@OK READER: BTN ON");
            }
            #endif
        }
        else{
            // OFF
            if (!Reader.Enabled) READER_Pin_EnableLeds   = 1;
            sprintf(sStr5, "BTN OFF %u", Reader.BtnTime);
            printReply(0, 3, "READER", sStr5);
            #ifdef LIB_I2C
            if (Reader.NotifyMaster){
                I2C_send(0, 0, I2C_Address_Master, 0, "@OK READER: BTN OFF");
            }
            #endif
        }
    }
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
            Reader.Align = (unsigned char) atoi(pArg2);
        }
    }
    else if (strequal(pArg1, "userstart")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.UserStart  = strequal(pArg2, "on") || strequal(pArg2, "1");
        }
    }
    else if (strequal(pArg1, "store")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Store     = strequal(pArg2, "on") || strequal(pArg2, "1");;
        }
    }
    else if (strequal(pArg1, "echo")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.EchoToUsb = strequal(pArg2, "on") || strequal(pArg2, "1");;
        }
    }
    else if (strequal(pArg1, "notify")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.NotifyMaster = strequal(pArg2, "on") || strequal(pArg2, "1");;
        }
    }
    else if (strequal(pArg1, "mode")){
        pArg2 = strtok(NULL,  txtWhitespace);
        if (pArg2){
            Reader.Input->Configs = (unsigned char) atoi(pArg2);
            Reader.Input->Status  = 0;
            ring_clear(Reader.Input->Ring);
        }
    }
    else if (strequal(pArg1, "clear")){
        ring_clear(Reader.Input->Ring);
    }
    else{
        bOK = false;
        strcpy(sReply, txtErrorInvalidArgument);
    }
    
    sprintf(sReply, 
        " T=%ums S=%u A=%u Str=%u UsSt=%u Notify=%u Mode=%02x Echo=%u Dbg=%u", 
        Reader.Time,
        Reader.Step,
        Reader.Align,
        Reader.Store,
        Reader.UserStart,
        Reader.NotifyMaster,
        Reader.Input->Configs,
        Reader.EchoToUsb,
        Reader.Debug
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
        Reader.Requestor = idBuffer;
        Reader_start(1, 1);
    }
    else{
        Reader.Requestor = idBuffer;
        Reader_start(atoi(pArgs), 0);
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
