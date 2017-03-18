/* 
 * File:   app_helpers.h
 * Author: Javier
 *
 * Created on February 21, 2017, 11:25 PM
 */

/** MACROS **/
#define strequal(a, b) (strcmp(a, b) == 0)

/** PROTOTYPES **/

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
void          any2binstr(char *sStr, unsigned long iNum, unsigned char nLen);

unsigned long Clock_getTime(void);
void          clock2str(char *sStr, unsigned long ms);
unsigned char hexstr2byte(unsigned char *sStr);
unsigned char hex2byte(unsigned char c);
inline void   str_append(unsigned char *pStr, unsigned char cChar);
void          str_append_safe(unsigned char *pStr, unsigned char cChar, unsigned char nMaxlen);


// EEPROM
unsigned char EEPROM_read(unsigned char addr);
void          EEPROM_write(unsigned char addr, unsigned char val);
