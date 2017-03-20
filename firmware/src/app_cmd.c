/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "usb/usb.h"
#include "usb/usb_device_cdc.h"

#include "data_eeprom.h"

#if defined(LIB_KEYS)
    #include "service_keys.h"
#endif
#if defined(LIB_MUSIC)
    #include "service_music.h"
#endif

#include "app_globals.h"
#include "app_main.h"
#include "app_io.h"
#include "app_helpers.h"

#include "app_programs.h"

//#include "system_config.h"

#include "app_cmd.h"

/** FUNCTIONS *******************************************************/
void APP_CMD_ping(Ring_t *pBuffer, unsigned char *pArgs){
    printReply(pBuffer, 1, "PONG", pArgs);
}

void APP_CMD_uptime(Ring_t *pBuffer, unsigned char *pArgs){
    clock2str(sStr1, 0);
    printReply(0, 1, "UPTIME", sStr1);
}
        
void APP_CMD_debug(Ring_t *pBuffer, unsigned char *pArgs){
    bool bOK = true;
    sReply[0] = 0x00;
    
    str2lower(pArgs);   
        
    if (strequal(pArgs, "on") || strequal(pArgs, "1")){
        MasterDebug = 1;
    }
    else if (strequal(pArgs, "off") || strequal(pArgs, "0")){
        MasterDebug = 0;
    }
    else if (strlen(pArgs)){
        bOK = false;
        strcpy(sReply, txtErrorUnknownArgument);
        strcat(sReply, ". Now: ");
    }

    strcat(sReply, MasterDebug > 0 ? txtOn : txtOff);
    //strcat(sReply, " ");
    //strcat(sReply, MasterDebugMsg);

    printReply(0, bOK, "DEBUG", sReply);
}


void APP_CMD_read(Ring_t *pBuffer, unsigned char *pArgs){
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
    
    printReply(0, bOK, "READ", sReply);
}

void APP_CMD_write(Ring_t *pBuffer, unsigned char *pArgs){
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
            pReg = TRISA + n;
            *pReg = 0x00; // All outputs (this may interfere with USB, be careful)
            
            pReg = LATA + n;
            *pReg = (unsigned char) atoi(pArg2);
            
            sReply[0] = pArg1[0];
            sReply[1] = '=';
            byte2binstr(&sReply[2], *pReg);
        }
    }
    
    printReply(0, bOK, "WRITE", sReply);
}


void APP_CMD_var(Ring_t *pBuffer, unsigned char *pArgs){}
void APP_CMD_var_dummy(Ring_t *pBuffer, unsigned char *pArgs){
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
                pChar = &MasterDebug; break;
            case 1: 
                /*** MASTER CLOCK */
                var    = MasterClockTickCount; break;
            case 2: 
                pChar  = &MasterClock.Tick; break;
            case 3: 
                pLong  = &MasterClock.MS; break;
                /*** MASTER NOTIFY ***/
            case 10: 
                pInt   = &MasterClock.NotifyCounter; break;
            case 11: 
                pInt   = &MasterClock.NotifyTime; break;
            case 12: 
                pLong  = &MasterClock.NotifyNow; break;

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
                sprintf(sReply, "%u Char %3u %2x %s @ %04u", (unsigned int) num, *pChar, *pChar, sStr1, pChar);
            }
            else if (pInt){
                int2binstr(sStr1, *pInt);
                sprintf(sReply, "%u Int %5u %4u %s @ %04u", (unsigned int) num, *pInt, *pInt, sStr1, pInt);
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

void APP_CMD_var_ee(Ring_t *pBuffer, unsigned char *pArgs){
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
                
 

void APP_CMD_mem(Ring_t *pBuffer, unsigned char *pArgs){
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

void APP_CMD_dump(Ring_t *pBuffer, unsigned char *pArgs){
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