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
#define I2C_sizeInput     16

typedef struct {
    unsigned char Enabled;     // 0 = Off / 1 = On
    unsigned char Slave;       // 0 = Master / 1 = Slave
    unsigned char State;       // State Machine: 0 = Off / 1 = 
    unsigned char AutoIdle;    // Set to idle when done with transaction
    unsigned char Address;     // Slave address 7 bits
    unsigned char *Output;     // Output buffer
    unsigned char InputPos;    // Input buffer position
    unsigned char *Input;      // Input buffer
} i2c_t;

i2c_t MasterI2C;

unsigned char InputBuffer[I2C_sizeInput];

unsigned char I2C_Master_send(unsigned char addr, const unsigned char *buff, const unsigned char bAutoIdle);

/*** MASTER FUNCTIONS ***/
inline void I2C_Master_init(void){
    MasterI2C.Slave   = 0;       // Master
    MasterI2C.Input   = &InputBuffer;
    MasterI2C.InputPos= 0;       // Link Input buffer and pos
    MasterI2C.State   = 0;       // State machine

    // Configure as inputs
    TRISBbits.RB0     = 1;
    TRISBbits.RB1     = 1;
    SSPSTAT           = 0;
    SSPCON1bits.SSPM  = 0b1000;  // I2C Master Mode
    SSPSTATbits.SMP   = 1;       // Disable slew rate
    SSPADD            = (BOARD_XTAL_FREQ / (4 * I2C_BAUD)) - 1; //Configure baud rate
    //Enable interrupts
    PIE1bits.SSPIE    = 1;       //Enable SSP interrupts
    PIR1bits.SSPIF    = 0;       //Clear interrupt flag
    SSPCON1bits.SSPEN = 1;       // Enable MSSP
}

void I2C_Master_service(void){
    if (I2C_InterruptFlag){
        /*
        if (MasterI2C.State){
            byte2binstr(sStr1, SSPSTAT);
            byte2binstr(sStr2, SSPCON1);
            byte2binstr(sStr3, SSPCON2);
            printf("SM=%u STAT=%s CON1=%s CON2=%s\r\n", MasterI2C.State, sStr1, sStr2, sStr3);

        }
        */

        switch(MasterI2C.State){
        /* Master Write */
            // 1 - Initiate transfer (START)
            case 1: 
                if (I2C_Master_busy){
                    // Error: I2C busy
                    MasterI2C.State = 255;
                    return;
                }
                if (*MasterI2C.Output == 0){
                    // Error: Trying to send empty packet
                    MasterI2C.State = 254;
                    return;
                }
                SSPCON2bits.SEN   = 1; //Send Start Condition
                MasterI2C.State = 2;
            // 2 - Send Address
            case 2:
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    MasterI2C.State = 253;
                    return;
                } 
                MasterI2C.State = 3;
                SSPBUF = MasterI2C.Address & 0b11111110; //Send addr + /Write bit
                break;
            // 3 - Send data until null (repeated)
            case 3: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    MasterI2C.State = 252;
                    return;
                } 
                MasterI2C.State = 3;
                SSPBUF = *MasterI2C.Output;
                MasterI2C.Output++;
                if (! *MasterI2C.Output){
                    // Null found
                    MasterI2C.State = 4;
                }
                break;
            // 4 - Null
            case 4: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    MasterI2C.State = 251;
                    return;
                } 
                MasterI2C.State = 5;
                SSPBUF = 0x00;
                break;
            // 5 - Stop
            case 5: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    MasterI2C.State = 250;
                    return;
                } 
                MasterI2C.State = 6;
                SSPCON2bits.PEN = 1; //Send Stop Condition
                break;
            // 6 - Check Stop ACK
            case 6: 
                if (I2C_Master_busy){
                    break;
                }
                if(SSPCON2bits.ACKSTAT){
                    // Error: No ACK is received
                    MasterI2C.State = 249;
                    return;
                } 
                MasterI2C.State = MasterI2C.AutoIdle ? 0 : 7;
                break;
            // 7 - Done with output, waiting for app to clear state to idle

        }
        I2C_InterruptFlag = 0; 
    }
}

unsigned char I2C_Master_send(unsigned char addr, const unsigned char *buff, const unsigned char bAutoIdle){
    if (MasterI2C.State){
        // We're not idle or there was an uncleared error!
        return MasterI2C.State;
    }
    if (I2C_Master_busy){
        // Busy error
        return 255;
    }
    if (buff[0] == 0){
        // Error: Trying to send empty packet
        return 254;
    }

    MasterI2C.AutoIdle = bAutoIdle;
    MasterI2C.Address  = addr;
    MasterI2C.Output   = buff;
    MasterI2C.State    = 2;
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
    MasterI2C.Slave   = 1;           // Slave
    MasterI2C.Input   = &InputBuffer;
    MasterI2C.InputPos= 0;           // Link Input buffer and pos
    MasterI2C.State   = 0;           // State machine
    
    TRISBbits.RB0     = 1;           // Input
    TRISBbits.RB1     = 1;           // Input
    SSPSTAT           = 0x80;        // Disable slew rate
    SSPADD            = MasterI2C.Address; // Slave Address
    SSPCON2           = 0x00;
    SSPCON1           = 0b00110110;  // SSPEN = 1 Enable /CKP = 1 Release Clock /SSPM[4] = 0110 I2C Slave Mode
    PIE1bits.SSPIE    = 1;           // Enable SSP interrupts
    PIR1bits.SSPIF    = 0;           // Clear interrupt flag
}

inline void I2C_Slave_service(void){
    unsigned char cChar;
    const unsigned char txtI2C[] = "I2C";

    
    if(SSPIF == 1){
        SSPCON1bits.CKP = 0; // Hold clock

        byte2binstr(sStr1, SSPSTAT);
        byte2binstr(sStr2, SSPCON1);
        byte2binstr(sStr3, SSPCON2);
        sprintf(sReply, "SM=%u STAT=%s CON1=%s CON2=%s", MasterI2C.State, sStr1, sStr2, sStr3);
       
        printReply(3, txtI2C, sReply);

        if (SSPCON1bits.SSPOV || SSPCON1bits.WCOL){ //If overflow or collision
          cChar = SSPBUF; // Read the previous value to clear the buffer
          SSPCON1bits.SSPOV = 0; // Clear the overflow flag
          SSPCON1bits.WCOL  = 0; // Clear the collision bit
        }
        
        if(!SSPSTATbits.DA){
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
                /* For every byte that was read... */
                if (cChar){
                    MasterI2C.Input[MasterI2C.InputPos] = cChar;
                    MasterI2C.InputPos++;
                    if (MasterI2C.InputPos >= I2C_sizeInput){
                        // Error: Buffer overflow
                        printReply(4, txtI2C, MasterI2C.Input);
                        MasterI2C.InputPos = 0;
                    }
                    MasterI2C.Input[MasterI2C.InputPos] = 0;
                }
                else{
                    MasterI2C.Input[MasterI2C.InputPos] = 0x00;
                    if (!posCommand){
                        strcpy(bufCommand, MasterI2C.Input);
                        MasterI2C.InputPos = 0;
                        MasterI2C.Input[MasterI2C.InputPos] = 0x00;
                        APP_executeCommand();
                    }
                    else{
                        printReply(3, txtI2C, MasterI2C.Input);
                        MasterI2C.InputPos++;
                        if (MasterI2C.InputPos >= I2C_sizeInput){
                            // Error: Buffer overflow
                            printReply(4, txtI2C, MasterI2C.Input);
                            MasterI2C.InputPos = 0;
                        }
                        MasterI2C.Input[MasterI2C.InputPos] = 0;
                    }
                }

                printReply(3, txtI2C, MasterI2C.Input);
            }
            else if(SSPSTATbits.RW){
                // READ
                //If last byte was Data + Read
                SSPSTATbits.BF = 0;
                if (*MasterI2C.Output){
                    SSPBUF = *MasterI2C.Output;
                    MasterI2C.Output++;
                }
                else{
                    // Buffer underrun / End of buffer
                    SSPBUF = 0;
                }
            }        
        }

        
        SSPIF = 0;
        SSPCON1bits.CKP = 1; // Release clock
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
        sprintf(sReply, "SM=%u STAT=%s CON1=%s CON2=%s", MasterI2C.State, sStr1, sStr2, sStr3);
    }
    else{
        if (MasterI2C.Slave){
            bOK = false;
            strcpy(sReply, txtErrorInvalidArgument);
        }
        else{
            // Just to make sure we don't overflow
            //pArgs[4]  = 0x00;
            nResult = I2C_Master_send(I2C_ADDRESS_PUNCHER, pArgs, 1);
            sprintf(sReply, "%s = %u", "Result", nResult);
        }
    }

    printReply(1, "I2C", sReply);
}