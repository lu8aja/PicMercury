/* 
 * File:   ring.h
 * Author: Javier
 *
 * Created on March 6, 2017, 2:10 AM
 */

#define LIB_RING

#include "lib_heap.h"


#define ring_is_empty(R) (R->Tail == R->Head)


typedef struct {
	unsigned char Head;   
	unsigned char Tail;   
	unsigned char Size;   
	unsigned char *Buffer;
} Ring_t;

// Ring Factory: Allocates a buffer in the Heap and generates the associated structure
Ring_t * ring_new(const unsigned char nSize);

// Clear the buffer (makes Head & Tail = 0))
inline void ring_clear(Ring_t *Ring);

// Gets the used space
unsigned char ring_strlen(Ring_t *Ring);

// Calculates the available space
inline unsigned char ring_available(Ring_t *Ring);

// Writes a character into the buffer
unsigned char ring_write(Ring_t *Ring, unsigned char data);

// Reads a character from the buffer, freeing it
unsigned char ring_read(Ring_t *Ring, unsigned char *Data);

// Peeps a character from the buffer without changing head or tail
unsigned char ring_peep(Ring_t *Ring, unsigned char *Data);
unsigned char ring_peep_pos(Ring_t *Ring, unsigned char *Data, unsigned char nPos);
// Asserts a whole null terminated  string from the buffer without freeing it
unsigned char ring_assert(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull);

// Reads a whole null terminated string from the buffer freeing it
unsigned char ring_str(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, unsigned char bHaltNull);

// Reads a whole null terminated string from the buffer, freeing it and appending it to an existing str
unsigned char ring_strcat(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxSize, unsigned char bHaltNull);

// Read the buffer into a null terminated string up to a given delimiter
unsigned char ring_strtok(Ring_t *Ring, unsigned char *pStr, unsigned char nMaxLen, const unsigned char *pDelimiters);

// Writes a whole null terminated string into the buffer
unsigned char ring_append(Ring_t *Ring, unsigned char *pStr);


