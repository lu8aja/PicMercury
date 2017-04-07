# PicMercury Firmware

This firmware is designed for the family of boards used in the I/O of the Clementina/Mercury project.
The boards' PCBs are available at the boards folder in Eagle format.

## Commands

### version / ver
Always available, it identifies the board you are connected to. It has no arguments.

### status
Always available, it displays error flags and some basic status information for the board you are connected to. It has no arguments.

### heap
Always available, it displays the heap usage (the heap is used to source the buffers) and is useful only when debuging. It has no arguments.

### uptime
Always available, it displays the time since the board started. It has no arguments.

### ping
Always available, it echoes whatever argument you passed to it. Argument: freetext to be echoed.

### reset
Always available, it does a software reset of the device, if you do this over USB you may get in trouble as the USB port may hangup in windows if you reset while still connected to it. It has an optional argument which is the number of ms to wait before reset (max 65535), by default 5000 (5 seconds), this way it allows you to disconnect from USB and wait for the reset without causing issues.

Usage:

**reset** _waittime_

Example:

`reset` _will reset after 5 seconds_
`reset 60000` _will reset after a minute_
`reset 500` _will reset after half second_

### cfg
Always available, it is a pseudo command to access the configuration to certain other commands (punch, serial, i2c, etc)
It is the specific command the one that defines what configs are available.

Usage:

**cfg** _command_ _options_

Example:

`cfg serial on 5 1 20 3 1 3 3`

### read / r
It sets a port or pin as input, and echoes back it level. Keep in mind this might affect peripherials connected to it!
Ports are identified with their letters (Ex: `A`). Pins are identified with letter plus number (Ex: `D5`).

Usage:

**read** _pin-or-port_

Examples:

`read d6`

`read B`

### write / w
It sets a port or pin as output, and sets its level. Keep in mind this might affect peripherials connected to it!
Ports are identified with their letters (Ex: `A`). Pins are identified with letter plus number (Ex: `D5`).

Usage:

**write** _pin-or-port_ _value_

Examples:

`write d6 1`

`write B 255`

### i2c
The boards communicate with each other over the I2C bus. Thus one of them, the console is a master and the others are slaves.
Commands to be executed remotely, must start with the `$` sign.

Target can be:

	* p = Puncher
	* r = Reader
	* m = Monitor
	* c = Console (the master)

The target can have an optional buffer id, to direct the message to a speciffic buffer in the target board.

	* 0 = USB Output
	* 1 = I2C Output
	* 2 = Puncher Output
	* 3 = Serial Output

Usage:

**i2c** _target_ _message_

Examples:

`i2c r hello` _will just display hello at the USB of the reader_

`i2c r $cfg serial on` _will remotely enable the serial port_

`i2c c $reset` _will reset the master console_

`i2c p3 This is a message` _will directly output the message to the TTY if the serial is enabled_

`i2c p2 Punch this boy!` _will directly punch the message if the puncher is enabled_

Configs

`cfg i2c on`
`cfg i2c off`

### led
Available only on the console. Set or read a led status.

### key
Available only on the console. Read console keys.

### run
Available only on the console, it runs a program.

There are two types of programs currently:

	* **String programs** are simply a string, null separated, null terminated, that imply a sequence of commands. This is currently saved in ROM, but might be also saved in EEPROM. As space is a premium in this platform, these programs are likely going to be deprecated.
	* **Function programs** are C functions located at app_programs.c In this way you have the full C power available, and you are not restricted to the existing commands.

Programs can be run from command line or from console keyboard. To run from keyboard you must select the program id at FUNCTION and press PREPULSE.

Usage:

**run** _id_ _input_ _address_

Examples:

`run 128 199 768` _Will check if 199 is prime and output the result to both punch and serial through I2C_

`run 64 1000 512` _Will print all primes from 1 to 1000 and will output the result to the serial through I2C_

Programs are identified by an id.
	* **0** = Boot sequence (light show)
	* **1** = Play music
	* **32** = Music playing from the console keys, using the INPUT row as frequency
	* **64** = Calcs primes up to a number argument (INPUT if you run it from console). Optionally sends to 512=serial / 256=punch according to a second argument (ADDRESS if you run from the console)
	* **128** = Checks if a given number argument is prime (INPUT if you run it from console). Optionally sends to 512=serial / 256=punch according to a second argument (ADDRESS if you run from the console)

All these programs are just to show some activity and will likely change a lot over time.

### delay / d
Set the delay of a specific step in a program, in practice this can be used for separating the commands of a "string program".

### music / tone / t
Controls the tone generator, either to play a specific tone or to play saved music.

### serial
The Puncher board has a software based TTY current loop builtin ready to work at slows speeds (up to 110 bauds).
The message can have escaped characters (`\r`, `\n`, `\0`, `\a`, `\\`, `\xNN`)

Usage:

**serial** _message_

Examples:

`serial Hello boy!\r\r\n This is a rather long message` _will just display that message at the TTY_

Configs

**cfg serial off**

**cfg serial on** _data-bits_ _stop-bits_ _symbol-period-ms_ _transcoding_ _half-duplex_ _tx-invert_ _rx-invert_

Where:

	_data-bits_ = Data bits: 1 to 8, typically 5 for TTY

	_stop-bits_ = Stop bits: 1 to 7, typically 1 or 2 for TTY

	_symbol-period-ms_ = Number of ms for each bit: 1 to 255, typically 20ms = 50bauds / 9ms = 110 bauds

	_transcoding_ = ASCII to ITA2 transcoding bit mask: 0=ASCII, 1=Bit 6 is Shift Status, 2=ASCII to ITA2+Bit6. (Use 3 for ASCII to ITA2)

	_half-duplex_ = If set to 1 the loop is connected serially, thus in half duplex mode. When transmitting RX will be disabled, and when receiving TX will be disabled.

	_tx-invert_ = Bitmask for TX pin inversion control. 1 = Data is inverted / 2 = Control (idle, start & stop bits) is inverted

	_rx-invert_ = Bitmask for RX pin inversion control. 1 = Data is inverted / 2 = Control (idle, start & stop bits) is inverted

Example:

`serial on 5 2 20 3 1 3 3` _5N2 at 50bauds ASCII->ITA2, HalfDuplex, totally inverted TX, control inverted RX_

`serial on 8 1 9 0 1 0 0` _8N1 at 110bauds ASCII, HalfDuplex, nothing inverted_

`cfg serial off` _Will turn off the serial port_

### punch
Controls the tape puncher.
Be careful if you set a time config too small it will jam the paper and if you set the time too large it will consume a lot of current and may damage the solenoids.

Usage:

**punch** _message_

Examples:

`punch Hello boy!\r\r\n This is a rather long message` _will punch a tape with this message_

Configs

**cfg punch** _setting_ _value_

Where:

	_setting_ = The setting to be displayed or changed

	_value_ = Is optional according to the setting, if omitted it will display the current value

Settings:

	`on`  = Turns on the puncher

	`off` = Turns off the puncher

	`mode` = Transcoding mode: 0 = ASCII / 1=ITA2 6 bits -> ITA2 / 2=ASCII -> ITA2 6 bits / 3 = ASCII -> ITA2

	`time.punch` = Time in ms the punched should be punching the holes

	`time.gap1`  = Dead time in ms between punching and advancing

	`time.advance` = Time in ms the puncher should turn on the advance solenoid

	`time.gap2`  = Dead time between advance and the punch of the next character


Example:

`cfg punch time.punch 20`

`cfg punch off`
