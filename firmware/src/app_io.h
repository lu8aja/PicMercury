/* 
 * File:   app_io.h
 * Author: Javier
 *
 * Created on February 21, 2017, 8:11 PM
 */

// 

#define CFG_BOARD_XTAL_FREQ 20000000

/*** KEYS Related ***/
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


    typedef struct {
            unsigned char out_port;
            unsigned char out_bit;
            unsigned char in_port;
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


/*** LED Related ***/
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

    
    /*
    #define MasterLedMapLen 16
    extern const unsigned char MasterLedMap[];
    */

/*** I2C Related ***/
    #define CFG_I2C_ADDRESS_MASTER  0x01       // NEVER change this one, it is used by slaves to notify master, nothing to do with actual I2C addresses!
    #define CFG_I2C_ADDRESS_CONSOLE 0b00010000 // Not actually used over the wire because it is master
    #define CFG_I2C_ADDRESS_PUNCHER 0b00100000
    #define CFG_I2C_ADDRESS_READER  0b01000000
    #define CFG_I2C_ADDRESS_CRTS    0b10000000

/*** SoftSerial Related ***/
    #define CFG_SOFTSERIAL_TX_Port        'B'
    #define CFG_SOFTSERIAL_TX_Pin          5
    #define CFG_SOFTSERIAL_TX_InvertData   1
    #define CFG_SOFTSERIAL_TX_InvertCtrl   1
    #define CFG_SOFTSERIAL_RX_Port        'B'
    #define CFG_SOFTSERIAL_RX_Pin          4
    #define CFG_SOFTSERIAL_RX_InvertData   0
    #define CFG_SOFTSERIAL_RX_InvertCtrl   1
    #define CFG_SOFTSERIAL_RX_DataBits     5
    #define CFG_SOFTSERIAL_RX_StopBits     1
    #define CFG_SOFTSERIAL_RX_Period       20
    #define CFG_SOFTSERIAL_HalfDuplex      1
    #define CFG_SOFTSERIAL_Transcode       3
/*** Misc ***/

#define INPUT   1
#define OUTPUT  0



