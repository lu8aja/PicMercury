/* 
 * File:   heap.h
 * Author: Javier
 *
 * Created on March 6, 2017, 2:44 AM
 */

#define LIB_HEAP

#ifndef HEAP_SIZE
    #define HEAP_SIZE 64
#endif


extern unsigned char Heap[HEAP_SIZE];

unsigned char *heap_alloc(unsigned char len);

void heap_free(unsigned char *ptr);
