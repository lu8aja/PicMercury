
#include "service_usb.h"

extern void APP_executeCommand(unsigned char idBuffer, unsigned char *pLine);



inline void USB_service(void){
    if(USB_getDeviceState() < CONFIGURED_STATE || USB_isDeviceSuspended()){
        return;
    }
    
    USB_input();
    
    USB_output();
}

void USB_input(void){
    uint8_t i;
    uint8_t n;
    uint8_t nBytes;
    if ( USB_getDeviceState() < CONFIGURED_STATE || USB_isDeviceSuspended() || !USBUSARTIsTxTrfReady()){
        // Ensure we have USB up, but also clear the output buffer before trying to do more stuff
        return;
    }
    
    do {
        nBytes = getsUSBUSART(bufChunk, 1);
        if (nBytes > 0) {
            /* For every byte that was read... */
            if (bufChunk[0] == 0x0D || bufChunk[0] == 0x0A){
                if (posUsbCommand){
                    bufUsbCommand[posUsbCommand] = 0x00;
                    APP_executeCommand(0, bufUsbCommand);
                    posUsbCommand = 0;
                    bufUsbCommand[0] = 0x00;
                    break;
                }
            }
            else{
                bufUsbCommand[posUsbCommand] = bufChunk[0];
                posUsbCommand++;
                bufUsbCommand[posUsbCommand] = 0x00;
            }
        }
    }
    while(nBytes);
}

void USB_output(void){
    unsigned char len;
    if ( USB_getDeviceState() < CONFIGURED_STATE || USB_isDeviceSuspended()){
        return;
    }

    if(posOutput > 0 && USBUSARTIsTxTrfReady()){
        len = posOutput >= sizeOutUsb ? sizeOutUsb : posOutput;
        strncpy(bufUsbTmp, bufUsbOutput, len);
        putUSBUSART(bufUsbTmp, len);
        strcpy(bufUsbOutput, &bufUsbOutput[len]);
        posOutput = posOutput - len;
        bufUsbOutput[posOutput] = 0x00;
    }

    CDCTxService();
}

void USB_configured(void){
    CDCInitEP();
    line_coding.bCharFormat = 0;
    line_coding.bDataBits   = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate   = 9600;
    
    printReply(0, 3, "VERSION", txtVersion);
}

