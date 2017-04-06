# PicMercury
This project consists of 4 different boards with PIC18F4550 USB capable microcontrollers
performing several basic I/O actions for the Mercury/Clementina project. The project aims to create
a real size mockup of a Ferranti Mercury valve based computer from the late 1950s.

# Boards
There are 4 boards distributed through the mockup, all communicating through I2C as well as having USB
access to each one, be it for normal access or to use the pieces as standalone.

## Console
The console consists of a keyboard (a 5 x 8 diode matrix with switches and push buttons), an array of 15 leds
(a 4 x 4 diode matrix), a speaker and Plessey connectors to other parts.

The keyboard is logically arranged in 4 10-bits words named Function, Address, Input and Switches

The board controlling the console, acts also as master for I2C communications, allowing a USB connection to
act upon the other boards as well.

## Puncher
The puncher acts as a controller to the punching block "donated" by a Siemens T1000 teleprinter. This punching
block is easily controlled by 7 TTL signals (5 bit punchers, 1 guide puncher and 1 advance solenoid) and
requires 40VDC for all the solenoids.

Additionally, the puncher board has an embedded TTY current loop suitable to control any standard 20mA
teleprinter, like a Siemens T100. Implemented in software it is capable of any arbitrarily low baud rate up
to 110 bauds, level inversions for control and/or data, custom data/stop bit counts.

## Reader
The reader board controls the reader block "donated" by a Siemens T1000 teleprinter, with a stepper motor
and optical paper tape reading.

## Monitor
The original Ferranti Mercury featured a panel with 2 3-inches CRTs and 2 rotating switches that could work
as oscilloscope to serially read values from registers.
The monitor board has proper deflection circuits for driving DG7-32 electrostatic CRTs, with 3 8bits outputs
as R2R DACs. To economize reading the rotary switches, they feature resistors as a voltage divider and an ADC
input of the PIC reads the analog value and obtains the corresponding switch position.
When emulating the Ferranti the console master can write virtual registers and the selector chooses
which register to display.

Additionally and purely for amusement, the USB can directly drive both CRTs, be it for producing something
similar to the dotted Williams Kilburn memories as the Ferranti Mark I used, or other effects. It might be
useful for debug purposes as well. Routines for generating 7 segment numbers are provided.
