#include <xc.h>
#include <ctype.h>

#define LIB_HEAP

#ifndef HEAP_SIZE
    #define HEAP_SIZE 64
#endif

unsigned char Heap[HEAP_SIZE];
unsigned char *pHeapNext;

unsigned char *heap_alloc(unsigned char len){
    unsigned char *ptr;
    if (!pHeapNext){
        pHeapNext = &Heap;    
    }
    ptr = pHeapNext;
    pHeapNext += len;
    if (pHeapNext >= &Heap + HEAP_SIZE){
        // Forget it! We don't have enough space
        pHeapNext = ptr;
        return 0;
    }
    return ptr;    
}

void heap_free(unsigned char *ptr){
    if (ptr && ptr < pHeapNext){
        *pHeapNext = (unsigned char *) ptr;
    }
}