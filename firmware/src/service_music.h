/* 
 * File:   app_music.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:04 PM
 */

#define LIB_MUSIC

typedef struct {
    unsigned char address;
    unsigned char length;
    unsigned char first_note;
} midi_t;

typedef struct {
    unsigned char enabled;      // 0 = Off / 1 = On
    unsigned char start;        // 0 = Off / 1 = Start
    unsigned char mode;         // 0 = Single tone / 1 = Music
    unsigned char restart;      // 0 = Disable when Time is reached / 1 = Restart either music from step 0 or steady tone
    unsigned char notify;       // Notify events
    unsigned char debug;        // Debug
    unsigned char song;         // Song number
    unsigned int  tempo;        // For music: Time per beat in ms
    signed   char pitch;        // For music: Number of notes to shift the MIDI note
    unsigned char address;      // Song address in eeprom
    unsigned char length;       // Song length in eeprom, number of notes
    unsigned char step;         // Current note/step
    unsigned char note;         // Current note
    unsigned char period;       // Current tone semi-period
    unsigned char beats;        // Current note duration beats
    unsigned int  time;         // Current note duration in ms
    // Runtime counters
    unsigned char tick;         // Current tick counter (between wave inversions) 0 .. Period
    unsigned int  counter;      // Current note time counter 0 .. Time
} music_t;


extern music_t MasterMusic;
extern midi_t  MasterMidi;


void          Music_init(void);
inline void   Music_tick(void);
inline void   Music_beat(void);
void          Music_service(void);
unsigned char Music_getMidiPeriod(unsigned char nNote);
void          Music_setSingleTone(const unsigned char nPeriod, const unsigned char nTime);
void          Music_getStatus(unsigned char *pStatus);
inline unsigned char Music_checkCmd(Ring_t * pBuffer, unsigned char *pCommand, unsigned char *pArgs);
void          Music_cmd(Ring_t * pBuffer, unsigned char *pArgs);