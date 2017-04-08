#include <xc.h>
#include <stdio.h>
#include <string.h>
#include "lib_ring.h"


Ring_t * ring_new(const unsigned char nSize){
    Ring_t *pRing = (Ring_t *) Heap_alloc(sizeof(Ring_t));
    
    if (!pRing){
        return NULL;
    }
    
    // For security to avoid buffer overruns a 0 is at the end of the buffer
    pRing->Buffer = Heap_alloc(nSize);
    if (!pRing->Buffer){
        pRing->Size = 0;
        pRing->Head   = 0;
        pRing->Tail   = 0;
    }
    else{
        pRing->Size   = nSize - 1; 
        pRing->Head   = pRing->Size - 1;
        pRing->Tail   = pRing->Head;

        unsigned char n = nSize;
        while (n){
            pRing->Buffer[n] = 0;
            n--;
        }
        pRing->Buffer[n] = 0;
    }
    return pRing;
}


inline void ring_clear(Ring_t *Ring){
    Ring->Head = Ring->Size - 1;
    Ring->Tail = Ring->Head;
}

unsigned char ring_strlen(Ring_t *Ring){
	return Ring->Tail >= Ring->Head ? Ring->Tail - Ring->Head : Ring->Size - Ring->Head + Ring->Tail;
}

inline unsigned char ring_available(Ring_t *Ring){
    if (!Ring){
        return 0;
    }
	if (Ring->Tail >= Ring->Head){
        return Ring->Size - Ring->Tail + Ring->Head;
    }
    else{
        return Ring->Head - Ring->Tail;
    }
}

unsigned char ring_write(Ring_t *Ring, unsigned char data){
    Ring->Tail = Ring->Tail + 1;
    if (Ring->Tail >= Ring->Size) Ring->Tail = 0;
    if (Ring->Tail == Ring->Head){
        // After incrementing we stepped into the head, so undo it
    	if (Ring->Tail){
    	    Ring->Tail = Ring->Tail - 1;
    	}
    	else{
    	    Ring->Tail = Ring->Size - 1;
    	}
    	return 0;
    }
    Ring->Buffer[Ring->Tail] = data;
    return 1;
}

unsigned char ring_read(Ring_t *Ring, unsigned char *Data){
	if (Ring->Tail == Ring->Head){
	    // Quickly detect empty condition
		return 0;
	}
    Ring->Head = Ring->Head + 1;
	if (Ring->Head >= Ring->Size) Ring->Head = 0;
	*Data = Ring->Buffer[Ring->Head];
    if (Ring->Tail == Ring->Head){
        // We have emptied the buffer, lets move the pointers
        // This is merely for convenience so it is more likely the buffer will be continuous
        Ring->Head = Ring->Size - 1;
        Ring->Tail = Ring->Head;
    }
	return 1;
}

unsigned char ring_get(Ring_t *Ring){
    unsigned char nChar;
	if (Ring->Tail == Ring->Head){
	    // Quickly detect empty condition
		return 0;
	}
    Ring->Head = Ring->Head + 1;
	if (Ring->Head >= Ring->Size) Ring->Head = 0;
	nChar = Ring->Buffer[Ring->Head];
    if (Ring->Tail == Ring->Head){
        // We have emptied the buffer, lets move the pointers
        // This is merely for convenience so it is more likely the buffer will be continuous
        Ring->Head = Ring->Size - 1;
        Ring->Tail = Ring->Head;
    }
	return nChar;
}

unsigned char ring_peep(Ring_t *Ring, unsigned char *Data){
	if (Ring->Tail == Ring->Head){
	    // Quickly detect empty condition and buffer overrun
		return 0;
	}
    unsigned char nRead = Ring->Head;
	nRead++;
	if (nRead >= Ring->Size) nRead = 0;
    *Data = Ring->Buffer[nRead];
    
	return 1;
}

unsigned char ring_peep_pos(Ring_t *Ring, unsigned char *Data, unsigned char nPos){
	if (Ring->Tail == Ring->Head || nPos >= Ring->Size){
	    // Quickly detect empty condition and buffer overrun
		return 0;
	}
    unsigned char nRead = Ring->Head;
	nRead++;
    nRead += nPos;
	if (nRead >= Ring->Size) nRead -= Ring->Size;
    if ((Ring->Tail > Ring->Head && (nRead >= Ring->Tail || nRead <= Ring->Head))
        ||
        (Ring->Tail < Ring->Head && nRead >= Ring->Tail && nRead <= Ring->Head)
    ){
        // Trying to read outside the buffer
		return 0;
    }
    
    
	*Data = Ring->Buffer[nRead];
	return 1;
}

unsigned char ring_tail(Ring_t *Ring){
	if (Ring->Tail == Ring->Head){
	    // Quickly detect empty condition
		return 0;
	}
	return Ring->Buffer[Ring->Tail];
}

unsigned char ring_assert(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull){
	unsigned char n = 0;

    unsigned char nHead = Ring->Head;
    
	while (Ring->Tail != nHead) {
        nHead++;
        if (nHead >= Ring->Size) nHead = 0;
        
        *pStr = Ring->Buffer[nHead];
        
		if (bHaltNull && *pStr == 0){
            // For null terminated strings we don't want to count the null
			return n;
		}
        
		n++;
		pStr++;

		if (n >= nMaxLen){
			*pStr = 0;
			return n;
		}
	}
    *pStr = 0;
	return n;
}

unsigned char ring_str(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull){
	unsigned char n = 0;
    
	while (ring_read(Ring, pStr)){
		if (bHaltNull && *pStr == 0){
			return n;
		}
   		n++;
		pStr++;
		if (n >= nMaxLen){
			*pStr = 0;
			return n;
		}
		
	}
    *pStr = 0;
	return n;
}

unsigned char ring_strcat(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxSize, unsigned char bHaltNull){
	unsigned char n = 0;
	
	while (*pStr){
		pStr++;
		nMaxSize--;
		if (!nMaxSize){
			return 0;
		}
	}

	while (ring_read(Ring, pStr)){
		n++;
		pStr++;
		if (bHaltNull && *pStr == 0){
			return n;
		}
		if (n >= nMaxSize){
			*pStr = 0;
			return n;
		}
		
	}
	return n;
}

unsigned char ring_strtok(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, const unsigned char *pDelimiters){
    unsigned char i;
    unsigned char lenDelimiters = 0;
	unsigned char n = 0;

	if (!nMaxLen){
		return 0;
	}
    
	while (pDelimiters[lenDelimiters]) lenDelimiters++;
    
	while (ring_read(Ring, pStr)){
		if (*pStr == 0){
			return n;
		}
		i = lenDelimiters;
		while (i){
			i--;
			if (*pStr == pDelimiters[i]){
				*pStr = 0;
				return n;
			}
		}
		n++;
		pStr++;
		if (n >= nMaxLen){
			*pStr = 0;
			return n;
		}
		
	}
	return n;
}

unsigned char ring_append(Ring_t *Ring, const unsigned char *pStr){
    unsigned char n = 0;
    while(*pStr){
        if (!ring_write(Ring, *pStr)){
            return n;
        };
        n++;
        pStr++;
    }
    return n;
}


unsigned char ring_appendEscaped(Ring_t *Ring, const unsigned char *pStr){
    unsigned char n     = 0;
    unsigned char cChr  = 0;
    unsigned char nEsc  = 0;
    unsigned char nHigh = 0;
    unsigned char cEsc  = 0;
    
    while(cChr = *pStr){
        pStr++;
        if (nEsc == 1){
            if      (cChr == '/' || cChr == '\\'){ nEsc = 0;}
            else if (cChr == 'r' || cChr == 'R') { cChr = '\r'; nEsc = 0;}
            else if (cChr == 'n' || cChr == 'N') { cChr = '\n'; nEsc = 0;}
            else if (cChr == 'a' || cChr == 'A') { cChr = '\a'; nEsc = 0;}
            else if (cChr == 'x' || cChr == 'X') { nEsc = 2; }
            else if (cChr == 'h' || cChr == 'H') { nEsc = 4; }
            else if (cChr == '0') { cChr = '\0'; nEsc = 0;}
            else {
                // Wasn't an escape sequence it was \ followed by something else
                // We add the pending \ and then set esc 0 to add the current chr
                if (!ring_write(Ring, cEsc)){
                    return n;
                }
                n++;
                nEsc = 0;
            }
        }
        else if (nEsc == 2){
            // Single HEX Escaped first nibble
            nHigh = cChr;
            nEsc++;
        }
        else if (nEsc == 3){
            // Single HEX escaped second nibble
            cChr = (hex2byte(nHigh) << 4) | hex2byte(cChr);
            nEsc = 0;
        }
        else if (nEsc == 4){
            // HEX Stream Escaped first nibble
            // This expects a continuous stream of hex characters until period
            if (cChr == '.'){
                nEsc = 0;
                // We avoid adding the current chr
                continue;
            }
            // HEX Stream Escaped first nibble
            nHigh = cChr;
            nEsc = 5;
        }
        else if (nEsc == 5){
            // HEX Stream Escaped second nibble
            cChr = (hex2byte(nHigh) << 4) | hex2byte(cChr);
            // This expects a continuous stream of hex characters until period
            nEsc = 4;
            if (!ring_write(Ring, cChr)){
                return n;
            }
            n++;
        }
        else if (cChr == '\\' || cChr == '/'){
            // AS ITA2 doesn't have \, we can also use / instead
            cEsc = cChr; 
            nEsc = 1;
        }
        
        if (!nEsc){
            if (!ring_write(Ring, cChr)){
                return n;
            }
            n++;
        };
    }
    return n;
}

unsigned char ring_findChr(Ring_t *Ring, unsigned char cSearch, unsigned char bHaltNull){
	unsigned char n = 0;
	unsigned char nChar = 0;

    unsigned char nHead = Ring->Head;
    
	while (Ring->Tail != nHead) {
        nHead++;
        if (nHead >= Ring->Size) nHead = 0;
        
        nChar = Ring->Buffer[nHead];
        
        if (nChar == cSearch){
            return n;
        }
        
		if (bHaltNull && cSearch == 0){
            // If we are indeed searching for null, it will return before this
            // We use 255 as a flag meaning no found, because we are using 0 based results
			return 255;
		}
        
		n++;
    }
	return 255;
}

void ring_dump(Ring_t *Ring, unsigned char * pStr){
    unsigned char n;
    unsigned char c;
    unsigned char sTmp[6] = "";
    
    if (!Ring){
        strcpy(pStr, "NULL Ring\r\n");
        return;
    }
    
    *pStr = 0;
    
    sprintf(pStr, "\r\nH:%u T:%u S:%u A:%u\r\n", 
        Ring->Head,
        Ring->Tail,
        Ring->Size,
        ring_available(Ring)
    );
    
    for (n = 0; n < Ring->Size; n++){
        c = Ring->Buffer[n];
        sprintf(sTmp, " %02x %c", c, (c > 32 ? c : '_'));
        strcat(pStr, sTmp);
    }
    
    strcat(pStr, txtCrLf);
}