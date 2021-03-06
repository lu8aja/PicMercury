
#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>


#include "lib_helpers.h"

#include "app_globals.h"
#include "app_io.h"
#include "service_usb.h"

#if defined(LIB_LEDS) // Used for overflow light only
    #include "service_leds.h"
#endif
#if defined(LIB_MUSIC) // Used for overflow beep only
    #include "service_music.h"
#endif

/*** STDIO ***/
void printReply(unsigned char idBuffer, const unsigned char nType, const unsigned char *pCmd, const unsigned char *pReply){
    Ring_t *pBuffer = 0;

    if (idBuffer > 0 && idBuffer < 8){
        pBuffer = System.Buffers[idBuffer];
        if (pBuffer){
            switch (nType){
                case 0:
                    ring_append(pBuffer, "-ERROR ");
                    break;
                case 1:
                    ring_append(pBuffer, "+OK ");
                    break;
                case 2:
                    ring_append(pBuffer, "!ERROR ");
                    break;
                case 3:
                    ring_append(pBuffer, "@OK ");
                    break;
                case 8:
                    ring_append(pBuffer, "$");
                    break;
            }
            if (pCmd){
                ring_append(pBuffer, pCmd);
            }
            if (pReply && pReply[0]){
                if (pCmd) ring_append(pBuffer, ": ");
                ring_append(pBuffer, pReply);
            }
            ring_append(pBuffer, txtCrLf);
            return;
        }
    }

    // USB
    if ( USB_getDeviceState() < CONFIGURED_STATE || USB_isDeviceSuspended()){
        return;
    }

    switch (nType){
        case 0:
            print("-ERROR ");
            break;
        case 1:
            print("+OK ");
            break;
        case 2:
            print("!ERROR ");
            break;
        case 3:
            print("@OK ");
            break;
    }
    if (pCmd){
        print(pCmd);
    }
    if (pReply && strlen(pReply)){
        if (pCmd) print(": ");
        print(pReply);
    }
    print(txtCrLf);
}

// Function used by stdio (printf, etc)
void putch(const unsigned char byte){
    if ( USB_getDeviceState() < CONFIGURED_STATE || USB_isDeviceSuspended()){
        return;
    }
    
    if (posOutput >= sizeOutput){
        USB_output();
        if (posOutput >= sizeOutput){
            
            #if defined(LIB_MUSIC)
                Music_setSingleTone(6, 16);
            #endif

            #if defined(LIB_LEDS)
                bit_set(MasterLedStatus, LED_ALARM);
            #endif
            
            //MasterConsoleStatus.bufferOverrun = 1;
            strcpy(bufUsbOutput, "\r\n!ERROR OVERFLOW ");
            posOutput = 19;
        }
    }

    bufUsbOutput[posOutput] = byte;
    posOutput++;
    bufUsbOutput[posOutput] = 0x00;
}


void print(const unsigned char *pStr){
    while (*pStr){
        putch(*pStr);
        pStr++;
    }
}

/*** STRING ***/


inline void str_append(unsigned char *pStr, unsigned char cChar){
    // Only use this one if you are sure you will not overrun!
    while (*pStr) pStr++;
    *pStr = cChar;
    pStr++;
    *pStr  = 0;
}

void str_append_safe(unsigned char *pStr, unsigned char cChar, unsigned char nMaxLen){
    if (nMaxLen && (strlen(pStr) + 1) >= nMaxLen){
        // Silently fail to add because we're over the allocated buffer space
        return;
    }
    
    pStr += strlen(pStr);
    *pStr = cChar;
    pStr++;
    *pStr  = 0;
}

void str_dump(unsigned char *pDumped, const unsigned char *pStr, unsigned char nLen){
    unsigned char n = 0;
    
    if (pStr == NULL){
        *pDumped = 'X';
        *pDumped++;
        *pDumped = 0;
        return;
    }
    
    if (!nLen){
        while (*pStr){ nLen++; pStr++; }
        pStr -= nLen;
    }
    
    while (nLen--){
       *pDumped = tohex(*pStr >> 4);
        pDumped++;
        
       *pDumped = tohex(*pStr & 0x0f);		
        pDumped++;
        
       *pDumped = ' ';
        pDumped++;

       *pDumped = *pStr < 32 ? 254 : *pStr;
        pDumped++;
        
        pStr++;
        
       *pDumped = ' ';
        pDumped++;
        
        n += 5;
        if (n >= 80){
           *pDumped = '\r';
            pDumped++;
           *pDumped = '\n';
            pDumped++;
            n -= 82;
        }
    }
    
    *pDumped = 0;
}


/*** CONVERSIONS ***/
void byte2hex(const char cChar, char *sStr1){
    unsigned char x;
	x = (cChar & 0xF0) >> 4;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[0] = x;
	x = cChar & 0x0F;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[1] = x;
	sStr1[2] = 0x00;
} //end byte2hex

void int2hex(unsigned int iNum, char *sStr1){
    unsigned char x;
	x = (iNum & 0xF000) >> 12;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[0] = x;
	x = (iNum & 0x0F00) >> 8;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[1] = x;
	x = (iNum & 0x00F0) >> 4;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[2] = x;
	x = iNum & 0x000F;
	x = (x < 10) ? x | 0x30 : x + 0x37;		
	sStr1[3] = x;
	sStr1[4] = 0x00;
} //end int2hex

void str2hex(const char *sStr2, char *sStr1){
    unsigned char len2, i, j, x;
	len2 = i = j = 0;
    do{ len2++; if (len2 == 255) break; } while (*sStr2++); // Break loop once max len is reached.
    sStr2 -= len2;                    // Re-adjust pointer to its initial location
	for (i = 0; i < len2; i++){
		x = (sStr2[i] & 0xF0) >> 4;
		x = (x < 10) ? x | 0x30 : x + 0x37;		
		sStr1[j] = x;
		j ++;
		x = sStr2[i] & 0x0F;
		x = (x < 10) ? x | 0x30 : x + 0x37;		
		sStr1[j] = x;
		j++;
	}
	sStr1[j] = 0x00;
} //end str2hex

inline void str2lower(unsigned char *pStr){
    if (pStr == NULL) return;
    while (*pStr){
        // if (*pStr >= 'A' && *pStr <= 'Z') *pStr =  *pStr | 0x60;
        *pStr =  tolower(*pStr);
        pStr++;
    }
}

inline void str2upper(unsigned char *pStr){
    if (pStr == NULL) return;
    while (*pStr){
        // if (*pStr >= 'a' && *pStr <= 'z') *pStr =  *pStr & 0x9f;
        *pStr =  toupper(*pStr);
        pStr++;
    }
}


void byte2binstr(char *sStr, unsigned char iNum){
	sStr[8] = 0x00;
	sStr[7] = (iNum & 0x01)  ? '1' : '0';
	sStr[6] = (iNum & 0x02)  ? '1' : '0';
	sStr[5] = (iNum & 0x04)  ? '1' : '0';
	sStr[4] = (iNum & 0x08)  ? '1' : '0';
	sStr[3] = (iNum & 0x10)  ? '1' : '0';
	sStr[2] = (iNum & 0x20)  ? '1' : '0';
	sStr[1] = (iNum & 0x40)  ? '1' : '0';
	sStr[0] = (iNum & 0x80 ) ? '1' : '0';
} //end byte2binstr

void int2binstr(char *sStr, unsigned int iNum){
    unsigned int  n = 1;
    unsigned char m = 16;
	sStr1[m] = 0x00;
   
    do {
        m--;
        sStr[m] = (iNum & n)   ? '1' : '0';
        n = n << 1;
    }
    while (m);
} //end int2binstr

void any2binstr(char *sStr, unsigned long iNum, unsigned char nLen){
    unsigned long n = 1;
    if (nLen > 32){
        nLen = 32;
    }
	sStr1[nLen] = 0x00;
   
    do {
        nLen--;
        sStr[nLen] = (iNum & n)   ? '1' : '0';
        n = n << 1;
    }
    while (nLen);
}


unsigned char hexstr2byte(unsigned char *sStr){
    return (hex2byte(*sStr) << 4) | hex2byte(*(sStr + 1));
}

unsigned char hex2byte(unsigned char c){
    if (c >= 'a' && c <= 'f'){
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F'){
        return c - 'A' + 10;
    }
    if (c >= '0' && c <= '9'){
        return c - '0';
    }
    return 0;
}


/*** CLOCK ***/

unsigned long Clock_getTime(void){
    unsigned char tick = System.Clock.Tick;
    unsigned long ms   = System.Clock.MS;
    if (tick != System.Clock.Tick){
        // This is to avoid interim changes as operations with "long" are not atomic
        ms   = System.Clock.MS;
    }
    return ms;
}


void Clock_getStr(char *sStr, unsigned long ms){
	unsigned char tick, len, i, h;
    
    if (!ms){
        ms   = Clock_getTime();
    }

	len = 15;
	                                          sStr[len] = 0x00;
	len--;                      i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = '.';
	len--; ms  -= i; ms /= 10;  i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 6;  sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = ':';
	len--; ms  -= i; ms /= 6;   i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;  i = ms  % 6;  sStr[len] = i   | 0x30;
	len--;                                    sStr[len] = ':';
		   // 24 is not divisible by 10, so this means extra care for the hours presentation
	       ms  -= i; ms /= 6;   h = ms  % 24; ms  -= h; ms /= 24;  
	       // now we have h hours and ms days
	len--;                      i = h   % 10; sStr[len] = i   | 0x30;
	len--; h   -= i; h  /= 10;                sStr[len] = h   | 0x30;
	len--;                                    sStr[len] = 'd';
	len--;                      i = ms  % 10; sStr[len] = i   | 0x30;
	len--; ms  -= i; ms /= 10;                sStr[len] = ms  | 0x30;
	       // long 4 byte unsigned, expressing ms, will wrap at arround day 49, so no more than 2 digits for the day are needed
}

/*** EEPROM ***/
unsigned char EEPROM_read(unsigned char addr){
    EEADR = addr;
    EECON1bits.CFGS = 0;
    EECON1bits.EEPGD = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

void EEPROM_write(unsigned char addr, unsigned char val){
    EEADR  = addr;
    EEDATA = val;
    
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS  = 0;
    EECON1bits.WREN  = 1;
    INTCONbits.GIE   = 0;
    
    EECON2 = 0x55; // 55AAWrite sequence, it MUST be like this by desgin
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    //while(EECON1bits.WR);
    INTCONbits.GIE = 1;
    EECON1bits.WREN = 0;
}

/*** PIN I/O ***/

/*      A   B   C   D   E  
 TRIS   F92 F93 F94 F95 F96
 LAT    F89 F8A F8B F8c F8D
 PORT   F80 F81 F82 F83 F84
 */

void pin_write(unsigned char nPort, unsigned char nBit, unsigned char nVal){
    volatile unsigned char *pReg = &LATA - 1 + nPort;
    bit_write(*pReg, nBit, nVal);
}

unsigned char pin_read(unsigned char nPort, unsigned char nBit){
    volatile unsigned char *pReg = &PORTA - 1 + nPort;
    return bit_read(*pReg, nBit);
}

void pin_cfg(unsigned char nPort, unsigned char nBit, unsigned char nDirection){
    volatile unsigned char *pReg = &TRISA - 1 + nPort;
    bit_write(*pReg, nBit, nDirection);
}