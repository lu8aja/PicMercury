

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
    //#include "service_monitor.h"   // Pin monitor
    #include "service_program.h"   // Program service
#endif


/** PUBLIC PROTOTYPES ***************************************/

inline void   APP_init(void);
inline void   APP_main(void);
void          APP_executeCommand(unsigned char idBuffer, unsigned char *pLine);


/** FUNCTIONS *******************************************************/
inline void APP_init(void){
    ADCON1 = 0b00001111;  // We are not using any analog input
                          // This is specially important for I2C

    System.Errors = 0;
    System.Clock.NotifyTime = 60000;
    System.Buffers[0] = 0;
    System.Buffers[1] = 0;
    System.Buffers[2] = 0;
    System.Buffers[3] = 0;
    System.Buffers[4] = 0;
    System.Buffers[5] = 0;
    System.Buffers[6] = 0;
    System.Buffers[7] = 0;
    
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
        SoftSerial_init(&SoftSerial);
        SoftSerial_load(&SoftSerial);
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
                I2C.Address = CFG_I2C_ADDRESS_MONITOR;
            #else // Some other unknown device!?
                I2C.Address = 0b01111111;
            #endif
            I2C_Slave_init();
        #endif
    #endif

	// Timer0 setup (check globals)
	T0CON               = System_ClockTimer;
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
        if (System.Clock.Tick){
            System.Clock.Tick--;
        }
        else {
            System.Clock.Tick = System_ClockTickCount;
            // All counters that need ms updates
            
            // MASTER CLOCK MS
            System.Clock.MS++;
            
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
            if (System.Clock.NotifyCounter){
                System.Clock.NotifyCounter--;
            }
            else {
                if (System.Config.Reset){
                    System.Config.Reset = 0;
                    Reset();
                }
                if (System.Clock.NotifyTime){
                    System.Clock.NotifyCounter = System.Clock.NotifyTime;
                    if (posUsbCommand == 0 && posOutput  == 0){
                        System.Clock.NotifyNow = Clock_getTime();
                    }
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
    if (Puncher.Enabled && !Puncher.Tick){
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

    if (System.Clock.NotifyNow){
        Clock_getStr(sStr1, System.Clock.NotifyNow);
        printReply(0, 3, "UPTIME", sStr1);
        System.Clock.NotifyNow = 0;
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


void APP_executeCommand(unsigned char idBuffer, unsigned char *pLine){
    unsigned char *pCommand;
    unsigned char *pArgs    = NULL;
    unsigned char *pArg1    = NULL;
    unsigned char nExecuted = 0;
    unsigned char n;
    unsigned char val;

    print(txtCrLf);    

    if (pLine[0] == 0x00 || pLine == NULL){
        // Empty line
        return;
    }
    
    pLine[strlen(pLine) + 1] = 0; // Workaround for args parsing
    
    // Get the command
    pCommand = strtok(pLine, txtWhitespace);

    if (pCommand == NULL || (pCommand[0] == '$' && strlen(pCommand) == 1)){ 
        // Likely a line starting with space
        printReply(idBuffer, 0, txtErrorMissingCommand, pCommand);
        return;
    }
    if (pCommand[0] == '$'){
        // We support both direct commands (as used at USB for convenience) and commands starting with $
        pCommand++;
    }
    
    // Get the arguments
    str2lower(pCommand);    

    pArgs = pCommand + strlen(pCommand) + 1;

    if (strequal(pCommand, txtCmdConfig)){
        // CONFIG special command
        // As it is a config, get the first argument to know what you are configuring
        pArg1 = strtok(pArgs, txtWhitespace);
        if (pArg1 == NULL){
            printReply(idBuffer, 0, txtErrorUnknownArgument, 0);
            return;
        }

        str2lower(pArg1);    

        // This is done so if you enter: "cfg serial blah"
        // The command ends up being "cfg serial" and the arguments "blah"
        // It is done this way to make it easy for the services to identify
        // which one you are configuring

        // Fuse together command with arg1 changing the null terminator to a space
        pCommand[strlen(pCommand)] = ' ';
        // Reposition arguments pointer
        pArgs += strlen(pArg1);           
        pArgs++;
    }

    //printf("<%s><%s>\r\n", pCommand, pArgs);
    
    #ifdef LIB_CMD
        if (!nExecuted) nExecuted = Cmd_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_I2C
        if (!nExecuted) nExecuted = I2C_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_SOFTSERIAL
        if (!nExecuted) nExecuted = SoftSerial_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_PUNCHER
        if (!nExecuted) nExecuted = Puncher_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_MUSIC
        if (!nExecuted) nExecuted = Music_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_LEDS
        if (!nExecuted) nExecuted = Leds_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_KEYS
        if (!nExecuted) nExecuted = Keys_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_MONITOR
        if (!nExecuted) nExecuted = Monitor_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    #ifdef LIB_PROGRAM
        if (!nExecuted) nExecuted = Program_checkCmd(idBuffer, pCommand, pArgs);
    #endif

    if (!nExecuted){
        /* Non Library/Board dependent commands */

        // VERSION
        if (strequal(pCommand, "version") || strequal(pCommand, "ver")){
            printReply(idBuffer, 1, "VERSION", txtVersion);
            nExecuted = 1;
        }
        // HEAP
        else if (strequal(pCommand, "heap")){
            sprintf(sReply, "%u of %u", Heap_Next - Heap, sizeof(Heap));
            printReply(idBuffer, 1, "HEAP", sReply);
            nExecuted = 1;
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
        // UART
        else if (strequal(ptrCommand, "uart")){
    //        MasterSerialOut = (unsigned char) atoi(ptrArgs);
        }
*/
    }
        
    
    if (!nExecuted){
        printReply(idBuffer, 0, txtErrorUnknownCommand, 0);
    }
}



