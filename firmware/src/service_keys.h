/* 
 * File:   service_keys.h
 * Author: Javier
 *
 * Created on March 1, 2017, 4:24 AM
 */

#define LIB_KEYS

typedef struct {
    unsigned char Enabled;      // 0 = Off / 1 = On
    unsigned char Time;         // Buttons check interval (ms) 0  is disabled (def:30)
    unsigned char Run;          // Allows running programs from keyboard using the prepulse button and the address row
    // Runtime counters
    unsigned char Tick;         // Tick counter (runtime)
    unsigned char Bit;          // Bit pointer 0..7    
    // Runtime status
    unsigned int  Function;      // Runtime value for Function keys
    unsigned int  Address;       // Runtime value for Address keys
    unsigned int  Input;         // Runtime value for Input keys
    unsigned int  Switches;      // Runtime value for misc console switches
} keys_t;


extern keys_t MasterKeys;

void Keys_init(void);
inline void Keys_tick(void);
inline void Keys_service(void);
void Keys_checkButtons(void);
void Keys_getKeys(void);
void Keys_getStatusReply(void);
inline unsigned char Keys_checkCmd(Ring_t * pBuffer, unsigned char *pCommand, unsigned char *pArgs);
void Keys_cmd(Ring_t * pBuffer, unsigned char *pArgs);
