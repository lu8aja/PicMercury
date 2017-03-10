#include <xc.h>

#include <heap.h>

#define LIB_RING


typedef struct {
	unsigned char Head;   
	unsigned char Tail;   
	unsigned char Size;   
	unsigned char *Buffer;
} ring_t;

ring_t * ring_new(const unsigned char nSize);

inline void ring_clear(ring_t *Ring);

inline unsigned char ring_available(ring_t *Ring);

unsigned char ring_strlen(ring_t *Ring);

unsigned char ring_write(ring_t *Ring, unsigned char data);

unsigned char ring_read(ring_t *Ring, unsigned char *Data);

unsigned char ring_assert(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull);

unsigned char ring_str(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull);

unsigned char ring_strcat(ring_t *Ring, unsigned char *pStr, unsigned char nMaxSize, unsigned char bHaltNull);

unsigned char ring_strtok(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, const unsigned char *pDelimiters);

unsigned char ring_append(ring_t *Ring, unsigned char *pStr);

ring_t * ring_new(const unsigned char nSize){
    //unsigned char buffer[nSize];
    unsigned char n = 0;
    
    //ring_t Ring = {0,0,0,0};
    ring_t *pRing = heap_alloc(sizeof(ring_t));
    // For security to avoid buffer overruns a 0 is at the end of the buffer
    pRing->Size   = nSize - 1; 
    pRing->Head   = pRing->Size - 1;
    pRing->Tail   = pRing->Head;
    pRing->Buffer = heap_alloc(nSize);
    if (!pRing->Buffer){
        pRing->Size = 0;
    }
    else{
        n = nSize;
        while (n){
            pRing->Buffer[n] = 0;
            n--;
        }
        pRing->Buffer[n] = 0;
    }
    return pRing;
}


inline void ring_clear(ring_t *Ring){
    Ring->Head = Ring->Size - 1;
    Ring->Tail = Ring->Head;
}

inline unsigned char ring_available(ring_t *Ring){
	if (Ring->Tail >= Ring->Head){
        Ring->Size - Ring->Tail - Ring->Head;
    }
    else{
        Ring->Head - Ring->Tail;
    }
}

unsigned char ring_strlen(ring_t *Ring){
	return Ring->Tail >= Ring->Head ? Ring->Tail - Ring->Head : Ring->Size - Ring->Head + Ring->Tail;
}

unsigned char ring_write(ring_t *Ring, unsigned char data){
    Ring->Tail++;
    if (Ring->Tail >= Ring->Size){ Ring->Tail = 0;}
    if (Ring->Tail == Ring->Head){
        // After incrementing we stepped into the head, so undo it
    	if (Ring->Tail){
    	    Ring->Tail--;
    	}
    	else{
    	    Ring->Tail = Ring->Size - 1;
    	}
    	return 0;
    }
    Ring->Buffer[Ring->Tail] = data;
    return 1;
}

unsigned char ring_read(ring_t *Ring, unsigned char *Data){
	if (Ring->Tail == Ring->Head){
	    // Empty
	    *Data = 0;
		return 0;
	}
	Ring->Head++;
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

unsigned char ring_assert(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull){
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

unsigned char ring_str(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull){
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

unsigned char ring_strcat(ring_t *Ring, unsigned char *pStr, unsigned char nMaxSize, unsigned char bHaltNull){
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

unsigned char ring_strtok(ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, const unsigned char *pDelimiters){
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

unsigned char ring_append(ring_t *Ring, unsigned char *pStr){
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