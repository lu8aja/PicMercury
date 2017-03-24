

/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>


#include "app_globals.h"

#include "data_eeprom.h"

#include "service_usb.h"       // USB library
#include "service_i2c.h"       // I2C library

#include "app_io.h"
#include "app_cmd.h"

// Optional libraries with associated boards and commands

#if defined(DEVICE_PUNCHER)
    #include "service_puncher.h"    // Puncher library with associated PUNCH cmd
    #include "service_softserial.h" // TTY 5N1
#endif

#if defined(DEVICE_CONSOLE)
    #include "service_leds.h"      // Led matrix
    #include "service_keys.h"      // Keyboard diode matrix
    #include "service_music.h"     // Tone generator
    // #include "service_uart.h"      // UART
    #include "service_monitor.h"   // Pin monitor
    #include "service_program.h"   // Program service
#endif


/** PUBLIC PROTOTYPES ***************************************/

inline void   APP_init(void);
inline void   APP_main(void);
void          APP_executeCommand(Ring_t *pBuffer, unsigned char *pLine);


/** FUNCTIONS *******************************************************/
inline void APP_init(void){
    ADCON1 = 0b00001111;  // We are not using any analog input
                          // This is specially important for I2C

    #ifdef LIB_KEYS
        Keys_init();
    #endif
    
    #ifdef LIB_LEDS
        Leds_init();
    #endif

    #ifdef LIB_MUSIC
        Music_init();
    #endif

    #ifdef LIB_PUNCHER
        Puncher_init(1, 3);
    #endif

    #ifdef LIB_PROGRAM
        Program_init();
    #endif

    #ifdef LIB_UART
        UART_init();
    #endif

        
    #ifdef LIB_SOFTSERIAL
        SoftSerial_init(&SoftSerial,
            CFG_SOFTSERIAL_TX_Port,
            CFG_SOFTSERIAL_TX_Pin,
            CFG_SOFTSERIAL_TX_InvertData,
            CFG_SOFTSERIAL_TX_InvertCtrl,
            CFG_SOFTSERIAL_RX_Port,
            CFG_SOFTSERIAL_RX_Pin,
            CFG_SOFTSERIAL_RX_InvertData,
            CFG_SOFTSERIAL_RX_InvertCtrl,
            CFG_SOFTSERIAL_HalfDuplex
        );
        
        SoftSerial_config(&SoftSerial,
            0,
            CFG_SOFTSERIAL_RX_DataBits,
            CFG_SOFTSERIAL_RX_StopBits,
            CFG_SOFTSERIAL_RX_Period,
            CFG_SOFTSERIAL_Transcode
        );
    #endif
    
    #if defined(LIB_I2C)
        #if defined(DEVICE_I2C_MASTER)
            I2C_Master_init();
        #else
            #if defined(DEVICE_CONSOLE)
                I2C.Address = CFG_I2C_ADDRESS_CONSOLE;
            #elif defined(DEVICE_PUNCHER)
                I2C.Address = CFG_I2C_ADDRESS_PUNCHER;
            #elif defined(DEVICE_READER)
                I2C.Address = CFG_I2C_ADDRESS_READER;
            #elif defined(DEVICE_CRTS)
                I2C.Address = CFG_I2C_ADDRESS_CRTS;
            #else // Some other unknown device!?
                I2C.Address = 0b01111111;
            #endif
            I2C_Slave_init();
        #endif
    #endif

	// Timer0 setup (check globals)
	T0CON               = MasterClockTimer;
	INTCONbits.TMR0IE   = 1;
	T0CONbits.TMR0ON    = 1;          // Enable
    
    INTCONbits.GIE      = 1;          // Enable global interrupts
    INTCONbits.PEIE     = 1;          // Enable peripheral interrupts
} // End APP_init

void interrupt APP_interrupt_high(void){             // High priority interrupt
	// CLOCK: Timer0 overflow int
    
    #if defined(LIB_I2C)
    if (I2C_InterruptFlag){
        #if defined(DEVICE_I2C_MASTER)
            I2C_Master_interrupt();
        #else
            I2C_Slave_interrupt();
        #endif
    }
    #endif
    
	if (INTCONbits.TMR0IF){
        // MUSIC
        #ifdef LIB_MUSIC
            Music_tick();
        #endif
        
        // MASTER CLOCK
        if (MasterClock.Tick){
            MasterClock.Tick--;
        }
        else {
            MasterClock.Tick = MasterClockTickCount;
            // All counters that need ms updates
            
            // MASTER CLOCK MS
            MasterClock.MS++;
            
            // SOFT SERIAL ms tick
            #ifdef LIB_SOFTSERIAL
                SoftSerial_tick(&SoftSerial);
            #endif

            // MUSIC ms beat
            #ifdef LIB_MUSIC
                Music_beat();
            #endif

            // KEYS ms tick
            #ifdef LIB_KEYS
                Keys_tick();
            #endif
            
            // LEDS ms tick
            #ifdef LIB_LEDS
                Leds_tick();
            #endif
            
            // PUNCHER ms tick
            #ifdef LIB_PUNCHER
                Puncher_tick();
            #endif
                            
            // PROGRAM ms tick
            #ifdef LIB_PROGRAM
                Program_tick();
            #endif

            #if defined(LIB_I2C)
                I2C_tick();
            #endif

            // MASTER NOTIFY
            if (MasterClock.NotifyCounter){
                MasterClock.NotifyCounter--;
            }
            else if (MasterClock.NotifyTime){
                MasterClock.NotifyCounter = MasterClock.NotifyTime;
                if (   posCommand == 0 
                    && posOutput  == 0 
                ){
                    unsigned char tick = MasterClock.Tick;
                    MasterClock.NotifyNow = Clock_getTime();
                }
            }
        }
        
            
		INTCONbits.TMR0IF = 0;
	}
    
    // USB
    if (USB_InterruptFlag){
        USB_serviceDeviceTasks();
    }
}

inline void APP_main(){

    #ifdef LIB_SOFTSERIAL
        SoftSerial_service(&SoftSerial);
    #endif

    
    #ifdef LIB_MUSIC
    if (MasterMusic.enabled){
        Music_service();
    }
    #endif
    
    #ifdef LIB_PUNCHER
    if (MasterPuncher.Enabled && !MasterPuncher.Tick){
        Puncher_service();
    }
    #endif

    
    #ifdef LIB_KEYS
    if (MasterKeys.Enabled){
        Keys_service();
    }
    #endif    
    
    #ifdef LIB_LEDS
        Leds_service();
    #endif

    if (MasterClock.NotifyNow){
        Clock_getStr(sStr1, MasterClock.NotifyNow);
        printReply(0, 3, "UPTIME", sStr1);
        MasterClock.NotifyNow = 0;
    }
    
    // Program
    #ifdef LIB_PROGRAM
        Program_service();
    #endif
                
    // Monitor
    #ifdef LIB_MONITOR
        Monitor_service();
    #endif
    
    // UART
    #ifdef LIB_UART
        UART_service();
    #endif

    // I2C
    #ifdef LIB_I2C
        #ifdef DEVICE_I2C_MASTER
            I2C_Master_service();
        #else
            I2C_Slave_service();
        #endif
    #endif
    
    // USB I/O handling
    if( USB_getDeviceState() >= CONFIGURED_STATE && !USB_isDeviceSuspended()){
        USB_service();
    }
}


void APP_executeCommand(Ring_t *pBuffer, unsigned char *pLine){
    unsigned char *ptrCommand;
    unsigned char *ptrArgs;
    unsigned char n;
    unsigned char val;

    if (pLine[0] == 0x00){
        return;
    }
   
    pLine[strlen(pLine) + 1] = 0; // Workaround for args parsing
    
    // Get the command
    ptrCommand = strtok(pLine, txtWhitespace);
    ptrArgs = NULL;
    // Get the arguments
    if (ptrCommand != NULL){
        ptrArgs = &pLine[strlen(ptrCommand) + 1];
    }

    print(txtCrLf);    
    
    str2lower(ptrCommand);    

    if (ptrCommand == NULL){    
        printReply(pBuffer, 0, txtErrorMissingCommand, ptrCommand);
    }
    // PING
    else if (strequal(ptrCommand, "ping")){
        APP_CMD_ping(pBuffer, ptrArgs);
    }
    // UPTIME
	else if (strequal(ptrCommand, "uptime")){
        APP_CMD_uptime(pBuffer, ptrArgs);
	}
    // DEBUG
    else if (strequal(ptrCommand, "debug")){
        APP_CMD_debug(pBuffer, ptrArgs);
    }
    // READ
    else if (strequal(ptrCommand, "read") || strequal(ptrCommand, "r")){
        APP_CMD_read(pBuffer, ptrArgs);
    }
    // WRITE
    else if (strequal(ptrCommand, "write") || strequal(ptrCommand, "w")){
        APP_CMD_write(pBuffer, ptrArgs);
    }
/*
    // VAR
    else if (strequal(ptrCommand, "var")){
        //APP_CMD_var(ptrArgs);
    }
     * */
    // MEM
    /*
    else if (strequal(ptrCommand, "mem")){
        APP_CMD_mem(ptrArgs);
    }
     */
    // DUMP
    /*
    else if (strequal(ptrCommand, "dump")){
        APP_CMD_dump(ptrArgs);
    }
     * */

    #ifdef LIB_SOFTSERIAL
    else if (strequal(ptrCommand, "serial")){
        SoftSerial_cmd(pBuffer, ptrArgs);
    }
    #endif

    // UART
    else if (strequal(ptrCommand, "uart")){
//        MasterSerialOut = (unsigned char) atoi(ptrArgs);
    }
    // PROGRAM
    #if defined(LIB_PROGRAM)
    // DELAY
    else if (strequal(ptrCommand, "delay") || strequal(ptrCommand, "d")){
        MasterProgram.Tick = atol(ptrArgs);
    }
    // RUN
    else if (strequal(ptrCommand, "run")){
        Program_cmd(pBuffer, ptrArgs);
    }
    #endif
    // I2C
    #ifdef LIB_I2C
    else if (strequal(ptrCommand, "i2c")){
        I2C_cmd(pBuffer, ptrArgs);
    }
    #endif
    // MONITOR
    #ifdef LIB_MONITOR
    else if (strequal(ptrCommand, "monitor")){
        Monitor_cmd(pBuffer, ptrArgs);
    }
    #endif
    // LED
    #ifdef LIB_LEDS
    else if (strequal(ptrCommand, "led") || strequal(ptrCommand, "l")){
        Leds_cmd(pBuffer, ptrArgs);
    }
    #endif
    // KEYS
    #ifdef LIB_KEYS
    else if (strequal(ptrCommand, "keys")){
        Keys_cmd(pBuffer, ptrArgs);
    }
    #endif
    // MUSIC
    #ifdef LIB_MUSIC
    else if (strequal(ptrCommand, "music") || strequal(ptrCommand, "tone") || strequal(ptrCommand, "t")){
        Music_cmd(pBuffer, ptrArgs);
    }
    #endif
    // PUNCH
    #ifdef LIB_PUNCHER
    else if (strequal(ptrCommand, "punch")){
        Puncher_cmd(pBuffer, ptrArgs);
    }
    #endif
    else if (strequal(ptrCommand, "heap")){
        sprintf(sReply, "%u of %u", Heap_Next - Heap, sizeof(Heap));
        printReply(pBuffer, 1, "HEAP", sReply);
    }
    // VERSION
    else if (strequal(ptrCommand, "version")){
        printReply(pBuffer, 1, "VERSION", txtVersion);
    }
    else {
        printReply(pBuffer, 0, txtErrorUnknownCommand, 0);
    }
}



