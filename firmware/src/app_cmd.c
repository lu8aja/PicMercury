/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "app_cmd.h"

#include "service_i2c.h"

#ifdef DEVICE_PUNCHER
    #include "service_puncher.h"    // Puncher library with associated PUNCH cmd
    #include "service_softserial.h" // TTY 5N1
#endif

#ifdef LIB_SOFTSERIAL
    #include "service_softserial.h"
#endif
#ifdef LIB_PUNCHER
    #include "service_puncher.h"
#endif
#ifdef LIB_KEYS
    #include "service_keys.h"
#endif
#ifdef LIB_MUSIC
    #include "service_music.h"
#endif

inline unsigned char Cmd_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs){

    // PING
    if (strequal(pCommand, "ping")){
        APP_CMD_ping(idBuffer, pArgs);
    }
    // STATUS
	else if (strequal(pCommand, "status")){
        APP_CMD_status(idBuffer, pArgs);
	}
    // RESET
	else if (strequal(pCommand, "reset")){
        APP_CMD_reset(idBuffer, pArgs);
	}
    // UPTIME
	else if (strequal(pCommand, "uptime")){
        APP_CMD_uptime(idBuffer, pArgs);
	}
    // DEBUG
    else if (strequal(pCommand, "debug")){
        APP_CMD_debug(idBuffer, pArgs);
    }
    // READ
    else if (strequal(pCommand, "read") || strequal(pCommand, "r")){
        APP_CMD_read(idBuffer, pArgs);
    }
    // WRITE
    else if (strequal(pCommand, "write") || strequal(pCommand, "w")){
        APP_CMD_write(idBuffer, pArgs);
    }
    else{
        return 0;
    }
    return 1;
}


    
/** FUNCTIONS *******************************************************/
void APP_CMD_ping(unsigned char idBuffer, unsigned char *pArgs){
    printReply(idBuffer, 1, "PONG", pArgs);
}

void APP_CMD_version(unsigned char idBuffer, unsigned char *pArgs){
    printReply(idBuffer, 1, "VERSION", txtVersion);
}


void APP_CMD_reset(unsigned char idBuffer, unsigned char *pArgs){
    System.Clock.NotifyCounter = pArgs[0] ? atoi(pArgs) : 5000;
    System.Config.Reset        = 1;
    printReply(idBuffer, 1, "RESET", txtOk);
}


void APP_CMD_status(unsigned char idBuffer, unsigned char *pArgs){
    byte2binstr(sStr1, System.Errors);
    
    sprintf(sReply, "Errors=%s\r\n", sStr1);
    
    sprintf(&sReply[strlen(sReply)], 
        " I2C In=%x Out=%x",
        I2C.Input->Buffer,
        I2C.Output->Buffer
    );
    
    #ifdef LIB_SOFTSERIAL
        sprintf(&sReply[strlen(sReply)], 
            " / Puncher=%x",
            Puncher.Output->Ring->Buffer
        );
    #endif
    #ifdef LIB_PUNCHER
        sprintf(&sReply[strlen(sReply)], 
            " / Serial In=%x Out=%x",
            SoftSerial.Input->Ring->Buffer,
            SoftSerial.Output->Ring->Buffer
        );
    #endif
    
    printReply(idBuffer, 1, "STATUS", sReply);
}

void APP_CMD_uptime(unsigned char idBuffer, unsigned char *pArgs){
    Clock_getStr(sStr1, 0);
    printReply(idBuffer, 1, "UPTIME", sStr1);
}

void APP_CMD_debug(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    sReply[0] = 0x00;
    
    str2lower(pArgs);   
        
    if (strequal(pArgs, "on") || strequal(pArgs, "1")){
        System.Config.Debug = 1;
    }
    else if (strequal(pArgs, "off") || strequal(pArgs, "0")){
        System.Config.Debug = 0;
    }
    else if (strlen(pArgs)){
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
        strcat(sReply, ". Now: ");
    }

    strcat(sReply, System.Config.Debug > 0 ? txtOn : txtOff);
    //strcat(sReply, " ");
    //strcat(sReply, System.Config.DebugMsg);

    printReply(idBuffer, bOK, "DEBUG", sReply);
}


void APP_CMD_read(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char nBit = 255; // Double purpose, bit number for single pin or TRIS value for whole port
    unsigned char nMax = '7';
    volatile unsigned char *pPort;
    volatile unsigned char *pTris;
    
    sReply[0] = 0x00;
    
    str2upper(pArgs);
    
    switch (pArgs[0]){
        case 'A':
            // RA6 OSC2 / RA7 n/a
            pPort = &PORTA; pTris = &TRISA; nMax = '5'; nBit = 0b00111111; break;
        case 'B':
            pPort = &PORTB; pTris = &TRISB; break;
        case 'C':
            // RC3 n/a / RC4 D- ReadOnly / RC5 D+ ReadOnly / RC6 TX UART / RC7 RX UART
            pPort = &PORTC; pTris = &TRISC; nMax = '2'; nBit = 0b10000111; break;
        case 'D':
            pPort = &PORTD; pTris = &TRISD; break;
        case 'E':
            // RE3..7 Does not exist
            pPort = &PORTE; pTris = &TRISE; nMax = '2'; nBit = 0b00000111;  break;
        default: 
            bOK = false;
            strcat(sReply, txtErrorUnknownArgument);
            break;
    }
    
    if (bOK){
        if (pArgs[1]){
            // Single pin
            if (pArgs[1] >= '0' && pArgs[1] <= nMax){
                nBit = pArgs[1] - 48;
                bit_set(*pTris, nBit);
                
                sReply[0] = pArgs[0];
                sReply[1] = pArgs[1];
                sReply[2] = '=';
                sReply[3] = bit_read(*pPort, nBit) + 48;
                sReply[4] = 0;
            }
            else {
                bOK = false;
                strcat(sReply, txtErrorUnknownPin);
            }
        }
        else{
            // Whole port
            *pTris = nBit;
            
            sReply[0] = pArgs[0];
            sReply[1] = '=';
            byte2binstr(&sReply[2], *pPort);

        }
    }
    
    printReply(idBuffer, bOK, "READ", sReply);
}

void APP_CMD_write(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;

    unsigned char n = 0;
    volatile unsigned char *pReg;

    sReply[0] = 0x00;

    str2upper(pArgs);
    
    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1 || !pArg2){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }

    if (bOK){
        if (pArg1[0] < 'A' || pArg1[0] > 'E'){
            bOK = false;
            strcat(sReply, txtErrorUnknownArgument);
        }
    }

    if (bOK){
        if (pArg1[1]){
            
            if ( (pArg1[0] == 'A' && pArg1[1] > '5') 
             || ((pArg1[0] == 'C' || pArg1[0] == 'E') && pArg1[1] > '2') 
            ){
                // RA6 OSC2 / RA7 n/a
                // RC3 n/a / RC4 D- ReadOnly / RC5 D+ ReadOnly / RC6 TX UART / RC7 RX UART
                // RE3..7 Does not exist
                
                bOK = false;
                strcat(sReply, txtErrorUnknownPin);
            }
            
            if (bOK){
                sReply[0] = pArg1[0];
                sReply[1] = pArg1[1];
                sReply[2] = '=';

                // Single pin
                pin_cfg(pArg1[0], pArg1[1] - 48, 0);

                if (strequal(pArg2, "ON") || strequal(pArg2, "1")){
                    pin_write(pArg1[0], pArg1[1] - 48, 1);
                    sReply[3] = '1';
                }
                else{
                    pin_write(pArg1[0], pArg1[1] - 48, 0);
                    sReply[3] = '0';
                }
                sReply[4] = 0;
            }
        }
        else{
            // Whole port
            n = pArg1[0] - 'A';
            pReg = &TRISA + n;
            *pReg = 0x00; // All outputs (this may interfere with USB, be careful)
            
            pReg = &LATA + n;
            *pReg = (unsigned char) atoi(pArg2);
            
            sReply[0] = pArg1[0];
            sReply[1] = '=';
            byte2binstr(&sReply[2], *pReg);
        }
    }
    
    printReply(idBuffer, bOK, "WRITE", sReply);
}


void APP_CMD_var(unsigned char idBuffer, unsigned char *pArgs){}
void APP_CMD_var_dummy(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    unsigned char *pArg3 = NULL;
    
    volatile unsigned char *pChar = NULL;
    volatile unsigned int  *pInt  = NULL;
    volatile unsigned long *pLong = NULL;

    unsigned int  num = 0;
    unsigned long val = 0;
    unsigned char var = 0;

    sReply[0] = 0x00;

    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);
    pArg3 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        num = atoi(pArg1);
        switch (num){
                 /*** MASTER DEBUG ***/
            case 0: 
                pChar = &System.Config.Debug; break;
            case 1: 
                /*** MASTER CLOCK */
                var    = System_ClockTickCount; break;
            case 2: 
                pChar  = &System.Clock.Tick; break;
            case 3: 
                pLong  = &System.Clock.MS; break;
                /*** MASTER NOTIFY ***/
            case 10: 
                pInt   = &System.Clock.NotifyCounter; break;
            case 11: 
                pInt   = &System.Clock.NotifyTime; break;
            case 12: 
                pLong  = &System.Clock.NotifyNow; break;

        #if defined(LIB_LEDS)
                /*** MASTER LEDS ***/
                // Configs
            case 20: 
                pChar  = &MasterLeds.Time; break;
            case 21: 
                pChar  = &MasterLeds.StepEnabled; break;
            case 22: 
                pChar  = &MasterLeds.StepRestart; break;
            case 23: 
                pInt   = &MasterLeds.StepTime; break;
                // Runtime
            case 30: 
                pInt   = &MasterLeds.Status; break;
            case 31: 
                pChar  = &MasterLeds.Tick; break;
            case 32: 
                pInt   = &MasterLeds.StepTick; break;
            case 33: 
                pChar  = &MasterLeds.Step; break;
        #endif

        #if defined(LIB_KEYS)
                /*** MASTER MUSIC ***/
                // Configs
            case 40: 
                pChar  = &MasterMusic.enabled; break;
            case 41: 
                pChar  = &MasterMusic.mode; break;
            case 42: 
                pInt   = &MasterMusic.tempo; break;
            case 43: 
                pChar  = &MasterMusic.pitch; break;
            case 44: 
                pChar  = &MasterMusic.restart; break;
            case 45: 
                pChar  = &MasterMusic.period; break;
            case 46: 
                pInt   = &MasterMusic.time; break;
                // Runtime
            case 60: 
                pChar  = &MasterMusic.address; break;
            case 61: 
                pChar  = &MasterMusic.length; break;
            case 62: 
                pChar  = &MasterMusic.tick; break;
            case 63: 
                pInt   = &MasterMusic.counter; break;
            case 64: 
                pChar  = &MasterMusic.step; break;
        #endif

        #if defined(LIB_KEYS)
                /*** MASTER KEYS ***/
            case 70: 
                pInt   = &MasterKeys.Function; break;
            case 71: 
                pInt   = &MasterKeys.Address; break;
            case 72: 
                pInt   = &MasterKeys.Input; break;
            case 73: 
                pInt   = &MasterKeys.Switches; break;
                /*** MASTER BUTTONS ***/
            case 80: 
                pChar  = &MasterKeys.Tick; break;
            case 81: 
                pChar  = &MasterKeys.Bit; break;
            case 82: 
                pChar  = &MasterKeys.Time; break;
            case 83: 
                pChar  = &MasterKeys.Run; break;
        #endif

        #if defined(LIB_PROGRAM)
                /*** MASTER PROGRAM ***/
                // Configs
            case 90: 
                pLong  = &MasterProgram.Time; break;
            case 91: 
                pInt   = &MasterProgram.Run; break;
            case 92: 
                pInt   = &MasterProgram.Enabled; break;
                // Runtime
            case 93: 
                pLong  = &MasterProgram.Tick; break;
            case 94: 
                pInt   = &MasterProgram.Step; break;
        #endif
                /*** PORTS ***/  
            case 100: 
                pChar  = &PORTA; break;
            case 101: 
                pChar  = &PORTB; break;
            case 102: 
                pChar  = &PORTC; break;
            case 103: 
                pChar  = &PORTD; break;
            case 104: 
                pChar  = &PORTE; break;
            case 110: 
                pChar  = &TRISA; break;
            case 111: 
                pChar  = &TRISB; break;
            case 112: 
                pChar  = &TRISC; break;
            case 113: 
                pChar  = &TRISD; break;
            case 114: 
                pChar  = &TRISE; break;
            case 120: 
                pChar  = &LATA; break;
            case 121: 
                pChar  = &LATB; break;
            case 122: 
                pChar  = &LATC; break;
            case 123: 
                pChar  = &LATD; break;
            case 124: 
                pChar  = &LATE; break;
                
                
            case 200: 
                pChar  = &ADCON0; break;
            case 201: 
                pChar  = &ADCON1; break;
            case 202: 
                pChar  = &ADCON2; break;
            case 210: 
                pChar  = &TXSTA; break;
            case 211: 
                pChar  = &RCSTA; break;
            case 212: 
                pChar  = &TXREG; break;
            case 213: 
                pChar  = &RCREG; break;
            case 214: 
                pChar  = &BAUDCON; break;
            case 215: 
                pChar  = &SPBRGH; break;
            case 216: 
                pChar  = &SPBRG; break;
            default:
                if (num >= 0x0F60 && num <= 0x0FFF){
                    // Special registers
                    pChar = (unsigned char *) num;
                }
                else{
                    bOK = false;
                    strcat(sReply, txtErrorInvalidArgument);

                }
        }
        
        if (bOK){
            if (pArg2){
                val = atoi(pArg2);
                if (pChar){
                    *pChar = val;
                }
                else if (pInt){
                    *pInt = val;
                }
                else if (pLong){
                    *pLong = val;
                }
                else {
                    bOK = false;
                    // No need for explanation, as the reply will show that it is a constant
                }
            }
            

            if (pChar){
                byte2binstr(sStr1, *pChar);
                sprintf(sReply, "%u Char %3hu %2hx %s @ %04u", num, *pChar, *pChar, sStr1, (unsigned int) pChar);
            }
            else if (pInt){
                int2binstr(sStr1, *pInt);
                sprintf(sReply, "%u Int %5hu %4hx %s @ %04u", num, *pInt, *pInt, sStr1, (unsigned int) pInt);
            }
            else if (pLong){
                any2binstr(sStr5, *pLong, 32);
                *pLong = val;
                sprintf(sReply, "%u Long %lu %s", num, *pLong, sStr5);
            }
            else {
                sprintf(sReply, "%u Const %u (readonly)", num, var);
            }
        }
    }
    printReply(0, bOK, "VAR", sReply);
}

void APP_CMD_var_ee(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned char addr;
    unsigned char val;

    sReply[0] = 0x00;

    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);
    

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        addr = atoi(pArg1);
        if (pArg2){
            val = (unsigned char) atoi(pArg2);
            EEPROM_write(addr, val);
            sprintf(sReply, "W %02x <- %02x", addr, val);
        }
        else{
            val = (unsigned char) EEPROM_read(addr);
            sprintf(sReply, "R %02x = %02x", addr, val);
        }
    }
    
    printReply(0, bOK, "VAR", sReply);
}
                
 

void APP_CMD_mem(unsigned char idBuffer, unsigned char *pArgs){
    /*
    bool bOK = true;
    unsigned char *pArg1 = NULL;
    unsigned char *pArg2 = NULL;
    
    unsigned char *pChar = NULL;

    sReply[0] = 0x00;

    pArg1 = strtok(pArgs, txtWhitespace);
    pArg2 = strtok(NULL,  txtWhitespace);

    if (!pArg1){
        bOK = false;
        strcat(sReply, txtErrorMissingArgument);
    }
    else {
        pChar = atoi(pArg1);
        if (pArg2){
            *pChar = atoi(pArg2);
        }
        byte2binstr(sStr1, *pChar);
        sprintf(sReply, "%4x = %2x %3u %s", pChar, *pChar, *pChar, sStr1);
    }
        
    printReply(bOK, "MEM", sReply);
     * */
}

/*
void APP_CMD_dumptest(unsigned char *pArgs){
    unsigned char n = 0;
    unsigned char m = 0;
    for (m = 'A'; m < 'Z'; m++){
        putch('\r');
        putch('\n');
        putch(m);
        putch(' ');
        for (n = 'a'; n <= 'z'; n++){
            putch(n);
        }
    }
    putch('\r');
    putch('\n');
    printf(" %u\r\n", posOutput);

}
*/

void APP_CMD_dump(unsigned char idBuffer, unsigned char *pArgs){
    /*
    bool bOK = true;
    unsigned char *pChar = 0x0f60;
    unsigned char *pMax  = 0x1000;
    
    sReply[0] = 0x00;

    while(pChar < pMax){
        printf("\t%04x =", pChar);
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        printf(" %02x", *pChar);
        pChar++;
        print(txtCrLf);
        
        // Flush often to avoid buffer overrun
        APP_outputUsb();
    }
        
    printReply(bOK, "DUMP", 0);
    */
}