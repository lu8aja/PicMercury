/* 
 * File:   ring.h
 * Author: Javier
 *
 * Created on March 6, 2017, 2:10 AM
 */

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