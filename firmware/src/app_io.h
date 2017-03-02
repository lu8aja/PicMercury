/* 
 * File:   app_io.h
 * Author: Javier
 *
 * Created on February 21, 2017, 8:11 PM
 */

// 

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
extern button_map_t MasterButtonsMap[];

#define MasterButtonsHistoryBits 7
typedef struct {
        unsigned char status:1;
        unsigned char history:7;
} button_status_t;

#define MasterButtonsLen 3
extern button_status_t MasterButtons[];

#define MasterLedMapLen 16
extern const unsigned char MasterLedMap[];