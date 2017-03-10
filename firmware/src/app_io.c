#include <xc.h>

#include "app_globals.h"

#define BOARD_XTAL_FREQ 20000000

#define PIN_HOOT_OUT  LATCbits.LATC2
#define PIN_HOOT_TRIS TRISCbits.RC2

#define PIN_KEYS_OUT_0 LATEbits.LATE0 
#define PIN_KEYS_OUT_1 LATEbits.LATE1 
#define PIN_KEYS_OUT_2 LATEbits.LATE2 
#define PIN_KEYS_OUT_3 LATCbits.LATC0 
#define PIN_KEYS_OUT_4 LATCbits.LATC1

#define PIN_KEYS_OUT_0_TRIS TRISEbits.RE0 
#define PIN_KEYS_OUT_1_TRIS TRISEbits.RE1 
#define PIN_KEYS_OUT_2_TRIS TRISEbits.RE2 
#define PIN_KEYS_OUT_3_TRIS TRISCbits.RC0 
#define PIN_KEYS_OUT_4_TRIS TRISCbits.RC1


#define TTY_TX        TRISBbits.RB2        // TX Pin
#define SOFTSERIAL_TX_TRIS   TRISBbits.RB2        // TX Tris
#define TTY_RX        TRISBbits.RB2        // RX Pin
#define SOFTSERIAL_RX_TRIS   TRISBbits.RB2        // RX Tris



#define PIN_KEYS_IN      PORTD
#define PIN_KEYS_IN_TRIS TRISD

#define LEDS_ANODES          0b00111100
#define LEDS_ANODES_SHIFT    2
#define LEDS_ANODES_TRIS     TRISB
#define LEDS_ANODES_LAT      LATB

#define LEDS_CATHODES        0b00001111
#define LEDS_CATHODES_SHIFT  0
#define LEDS_CATHODES_TRIS   TRISA
#define LEDS_CATHODES_LAT    LATA

#define LED_USB    0x0f
#define LED_ALARM  0x00

#define I2C_ADDRESS_PUNCHER 2
#define I2C_ADDRESS_READER  4
#define I2C_ADDRESS_CRTS    8

#define INPUT   1
#define OUTPUT  0

typedef struct {
        volatile unsigned char *out_port;
        unsigned char out_bit;
        volatile unsigned char *in_port;
        unsigned char in_bit;
} button_map_t;

#define MasterButtonsMapLen 3
button_map_t MasterButtonsMap[] = {
    {&LATC, 0, &PORTD, 0}, // C0/D0 CLR_TAPE
    {&LATC, 0, &PORTD, 1}, // C0/D1 INITIAL_TRANSFER
    {&LATC, 1, &PORTD, 1}  // C1/D1 PREPULSE
};

#define MasterButtonsHistoryBits 7
typedef struct {
        unsigned char status:1;
        unsigned char history:7;
} button_status_t;

#define MasterButtonsLen 3
button_status_t MasterButtons[] = {
    {0, 0}, // C0/D0 CLR_TAPE
    {0, 0}, // C0/D1 INITIAL_TRANSFER
    {0, 0}  // C1/D1 PREPULSE
};

// First nibble goes to RB5~2 as 1, second nibble goes to RA3~0
/* No longer used
#define MasterLedMapLen 16
const unsigned char MasterLedMap[] = {
                // n name B5432 A3210,
    0b11101110, // 0 B2A0 0001   0001 LED_FUSE_ALARM
    0b11101101, // 1 B2A1 0001   0010 LED_MAG
    0b11101011, // 2 B2A2 0001   0100 LED_SAC_POSITIVE
    0b11100111, // 3 B2A3 0001   1000 LED_SAC_NOTZERO
    0b11011110, // 4 B3A0 0010   0001 LED_ACC_MULT
    0b11011101, // 5 B3A1 0010   0010 LED_MAG_PARITY
    0b11011011, // 6 B3A2 0010   0100 LED_BTEST_POSITIVE
    0b11010111, // 7 B3A3 0010   1000 LED_BTEST_NOTZERO
    0b11111111, // 8 B4A0 0100   0001 DOES NOT EXIST IN HARDWARE
    0b10111101, // 9 B4A1 0100   0010 LED_PARITY_STOP
    0b10111011, // A B4A2 0100   0100 LED_SHIFT31
    0b10110111, // B B4A3 0100   1000 LED_STOP_FLIPFLOP
    0b01111110, // C B5A0 1000   0001 LED_AUTO
    0b01111101, // D B5A1 1000   0010 LED_CONTINUOUS
    0b01111011, // E B5A2 1000   0100 LED_INHIBIT_PARITY
    0b01110111, // F B5A3 1000   1000 LED_WRITE_CURRENT 
};
*/




void pin_write(unsigned char nPort, unsigned char nBit, unsigned char nVal);
unsigned char pin_read(unsigned char nPort, unsigned char nBit);
void pin_cfg(unsigned char nPort, unsigned char nBit, unsigned char nDirection);

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