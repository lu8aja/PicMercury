
#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"

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
    unsigned char TxPos;       // Current byte position in buffer
    unsigned char *Output;     // Output buffer (must be binary safe)
    
    unsigned char RxState;     // State Machine: 0 = Off / 1 = 
    unsigned char RxTick;      // Time ticker in ms
    unsigned char RxByte;      // Current byte being shifted in
    unsigned char RxBit;       // Current bit
    unsigned char RxPos;       // Current byte position in buffer
    unsigned char *Input;      // Output buffer (must be binary safe)
    #if LIB_SOFTSERIAL_DEBUG
    unsigned char SyncPort;    // Sync Port (A..E or 1..5)
    unsigned char SyncBit;     // Sync Pin Bit (0..7)
    #endif
} SoftSerial_t;


SoftSerial_t SoftSerial;

#define sizeSerialOutput 32
unsigned char bufSerialOutput[sizeSerialOutput + 1];
unsigned char bufSerialInput[16];

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
        pSerial->TxTick      = 0;
        pSerial->TxState     = 0;
        pSerial->TxPos        = 0;
        pSerial->Output       = &bufSerialOutput;
        pSerial->Output[0]    = 0;
        pSerial->Status      |= 1;
    }
    
    if (nEnabled & 2){
        if (pSerial->RxPort > 0 && pSerial->RxPort < 6 && pSerial->RxPin < 8){
            pin_cfg(pSerial->RxPort, pSerial->RxPin, 1);
        }
        pSerial->RxTick       = 0;
        pSerial->RxState      = 0;
        pSerial->RxPos        = 0;
        pSerial->Input        = &bufSerialInput;
        pSerial->Input[0]     = 0;
        pSerial->Status      |= 2;
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
            //ring_write(pSerial->Input, pSerial->RxByte);
            pSerial->Input[pSerial->RxPos] = pSerial->RxByte;
            pSerial->RxPos++;
            if (pSerial->RxPos > 15){
                // Buffer overflow
                pSerial->ErrorRxOverflow = 1;
                // Choose to either start overwriting the buffer, or discarding information
                // Overwrite
                //pSerial->RxPos = 0;
                // Discard
                pSerial->RxPos--;
            }
            pSerial->Input[pSerial->RxPos] = 0;
            #if LIB_SOFTSERIAL_DEBUG
            if (pSerial->Debug){
                byte2binstr(sStr4, pSerial->RxByte);
                printf("%02u [%s] <%s>\r\n", pSerial->RxPos, sStr4, pSerial->Input);
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
        pSerial->TxByte = pSerial->Output[pSerial->TxPos];
        if (pSerial->TxByte){
            pSerial->TxPos++;
        //if (ring_read(pSerial->Output, &(pSerial->TxByte))){
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
        pSerial->TxByte = pSerial->Output[pSerial->TxPos];
        if (pSerial->TxByte){
            pSerial->TxPos++;
            //if (ring_read(pSerial->Output, &(pSerial->TxByte))){
            pSerial->TxState = 1;
        }
        else {
            #if LIB_SOFTSERIAL_DEBUG
            if (pSerial->TxRepeated){
                pSerial->TxState = 1;
                pSerial->TxPos   = 0;
                pSerial->TxByte  = pSerial->Output[0];
                pSerial->TxPos++;
            }
            else{
                pSerial->TxState = 0;
                pSerial->TxPos = 0;
                pSerial->Output[pSerial->TxPos] = 0;
            }
            #endif
        }

    }

    #if LIB_SOFTSERIAL_DEBUG
    if (pSerial->Debug){
        if (pSerial->TxTick || pSerial->TxState){
            print("                                       ");
            printf("OUT %u ", MasterClockMS);
            printf("n=%u S=%u T=%u ", pSerial->TxPos, pSerial->TxState, pSerial->TxTick);
            byte2binstr(sStr1, pSerial->TxByte);
            printf("B=%s ", sStr1);
            printf("b=%u\r\n", bit_read(LATB, 2));
        }
    }
    #endif

}


void SoftSerial_cmd(unsigned char *pArgs){
    bool bOK = true;
    bool bShowStatus = true;
    unsigned char *pArg = NULL;
    unsigned char n          = 0;
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
                SoftSerial.Output[0] = (unsigned char) atoi(pArg);
                SoftSerial.Output[1] = 0;
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
            print(SoftSerial.Output);
            print(txtCrLf);
            for (n = 0; n <= sizeSerialOutput; n++){
                printf(" %02x %c", SoftSerial.Output[n], SoftSerial.Output[n] > 32 ? SoftSerial.Output[n] : '_');
            }
            print("\r\nIN: ");
            print(SoftSerial.Input);
            print(txtCrLf);
            for (n = 0; n <= 15; n++){
                printf(" %02x %c", SoftSerial.Input[n], SoftSerial.Input[n] > 32 ? SoftSerial.Input[n] : '_');
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
        SoftSerial_init(&SoftSerial, 'B', 2, 1, 'B', 3, 1);
        SoftSerial_enable(&SoftSerial, 3, 0, 0, 0);
    }
    /*
    else if (SoftSerial.TxState){
        bOK = false;
        strcpy(sReply, txtErrorBusy);
        bShowStatus = false;
    }
     * */
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
        
        SoftSerial_init(&SoftSerial, 'B', 2, 1, 'B', 3, 1);
        SoftSerial_enable(&SoftSerial, 3, nDataBits, nStopBits, nBitPeriod);
    }
    else{
        // We've got text to send
        if (SoftSerial.Status & 1){
            if (strlen(pArgs) < sizeSerialOutput){
                strcpy(SoftSerial.Output, pArgs);
                bShowStatus = false;
                printf("<%s> %u", SoftSerial.Output, strlen(SoftSerial.Output));
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
        sprintf(sReply, "On Tx=%s %u  %u (%c%u%c) Rx=%s %u %u (%c%u%c) %uN%u T=%u OF=%u FR=%u", 
            SoftSerial.RxEnabled ? txtOn : txtOff,
            SoftSerial.TxState,
            SoftSerial.TxPos,
            SoftSerial.TxPort + 'A' - 1,
            SoftSerial.TxPin,
            SoftSerial.TxInvert ? 'i' : 'n',
            SoftSerial.RxEnabled ? txtOn : txtOff,
            SoftSerial.RxState,
            SoftSerial.RxPos,
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