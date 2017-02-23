/* 
 * File:   app_helpers.h
 * Author: Javier
 *
 * Created on February 21, 2017, 11:25 PM
 */

#include "app_main.h"

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

void          printReply(const unsigned char nType, const unsigned char *pCmd, const unsigned char *pReply);
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
