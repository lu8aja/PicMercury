/* 
 * File:   service_softserial.h
 * Author: Javier
 *
 * Created on March 9, 2017, 9:50 PM
 */

#define LIB_SOFTSERIAL

#include "lib_transcoder.h"

#define SeoftSerial_Debug 1


#define SoftSerial_TX_ENABLED  1
#define SoftSerial_RX_ENABLED  2
#define SoftSerial_HALF_DUPLEX_FLAG 4
#define SoftSerial_FULL_DUPLEX_FLAG 8

#define SoftSerial_HALF_DUPLEX 7
#define SoftSerial_FULL_DUPLEX 11

#ifndef SoftSerial_sizeOutput
    #define SoftSerial_sizeOutput 16
#endif
#ifndef SoftSerial_sizeInput
    #define SoftSerial_sizeInput 16
#endif

typedef struct {
    union {
        unsigned char Configs;               // Status Register
        struct {
            unsigned Enabled:1;             // [01] Tx Enabled
            unsigned TxEnabled:1;           // [02] Tx Enabled
            unsigned RxEnabled:1;           // [04] Rx Enabled
            unsigned TxInvert:1;            // [08] Tx Invert
            unsigned RxInvert:1;            // [10] Rx Invert
            unsigned HalfDuplex:1;          // [20] 1 = HalfDuplex: Disables RX while TX, and TX while RX, (to suppress reentrant input or corrupting output, in a current loop)
            unsigned Transcode:1;           // [40] 1 = ASCII -> ITA2
            unsigned TxRepeated:1; // Repeated send, does not clear the out buffer and restarts the pointer (Only available when compiled with the DEBUG flag)
        };
    };
    union {
        unsigned char Status;               // Status Register
        struct {
            unsigned Debug:1;               // [01] Debug to console (Only available when compiled with the DEBUG flag)
            unsigned ErrorRxFraming:1;      // [02] Rx Framing Error (stop bit))
            unsigned ErrorRxOverflow:1;     // [04] Rx (Input) buffer overflow
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
    Transcoder_t *Output;      // Output buffer (must be binary safe)
    
    unsigned char RxState;     // State Machine: 0 = Off / 1 = 
    unsigned char RxTick;      // Time ticker in ms
    unsigned char RxByte;      // Current byte being shifted in
    unsigned char RxBit;       // Current bit
    Transcoder_t *Input;       // Input buffer (must be binary safe)
    
    #if SeoftSerial_Debug
    unsigned char SyncPort;    // Sync Port (A..E or 1..5)
    unsigned char SyncBit;     // Sync Pin Bit (0..7)
    #endif
} SoftSerial_t;

extern SoftSerial_t SoftSerial;

void        SoftSerial_init(SoftSerial_t *pSerial, unsigned char nTxPort, unsigned char nTxPin, unsigned char nTxInvert, unsigned char nRxPort, unsigned char nRxPin, unsigned char nRxInvert, unsigned char nDuplex, unsigned char nTranscode);
void        SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod);
inline void SoftSerial_tick(SoftSerial_t *pSerial);
inline void SoftSerial_service(SoftSerial_t *pSerial);
inline void SoftSerial_service_rx(SoftSerial_t *pSerial);
inline void SoftSerial_service_tx(SoftSerial_t *pSerial);
void        SoftSerial_cmd(Ring_t * pBuffer, unsigned char *pArgs);
inline unsigned char SoftSerial_read(SoftSerial_t *pSerial, unsigned char *pStr, unsigned char nMaxLen);
inline unsigned char SoftSerial_write(SoftSerial_t *pSerial, unsigned char *pStr);
