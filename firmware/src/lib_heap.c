#include <xc.h>
#include <ctype.h>

#include "app_globals.h"

#define LIB_HEAP

#ifndef HEAP_Size
    #define HEAP_Size 64
#endif

unsigned char Heap[HEAP_Size];
unsigned char *pHeapNext;

unsigned char *Heap_alloc(unsigned char len){
    unsigned char *ptr;
    if (!pHeapNext){
        pHeapNext = &Heap;    
    }
    ptr = pHeapNext;
    pHeapNext += len;
    if (pHeapNext >= &Heap + HEAP_Size){
        // Forget it! We don't have enough space
        pHeapNext = ptr;
        return 0;
    }
    return ptr;    
}

void Heap_free(unsigned char *ptr){
    if (ptr && ptr < pHeapNext){
        *pHeapNext = (unsigned char *) ptr;
    }
}