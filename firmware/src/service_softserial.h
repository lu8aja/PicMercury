/* 
 * File:   service_softserial.h
 * Author: Javier
 *
 * Created on March 9, 2017, 9:50 PM
 */

#define LIB_SOFTSERIAL

#define LIB_SOFTSERIAL_DEBUG 1

typedef struct {
    union {
        unsigned char Status;               // Status Register
        struct {
            unsigned TxEnabled:1;           // [01] Tx Enabled
            unsigned RxEnabled:1;           // [02] Rx Enabled
            unsigned TxInvert:1;            // [04] Tx Invert
            unsigned RxInvert:1;            // [08] Rx Invert
            unsigned Debug:1;               // [10] Debug to console (Only available when compiled with the DEBUG flag)
            unsigned TxRepeated:1;          // [20] Repeated send, does not clear the out buffer and restarts the pointer (Only available when compiled with the DEBUG flag)
            unsigned ErrorRxFraming:1;      // [40] Rx Framing Error (stop bit))
            unsigned ErrorRxOverflow:1;     // [80] Rx (Input) buffer overflow
        };
    };
    unsigned char BitPeriod;   // Bit period in ms
    unsigned char DataBits;    // Data Bits (1 .. 8)
    unsigned char StopBits;    // Stop Bits (1 .. 8)
    unsigned char TxPort;      // TX Output Port (A..E or 1..5)
    unsigned char TxPin;       // TX Pin Bit (0..7)
    unsigned char RxPort;      // RX Input Port (A..E or 1..5)
    unsigned char RxPin;       // RX Pin Bit (0..7)

    unsigned char TxState;     // State Machine: 0 = Off / 1 = 
    unsigned char TxTick;      // Time ticker in ms
    unsigned char TxByte;      // Current byte being shifted out
    ring_t       *Output;      // Output buffer (must be binary safe)
    
    unsigned char RxState;     // State Machine: 0 = Off / 1 = 
    unsigned char RxTick;      // Time ticker in ms
    unsigned char RxByte;      // Current byte being shifted in
    unsigned char RxBit;       // Current bit
    ring_t       *Input;       // Input buffer (must be binary safe)
    #if LIB_SOFTSERIAL_DEBUG
    unsigned char SyncPort;    // Sync Port (A..E or 1..5)
    unsigned char SyncBit;     // Sync Pin Bit (0..7)
    #endif
} SoftSerial_t;


extern SoftSerial_t SoftSerial;

void SoftSerial_init(SoftSerial_t *pSerial, unsigned char nTxPort, unsigned char nTxPin, unsigned char nTxInvert, unsigned char nRxPort, unsigned char nRxPin, unsigned char nRxInvert);

void SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod);

inline void SoftSerial_tick(SoftSerial_t *pSerial);

inline void SoftSerial_service(SoftSerial_t *pSerial);

void SoftSerial_cmd(unsigned char *pArgs);

inline unsigned char SoftSerial_read(SoftSerial_t *pSerial, unsigned char *pStr, unsigned char nMaxLen);

inline unsigned char SoftSerial_write(SoftSerial_t *pSerial, unsigned char *pStr);
