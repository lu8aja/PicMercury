#include <string.h>
#include <stdio.h>
#include <stdbool.h>


#include "app_main.h"
#include "app_globals.h"

#include "service_i2c.h"
#include "service_keys.h"      // Keyboard diode matrix
#include "service_leds.h"      
#include "service_music.h"

#define MasterProgramsLen 3
const unsigned char *MasterPrograms[] = {
    "l steps 1\0d 38000\0t 440 5\0\0",
    "t play\0d 30000\0\0",
    "t 500 2\0d 30000\0run 2\0\0",
    "t keys\0d 500\0run keys\0\0",
    "serial on\0d 50\0serial off\0\0"
};


unsigned char isPrime(unsigned int nNumber);



unsigned char Program_custom_isPrime(unsigned char nStep){
    unsigned int nNumber = MasterKeys.Input;
    unsigned int nTest;
    
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
    if (MasterKeys.Address & 512){
        strcpy(sReply, "punch s ");
        strcat(sReply, sStr5);
        I2C_send(0, CFG_I2C_ADDRESS_PUNCHER, sReply);
    }
    else if (MasterKeys.Address & 256){
        strcpy(sReply, "serial ");
        strcat(sReply, sStr5);
        I2C_send(0, CFG_I2C_ADDRESS_PUNCHER, sReply);
    }
    
    return 0;
}


unsigned char Program_custom_calcPrimes(unsigned char nStep){
    static unsigned int nNumber = 1;
    static unsigned int nCol    = 0;
    
    unsigned int nTest;
    
    if (MasterKeys.Function >> 2 != 0x40){
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
        while (nNumber < 65530){
            nTest = isPrime(nNumber);    
            if (!nTest){
                break;
            }
            nNumber++;
            nNumber++;
        }
        
        if (nNumber < 65530){
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
    if (MasterKeys.Address & 512){
        strcpy(sReply, "punch s ");
        strcat(sReply, sStr5);
        I2C_send(0, CFG_I2C_ADDRESS_PUNCHER, sReply);
    }
    else if (MasterKeys.Address & 256){
        strcpy(sReply, "serial ");
        strcat(sReply, sStr5);
        I2C_send(0, CFG_I2C_ADDRESS_PUNCHER, sReply);
    }
    return nStep;
}



unsigned char isPrime(unsigned int nNumber){
    unsigned int nTest;
    if (nNumber == 0){
        return 1;
    }
    if (nNumber == 1){
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
            nPeriod = (unsigned char) ((MasterClockTickCount * 500) / iFreq);
            Music_setSingleTone(nPeriod, 2000);
            sprintf(sReply, "T=%u", nPeriod);
            nStep   = 1;
        }
    }
    
    printReply(0, bOK | 2, "U_MUSIC", sReply);
#endif
    return nStep;
}