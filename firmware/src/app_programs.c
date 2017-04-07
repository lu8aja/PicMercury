#include <string.h>
#include <stdio.h>
#include <stdbool.h>



#include "app_globals.h"
#include "app_main.h"
#include "service_program.h"
#include "service_i2c.h"
#include "service_keys.h"      // Keyboard diode matrix
#include "service_leds.h"      
#include "service_music.h"

#define MasterProgramsLen 3
const unsigned char *MasterPrograms[] = {
    "l steps 1\0d 38000\0t 440 5\0\0",
    "t play\0d 30000\0\0",
    "t 500 2\0d 30000\0run 2\0\0"
};


unsigned char isPrime(unsigned int nNumber);



unsigned char Program_custom_isPrime(unsigned char nStep){
    unsigned int nNumber;
    unsigned int nTest;
    
    nNumber = MasterProgram.Console ? MasterKeys.Input : MasterProgram.Input;
    
    if (nNumber){
        nTest = isPrime(nNumber);

        if (nTest > 1){
            sprintf(sStr5, "\r\nNO, %u es divisible por %u\r\n", nNumber, nTest);
        }
        else{
            sprintf(sStr5, "\r\nSI, %u ES PRIMO!\r\n", nNumber);
        }
    }
    else{
        strcpy(sStr5, "\r\nNO, 0 no es primo\r\n");
    }
    
    // Output locally
    print(sStr5);
    
    // Punch
    if ((MasterProgram.Console && MasterKeys.Address    & 512) 
    || (!MasterProgram.Console && MasterProgram.Address & 512)
    ){
        strcpy(sReply, "$punch ");
        strcat(sReply, sStr5);
        I2C_send(0, 0, CFG_I2C_ADDRESS_PUNCHER, 0, sReply);
    }
    // Serial
    if ((MasterProgram.Console && MasterKeys.Address    & 256) 
    || (!MasterProgram.Console && MasterProgram.Address & 256)
    ){
        strcpy(sReply, "$serial ");
        strcat(sReply, sStr5);
        I2C_send(0, 0, CFG_I2C_ADDRESS_PUNCHER, 0, sReply);
    }
    
    return 0;
}


unsigned char Program_custom_calcPrimes(unsigned char nStep){
    static unsigned int nNumber = 1;
    static unsigned int nCol    = 0;
    unsigned int nMax;    
    unsigned int nTest;
    
    nMax = MasterProgram.Console ? MasterKeys.Input : MasterProgram.Input;

    if (MasterProgram.Console && MasterKeys.Function >> 2 != 0x40){
        strcpy(sStr5, "\r\n- ABORTED -\r\n");
        nCol  = 0;
        nStep = 0;
        bit_clear(MasterLeds.Status, 0x0d);

    }
    else if (nStep == 0){
        nNumber = 1;
        nStep   = 1;
        strcpy(sStr5, "\r\nCalculando Primos:\r\n2, ");
        bit_set(MasterLeds.Status, 0x0d);
        // Quick hack, 2 is printed directly, and then we move +2 every time through all the odd numbers
    }
    else {
        nNumber++;
        nNumber++;
        while (nNumber <= nMax){
            nTest = isPrime(nNumber);    
            if (!nTest){
                break;
            }
            nNumber++;
            nNumber++;
        }
        
        if (nNumber <= nMax){
            sprintf(sStr1, "%u, ", nNumber);
        }
        else{
            strcpy(sStr1, "\r\n- FIN -\r\n");
            nCol  = 0;
            nStep = 0;
            bit_clear(MasterLeds.Status, 0x0d);
        }
        
        nCol += strlen(sStr1);
        if (nCol > 70){
            sprintf(sStr5, "\r\n%s", sStr1);
            nCol = strlen(sStr1);
        }
        else{
            strcpy(sStr5, sStr1);
        }
    }
    
    print(sStr5);
    
    // Punch
    if ((MasterProgram.Console && MasterKeys.Address    & 512) 
    || (!MasterProgram.Console && MasterProgram.Address & 512)
    ){
        strcpy(sReply, "$punch ");
        strcat(sReply, sStr5);
        I2C_send(0, 0, CFG_I2C_ADDRESS_PUNCHER, 0, sReply);
    }
    // Serial
    if ((MasterProgram.Console && MasterKeys.Address    & 256) 
    || (!MasterProgram.Console && MasterProgram.Address & 256)
    ){
        strcpy(sReply, "$serial ");
        strcat(sReply, sStr5);
        I2C_send(0, 0, CFG_I2C_ADDRESS_PUNCHER, 0, sReply);
    }
    return nStep;
}



unsigned char isPrime(unsigned int nNumber){
    unsigned int nTest;
    if (nNumber == 0 || nNumber == 1){
        return 1;
    }
    if (nNumber == 2){
        return 0;
    }
    if (nNumber % 2 == 0){
        return 2;
    }
    for (nTest = (nNumber >> 1) | 0x01; nTest > 1; nTest = nTest - 2){
        if (nNumber % nTest == 0){
            return nTest;
        }
    }
    return 0;
}



unsigned char Program_custom_doMusic(unsigned char nStep){
#ifdef LIB_MUSIC
    bool bOK = true;
    unsigned int  iFreq;
    unsigned char nPeriod;
    
    if (MasterKeys.Function >> 2 != 0x20){
        strcpy(sReply, "Aborted");
        nStep = 0;
        bit_clear(MasterLeds.Status, 0x0d);
    }
    else if (nStep == 0){
        iFreq = MasterKeys.Input << 1;
        
        bit_clear(MasterLeds.Status, 2);
        bit_clear(MasterLeds.Status, 3);
        
        if (iFreq < 25){
            bit_set(MasterLeds.Status, 3);
            strcpy(sReply, "TooSmall");
            bOK = false;
        }
        else if (iFreq > 4000){
            bit_set(MasterLeds.Status, 2);
            strcpy(sReply, "TooBig");
            bOK = false;
        }
        else{
            bit_set(MasterLeds.Status, 0x0d);
            // Calculate period from frequency
            nPeriod = (unsigned char) ((System_ClockTickCount * 500) / iFreq);
            Music_setSingleTone(nPeriod, 2000);
            sprintf(sReply, "T=%u", nPeriod);
            nStep   = 1;
        }
    }
    
    printReply(0, bOK | 2, "U_MUSIC", sReply);
#endif
    return nStep;
}