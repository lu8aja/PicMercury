/* 
 * File:   service_monitor.h
 * Author: Javier
 *
 * Created on March 14, 2017, 9:50 PM
 */

#define LIB_MONITOR

typedef struct {
    struct {
        unsigned char Enabled:1;
        unsigned char A:1;
        unsigned char B:1;
        unsigned char C:1;
        unsigned char D:1;
        unsigned char E:1;
        unsigned char :1;
        unsigned char :1;
    } Configs;
    unsigned char Port[5];
    unsigned char Tris[5];
} monitor_t;

/*** Status of IO ***/

extern monitor_t MasterMonitor;

void Monitor_service(void);
void Monitor_checkPins(unsigned char cPortName);
inline unsigned char Monitor_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs);
void Monitor_cmd(unsigned char idBuffer, unsigned char *pArgs);



