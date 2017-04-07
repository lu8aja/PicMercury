/* 
 * File:   app_helpers.h
 * Author: Javier
 *
 * Created on February 21, 2017, 11:25 PM
 */
#include "lib_ring.h"

/*** MACROS ***/
// Bit Operation macros
#define bit_set(A,n)           ( A |=   (1 << n))                            // Set bit number n in byte b 
#define bit_clear(A,n)         ( A &= (~(1 << n)))                           // Clear bit number n in byte b
#define bit_flip(A,n)          ( A ^=   (1 << n))                            // Flip bit number n in byte b 
#define bit_read(A,n)          ((A  &   (1 << n)) >> n)                      // Read bit number n in byte b 
#define bit_write(A,n,v)       (v ? (A |= (1 << n)) : (A &= (~((1 << n)))))  // Write value v to bit n of A 
#define bit_mask_write(A,m,v)  (v ? (A |= m) : (A &= (~m)))                  // According to v set or clear based upon a mask
#define bit_is_set(A,n)        (A   & (1 << n))                              // Test if bit number n in byte b is set
#define bit_is_clear(A,n)      (!(A & (1 << n)))                             // Test if bit number n in byte b is clear
#define reciprocal(a, fp)      ( (( 1 << fp) + a - 1) / a )                  // Reciprocal 1/x without using floats
// String macros
#define strequal(a, b)         (strcmp(a, b) == 0)
#define tohex(a)               (((a) < 10) ? ((a) | 0x30) : ((a) + 0x37))

/*** PUBLIC PROTOTYPES ***/

// STDIO related
void          printReply(unsigned char idBuffer, const unsigned char nType, const unsigned char *pCmd, const unsigned char *pReply);
void          putch(const unsigned char byte);
void          print(const unsigned char *pStr);

// STRING
inline void   str_append(unsigned char *pStr, unsigned char cChar);
void          str_append_safe(unsigned char *pStr, unsigned char cChar, unsigned char nMaxlen);
void          str_dump(unsigned char *pDumped, const unsigned char *pStr, unsigned char nLen);

// CONVERSIONS
unsigned long str2long(const char *sStr);
void          byte2hex(const char cChar, char *sStr1);
void          int2hex(unsigned int iNum, char *sStr1);
void          str2hex(const char *sStr2, char *sStr1);
inline void   str2lower(unsigned char *pStr);
inline void   str2upper(unsigned char *pStr);
void          byte2binstr(char *sStr, unsigned char iNum);
void          int2binstr(char *sStr, unsigned int iNum);
void          any2binstr(char *sStr, unsigned long iNum, unsigned char nLen);
unsigned char hexstr2byte(unsigned char *sStr);
unsigned char hex2byte(unsigned char c);

// CLOCK
unsigned long Clock_getTime(void);
void          Clock_getStr(char *sStr, unsigned long ms);

// EEPROM
unsigned char EEPROM_read(unsigned char addr);
void          EEPROM_write(unsigned char addr, unsigned char val);

// PIN I/O
void pin_cfg(unsigned char nPort, unsigned char nBit, unsigned char nDirection);
void pin_write(unsigned char nPort, unsigned char nBit, unsigned char nVal);
unsigned char pin_read(unsigned char nPort, unsigned char nBit);

