/* 
 * File:   service_puncher.h
 * Author: Javier
 *
 * Created on March 1, 2017, 1:36 AM
 */


#define LIB_PUNCHER

#define PUNCHER_TRIS TRISD        // Port tris
#define PUNCHER_LAT  LATD         // Port latch
#define PUNCHER_BIT_ADVANCE  0b10000000 // Bitmask for advancing the paper
#define PUNCHER_BIT_GUIDE    0b01000000 // Bitmask for the guide hole
#define PUNCHER_MASK_DATA    0b00011111 // Data mask
#define PUNCHER_MASK_SHIFT   0b00100000 // Shift mask (used for mode 1))

#define PUNCHER_ITA_LTRS     0b00011111 // 
#define PUNCHER_ITA_FIGS     0b00011011 // 
#define PUNCHER_ITA_NULL     0b00000000 // 


typedef struct {
    unsigned char Enabled;     // 0 = Off / 1 = On
    unsigned char Mode;        // Mode Bitfield Bit0 = 0:Direct / 1:shifted | Bit1 = 1:ASCII | Bit2 = 4:HEX
    unsigned char Shift;       // Mode 1 Shift: 0 = LTRS / 1 = FIGS
    unsigned char State;       // State Machine: 0 = Off / 1 = 
    unsigned char TimePunch;   // Time in ms to punch the holes
    unsigned char TimeGap1;    // Time in ms between punch and advance
    unsigned char TimeAdvance; // Time in ms to advance the paper
    unsigned char TimeGap2;    // Time in ms to rest before next command
    
    unsigned char Tick;        // Time ticker in ms
    unsigned char *Output;     // Output buffer (must be binary safe))
    unsigned char Len;         // Output buffer counter n..0
} puncher_t;

extern puncher_t MasterPuncher;


void          Puncher_init(unsigned char bEnabled, unsigned char nMode);
inline void   Puncher_tick(void);
void          Puncher_service(void);
unsigned char Puncher_write(unsigned char *buff, unsigned char len);
void          Puncher_cmd(unsigned char *pArgs);