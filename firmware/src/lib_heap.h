/* 
 * File:   heap.h
 * Author: Javier
 *
 * Created on March 6, 2017, 2:44 AM
 */

#define LIB_HEAP


#include "app_globals.h"

#ifndef HEAP_Size
    #define HEAP_Size 64
#endif


extern unsigned char Heap[HEAP_Size];

unsigned char *Heap_alloc(unsigned char len);

void Heap_free(unsigned char *ptr);
