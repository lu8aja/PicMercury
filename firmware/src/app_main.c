

/** INCLUDES *******************************************************/
#include <xc.h>
#include "system.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "ring.h"

#include "usb.h"
#include "usb_device_cdc.h"
#include "system_config.h"

#include "flags.h"

#include "data_eeprom.h"

#include "app_globals.h"
#include "app_io.h"
#include "app_helpers.h"
#include "app_cmd.h"

#include "service_i2c.h"       // I2C library

// Optional libraries with associated commands

#if defined(DEVICE_PUNCHER)
    #include "service_puncher.h"    // Puncher library with associated PUNCH cmd
    #include "service_softserial.h" // TTY 5N1
#endif

#if defined(DEVICE_CONSOLE)
    #include "service_leds.h"      // Led matrix
    #include "service_keys.h"      // Keyboard diode matrix
    #include "service_music.h"     // Tone generator
    // #include "service_uart.h"      // UART
    // #include "service_monitor.h"   // Pin monitor
    #include "service_program.h"   // Program service
#endif


/** PUBLIC PROTOTYPES ***************************************/

void         APP_init(void);
void         APP_main(void);
void         APP_executeCommand(unsigned char *pLine);
void         APP_USB_output(void);

/** PRIVATE PROTOTYPES ***************************************/
// USB
void         APP_USB_configured(void);
void         APP_USB_input(void);

/** FUNCTIONS *******************************************************/
void APP_init(void){
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
        Puncher_init(1, 1);
    #endif

    #ifdef LIB_PROGRAM
        Program_init();
    #endif

    #ifdef LIB_UART
        UART_init();
    #endif

        
    #ifdef LIB_SOFTSERIAL
        SoftSerial_init(&SoftSerial,
            SOFTSERIAL_TX_Port,
            SOFTSERIAL_TX_Pin,
            SOFTSERIAL_TX_Invert,
            SOFTSERIAL_RX_Port,
            SOFTSERIAL_RX_Pin,
            SOFTSERIAL_RX_Invert
        );
        
        SoftSerial_enable(&SoftSerial,
            0,
            SOFTSERIAL_RX_DataBits,
            SOFTSERIAL_RX_StopBits,
            SOFTSERIAL_RX_Period
        );
    #endif
    
    #if defined(LIB_I2C)
        #if defined(DEVICE_CONSOLE)
            I2C_Master_init();
        #else
            #if defined(DEVICE_PUNCHER)
                I2C.Address = I2C_ADDRESS_PUNCHER;
            #elif defined(DEVICE_READER)
                I2C.Address = I2C_ADDRESS_READER;
            #elif defined(DEVICE_CRTS)
                I2C.Address = I2C_ADDRESS_CRTS;
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
	if (INTCONbits.TMR0IF){
        
        // MUSIC
        #if defined(LIB_MUSIC)
        Music_tick();
        #endif
        
        // MASTER CLOCK
        if (MasterClockTick){
            MasterClockTick--;
        }
        else {
            MasterClockTick = MasterClockTickCount;
            // All counters that need ms updates
            
            // MASTER CLOCK MS
            MasterClockMS++;
            
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

            // MASTER NOTIFY
            if (MasterNotifyCounter){
                MasterNotifyCounter--;
            }
            else if (MasterNotify){
                MasterNotifyCounter = MasterNotify;
                if (   posCommand == 0 
                    && posOutput  == 0 
                ){
                    unsigned char tick = MasterClockTick;
                    MasterNotifyNow = MasterClockMS;
                    if (tick != MasterClockTick){
                        // This is to avoid interim changes as operations with "long" are not atomic
                        MasterNotifyNow = MasterClockMS;
                    }
                }
            }
        }
        
		INTCONbits.TMR0IF = 0;
	}
    
    #if defined(LIB_I2C)
    if (I2C_InterruptFlag){
        #if defined(DEVICE_CONSOLE)
            I2C_Master_interrupt();
        #else
            I2C_Slave_interrupt();
        #endif
        I2C_InterruptFlag = 0; // Clear interrupt
    }
    #endif
    
    // USB
    if (USB_InterruptFlag){
        USBDeviceTasks();
    }
}

void APP_main(){
    
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

    if (MasterNotifyNow){
        clock2str(sStr1, MasterNotifyNow);
        printReply(3, "UPTIME", sStr1);
        MasterNotifyNow = 0;
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
        #ifdef DEVICE_CONSOLE
            //I2C_Master_service();
        #else
            I2C_Slave_service();
        #endif
    #endif
    
    // USB I/O handling
    APP_USB_input();
    
    APP_USB_output();
}

void APP_USB_input(void){
    uint8_t i;
    uint8_t n;
    uint8_t nBytes;

    if(!USBUSARTIsTxTrfReady()){
        return;
    }
    
    do {
        nBytes = getsUSBUSART(bufChunk, 1);
        if (nBytes > 0) {
            /* For every byte that was read... */
            if (bufChunk[0] == 0x0D || bufChunk[0] == 0x0A){
                if (posCommand){
                    bufCommand[posCommand] = 0x00;
                    APP_executeCommand(bufCommand);
                    posCommand = 0;
                    bufCommand[0] = 0x00;
                    break;
                }
            }
            else{
                bufCommand[posCommand] = bufChunk[0];
                posCommand++;
                bufCommand[posCommand] = 0x00;
            }
        }
    }
    while(nBytes);
}

void APP_USB_output(void){
    unsigned char len;
    if(posOutput > 0 && USBUSARTIsTxTrfReady()){
        len = posOutput >= sizeOutUsb ? sizeOutUsb : posOutput;
        strncpy(bufTmp, bufOutput, len);
        putUSBUSART(bufTmp, len);
        strcpy(bufOutput, &bufOutput[len]);
        posOutput = posOutput - len;
        bufOutput[posOutput] = 0x00;
    }

    CDCTxService();
}



void APP_executeCommand(unsigned char *pLine){
    unsigned bOK = true;
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
        printReply(0, txtErrorMissingCommand, ptrCommand);
    }
    // PING
    else if (strequal(ptrCommand, "ping")){
        APP_CMD_ping(ptrArgs);
    }
    // UPTIME
	else if (strequal(ptrCommand, "uptime")){
        APP_CMD_uptime(ptrArgs);
	}
    // DEBUG
    /*
    else if (strequal(ptrCommand, "debug")){
       //APP_CMD_debug(ptrArgs);
    }
    // READ
    else if (strequal(ptrCommand, "read") || strequal(ptrCommand, "r")){
        //APP_CMD_read(ptrArgs);
    }
    // WRITE
    else if (strequal(ptrCommand, "write") || strequal(ptrCommand, "w")){
        //APP_CMD_write(ptrArgs);
    }
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
        SoftSerial_cmd(ptrArgs);
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
        Program_cmd(ptrArgs);
    }
    #endif
    // I2C
    #ifdef LIB_I2C
    else if (strequal(ptrCommand, "i2c")){
        I2C_cmd(ptrArgs);
    }
    #endif
    // MONITOR
    #ifdef LIB_MONITOR
    else if (strequal(ptrCommand, "monitor")){
        Monitor_cmd(ptrArgs);
    }
    #endif
    // LED
    #ifdef LIB_LEDS
    else if (strequal(ptrCommand, "led") || strequal(ptrCommand, "l")){
        Leds_cmd(ptrArgs);
    }
    #endif
    // KEYS
    #ifdef LIB_KEYS
    else if (strequal(ptrCommand, "keys")){
        Keys_cmd(ptrArgs);
    }
    #endif
    // MUSIC
    #ifdef LIB_MUSIC
    else if (strequal(ptrCommand, "music") || strequal(ptrCommand, "tone") || strequal(ptrCommand, "t")){
        Music_cmd(ptrArgs);
    }
    #endif
    // PUNCH
    #ifdef LIB_PUNCHER
    else if (strequal(ptrCommand, "punch")){
        Puncher_cmd(ptrArgs);
    }
    #endif
    // VERSION
    else if (strequal(ptrCommand, "version")){
        printReply(1, "VERSION", txtVersion);
    }
    else {
        printReply(0, txtErrorUnknownCommand, 0);
    }
}

/* USB stuff within APP */

void APP_USB_configured(void){
    CDCInitEP();
    line_coding.bCharFormat = 0;
    line_coding.bDataBits   = 8;
    line_coding.bParityType = 0;
    line_coding.dwDTERate   = 9600;
    
    printReply(3, "VERSION", txtVersion);
}

