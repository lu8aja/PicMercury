/* 
 * File:   app_midi.h
 * Author: Javier
 *
 * Created on February 21, 2017, 9:07 PM
 */

// freq = (440Hz / 32) * (2 ^ ((note - 9) / 12));
// semiperiod = (ticks * 1000 / 2) / freq

const  unsigned char MasterToneMidiTable[] = {
    352	, // 	0	,	24	C	=	32.70319566
    332	, // 	1	,	25	Db	=	34.64782887
    313	, // 	2	,	26	D	=	36.70809599
    296	, // 	3	,	27	Eb	=	38.89087297
    279	, // 	4	,	28	E	=	41.20344461
    263	, // 	5	,	29	F	=	43.65352893
    249	, // 	6	,	30	Gb	=	46.24930284
    235	, // 	7	,	31	G	=	48.9994295
    222	, // 	8	,	32	Ab	=	51.9130872
    209	, // 	9	,	33	A	=	55
    197	, // 	10	,	34	Bb	=	58.27047019
    186	, // 	11	,	35	B	=	61.73541266
    176	, // 	12	,	36	C	=	65.40639133
    166	, // 	13	,	37	Db	=	69.29565774
    157	, // 	14	,	38	D	=	73.41619198
    148	, // 	15	,	39	Eb	=	77.78174593
    140	, // 	16	,	40	E	=	82.40688923
    132	, // 	17	,	41	F	=	87.30705786
    124	, // 	18	,	42	Gb	=	92.49860568
    117	, // 	19	,	43	G	=	97.998859
    111	, // 	20	,	44	Ab	=	103.8261744
    105	, // 	21	,	45	A	=	110
    99	, // 	22	,	46	Bb	=	116.5409404
    93	, // 	23	,	47	B	=	123.4708253
    88	, // 	24	,	48	C	=	130.8127827
    83	, // 	25	,	49	Db	=	138.5913155
    78	, // 	26	,	50	D	=	146.832384
    74	, // 	27	,	51	Eb	=	155.5634919
    70	, // 	28	,	52	E	=	164.8137785
    66	, // 	29	,	53	F	=	174.6141157
    62	, // 	30	,	54	Gb	=	184.9972114
    59	, // 	31	,	55	G	=	195.997718
    55	, // 	32	,	56	Ab	=	207.6523488
    52	, // 	33	,	57	A	=	220
    49	, // 	34	,	58	Bb	=	233.0818808
    47	, // 	35	,	59	B	=	246.9416506
    44	, // 	36	,	60	C	=	261.6255653
    41	, // 	37	,	61	Db	=	277.182631
    39	, // 	38	,	62	D	=	293.6647679
    37	, // 	39	,	63	Eb	=	311.1269837
    35	, // 	40	,	64	E	=	329.6275569
    33	, // 	41	,	65	F	=	349.2282314
    31	, // 	42	,	66	Gb	=	369.9944227
    29	, // 	43	,	67	G	=	391.995436
    28	, // 	44	,	68	Ab	=	415.3046976
    26	, // 	45	,	69	A	=	440
    25	, // 	46	,	70	Bb	=	466.1637615
    23	, // 	47	,	71	B	=	493.8833013
    22	, // 	48	,	72	C	=	523.2511306
    21	, // 	49	,	73	Db	=	554.365262
    20	, // 	50	,	74	D	=	587.3295358
    18	, // 	51	,	75	Eb	=	622.2539674
    17	, // 	52	,	76	E	=	659.2551138
    16	, // 	53	,	77	F	=	698.4564629
    16	, // 	54	,	78	Gb	=	739.9888454
    15	, // 	55	,	79	G	=	783.990872
    14	, // 	56	,	80	Ab	=	830.6093952
    13	, // 	57	,	81	A	=	880
    12	, // 	58	,	82	Bb	=	932.327523
    12	, // 	59	,	83	B	=	987.7666025
    11	, // 	60	,	84	C	=	1046.502261
    10	, // 	61	,	85	Db	=	1108.730524
    10	, // 	62	,	86	D	=	1174.659072
    9	, // 	63	,	87	Eb	=	1244.507935
    9	, // 	64	,	88	E	=	1318.510228
    8	, // 	65	,	89	F	=	1396.912926
    8	, // 	66	,	90	Gb	=	1479.977691
    7	, // 	67	,	91	G	=	1567.981744
    7	, // 	68	,	92	Ab	=	1661.21879
    7	, // 	69	,	93	A	=	1760
    6	, // 	70	,	94	Bb	=	1864.655046
    6	, // 	71	,	95	B	=	1975.533205
};

const unsigned char MasterToneMidiTableLen = sizeof(MasterToneMidiTable) / sizeof(MasterToneMidiTable[0]);
