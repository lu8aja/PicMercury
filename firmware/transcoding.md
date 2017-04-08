# Transcoding

Teleprinters used for Telex normally have some kind of 5 bits code, be it ITA2, US-TTY or some variation.
For convenience the buffers used by the firmware have data stored in ASCII
_(actually 8 bits ASCII-like, be it codepage 437 or whatever you wish, it will actually depend how you configure your terminal)_,
up until the last moment, when the data needs to be output into the teletype or the paper tape, only then the bytes
get transcoded into a suitable 5 bits code.

The transcoding process is controlled by the **mode** configuration, which is a bitfield of several
individual steps that can be enabled or disabled individually.

## MODE 0 - ASCII
When all the transcoding is disabled, the output is standard 8 bits just as it is in the ring buffer.
This allows for example to connect to a 110 bauds ASCII terminal, or even to a Teletype ASR33.

## MODE 1 - ITA2 w/bit6-shift to ITA2 5 bits
When this mode is enabled, the bytes at the buffer are treated as 6 bits words, where the 6th bit serves
as an indicator of the shift that must be used for the character. The transcoder keeps track of the current
line shift status, thus it can be able to decide if a FIGS or LTRS insertion is required according to the bit 6.
If this mode is enabled alone, then the buffer must be treated as a binary buffer, certainly not as an ASCII one.

## MODE 2 - ASCII to ITA2 w/bit-6-shift
When this mode is enabled the bytes at the buffer are treated as ASCII characters, and they get converted by
means of a conversion array/string of 64 characters.
The characters being the ASCII ones and the position being the ITA2 w/6 bits. As a result a simple search for
the ASCII character will give the position as the ITA2 character. As some characters are duplicated
(Space, CR, LF, LTRS, FIGS), in boths shifts, to minimize shift changes the direction of the search must be
determined according to the current shift status, that is if LTRS then begining to end, and if FIGS, then end
to begining. This ensures that for a duplicated character the first one to be found will be the one at the
current shift.

The resulting character will be 6 bits, not a real ITA2 character. While this could be output onto the serial
as 6N1 it will be totally useless for regular teleprinters, so this option is normally combined with MODE 1 into MODE 3.

It should be noted in case that you want to send LTRS of FIGS  while in MODE 2 or MODE 3, you can use the SI
and SO ASCII characters.

The transcoder library comes with the following conversions arrays available, but to save program memory they
must be compiled with one or the other enabled, thus they are not available at runtime.
* ITA2
* ITA2-ES (Spanish)
* US-TTY

## MODE 3 - ASCII to ITA2 5 bits
As mentioned it is just MODE2 conversion followed by MODE1, thus ending with a regular ITA2 5 bits with
appropriate LTRS and FIGS inserted.

## Escaping
The transcoder library itself does not provide means to receive escaped characters or other formats like HEX.
It is up to the command inserting into the buffer to execute those conversions prior to insertion, that si the
case for example for $PUNCH and $SERIAL commands which perform **\** escape decoding.
