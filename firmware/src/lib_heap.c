#include <xc.h>
#include <ctype.h>

#include "app_globals.h"

#define LIB_HEAP

#ifndef HEAP_Size
    #define HEAP_Size 64
#endif

unsigned char Heap[HEAP_Size];
unsigned char *Heap_Next = 0;

unsigned char *Heap_alloc(unsigned char len){
    unsigned char *ptr;
    if (!Heap_Next){
        Heap_Next = &Heap;    
    }
    ptr = Heap_Next;
    Heap_Next += len;
    if (Heap_Next >= &Heap + HEAP_Size){
        // Forget it! We don't have enough space
        Heap_Next = ptr;
        System.Error.Heap = 1;
        return 0;
    }
    return ptr;    
}

void Heap_free(unsigned char *ptr){
    if (ptr && ptr < Heap_Next){
        Heap_Next = ptr;
    }
}