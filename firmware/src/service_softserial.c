
#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "ring.h"

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"

#define LIB_SOFTSERIAL

#define LIB_SOFTSERIAL_DEBUG 1


#define SOFTSERIAL_TX_ENABLED 1
#define SOFTSERIAL_RX_ENABLED 2



#ifndef SoftSerial_sizeOutput
    #define SoftSerial_sizeOutput 16
#endif
#ifndef SoftSerial_sizeInput
    #define SoftSerial_sizeInput 16
#endif


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


SoftSerial_t SoftSerial;

void SoftSerial_init(SoftSerial_t *pSerial, unsigned char nTxPort, unsigned char nTxPin, unsigned char nTxInvert, unsigned char nRxPort, unsigned char nRxPin, unsigned char nRxInvert);
void SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod);
inline void SoftSerial_tick(SoftSerial_t *pSerial);
inline void SoftSerial_service(SoftSerial_t *pSerial);
inline void SoftSerial_service_rx(SoftSerial_t *pSerial);
inline void SoftSerial_service_tx(SoftSerial_t *pSerial);
void SoftSerial_cmd(unsigned char *pArgs);


void SoftSerial_init(SoftSerial_t *pSerial, unsigned char nTxPort, unsigned char nTxPin, unsigned char nTxInvert, unsigned char nRxPort, unsigned char nRxPin, unsigned char nRxInvert){
    pSerial->Status    = 0;
    pSerial->Debug     = 1;
    pSerial->TxPort    = nTxPort & 0x07;
    pSerial->TxPin     = nTxPin  & 0x07;
    pSerial->TxInvert  = nTxInvert;
    pSerial->RxPort    = nRxPort & 0x07;
    pSerial->RxPin     = nRxPin  & 0x07;
    pSerial->RxInvert  = nRxInvert;
    
    pSerial->DataBits  = 8;
    pSerial->StopBits  = 1;
    pSerial->BitPeriod = 20;
    
    // Buffers should NEVER be created twice!
    if (!pSerial->Output){
        pSerial->Output = ring_new(SoftSerial_sizeOutput);
        if (!pSerial->Output->Buffer){
            pSerial->ErrorRxOverflow = 1;
        }
    }
    if (!pSerial->Input){
        pSerial->Input  = ring_new(SoftSerial_sizeInput);
    }
}

void SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod){
    if (nDataBits)  pSerial->DataBits  = nDataBits;
    if (nStopBits)  pSerial->StopBits  = nStopBits;
    if (nBitPeriod) pSerial->BitPeriod = nBitPeriod;
    
    if (nEnabled & 1){
        if (pSerial->TxPort > 0 && pSerial->TxPort < 6 && pSerial->TxPin < 8){
            pin_cfg(pSerial->TxPort, pSerial->TxPin, 0);
            pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvert);
        }
        pSerial->TxTick    = 0;
        pSerial->TxState   = 0;
        pSerial->TxEnabled = 1;
        ring_clear(pSerial->Output);
    }
    
    if (nEnabled & 2){
        if (pSerial->RxPort > 0 && pSerial->RxPort < 6 && pSerial->RxPin < 8){
            pin_cfg(pSerial->RxPort, pSerial->RxPin, 1);
        }
        pSerial->RxTick    = 0;
        pSerial->RxState   = 0;
        pSerial->RxEnabled = 1;
        ring_clear(pSerial->Input);
    }
    #if LIB_SOFTSERIAL_DEBUG
    pSerial->SyncPort = 3;
    pSerial->SyncBit  = 6;
    pin_cfg(SoftSerial.SyncPort, SoftSerial.SyncBit, 0);
    #endif
}

inline void SoftSerial_tick(SoftSerial_t *pSerial){
    if (pSerial->TxTick){
        pSerial->TxTick--;
    }
    if (pSerial->RxTick){
        pSerial->RxTick--;
    }
}

inline void SoftSerial_service(SoftSerial_t *pSerial){

    // INPUT
    if ((pSerial->Status & 0x02) && !pSerial->RxTick){
        SoftSerial_service_rx(pSerial);
    }
    // Output
    if ((pSerial->Status & 0x01) && !pSerial->TxTick){
        SoftSerial_service_tx(pSerial);
        
    }
    
}

inline void SoftSerial_service_rx(SoftSerial_t *pSerial){
    unsigned char nPin;
    
    nPin = pin_read(pSerial->RxPort, pSerial->RxPin) ^ pSerial->RxInvert;
    // 0 - Idle
    if (pSerial->RxState == 0){
        if (nPin){
            pSerial->RxState++;
            pSerial->RxTick  = pSerial->BitPeriod >> 2; // 1/4 period
        }
    }
    // 1 Sample again at quarter of Start Bit
    else if (pSerial->RxState == 1){
        if (nPin){
            pSerial->RxState++;
            pSerial->RxTick  = pSerial->BitPeriod >> 2; // 1/4 period
        }
        else{
            // False start, reset to 0
            pSerial->RxState = 0; 
        }
    }
    // 2 Sample middle of Start Bit
    else if (pSerial->RxState == 2){
        if (nPin){
            pSerial->RxState++;
            pSerial->RxByte  = 0;
            pSerial->RxBit   = 0;
            pSerial->RxTick  = pSerial->BitPeriod;
        }
        else{
            // False start, reset to 0
            pSerial->RxState = 0; 
        }
    }
    // 3 Sample middle of Data Bits
    else if (pSerial->RxState == 3){
        pSerial->RxByte  = pSerial->RxByte >> 1;
        if (nPin){
            pSerial->RxByte |= 1 << (pSerial->DataBits - 1);
        }
        pSerial->RxBit++;
        if (pSerial->RxBit >= pSerial->DataBits){
            // End of data bits, now go to stop bit
            pSerial->RxState++;
        }
        pSerial->RxTick = pSerial->BitPeriod;
    }
    // 4 Sample middle of Stop Bit
    else if (pSerial->RxState == 4){
        // Finish
        if (nPin){
            // Framing error (Invalid Stop Bit)
            // Flag it but keep waiting for the stop bit for 1/8t
            pSerial->ErrorRxFraming = 1;
            pSerial->RxTick = pSerial->BitPeriod >> 3;
            if (pSerial->Debug){
                printf("!FR IN  %u ", MasterClockMS);
                printf("S=%u T=%2u b=%u n=%u ", pSerial->RxState, pSerial->RxTick, pSerial->RxBit, nPin);
                byte2binstr(sStr4, pSerial->RxByte);
                printf("B=%s\r\n", sStr4);
            }
        }
        else{
            // Stop bit is correct, add to buffer
            if (ring_available(pSerial->Input)){
                ring_write(pSerial->Input, pSerial->RxByte);
            }
            else{
                // Buffer overflow
                pSerial->ErrorRxOverflow = 1;
                // Choose to either start overwriting the buffer, or discarding information
                // We are now discarding
            }
            
            #if LIB_SOFTSERIAL_DEBUG
            if (pSerial->Debug){
                byte2binstr(sStr4, pSerial->RxByte);
                ring_assert(pSerial->Input, sStr5, sizeof(sStr5), true);
                printf("%02u [%s] <%s>\r\n", ring_strlen(pSerial->Input), sStr4, sStr5);
            }
            #endif
            pSerial->RxState = 0;
        }
    }

    #if LIB_SOFTSERIAL_DEBUG
    if (pSerial->Debug){
        if (pSerial->RxTick || pSerial->RxState){
            printf("IN  %u ", MasterClockMS);
            printf("S=%u T=%2u b=%u n=%u ", pSerial->RxState, pSerial->RxTick, pSerial->RxBit, nPin);
            byte2binstr(sStr4, pSerial->RxByte);
            printf("B=%s\r\n", sStr4);
        }
    }
    #endif
}

inline void SoftSerial_service_tx(SoftSerial_t *pSerial){
    // 0 - Idle
    if (!pSerial->TxState){
        
        if (ring_read(pSerial->Output, &pSerial->TxByte)){
            pSerial->TxState = 1;
        }
        else{
            return;
        }
    }

    // 1 - Start bit
    if (pSerial->TxState == 1){
        pSerial->TxTick  = pSerial->BitPeriod;
        pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvert ? 0 : 1);
        pSerial->TxState = 2;
        #if LIB_SOFTSERIAL_DEBUG
            pin_write(pSerial->SyncPort, pSerial->SyncBit, 1);
        #endif
    }
    // 2..9 - Data bits
    else if (pSerial->TxState < 10){
        pSerial->TxTick = pSerial->BitPeriod;
        pin_write(pSerial->TxPort, pSerial->TxPin, (pSerial->TxByte & 0x01) ^ pSerial->TxInvert);
        pSerial->TxByte = pSerial->TxByte >> 1;
        if ((pSerial->TxState - 1) >= pSerial->DataBits){
            pSerial->TxState = 100;
        }
        else{
            pSerial->TxState++;
        }
        #if LIB_SOFTSERIAL_DEBUG
            pin_write(pSerial->SyncPort, pSerial->SyncBit, 0);
        #endif
    }
    // 100..254 - Stop bits
    else if (pSerial->TxState < 255){
        pSerial->TxTick = pSerial->BitPeriod;
        pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvert ? 1 : 0);
        if ((pSerial->TxState - 99) >= pSerial->StopBits){
            pSerial->TxState = 255;
        }
        else{
            pSerial->TxState++;
        }
    }
    // 255 - End
    else {
        if (ring_read(pSerial->Output, &pSerial->TxByte)){
            pSerial->TxState = 1;
        }
        #if LIB_SOFTSERIAL_DEBUG
        else if (pSerial->TxRepeated){
            // The ring buffer frees as soon as the character is read, 
            // so we cannot simply restart the buffer to resend it
            // Instead we move the pointer to 0 and send whatever until tail
            // Of course this does not mean it will be in order!
            // After all this is just for testing...
            
            //pSerial->Output->Head = 0;
            pSerial->TxByte = pSerial->Output->Buffer[0];            
            pSerial->TxState = 1;
        }
        #endif
        else{
            pSerial->TxState = 0;
        }
    }

    #if LIB_SOFTSERIAL_DEBUG
    if (pSerial->Debug){
        if (pSerial->TxTick || pSerial->TxState){
            print("                                       ");
            printf("OUT %u ", MasterClockMS);
            printf("S=%u T=%u ", pSerial->TxState, pSerial->TxTick);
            byte2binstr(sStr1, pSerial->TxByte);
            printf("B=%s ", sStr1);
            printf("b=%u\r\n", bit_read(LATB, 2));
        }
    }
    #endif

}

inline unsigned char SoftSerial_read(SoftSerial_t *pSerial, unsigned char *pStr, unsigned char nMaxLen){
    return ring_str(pSerial->Input, pStr, nMaxLen, 1);
}

inline unsigned char SoftSerial_write(SoftSerial_t *pSerial, unsigned char *pStr){
    return ring_append(pSerial->Output, pStr);
}

void SoftSerial_cmd(unsigned char *pArgs){
    bool bOK = true;
    bool bShowStatus = true;
    unsigned char *pArg = NULL;
    unsigned char n          = 0;
    unsigned char nLen       = 0;
    unsigned char nDataBits  = 0;
    unsigned char nStopBits  = 0;
    unsigned char nBitPeriod = 0;
    
    sReply[0] = 0x00;
    
    if (!pArgs[0]){
        
    }
    #if LIB_SOFTSERIAL_DEBUG
        else if(strequal(pArgs, "debug")){
            SoftSerial.Debug ^= 1;
        }
        else if(strequal(pArgs, "repeat")){
            SoftSerial.TxRepeated ^= 1;
        }
        else if((pArgs[0] == 's' && pArgs[1] == 'y' && pArgs[2] == 'n' && pArgs[3] == 'c' && pArgs[4] == ' ')){
            pArg = strtok(pArgs, txtWhitespace); // get rid of the sync
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                SoftSerial.SyncPort = (unsigned char) atoi(pArg);
                pArg = strtok(NULL, txtWhitespace);
                if (pArg){
                    SoftSerial.SyncBit = (unsigned char) atoi(pArg);
                }
            }
            printf("SYNC %c %u\r\n", SoftSerial.SyncPort + 'A' - 1, SoftSerial.SyncBit);
            pin_cfg(SoftSerial.SyncPort, SoftSerial.SyncBit, 0);
        }
        else if(pArgs[0] == 'x' && pArgs[1] == ' ' ){
            pArg = strtok(pArgs, txtWhitespace); // the "on"
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                ring_clear(SoftSerial.Output);
                ring_write(SoftSerial.Output, (unsigned char) atoi(pArg));
            }
        }
        else if(strequal(pArgs, "dump")){
            unsigned char *p = &SoftSerial;
            print("CONFIGS:\r\n");
            for (n = 0; n < sizeof(SoftSerial); n++){
                printf(" %02x", *p);
                p++;
            }
            print("\r\nOUT: ");
            nLen = ring_assert(SoftSerial.Output, sStr5, sizeof(sStr5), true);
            print(sStr5);
            print(txtCrLf);
            for (n = 0; n < nLen; n++){
                printf(" %02x %c", sStr5[n], sStr5[n] > 32 ? sStr5[n] : '_');
            }
            print("\r\nIN: ");
            nLen = ring_assert(SoftSerial.Input, sStr5, sizeof(sStr5), true);
            print(sStr5);
            print(txtCrLf);
            for (n = 0; n < nLen; n++){
                printf(" %02x %c", sStr5[n], sStr5[n] > 32 ? sStr5[n] : '_');
            }
            print(txtCrLf);
        }
        else if(strequal(pArgs, "dumptx")){
            print("\r\nOUT: ");
            nLen = SoftSerial.Output->Size;
            print(txtCrLf);
            for (n = 0; n < nLen; n++){
                printf(" %02x %c", SoftSerial.Output->Buffer[n], SoftSerial.Output->Buffer[n] > 32 ? SoftSerial.Output->Buffer[n] : '_');
            }
            print(txtCrLf);
        }

    #endif
    else if(strequal(pArgs, "off")){
        SoftSerial.Status   = 0;
        SoftSerial.TxState = 0;
        SoftSerial.RxState  = 0;
    }
    else if (strequal(pArgs, "on")){
        SoftSerial_enable(&SoftSerial, SOFTSERIAL_TX_ENABLED | SOFTSERIAL_RX_ENABLED, 0, 0, 0);
    }
    else if(pArgs[0] == 'o' && pArgs[1] == 'n' && pArgs[2] == ' '){
        pArg = strtok(pArgs, txtWhitespace); // the "on"
        pArg = strtok(NULL, txtWhitespace);
        if (pArg){
            nDataBits = (unsigned char) atoi(pArg);
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                nStopBits = (unsigned char) atoi(pArg);
                pArg = strtok(NULL, txtWhitespace);
                if (pArg){
                    nBitPeriod = (unsigned char) atoi(pArg);
                }
            }
        }
        
        SoftSerial_enable(&SoftSerial, SOFTSERIAL_TX_ENABLED | SOFTSERIAL_RX_ENABLED, nDataBits, nStopBits, nBitPeriod);
    }
    else{
        // We've got text to send
        if (SoftSerial.Status & 1){
            if (strlen(pArgs) < ring_available(SoftSerial.Output)){
                ring_append(SoftSerial.Output, pArgs);
                bShowStatus = false;
            }
            else {
                bOK = false;
                strcpy(sReply, txtErrorTooBig);
                bShowStatus = false;
            }
            
        }
        else{
            bOK = false;
        }
    }
    
    if (bShowStatus){
        sprintf(sReply, "On Tx=%s %u  (%c%u%c) Rx=%s %u (%c%u%c) %uN%u T=%u OF=%u FR=%u", 
            SoftSerial.RxEnabled ? txtOn : txtOff,
            SoftSerial.TxState,
            SoftSerial.TxPort + 'A' - 1,
            SoftSerial.TxPin,
            SoftSerial.TxInvert ? 'i' : 'n',
            SoftSerial.RxEnabled ? txtOn : txtOff,
            SoftSerial.RxState,
            SoftSerial.RxPort + 'A' - 1,
            SoftSerial.RxPin,
            SoftSerial.RxInvert ? 'i' : 'n',
            SoftSerial.DataBits,
            SoftSerial.StopBits,
            SoftSerial.BitPeriod,
            SoftSerial.ErrorRxOverflow,
            SoftSerial.ErrorRxFraming

        );
    }
    
    printReply(bOK, "SERIAL", sReply);
}