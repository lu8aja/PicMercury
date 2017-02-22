
#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "usb.h"
#include "usb_device_cdc.h"

#include "app_main.h"
#include "app_io.h"

/** MACROS **/
/* Bit Operation macros */
#define setbit(b,n)   ( b |=   (1 << n))        /* Set bit number n in byte b   */
#define clearbit(b,n) ( b &= (~(1 << n)))       /* Clear bit number n in byte b */
#define readbit(b,n)  ((b  &   (1 << n)) >> n)  /* Read bit number n in byte b  */
#define flipbit(b,n)  ( b ^=   (1 << n))        /* Flip bit number n in byte b  */

#define reciprocal(a, fp)  ( (( 1 << fp) + a - 1) / a )  /* Reciprocal 1/x without using floats */

#define bit_is_set(b,n)   (b   & (1 << n))     /* Test if bit number n in byte b is set   */
#define bit_is_clear(b,n) (!(b & (1 << n)))    /* Test if bit number n in byte b is clear */

#define strequal(a, b) (strcmp(a, b) == 0)


// Helper functions
void          putch(const unsigned char byte);
void          print(const unsigned char *pStr);
unsigned long str2long(const char *sStr);
void          byte2hex(const char cChar, char *sStr1);
void          int2hex(unsigned int iNum, char *sStr1);
void          str2hex(const char *sStr2, char *sStr1);
inline void   str2lower(unsigned char *pStr);
inline void   str2upper(unsigned char *pStr);
void          byte2binstr(char *sStr, unsigned char iNum);
void          int2binstr(char *sStr, unsigned int iNum);
void          clock2str(char *sStr, unsigned long ms);



// Function used by stdio (printf, etc)
void putch(const unsigned char byte){
    if (posOutput >= sizeOutput){
        if (USBUSARTIsTxTrfReady()){
            putsUSBUSART(bufOutput);
            posOutput = 0;
        }
        else{
            // TODO: TBD what to do
            //putsUSBUSART("\r\n-ERROR OUTPUT OVERFLOW!\r\n");
            MasterConsoleStatus.bufferOverrun = 1;
            setbit(MasterLedStatus, LED_ALARM);
            // Clear buffer entirely
            posOutput = 0;
            bufOutput[posOutput] = 0x00;
        }
    }

    bufOutput[posOutput] = byte;
    posOutput++;
    bufOutput[posOutput] = 0x00;
    if (posOutput >= sizeOutput && USBUSARTIsTxTrfReady()){
        putsUSBUSART(bufOutput);
        posOutput = 0;
    }
}


void print(const unsigned char *pStr){
    while (*pStr > 0){
        putch(*pStr);
        pStr++;
    }
}


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
    while (*pStr){
        // if (*pStr >= 'A' && *pStr <= 'Z') *pStr =  *pStr | 0x60;
        *pStr =  tolower(*pStr);
        pStr++;
    }
}

inline void str2upper(unsigned char *pStr){
    while (*pStr){
        // if (*pStr >= 'a' && *pStr <= 'z') *pStr =  *pStr & 0x9f;
        *pStr =  toupper(*pStr);
        pStr++;
    }
}


void byte2binstr(char *sStr, unsigned char iNum){
	sStr[8] = 0x00;
	sStr[7] = (iNum & 1)   ? '1' : '0';
	sStr[6] = (iNum & 2)   ? '1' : '0';
	sStr[5] = (iNum & 4)   ? '1' : '0';
	sStr[4] = (iNum & 8)   ? '1' : '0';
	sStr[3] = (iNum & 16)  ? '1' : '0';
	sStr[2] = (iNum & 32)  ? '1' : '0';
	sStr[1] = (iNum & 64)  ? '1' : '0';
	sStr[0] = (iNum & 128) ? '1' : '0';
} //end byte2binstr

void int2binstr(char *sStr, unsigned int iNum){
    unsigned int  n = 1;
    unsigned char m = 15;
	sStr1[16] = 0x00;
    sStr1[0]  = '0'; // Ints even unsigned have issues with msb
    
    do {
        sStr[m] = (iNum & n)   ? '1' : '0';
        if (m){
            m--;
            n = n << 1;
        }
    }
    while (m);
} //end int2binstr

void clock2str(char *sStr, unsigned long ms){
	unsigned char tick, len, i, h;
    
    if (!ms){
        tick = MasterClockTick;
        ms   = MasterClockMS;
        if (tick != MasterClockTick){
            // This is to avoid interim changes as operations with "long" are not atomic
            ms   = MasterClockMS;
        }
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
