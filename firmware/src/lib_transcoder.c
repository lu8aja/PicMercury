#include <xc.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "lib_transcoder.h"

#if (CFG_TRANSCODER_CODE == TRANSCODER_CODE_USTTY)
    // US-TTY
    const unsigned char txtTranscoderTable[] = {
    //       0     1     2     3    4    5     6    7    8     9    A    B     C    D    E    F
    // LTRS
    /*0*/    0  , 'E' , 0x0a, 'A', ' ', 'S' , 'I', 'U', 0x0d, 'D', 'R', 'J' , 'N', 'F', 'C', 'K',
    /*1*/    'T', 'Z' , 'L' , 'W', 'H', 'Y' , 'P', 'Q', 'O' , 'B', 'G', 0x0f, 'M', 'X', 'V', 0x0e,
    // FIGS
    /*2*/    0  , '3' , 0x0a, '-', ' ', '\'', '8', '7', 0x0d, '$', '4', '\a', ',', '!', ':', '(',
    /*3*/    '5', '\"', ')' , '2', '#', '6' , '0', '1', '9' , '?', '&', 0x0f, '.', '/', ';', 0x0e
    };
#endif


#if (CFG_TRANSCODER_CODE == TRANSCODER_CODE_ITA2)
    // ITA2
    const unsigned char txtTranscoderTable[] = {
    //       0     1     2     3    4    5     6    7    8     9    A    B     C    D    E    F
    // LTRS
    /*0*/    0  , 'E' , 0x0a, 'A', ' ', 'S' , 'I', 'U', 0x0d, 'D',  'R', 'J' , 'N', 'F', 'C', 'K',
    /*1*/    'T', 'Z' , 'L' , 'W', 'H', 'Y' , 'P', 'Q', 'O' , 'B',  'G', 0x0f, 'M', 'X', 'V', 0x0e,
    // FIGS
    /*2*/    0  , '3' , 0x0a, '-', ' ', '\'', '8', '7', 0x0d, 0x05, '4', 0x07, ',', '@', ':', '(',
    /*3*/    '5', '+' , ')' , '2', '$', '6' , '0', '1', '9' , '?',  '*', 0x0f, '.', '/', '=', 0x0e
    };
#endif

#if (CFG_TRANSCODER_CODE == TRANSCODER_CODE_ITA2_ES)
    // IT2-AR
    const unsigned char txtTranscoderTable[] = {
    //       0     1     2     3    4    5     6    7    8     9    A    B     C    D    E    F
    // LTRS
    /*0*/    0  , 'E' , 0x0a, 'A',  ' ', 'S' , 'I', 'U', 0x0d, 'D',  'R', 'J' , 'N', 'F', 'C', 'K',
    /*1*/    'T', 'Z' , 'L' , 'W',  'H', 'Y' , 'P', 'Q', 'O' , 'B',  'G', 0x0f, 'M', 'X', 'V', 0x0e,
    // FIGS
    /*2*/    0  , '3' , 0x0a, '-',  ' ', '\'', '8', '7', 0x0d, 0x05, '4', 0x07, ',', '$', ':', '(',
    /*3*/    '5', '+' , ')' , '2', 0xa5, '6' , '0', '1', '9' , '?',  '*', 0x0f, '.', '/', '=', 0x0e
    };
    // 0xa5 = Ñ in Code page 437
#endif


Transcoder_t * Transcoder_new(const unsigned char nSize){
 
    Transcoder_t *pTranscoder = (Transcoder_t *) Heap_alloc(sizeof(Transcoder_t));
    
    if (!pTranscoder){
        // Could not allocate the heap
        System.Error.PuncherOutput = 1;
        return NULL;
    }
    
    pTranscoder->Ring = ring_new(nSize);

    if (!pTranscoder->Ring){
        Heap_free(pTranscoder);
        System.Error.PuncherOutput = 1;
        // Could not allocate the ring buffer
        return NULL;
    }
    
    return pTranscoder;
}


unsigned char Transcoder_read(Transcoder_t *pTranscoder, unsigned char *pChar){
    unsigned char cIn;
    unsigned char cOut;
    unsigned char nAdv = 0;
    
    // Direct ASCII
    if (!ring_peep(pTranscoder->Ring, &cIn)){
        *pChar = 0;
        return 0;
    };
    nAdv = 1;
    
    
   if (pTranscoder->ModeIta2){
        cIn = toupper(cIn);

        // Find the corresponding ITA2 for the given ASCII
        // The search order is determined by the current shift
        if (pTranscoder->Shift){
            cOut = 64;
            do {
                cOut--;
                if (txtTranscoderTable[cOut] == cIn){
                    break;
                }
            } 
            while(cOut);
            
            if (cOut == 0){
                // Not found
                cOut = TRANSCODER_MASK_SHIFT | TRANSCODER_ITA_UNKNOWN; // A figs space
            }
        }
        else{
            cOut = 0;
            do {
                if (txtTranscoderTable[cOut] == cIn){
                    break;
                }
                cOut++;
            } 
            while(cOut < 64);
            
            if (cOut == 64){
                // Not found
                cOut = TRANSCODER_ITA_UNKNOWN; // A ltrs space
            }

        }
        //cOut |= TRANSCODER_MASK_NONZERO; // Add flag to avoid Mode0
    }
    else{
        cOut = cIn;
    }
    
    // Mode1 = ITA2+shift -> ITA2
    // Shift calculations with bit 6
    if (pTranscoder->ModeBit6){
        // 5 bit + Shift flag
        if (cOut & TRANSCODER_MASK_SHIFT){
            // Got FIGS
            if (!pTranscoder->Shift){
                // Must switch to figs first
                cOut = TRANSCODER_ITA_FIGS;
                nAdv = 0;
            }
        }
        else{
            // Got LTRS
            if (pTranscoder->Shift){
                // Must switch to ltrs first
                cOut = TRANSCODER_ITA_LTRS;
                pTranscoder->Shift = 0;
                nAdv = 0;
            }
        }
    }
    else{
        // Pure 5 bits mode
        cOut &= 0b00011111;
    }

    if (nAdv){
        // Move the pointer
        ring_read(pTranscoder->Ring, &cIn);
    }

    if (cOut == TRANSCODER_ITA_LTRS){
        pTranscoder->Shift = 0;
        if (pTranscoder->LinkedShifts && pTranscoder->LinkedConfigs){
            *pTranscoder->LinkedConfigs &= 0b01111111;
        }
    }
    else if (cOut == TRANSCODER_ITA_FIGS){
        pTranscoder->Shift = 1;
        if (pTranscoder->LinkedShifts && pTranscoder->LinkedConfigs){
            *pTranscoder->LinkedConfigs |= 0b10000000;
        }
    }
    
    *pChar = cOut;
    return 1;
}


unsigned char Transcoder_write(Transcoder_t *pTranscoder, unsigned char cIn, unsigned char bTest){
    unsigned char cOut;
    
    pTranscoder->LastPrintable = 0;
    
    if (pTranscoder->ModeIta2){
        
        if (cIn == TRANSCODER_ITA_FIGS){
            pTranscoder->Shift = 1;
            if (pTranscoder->LinkedShifts && pTranscoder->LinkedConfigs){
                *pTranscoder->LinkedConfigs |= 0b10000000;
            }
            pTranscoder->Char = 0x0f;
            return 1;
        }
        else if (cIn == TRANSCODER_ITA_LTRS){
            pTranscoder->Shift = 0;
            if (pTranscoder->LinkedShifts && pTranscoder->LinkedConfigs){
                *pTranscoder->LinkedConfigs &= 0b01111111;
            }
            pTranscoder->Char = 0x0e;
            return 1;
        }
        else if (cIn > sizeof(txtTranscoderTable)){
            cOut = TRANSCODER_ASCII_UNKNOWN;
            pTranscoder->Char = 0x00;
            if (pTranscoder->AvoidNull){
                // This is not a buffer overflow, so we flag as accepted
                return 1;
            }
        }
        else{
            if (pTranscoder->Shift) cIn |= TRANSCODER_MASK_SHIFT;
            cOut = txtTranscoderTable[cIn];
        }
        
    }
    else{
        // Direct binary (ASCII)
        cOut = cIn;
    }

    pTranscoder->Char = cOut;
    
    if (cOut || !pTranscoder->AvoidNull){
        if (cOut > 31 || cOut == '\r' || cOut == '\n'){
            pTranscoder->LastPrintable = 1;
        }
        if (!bTest){
            return ring_write(pTranscoder->Ring, cOut);
        }
        else{
            return 1;
        }

    }
    return 0;
}


