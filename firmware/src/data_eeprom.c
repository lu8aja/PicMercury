
#include <xc.h>

#define EEDATA_MIDI_ADDRESS    0
#define EEDATA_MIDI_LENGTH     1
#define EEDATA_MIDI_FIRST_NOTE 2

#define EEDATA_MUSIC_COUNT     3
#define EEDATA_MUSIC_0_ADDRESS 4
#define EEDATA_MUSIC_0_LENGTH  5
#define EEDATA_MUSIC_1_ADDRESS 6
#define EEDATA_MUSIC_1_LENGTH  7

#define EEDATA_PROGRAMS        8


/*** EEPROM ***/
__EEPROM_DATA(
	0x10,	// MIDI Table Address
	0x40,	// MIDI Table length
      30,   // MIDI TABLE First note (30)
       2,   // Music Count
	0x50,	// Music 0: Clementine Address
	0x40,	// Music 0: Clementine Length
	0,	    // Music 1: ?? Address
	0,	    // Music 1: ?? Address
);

__EEPROM_DATA(
	0xa0,	// PRG 0 Address
    0xbb,   // PRG 1 Address
	0xde,	// PRG 2 Address
	   0,	// PRG 3 Address
	   0,	//
	   0,	//
	   0,	//
	   0,	//
);

/*** MIDI Notes Table ***/
// freq = (440Hz / 32) * (2 ^ ((note - 9) / 12));
// semiperiod = (ticks * 1000 / 2) / freq

/* 10 */	__EEPROM_DATA(
	249	, /* 30	Gb    46.2 */	235	, /* 31	G    48.9 */	222	, /* 32	Ab    51.9 */	209	, /* 33	A    55.0 */
	197	, /* 34	Bb    58.2 */	186	, /* 35	B    61.7 */	176	, /* 36	C     65.4 */	166	, /* 37	Db   69.2 */
);
/* 18 */	__EEPROM_DATA(
	157	, /* 38	D    73.4 */	148	, /* 39	Eb   77.7 */	140	, /* 40	E    82.4 */	132	, /* 41	F    87.3 */
	124	, /* 42	Gb   92.4 */	117	, /* 43	G    97.9 */	111	, /* 44	Ab  103.8*/		105	, /* 45	A   110.0*/
);
/* 20 */	__EEPROM_DATA(
	99	, /* 46	Bb  116.5 */ 	93	, /* 47	B   123.4 */	88	, /* 48	C   130.8 */	83	, /* 49	Db  138.5 */
	78	, /* 50	D   146.8 */	74	, /* 51	Eb  155.5 */	70	, /* 52	E   164.8 */	66	, /* 53	F   174.6 */
);
/* 28 */	__EEPROM_DATA(
	62	, /* 54	Gb  184.9 */	59	, /* 55	G   195.9 */	55	, /* 56	Ab  207.6 */	52	, /* 57	A   220.0 */
	49	, /* 58	Bb  233.0 */	47	, /* 59	B   246.9 */	44	, /* 60	C   261.6 */	41	, /* 61	Db  277.1 */
);
/* 30 */	__EEPROM_DATA(
	39	, /* 62	D   293.6 */	37	, /* 63	Eb  311.1 */	35	, /* 64	E   329.6 */	33	, /* 65	F   349.2 */
	31	, /* 66	Gb  369.9 */	29	, /* 67	G   391.9 */	28	, /* 68	Ab  415.3 */	26	, /* 69	A   440.0 */
);
/* 38 */	__EEPROM_DATA(
	25	, /* 70	Bb  466.1 */	23	, /* 71	B   493.8 */	22	, /* 72	C   523.2 */	21	, /* 73	Db  554.3 */
	20	, /* 74	D   587.3 */	18	, /* 75	Eb  622.2 */	17	, /* 76	E   659.2 */	16	, /* 77	F   698.4 */
);
/* 40 */	__EEPROM_DATA(
	16	, /* 78	Gb  739.9 */	15	, /* 79	G   783.9 */	14	, /* 80	Ab  830.6 */	13	, /* 81	A   880.0 */
	12	, /* 82	Bb  932.3 */	12	, /* 83	B   987.7 */	11	, /* 84	C   1046.5*/	10	, /* 85	Db 1108.7*/
);
/* 48 */	__EEPROM_DATA(
	10	, /* 86	D  1174.6 */	9	, /* 87	Eb 1244.5 */	9	, /* 88	E  1318.5 */	8	, /* 89	F  1396.9 */
	8	, /* 90	Gb 1479.9 */	7	, /* 91	G  1567.9 */	7	, /* 92	Ab 1661.2 */	7	, /* 93	A  1760.0 */
);

/*** MUSIC: Clementine ***/ // // https://onlinesequencer.net/
/* 50 */	__EEPROM_DATA(	53,	12, /* 0	F5 */	53,	 4, /* 2	F5*/	53,	16, /* 4	F5 */	48,	16, /* 6	C5*/ );
/* 58 */	__EEPROM_DATA(	57,	12, /* 8	A5 */	57,	 4, /* 10	A5*/	57,	16, /* 12	A5 */	53,	16, /* 14	F5*/ );
/* 60 */	__EEPROM_DATA(	53,	12, /* 16	F5 */	57,	 4, /* 18	A5*/	60,	24, /* 20	C6 */	60,	 8, /* 22	C6*/ );
/* 68 */	__EEPROM_DATA(	58,	 8, /* 24	A#5*/	57,	 8, /* 26	A5*/	55,	32, /* 28	G5 */	55,	12, /* 30	G5*/ );
/* 70 */	__EEPROM_DATA(	57,	 4, /* 32	A5 */	58,	16, /* 34	A#5*/	58,	16, /* 36	A#5*/	57,	12, /* 38	A5*/ );
/* 78 */	__EEPROM_DATA(	55,	 4, /* 40	G5 */	57,	16, /* 42	A5*/	53,	16, /* 44	F5 */	53,	12, /* 46	F5*/ );
/* 80 */	__EEPROM_DATA(	57,	 4, /* 48	A5 */	55,	24, /* 50	G5*/	48,	 8, /* 52	C5 */	52,	 8, /* 54	E5*/ );
/* 88 */	__EEPROM_DATA(	55,	 8, /* 56	G5 */	53,	16, /* 58	F5*/	0,	 0,					 0,	 0               );

/* 90 */	__EEPROM_DATA(  0,  0,  0,  0,  0,  0,  0, 0 ); //
/* 98 */	__EEPROM_DATA(  0,  0,  0,  0,  0,  0,  0, 0 ); //

/* A0 */	__EEPROM_DATA('l', ' ', 's', 't', 'e', 'p', 's', ' '); //0= led steps on
/* A8 */	__EEPROM_DATA('1',  0 , 'd', ' ', '3', '8', '0', '0'); //   delay 38secs
/* B0 */	__EEPROM_DATA('0',  0 , 't', ' ', '4', '4', '0', ' '); //   tone 440 2
/* B8 */	__EEPROM_DATA('2',  0 ,  0 , 't', ' ', 'm', 'o', 'd'); //1= tone mode 1 (music))
/* C0 */	__EEPROM_DATA('e', ' ', '1',  0 , 't', ' ', 'r', 'e'); //   tone restart 0
/* C8 */	__EEPROM_DATA('s', 't', 'a', 'r', 't', ' ', '0',  0 ); //
/* D0 */	__EEPROM_DATA('t', ' ', '1',  0 , 'd', ' ', '3', '0'); //   tone on
/* D8 */	__EEPROM_DATA('0', '0', '0',  0 ,  0 ,  0 ,  0 ,  0 ); //   delay 30secs

/* E0 */	__EEPROM_DATA(' ', ' ', ' ', ' ' , ' ', ' ', ' ', 0 ); //
/* E8 */	__EEPROM_DATA(' ', ' ', ' ', ' ' , ' ', ' ' , 0 , 0 ); //
/* F0 */	__EEPROM_DATA(' ', ' ', ' ', ' ' , ' ', ' ', ' ', 0 ); //
/* F8 */	__EEPROM_DATA(' ', ' ', ' ', ' ' , ' ', ' ' , 0 , 0 ); //
