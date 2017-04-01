
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
#include "lib_helpers.h"
#include "app_io.h"



#define LIB_SOFTSERIAL

#include "service_softserial.h"

SoftSerial_t SoftSerial;


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
){
    pSerial->Status       = 0;
    pSerial->Configs      = 0;
    pSerial->CfgDebug     = 0;
    pSerial->RxColumn     = 0;
    
    pSerial->TxPort       = nTxPort & 0x07;
    pSerial->TxPin        = nTxPin  & 0x07;
    pSerial->TxInvertData = nTxInvertData;
    pSerial->TxInvertCtrl = nTxInvertCtrl;
    pSerial->RxPort       = nRxPort & 0x07;
    pSerial->RxPin        = nRxPin  & 0x07;
    pSerial->RxInvertData = nRxInvertData;
    pSerial->RxInvertCtrl = nRxInvertCtrl;
    pSerial->HalfDuplex   = nDuplex;
    
    // Lets default to 8N1 at 110 bauds
    pSerial->DataBits     = 8;
    pSerial->StopBits     = 1;
    pSerial->BitPeriod    = 9;
    
    pSerial->RxCommands   = 1; // Enable command handling by default
    pSerial->Debug        = 0;
    pSerial->RxEchoToUsb  = 1;
    
    // Buffers should NEVER be created twice!
    if (pSerial->Output == NULL){
        pSerial->Output = Transcoder_new(SoftSerial_sizeOutput);
        if (!pSerial->Output){
            // TODO find a better way to handle errors
            pSerial->ErrorRxOverflow = 1;
        }
    }
    if (pSerial->Input == NULL){
        pSerial->Input  = Transcoder_new(SoftSerial_sizeInput);
        if (!pSerial->Input){
            // TODO find a better way to handle errors
            pSerial->ErrorRxOverflow = 1;
        }
    }
    // Link buffers, used for HalfDuplex where the loop is shared 
    // between TX and RX, thus shift status is shared as well
    if (pSerial->Input && pSerial->Output){
        pSerial->Input->LinkedConfigs  = &pSerial->Output->Configs;
        pSerial->Output->LinkedConfigs = &pSerial->Input->Configs;
    }
}


void SoftSerial_enable(SoftSerial_t *pSerial, unsigned char nEnabled){
    if (nEnabled & SoftSerial_TX_ENABLE){
        if (pSerial->TxPort > 0 && pSerial->TxPort < 6 && pSerial->TxPin < 8){
            // Configure output pin, and output the idle value
            pin_cfg(pSerial->TxPort, pSerial->TxPin, 0);
            pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvertCtrl);
        }
        pSerial->Enabled   = 1;
        pSerial->TxTick    = 0;
        pSerial->TxState   = 0;
        pSerial->TxEnabled = 1;
        ring_clear(pSerial->Output->Ring);
        pSerial->Output->Shift = 0;
    }
    
    if (nEnabled & SoftSerial_RX_ENABLE){
        if (pSerial->RxPort > 0 && pSerial->RxPort < 6 && pSerial->RxPin < 8){
            // Configure input pin
            pin_cfg(pSerial->RxPort, pSerial->RxPin, 1);
        }
        pSerial->Enabled   = 1;
        pSerial->RxTick    = 0;
        pSerial->RxState   = 0;
        pSerial->RxEnabled = 1;
        ring_clear(pSerial->Input->Ring);
        pSerial->Input->Shift = 0;
    }
    
    // Duplex mode
    if (nEnabled & SoftSerial_HALF_DUPLEX_FLAG){
        pSerial->HalfDuplex = 1;
    }
    else if (nEnabled & SoftSerial_FULL_DUPLEX_FLAG){
        pSerial->HalfDuplex = 0;
    }
    
    pSerial->Input->LinkedShifts  = pSerial->HalfDuplex;
    pSerial->Output->LinkedShifts = pSerial->HalfDuplex;

    #if SeoftSerial_Debug
        pSerial->SyncPort = 3;
        pSerial->SyncBit  = 6;
        pin_cfg(pSerial->SyncPort, pSerial->SyncBit, 0);
    #endif
}


void SoftSerial_config(SoftSerial_t *pSerial, unsigned char nEnabled, unsigned char nDataBits, unsigned char nStopBits, unsigned char nBitPeriod, unsigned char nTranscode){
    if (nDataBits)  pSerial->DataBits  = nDataBits;
    if (nStopBits)  pSerial->StopBits  = nStopBits;
    if (nBitPeriod) pSerial->BitPeriod = nBitPeriod;

    pSerial->Input->Configs  = nTranscode;
    pSerial->Output->Configs = nTranscode;

    SoftSerial_enable(pSerial, nEnabled);
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
    unsigned char  nChar;
    
    // Due to half duplex logic, RX must go first to eventually block TX
    
    // INPUT
    if (!pSerial->RxTick && pSerial->RxEnabled){
        SoftSerial_service_rx(pSerial);
        
        if (pSerial->RxCommandRun){
            nChar = ring_findChr(pSerial->Input->Ring, '\n', 0);
            if (nChar < 255){
                if (nChar >= sizeof(bufCommand)){
                    printReply(pSerial->Output->Ring, 0, "TTY", txtErrorTooBig);
                    ring_write(pSerial->Output->Ring, '\r');
                    ring_write(pSerial->Output->Ring, '\n');
                    if (pSerial->RxEchoToUsb){
                        print("\r\nHEAP OVERFLOW: ");
                        printf("%u ", nChar);
                    }
                    ring_clear(pSerial->Input->Ring);
                }
                else{
                    
                    ring_str(pSerial->Input->Ring, bufCommand,  nChar, 0);
                    ring_get(pSerial->Input->Ring); // Discard \n
                    if (pSerial->RxEchoToUsb){
                        print("\r\nSERIAL CMD: ");
                        print(bufCommand);
                    }
                    APP_executeCommand(pSerial->Output->Ring, bufCommand);
                    ring_write(pSerial->Output->Ring, '\r');
                    ring_write(pSerial->Output->Ring, '\n');
                    
                    if (pSerial->RxEchoToUsb){
                        print(sReply);
                    }
                }
                
                if (ring_findChr(pSerial->Input->Ring, '\n', 0) == 255){
                    pSerial->RxCommandRun = 0;        
                }
            }
        }
    }

    // OUTPUT
    if (!pSerial->TxTick && pSerial->TxEnabled){
        SoftSerial_service_tx(pSerial);
        
    }
    
}

inline void SoftSerial_service_rx(SoftSerial_t *pSerial){
    unsigned char nPin;
    unsigned char nChar;
    unsigned char nAppend;
    
    // Read
    nPin = pin_read(pSerial->RxPort, pSerial->RxPin);

    // Perform inversions if needed
    if (pSerial->RxState == 3){
        nPin ^= pSerial->RxInvertData;
    }
    else {
        nPin ^= pSerial->RxInvertCtrl;
    }
        
    // STATUS MACHINE
    // 0 - Idle
    if (pSerial->RxState == 0){
        if (!nPin){
            pSerial->RxState++;
            pSerial->RxTick  = pSerial->BitPeriod >> 2; // 1/4 period
            
            if (pSerial->HalfDuplex) pSerial->TxEnabled = 0;
        }
    }
    // 1 Sample again at quarter of Start Bit
    else if (pSerial->RxState == 1){
        if (!nPin){
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
        if (!nPin){
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
        if (!nPin){
            // Framing error (Invalid Stop Bit)
            // Flag it but keep waiting for the stop bit for 1/8t
            pSerial->ErrorRxFraming = 1;
            pSerial->RxTick = pSerial->BitPeriod >> 3;
            if (pSerial->Debug && 0){
                printf("!FR IN  %lu ", MasterClock.MS);
                printf("S=%u T=%2u b=%u n=%u ", pSerial->RxState, pSerial->RxTick, pSerial->RxBit, nPin);
                byte2binstr(sStr4, pSerial->RxByte);
                printf("B=%s\r\n", sStr4);
            }
        }
        else{
            // Stop bit is correct, add to buffer or do something with it
            
            
            byte2binstr(sStr1, pSerial->RxByte);
            printf("[%s](%c)\r\n", sStr1, pSerial->RxByte);
            if (pSerial->RxEcho){
                // If we have a FullDuplex then Rx loop is separate from Tx loop, so what you write at the TTY
                // is not being printed at the paper, which is annoying, so we can echo it back
                if (pSerial->TxState == 0){
                    pSerial->TxByte = pSerial->RxByte;
                    pSerial->TxState = 1;
                }
                // else {
                // NOTE: If we are in full duplex, TxState might not be 0, and we would need to 
                // prepend into the ring buffer, which in turn might be full causing trouble
                // I am not implementing this case, because FullDuplex usage in real mechanical TTYs is unlikely
                // }
            }
            
            // COMMANDS are enabled
            // NOTE: This whole thing of commands being done here sucks, but I am in a hurry
            // TODO: Rework into something more decent be it at the transcoder or something separate
            if (pSerial->RxCommands){
                // Track column
                if (pSerial->Input->ModeIta2){
                    // ITA2
                    if (pSerial->RxByte == TRANSCODER_ITA_CR){
                        pSerial->RxColumn = 0;
                        nAppend = 0;
                        // We intentionally exclude CRs from the cmd line
                    }
                    else if (pSerial->RxByte == TRANSCODER_ITA_LF){
                        if (pSerial->RxCommandStart){
                            pSerial->RxCommandRun   = 1;
                            pSerial->RxCommandStart = 0;
                            // We need to ensure that one and only one LF is added to the cmd line
                            nAppend = 1;
                        }
                    }
                    else if (pSerial->RxByte != TRANSCODER_ITA_FIGS 
                        &&   pSerial->RxByte != TRANSCODER_ITA_LTRS
                        &&   pSerial->RxByte != TRANSCODER_ITA_NULL
                    ){
                        // Check if line is command (Column 0 && $)
                        if (!pSerial->RxColumn 
                            && pSerial->Input->Shift
                            && pSerial->RxByte == CFG_SOFTSERIAL_CommandCharIta2){
                            pSerial->RxCommandStart = 1;
                            nAppend = 0;
                        }
                        else{
                            nAppend = pSerial->RxCommandStart;
                        }
                        pSerial->RxColumn++;
                    }
                }
                else{
                    // ASCII
                    if (pSerial->RxByte == '\r'){
                        pSerial->RxColumn = 0;
                        nAppend = 0;
                        // We intentionally exclude CRs from the cmd line
                    }
                    else if (pSerial->RxByte == '\n'){
                        if (pSerial->RxCommandStart){
                            pSerial->RxCommandRun   = 1;
                            pSerial->RxCommandStart = 0;
                            // We need to ensure that one and only one LF is added to the cmd line
                            nAppend = 1;
                        }
                    }
                    else if (pSerial->RxByte > 31){
                        // Check if line is command (Column 0 && $)
                        if (!pSerial->RxColumn && pSerial->RxByte == CFG_SOFTSERIAL_CommandCharAscii){
                            pSerial->RxCommandStart = 1;
                            nAppend = 0;
                        }
                        else{
                            nAppend = pSerial->RxCommandStart;
                        }
                        pSerial->RxColumn++;
                    }
                }
            
                // Add to buffer or simply test the character
                if (!Transcoder_write(pSerial->Input, pSerial->RxByte, !nAppend)){
                    // Buffer overflow
                    pSerial->ErrorRxOverflow = 1;
                    // Choose to either start overwriting the buffer, or discarding information

                    // We are now discarding the entire buffer
                    ring_clear(pSerial->Input->Ring);
                    pSerial->RxCommandStart = 0;
                }
                
                if (pSerial->RxEchoToUsb && pSerial->Input->LastPrintable && pSerial->Input->Char){
                    putch(pSerial->Input->Char);
                }

            }
            else{
                // If commands are not enabled, we don't want to add to the buffer permanently,
                // but we need to convert it and keep shift status of the line
                // so we write in test mode
                Transcoder_write(pSerial->Input, pSerial->RxByte, 1);
                if (pSerial->RxEchoToUsb && pSerial->Input->LastPrintable && pSerial->Input->Char){
                     putch(pSerial->Input->Char);
                }
            }
            
            #if SeoftSerial_Debug
            if (pSerial->Debug){
                byte2binstr(sStr4, pSerial->RxByte);
                ring_assert(pSerial->Input->Ring, sStr5, sizeof(sStr5), true);
                printf("%02u [%s] <%s> %u\r\n", ring_strlen(pSerial->Input->Ring), sStr4, sStr5, pSerial->RxColumn);
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
        pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvertCtrl ? 0 : 1);
        pSerial->TxState = 2;
        #if SeoftSerial_Debug
            pin_write(pSerial->SyncPort, pSerial->SyncBit, 1);
        #endif
    }
    // 2..9 - Data bits
    else if (pSerial->TxState < 10){
        pSerial->TxTick = pSerial->BitPeriod;
        pin_write(pSerial->TxPort, pSerial->TxPin, (pSerial->TxByte & 0x01) ^ pSerial->TxInvertData);
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
        pin_write(pSerial->TxPort, pSerial->TxPin, pSerial->TxInvertCtrl ? 1 : 0);
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


inline unsigned char SoftSerial_checkCmd(Ring_t * pBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "cfg serial")){
        SoftSerial_cmd_cfg(pBuffer, pArgs);
        return 1;
    }
    else if (strequal(pCommand, "serial")){
        SoftSerial_cmd(pBuffer, pArgs);
        return 1;
    }
    return 0;
}


void SoftSerial_cmd_cfg(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    bool bShowStatus = true;
    unsigned char *pArg = NULL;
    SoftSerial_t *pSerial    = &SoftSerial; // For convenience
    unsigned char n          = 0;
    unsigned char c          = 0;
    unsigned char nLen       = 0;

    
    sReply[0] = 0x00;
    
    pArg = strtok(pArgs, txtWhitespace);
    
    if (!pArg){
        // Do nothing, just print data
    }
    #if SeoftSerial_Debug
        else if(strequal(pArg, "debug")){
            SoftSerial.Debug ^= 1;
        }
        else if(strequal(pArg, "repeat")){
            SoftSerial.TxRepeated ^= 1;
        }
        else if(strequal(pArg, "sync")){
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
        else if(strequal(pArg, "val")){
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                ring_clear(SoftSerial.Output->Ring);
                ring_write(SoftSerial.Output->Ring, (unsigned char) atoi(pArg));
            }
        }
        else if(strequal(pArg, "dump")){
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
        else if(strequal(pArg, "dumptx")){
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
    else if(strequal(pArg, "off")){
        SoftSerial.Enabled  = 0;
        SoftSerial.Status   = 0;
        SoftSerial.TxState  = 0;
        SoftSerial.RxState  = 0;
    }
    // on (DataBits) (StopBits) (Period) (Mode) (HD) (TxInvert) (RxInvert)
    else if (strequal(pArg, "on")){
        // Note: Horrible arrow pattern, but it is cheaper in ROM
        pArg = strtok(NULL, txtWhitespace);
        if (pArg){
            pSerial->DataBits = (unsigned char) atoi(pArg);
            pArg = strtok(NULL, txtWhitespace);
            if (pArg){
                pSerial->StopBits = (unsigned char) atoi(pArg);
                pArg = strtok(NULL, txtWhitespace);
                if (pArg){
                    pSerial->BitPeriod = (unsigned char) atoi(pArg);
                    pArg = strtok(NULL, txtWhitespace);
                    if (pArg){
                        pSerial->Input->Configs = atoi(pArg);
                        pSerial->Output->Configs = pSerial->Input->Configs;
                        pArg = strtok(NULL, txtWhitespace);
                        if (pArg){
                            pSerial->HalfDuplex = atoi(pArg) & 1;
                            pArg = strtok(NULL, txtWhitespace);
                            if (pArg){
                                n = atoi(pArg);
                                pSerial->TxInvertData = n & 1;
                                pSerial->TxInvertCtrl = (n & 2) >> 1;
                                pArg = strtok(NULL, txtWhitespace);
                                if (pArg){
                                    n = atoi(pArg);
                                    pSerial->RxInvertData = n & 1;
                                    pSerial->RxInvertCtrl = (n & 2) >> 1;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        SoftSerial_enable(&SoftSerial, SoftSerial_TX_ENABLE | SoftSerial_RX_ENABLE);
    }
    
    if (bShowStatus){
        sprintf(sReply, "%s Tx=%s %u  (%c%u%c%c) Rx=%s %u (%c%u%c%c) %uN%u T=%u HD=%u OF=%u FR=%u", 
            SoftSerial.Enabled      ? txtOn : txtOff,
            SoftSerial.RxEnabled    ? txtOn : txtOff,
            SoftSerial.TxState,
            SoftSerial.TxPort + 'A' - 1,
            SoftSerial.TxPin,
            SoftSerial.TxInvertCtrl ? 'i' : 'n',
            SoftSerial.TxInvertData ? 'i' : 'n',
            SoftSerial.RxEnabled    ? txtOn : txtOff,
            SoftSerial.RxState,
            SoftSerial.RxPort + 'A' - 1,
            SoftSerial.RxPin,
            SoftSerial.RxInvertCtrl ? 'i' : 'n',
            SoftSerial.RxInvertData ? 'i' : 'n',
            SoftSerial.DataBits,
            SoftSerial.StopBits,
            SoftSerial.BitPeriod,
            SoftSerial.HalfDuplex,
            SoftSerial.ErrorRxOverflow,
            SoftSerial.ErrorRxFraming
        );
    }
    
    printReply(pBuffer, bOK, "SERIAL", sReply);
}

void SoftSerial_cmd(Ring_t * pBuffer, unsigned char *pArgs){
    bool bOK = true;
    SoftSerial_t *pSerial    = &SoftSerial; // For convenience
    unsigned char nLen       = 0;

    
    sReply[0] = 0x00;
    
    nLen = strlen(pArgs);
    if (!nLen){
        bOK = false;
        strcpy(sReply, txtErrorMissingArgument);
    }
    else{
        // We've got text to send
        if (SoftSerial.Enabled){
            if (nLen < ring_available(SoftSerial.Output->Ring)){
                ring_append(SoftSerial.Output->Ring, pArgs);
            }
            else {
                bOK = false;
                strcpy(sReply, txtErrorTooBig);
            }
        }
        else{
            bOK = false;
            strcpy(sReply, txtOff);
        }
    }
    
    printReply(pBuffer, bOK, "SERIAL", sReply);
}