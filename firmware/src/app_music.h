/* 
 * File:   app_music.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:04 PM
 */

typedef struct {
        unsigned char note:8;
        unsigned char time:8;
} note_t;

// Midi based table
extern const note_t MasterToneMusic[];
extern const unsigned char MasterToneMusicType;
extern const unsigned char MasterToneMusicLen;



