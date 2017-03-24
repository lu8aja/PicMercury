/* 
 * File:   service_softserial.h
 * Author: Javier Albinarrate (LU8AJA)
 *
 * Created on March 9, 2017, 9:50 PM
 */

#define LIB_SOFTSERIAL

#include "lib_transcoder.h"

#define SeoftSerial_Debug           1


// CONSTANTS
#define SoftSerial_DISABLE          0x00
#define SoftSerial_TX_ENABLE        0x01
#define SoftSerial_RX_ENABLE        0x02
#define SoftSerial_HALF_DUPLEX_FLAG 0x04
#define SoftSerial_FULL_DUPLEX_FLAG 0x08

#define SoftSerial_HALF_DUPLEX 7
#define SoftSerial_FULL_DUPLEX 11

#ifndef SoftSerial_sizeOutput
    #define SoftSerial_sizeOutput 16
#endif
#ifndef SoftSerial_sizeInput
    #define SoftSerial_sizeInput 16
#endif

// TYPES
typedef struct {
    union {
        unsigned char Status;               // Status Register
        struct {
            unsigned Enabled:1;             // [01] Tx Enabled
            unsigned Debug:1;               // [02] Debug to console (Only available when compiled with the DEBUG flag)
            unsigned ErrorRxFraming:1;      // [04] Rx Framing Error (stop bit))
            unsigned ErrorRxOverflow:1;     // [08] Rx (Input) buffer overflow
            unsigned TxRepeated:1;          // [10] Repeated send, does not clear the out buffer and restarts the pointer (Only available when compiled with the DEBUG flag)
        };
    };
    union {
        unsigned char Configs;               // Status Register
        struct {
            unsigned TxEnabled:1;           // [01] Tx Enabled
            unsigned TxInvertData:1;        // [02] Tx Invert Data bits
            unsigned TxInvertCtrl:1;        // [04] Tx Invert Control bits
            unsigned RxEnabled:1;           // [08] Rx Enabled
            unsigned RxInvertData:1;        // [10] Rx Invert Data bits
            unsigned RxInvertCtrl:1;        // [20] Rx Invert Control bits
            unsigned HalfDuplex:1;          // [40] 1 = HalfDuplex: Disables RX while TX, and TX while RX, (to suppress reentrant input or corrupting output, in a current loop)
            unsigned Transcode:1;           // [80] 1 = ASCII -> ITA2

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

// PROTOTYPES
void SoftSerial_init(
    SoftSerial_t *pSerial,
    unsigned char nTxPort,
    unsigned char nTxPin,
    unsigned char nTxInvertData,
    unsigned char nTxInvertCtrl,
    unsigned char nRxPort,
    unsigned char nRxPin,
    unsigned char nRxInvertData,
    unsigned char nRxInvertCtrl,
    unsigned char nDuplex
);

void                 SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled);
void                 SoftSerial_config(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod, unsigned char nTranscode);
inline void          SoftSerial_tick(SoftSerial_t *pSerial);
inline void          SoftSerial_service(SoftSerial_t *pSerial);
inline void          SoftSerial_service_rx(SoftSerial_t *pSerial);
inline void          SoftSerial_service_tx(SoftSerial_t *pSerial);
inline unsigned char SoftSerial_read(SoftSerial_t *pSerial, unsigned char *pStr, unsigned char nMaxLen);
inline unsigned char SoftSerial_write(SoftSerial_t *pSerial, unsigned char *pStr);
inline unsigned char SoftSerial_checkCmd(Ring_t * pBuffer, unsigned char pCommand, unsigned char *pArgs);
void                 SoftSerial_cmd(Ring_t * pBuffer, unsigned char *pArgs);
