
#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "lib_ring.h"

#include "lib_transcoder.h"

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"



#define LIB_SOFTSERIAL

#include "service_softserial.h"

SoftSerial_t SoftSerial;


void SoftSerial_init(SoftSerial_t *pSerial, unsigned char nTxPort, unsigned char nTxPin, unsigned char nTxInvert, unsigned char nRxPort, unsigned char nRxPin, unsigned char nRxInvert, unsigned char nDuplex, unsigned char nTranscode){
    pSerial->Configs   = 0;
    pSerial->Status    = 0;
    pSerial->Debug     = 1;
    pSerial->TxPort    = nTxPort & 0x07;
    pSerial->TxPin     = nTxPin  & 0x07;
    pSerial->TxInvert  = nTxInvert;
    pSerial->RxPort    = nRxPort & 0x07;
    pSerial->RxPin     = nRxPin  & 0x07;
    pSerial->RxInvert  = nRxInvert;
    pSerial->HalfDuplex= nDuplex;
    
    pSerial->DataBits  = 8;
    pSerial->StopBits  = 1;
    pSerial->BitPeriod = 20;
    
    // Buffers should NEVER be created twice!
    if (pSerial->Output == NULL){
        pSerial->Output = Transcoder_new(SoftSerial_sizeOutput);
        if (pSerial->Output){
            pSerial->Output->Configs = nTranscode;
        }
        else{
            pSerial->ErrorRxOverflow = 1;
            //strcat(MasterDebugMsg, "O=E,");
        }
    }
    if (pSerial->Input == NULL){
        pSerial->Input  = Transcoder_new(SoftSerial_sizeInput);
        if (pSerial->Input){
            pSerial->Input->Configs = nTranscode;
            
            pSerial->Input->LinkedConfigs  = &pSerial->Output->Configs;
            pSerial->Output->LinkedConfigs = &pSerial->Input->Configs;
        }
        /*
        else{
            strcat(MasterDebugMsg, "I=E,");
        }
        */
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
        pSerial->Enabled   = 1;
        pSerial->TxEnabled = 1;
        ring_clear(pSerial->Output->Ring);
        pSerial->Output->Shift = 0;
    }
    
    if (nEnabled & 2){
        if (pSerial->RxPort > 0 && pSerial->RxPort < 6 && pSerial->RxPin < 8){
            pin_cfg(pSerial->RxPort, pSerial->RxPin, 1);
        }
        pSerial->RxTick    = 0;
        pSerial->RxState   = 0;
        pSerial->RxEnabled = 1;
        pSerial->Enabled   = 1;
        ring_clear(pSerial->Input->Ring);
        pSerial->Input->Shift = 0;
    }
    
    // Duplex mode
    if (nEnabled & 4){
        pSerial->HalfDuplex = 1;
    }
    else if (nEnabled & 8){
        pSerial->HalfDuplex = 0;
    }
    
    pSerial->Input->LinkedShifts  = pSerial->HalfDuplex;
    pSerial->Output->LinkedShifts = pSerial->HalfDuplex;

    
    
    #if SeoftSerial_Debug
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
    if (!pSerial->RxTick && pSerial->RxEnabled){
        SoftSerial_service_rx(pSerial);
    }
    // Output
    if (!pSerial->TxTick && pSerial->TxEnabled){
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
            
            if (pSerial->HalfDuplex) pSerial->TxEnabled = 0;
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
            if (pSerial->HalfDuplex) pSerial->TxEnabled = 1;
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
            if (pSerial->HalfDuplex) pSerial->TxEnabled = 1;
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
                printf("!FR IN  %lu ", MasterClock.MS);
                printf("S=%u T=%2u b=%u n=%u ", pSerial->RxState, pSerial->RxTick, pSerial->RxBit, nPin);
                byte2binstr(sStr4, pSerial->RxByte);
                printf("B=%s\r\n", sStr4);
            }
        }
        else{
            // Stop bit is correct, add to buffer
            if (!Transcoder_write(pSerial->Input, pSerial->RxByte)){
                // Buffer overflow
                pSerial->ErrorRxOverflow = 1;
                // Choose to either start overwriting the buffer, or discarding information
                // We are now discarding
            }
            
            #if SeoftSerial_Debug
            if (pSerial->Debug){
                byte2binstr(sStr4, pSerial->RxByte);
                ring_assert(pSerial->Input->Ring, sStr5, sizeof(sStr5), true);
                printf("%02u [%s] <%s>\r\n", ring_strlen(pSerial->Input->Ring), sStr4, sStr5);
            }
            #endif
            pSerial->RxState = 0;
            if (pSerial->HalfDuplex) pSerial->TxEnabled = 1;
        }
    }

    #if SeoftSerial_Debug
    if (pSerial->Debug){
        if (pSerial->RxTick || pSerial->RxState){
            printf("IN  %lu ", MasterClock.MS);
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
        if (Transcoder_read(pSerial->Output, &pSerial->TxByte)){
            pSerial->TxState = 1;
            if (pSerial->HalfDuplex) pSerial->RxEnabled = 0;
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
        #if SeoftSerial_Debug
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
        #if SeoftSerial_Debug
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
        if (Transcoder_read(pSerial->Output, &pSerial->TxByte)){
            pSerial->TxState = 1;
        }
        #if SeoftSerial_Debug
        else if (pSerial->TxRepeated){
            // The ring buffer frees as soon as the character is read, 
            // so we cannot simply restart the buffer to resend it
            // Instead we move the pointer to 0 and send whatever until tail
            // Of course this does not mean it will be in order!
            // After all this is just for testing...
            
            //pSerial->Output->Head = 0;
            pSerial->TxByte = pSerial->Output->Ring->Buffer[0];            
            pSerial->TxState = 1;
        }
        #endif
        else{
            pSerial->TxState = 0;
            if (pSerial->HalfDuplex) pSerial->RxEnabled = 1;
        }
    }

    #if SeoftSerial_Debug
    if (pSerial->Debug){
        if (pSerial->TxTick || pSerial->TxState){
            print("                                       ");
            printf("OUT %lu ", MasterClock.MS);
            printf("S=%u T=%u ", pSerial->TxState, pSerial->TxTick);
            byte2binstr(sStr1, pSerial->TxByte);
            printf("B=%s ", sStr1);
            printf("b=%u\r\n", bit_read(LATB, 2));
        }
    }
    #endif

}

inline unsigned char SoftSerial_read(SoftSerial_t *pSerial, unsigned char *pStr, unsigned char nMaxLen){
    return ring_str(pSerial->Input->Ring, pStr, nMaxLen, 1);
}

inline unsigned char SoftSerial_write(SoftSerial_t *pSerial, unsigned char *pStr){
    return ring_append(pSerial->Output->Ring, pStr);
}

void SoftSerial_cmd(unsigned char *pArgs){
    bool bOK = true;
    bool bShowStatus = true;
    unsigned char *pArg = NULL;
    unsigned char n          = 0;
    unsigned char c          = 0;
    unsigned char nLen       = 0;
    unsigned char nDataBits  = 0;
    unsigned char nStopBits  = 0;
    unsigned char nBitPeriod = 0;
    
    sReply[0] = 0x00;
    
    if (!pArgs[0]){
        
    }
    #if SeoftSerial_Debug
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
                ring_clear(SoftSerial.Output->Ring);
                ring_write(SoftSerial.Output->Ring, (unsigned char) atoi(pArg));
            }
        }
        else if(strequal(pArgs, "dump")){
            unsigned char *p = (unsigned char *) &SoftSerial;
            print("CONFIGS:\r\n");
            for (n = 0; n < sizeof(SoftSerial); n++){
                printf(" %02x", *p);
                p++;
            }
            print("\r\nOUT: ");
            nLen = ring_assert(SoftSerial.Output->Ring, sStr5, sizeof(sStr5), true);
            print(sStr5);
            print(txtCrLf);
            for (n = 0; n < nLen; n++){
                printf(" %02x %c", sStr5[n], sStr5[n] > 32 ? sStr5[n] : '_');
            }
            print("\r\nIN: ");
            nLen = ring_assert(SoftSerial.Input->Ring, sStr5, sizeof(sStr5), true);
            print(sStr5);
            print(txtCrLf);
            for (n = 0; n < nLen; n++){
                printf(" %02x %c", sStr5[n], sStr5[n] > 32 ? sStr5[n] : '_');
            }
            print(txtCrLf);
        }
        else if(strequal(pArgs, "dumptx")){
            printf("\r\nOUT: H:%u T:%u S:%u A:%u M:%2x Sh:%u\r\n", 
                SoftSerial.Output->Ring->Head,
                SoftSerial.Output->Ring->Tail,
                SoftSerial.Output->Ring->Size,
                ring_available(SoftSerial.Output->Ring),
                SoftSerial.Output->Configs,
                SoftSerial.Output->Shift
                );
            
            nLen = SoftSerial.Output->Ring->Size;
            for (n = 0; n < nLen; n++){
                c = SoftSerial.Output->Ring->Buffer[n];
                printf(" %02x %c", c, c > 32 ? c : '_');
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
        SoftSerial_enable(&SoftSerial, SoftSerial_TX_ENABLED | SoftSerial_RX_ENABLED, 0, 0, 0);
    }
    // on (DataBits) (StopBits) (Period) (HD) (TxInvert) (RxInvert)
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
                    pArg = strtok(NULL, txtWhitespace);
                    if (pArg){
                        SoftSerial.HalfDuplex = atoi(pArg) & 1;
                        pArg = strtok(NULL, txtWhitespace);
                        if (pArg){
                            SoftSerial.TxInvert = atoi(pArg) & 1;
                            pArg = strtok(NULL, txtWhitespace);
                            if (pArg){
                                SoftSerial.RxInvert = atoi(pArg) & 1;
                            }
                        }
                    }
                }
            }
        }
        
        SoftSerial_enable(&SoftSerial, SoftSerial_TX_ENABLED | SoftSerial_RX_ENABLED, nDataBits, nStopBits, nBitPeriod);
    }
    else{
        // We've got text to send
        if (SoftSerial.Enabled){
            if (strlen(pArgs) < ring_available(SoftSerial.Output->Ring)){
                ring_append(SoftSerial.Output->Ring, pArgs);
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
            strcpy(sReply, txtOff);
        }
    }
    
    if (bShowStatus){
        sprintf(sReply, "%s Tx=%s %u  (%c%u%c) Rx=%s %u (%c%u%c) %uN%u T=%u HD=%u OF=%u FR=%u", 
            SoftSerial.Enabled ? txtOn : txtOff,
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
            SoftSerial.HalfDuplex,
            SoftSerial.ErrorRxOverflow,
            SoftSerial.ErrorRxFraming
        );
    }
    
    printReply(bOK, "SERIAL", sReply);
}