
/*
 * Loosely based on https://electrosome.com/i2c-pic-microcontroller-mplab-xc8/
 * More info: https://www.i2c-bus.org/
 */


#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "app_globals.h"
#include "app_helpers.h"
#include "app_io.h"
#include "app_main.h"

#define LIB_I2C

#define I2C_BAUD 100000      // Desired Baud Rate in bps
#define I2C_InterruptFlag PIR1bits.SSPIF
#define I2C_Master_busy   ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F))
#define I2C_Slave_busy    (SSPCON2 & 0x1F)

#ifndef I2C_sizeInput
    // This is the default, but for convenience it can be defined at globals with the other buffers
    #define I2C_sizeInput     16
#endif

typedef struct {
    unsigned char Enabled;     // 0 = Off / 1 = On
    unsigned char Slave;       // 0 = Master / 1 = Slave
    unsigned char State;       // State Machine: 0 = Off / 1 = 
    unsigned char AutoIdle;    // Set to idle when done with transaction
    unsigned char Address;     // Slave address 7 bits
    unsigned char Execute;     // Must execute command
    unsigned char *Output;     // Output buffer
    unsigned char InputPos;    // Input buffer position
    unsigned char Input[I2C_sizeInput]; // Input buffer
} i2c_t;

i2c_t I2C;


unsigned char I2C_Master_send(unsigned char addr, const unsigned char *buff, const unsigned char bAutoIdle);

/*** MASTER FUNCTIONS ***/
inline void I2C_Master_init(void){
    I2C.Slave   = 0;       // Master
    I2C.InputPos= 0;       // Link Input buffer and pos
    I2C.Execute = 0;
    I2C.State   = 0;       // State machine


    // Configure as inputs
    TRISBbits.RB0     = 1;
    TRISBbits.RB1     = 1;
    SSPSTAT           = 0;
    SSPCON1bits.SSPM  = 0b1000;  // I2C Master Mode
    SSPSTATbits.SMP   = 1;       // Disable slew rate
    SSPADD            = (CFG_BOARD_XTAL_FREQ / (4 * I2C_BAUD)) - 1; //Configure baud rate
    //Enable interrupts
    PIE1bits.SSPIE    = 1;       //Enable SSP interrupts
    PIR1bits.SSPIF    = 0;       //Clear interrupt flag
    SSPCON1bits.SSPEN = 1;       // Enable MSSP
}

void I2C_Master_interrupt(void){
    if (I2C_InterruptFlag){

/*
        if (I2C.State){

            byte2binstr(sStr1, SSPSTAT);
            byte2binstr(sStr2, SSPCON1);
            byte2binstr(sStr3, SSPCON2);
            printf("SM=%u STAT=%s CON1=%s CON2=%s\r\n", I2C.State, sStr1, sStr2, sStr3);
        }
 */


        switch(I2C.State){
        /* Master Write */
            // 1 - Initiate transfer (START)
            case 1: 
                if (I2C_Master_busy){
                    // Error: I2C busy
                    I2C.State = 255;
                    return;
                }
                if (*I2C.Output == 0){
                    // Error: Trying to send empty packet
                    I2C.State = 254;
                    return;
                }
                SSPCON2bits.SEN   = 1; //Send Start Condition
                I2C.State = 2;
                break;
            // 2 - Send Address
            case 2:
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    I2C.State = 253;
                    return;
                } 
                I2C.State = 3;
                SSPBUF = I2C.Address & 0b11111110; //Send addr + /Write bit
                break;
            // 3 - Send data until null (repeated)
            case 3: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    I2C.State = 252;
                    return;
                } 
                I2C.State = 3;
                SSPBUF = *I2C.Output;
                I2C.Output++;
                
                if (! *I2C.Output){
                    // Null found
                    I2C.State = 4;
                }
                break;
            // 4 - Stop
            case 4: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    I2C.State = 250;
                    return;
                } 
                I2C.State = 5;
                SSPCON2bits.PEN = 1; //Send Stop Condition
                break;
            // 5 - Check Stop ACK
            case 5: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    I2C.State = 249;
                    return;
                } 
                I2C.State = 6;//
                //SSPCON2bits.RSEN   = 1; //Send ReStart Condition
                I2C.State = I2C.AutoIdle ? 0 : 6;
                break;
            // 6 - Done with output, send restart for input
            //case 6: 
/*
                if (I2C_Master_busy){
                    // Error: I2C busy
                    I2C.State = 248;
                    return;
                }
                I2C.State = 7;
                SSPBUF = I2C.Address | 0b00000001; //Send addr + Read bit
                
                break;
            case 7: 
                if (I2C_Master_busy){
                    // Error: I2C busy
                    I2C.State = 248;
                    return;
                }
                I2C.State = 8;
                unsigned char temp;
                temp = SSPBUF;
                if (!temp){
                    I2C.State = 8;
                }
                break;
            case 7: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    I2C.State = 247;
                    return;
                } 
                I2C.State = I2C.AutoIdle ? 0 : 6;
                break;
*/
        }
        I2C_InterruptFlag = 0; 
    }
}

unsigned char I2C_Master_send(unsigned char addr, const unsigned char *buff, const unsigned char bAutoIdle){
    I2C.State = 0;
    if (I2C.State){
        // We're not idle or there was an uncleared error!
        return I2C.State;
    }
    if (I2C_Master_busy){
        // Busy error
        return 255;
    }
    if (buff[0] == 0){
        // Error: Trying to send empty packet
        return 254;
    }
    // This is technically not a problem for the master but for our slaves... But the master cooperates
    if (strlen(buff) >= I2C_sizeInput){
        return 253;
    }

    I2C.AutoIdle = bAutoIdle;
    I2C.Address  = addr;
    I2C.Output   = buff;
    I2C.State    = 2;
    SSPCON2bits.SEN    = 1; //Send Start Condition
    
    return 0;
}

/*
unsigned short I2C_Master_read(unsigned short a){
  unsigned short temp;
  I2C_Master_Wait();
  RCEN = 1;
  I2C_Master_Wait();
  temp = SSPBUF;      //Read data from SSPBUF
  I2C_Master_Wait();
  ACKDT = (a)?0:1;    //Acknowledge bit
  ACKEN = 1;          //Acknowledge sequence
  return temp;
}
 * */


/*** SLAVE FUNCTIONS ***/

inline void I2C_Slave_init(void){
    I2C.Slave   = 1;           // Slave
    I2C.InputPos= 0;           // Link Input buffer and pos
    I2C.Execute = 0;
    I2C.State   = 0;           // State machine
    
    TRISBbits.RB0     = 1;           // Input
    TRISBbits.RB1     = 1;           // Input
    SSPSTAT           = 0x80;        // Disable slew rate
    SSPADD            = I2C.Address; // Slave Address
    SSPCON2           = 0x00;
    SSPCON1           = 0b00111110;  // SSPEN = 1 Enable /CKP = 1 Release Clock /SSPM[4] = 0110 I2C Slave Mode
    PIE1bits.SSPIE    = 1;           // Enable SSP interrupts
    PIR1bits.SSPIF    = 0;           // Clear interrupt flag
}


inline void I2C_Slave_service(void){
    if (I2C.Execute){
        if (I2C.Input[0]){
            APP_executeCommand(I2C.Input);
            I2C.InputPos = 0;
            I2C.Input[0] = 0x00;
        }
        I2C.Execute = 0;
    }
};
                    
inline void I2C_Slave_interrupt(void){
    unsigned char cChar;
    const unsigned char txtI2C[] = "I2C";

    
    if(I2C_InterruptFlag){
        SSPCON1bits.CKP = 0; // Hold clock

        if (SSPSTATbits.S && !SSPSTATbits.BF){
            if (I2C.Execute){
//putch('!');     
                I2C.State = 220;
            }
            // Start bit
            I2C.InputPos = 0;
            I2C.Input[I2C.InputPos] = 0x00;
//putch('<');
        }
        else if (SSPSTATbits.P){
//putch('>');
            I2C.State = 6;
            // Stop bit
            if (I2C.InputPos){
                I2C.Execute = 1;
            }
        }
        else {
            // Data transfers
            if (SSPCON1bits.SSPOV || SSPCON1bits.WCOL){ //If overflow or collision
              cChar = SSPBUF; // Read the previous value to clear the buffer
              SSPCON1bits.SSPOV = 0; // Clear the overflow flag
              SSPCON1bits.WCOL  = 0; // Clear the collision bit
              I2C.State = 219;
            }

            if(!SSPSTATbits.DA){
//putch('A');
                I2C.State = 3;
                // ADDRESS
                if (!SSPSTATbits.RW){ 
                    // Address + Write
                    cChar = SSPBUF; // Read the previous value to clear the buffer
                }
                else{
                    // WTF?
                }
            }
            else {
                // DATA
                if(!SSPSTATbits.RW){ 
                    // WRITE
                    cChar = SSPBUF;
//putch(cChar);
                    I2C.Input[I2C.InputPos] = cChar;
                    I2C.InputPos++;
                    if (I2C.InputPos >= I2C_sizeInput){
                        // Error: Buffer overflow
                        printReply(4, txtI2C, I2C.Input);
                        I2C.InputPos = 0;
                        I2C.State = 218;
                    }
                    I2C.Input[I2C.InputPos] = 0;
                }
                else if(SSPSTATbits.RW){
                    // READ
                    //If last byte was Data + Read
                    SSPSTATbits.BF = 0;
                    if (*I2C.Output){
                        SSPBUF = *I2C.Output;
                        I2C.Output++;
                    }
                    else{
                        // Buffer underrun / End of buffer
                        SSPBUF = 0;
                    }
                }        
            }
        }

        
        I2C_InterruptFlag = 0;
        SSPCON1bits.CKP   = 1; // Release clock
    }
}


void I2C_cmd(unsigned char *pArgs){
    bool bOK = true;
    unsigned char nResult;
    
    sReply[0] = 0x00;
    
    if (pArgs[0] == 0){
        byte2binstr(sStr1, SSPSTAT);
        byte2binstr(sStr2, SSPCON1);
        byte2binstr(sStr3, SSPCON2);
        sprintf(sReply, "SM=%u STAT=%s CON1=%s CON2=%s", I2C.State, sStr1, sStr2, sStr3);
    }
    else{
        if (I2C.Slave){
            bOK = false;
            strcpy(sReply, txtErrorInvalidArgument);
        }
        #if defined(DEVICE_CONSOLE)
        else{
            nResult = I2C_Master_send(I2C_ADDRESS_PUNCHER, pArgs, 1);
            sprintf(sReply, "%s = %u", "Result", nResult);
        }
        #endif
    }

    printReply(bOK, "I2C", sReply);
}