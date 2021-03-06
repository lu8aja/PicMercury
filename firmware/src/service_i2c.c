
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

#include "service_i2c.h"

i2c_t I2C;

void I2C_dump(void);

/*** MASTER FUNCTIONS ***/

inline void I2C_Master_init(void){
    I2C.Input  = ring_new(I2C_sizeInput);
    I2C.Output = ring_new(I2C_sizeOutput);

    System.Error.I2cInput  = (I2C.Input  && I2C.Input->Buffer)  ? 0 : 1;
    System.Error.I2cOutput = (I2C.Output && I2C.Output->Buffer) ? 0 : 1;
    
    if (!I2C.Input || !I2C.Output){
        I2C.State = 255; // Not enough heap for Master buffers
        return;
    }
    
    System.Buffers[1] = I2C.Output;
    
    //INTCON2bits.RBPU = 0; // Pullups
    
    I2C.Slave   = 0;         // Master
    I2C.Execute = 0;
    I2C.State   = 0;         // State machine
    I2C.Address = 0;         // Address of slave to pool
    I2C.Tick    = 0;

    TRISBbits.RB2 = 0;
    TRISBbits.RB3 = 0;
    
    // Configure MSSP Module
    I2C_Control_1        = 0;
    I2C_Pin_SDA          = 1;
    I2C_Pin_SCL          = 1;
    I2C_Status           = I2C_Cfg_Master_Status;
    I2C_Master_BaudRate  = 250;//(I2C_Cfg_BoardXtalFreq / (4 * I2C_Cfg_BaudRate)) - 1;
    I2C_Control_2        = I2C_Cfg_Master_Control_2;
    I2C_InterruptFlag    = 0;
    I2C_InterruptEnabled = 1;
    I2C_Control_1        = I2C_Cfg_Master_Control_1;
}

inline void I2C_Master_service(void){
    unsigned char nChar;
    unsigned char idFrom;
    unsigned char idTo;
    unsigned char idBuffer;

    if (!I2C_Enabled){
        return;
    }

    if (I2C.Execute){
        nChar = ring_findChr(I2C.Input, I2C_EndOfMsg, 0);
        if (nChar < 3){
            nChar++;
            while (nChar--) ring_get(I2C.Input);
            printReply(0, 2, "I2C", txtErrorCorrupt);
        }
        else if (nChar == 255){
            I2C.Execute = 0;
        }
        else {
            idTo   = ring_get(I2C.Input);
            idFrom = ring_get(I2C.Input);
            nChar--;
            nChar--;
            if (nChar >= sizeof(bufCommand)){
                printReply(0, 2, "I2C", txtErrorTooBig);
                if (idFrom){
                    ring_write(I2C.Output, idFrom);
                    ring_write(I2C.Output, idTo);
                    printReply(1, 2, "I2C", txtErrorTooBig);
                    ring_write(I2C.Output, I2C_EndOfMsg);
                }
            }
            else{
                ring_str(I2C.Input, bufCommand,  nChar, 0);
                ring_get(I2C.Input); // Discard EoM
                
                if ((idTo & 0xf0) != I2C_Address_Master){
                    // A slave is sending a message to another slave, relay it
                    I2C_send(idFrom, 0, idTo, 0, bufCommand);
                }
                else{
                    // Addressed to master
                    idBuffer = (idTo & 0x0e) >> 1;
                    if (bufCommand[0] == '$'){
                        // Command received
                        printReply(0, 8, "I2C", bufCommand);
                        ring_write(I2C.Output, idFrom);
                        ring_write(I2C.Output, idTo);
                        APP_executeCommand(1, &bufCommand[1]);
                        ring_write(I2C.Output, I2C_EndOfMsg);
                    }
                    else if(idBuffer == 0){
                        // Message received for USB
                        printReply(0, 3, "I2C", bufCommand);
                    }
                    else if(System.Buffers[idBuffer]){
                        // Message received for another buffer
                        ring_append(System.Buffers[idBuffer], bufCommand);
                    }
                }
            }
        }
    }

    if (I2C.Tick){
        return;
    }
    
    if (I2C.DiscardNoSlave){
        I2C_discardMsg("NoSlave");
        I2C.DiscardNoSlave = 0;
    }
    if (I2C.DiscardSlaveLost){
        I2C_discardMsg("SlaveLost");
        I2C.DiscardSlaveLost = 0;
    }

    
    if (I2C.State != I2C_STATE_Idle){
        /* Recover from errors */
        #if I2C_Debug > 8
            putch('~');
        #endif
        I2C.Requestor = 0;
        if (I2C_Master_busy){
            I2C.State = I2C_STATE_Write_MustStop;
            #if I2C_Debug > 8
                putch('}');
            #endif
        }
        else{
            I2C.State = I2C_STATE_Idle;
            #if I2C_Debug > 8
                putch('-');
            #endif
        }
    }

    if (I2C.State == I2C_STATE_Idle){
        // Idling, search for msg separator
        nChar = ring_findChr(I2C.Output, I2C_EndOfMsg, 0);
        if (nChar == 255){
            // Start a routine check for slaves
            I2C.State = I2C_STATE_Read_MustStart;
            // Select the next slave to process
            I2C.Address = I2C_Pool_next(I2C.Address);
            if (I2C.Address < I2C_Pool_Min || I2C.Address > I2C_Pool_Max){
                I2C.Address = I2C_Pool_Min;
            }
            I2C_Master_interrupt();
        }
        else if (nChar < 3){
            // The EOM flag should never be in positions 
            // 0 (address TO), 1 (requestor FROM) or 2 (first character of command)
            // as that would imply an empty msg.
            // To recover, lets read them out and retry the buffer later
            nChar++;
            while (nChar--) ring_get(I2C.Output);
            return;
        }
        else{
            // So we have some full command to send
            ring_read(I2C.Output, &I2C.Address);
            ring_read(I2C.Output, &I2C.Requestor);
            
            #if I2C_Debug > 4
                ring_assert(I2C.Output, sStr5, sizeof(sStr5), 0);
                printf("[A=%02x][R=%02x][%s]", I2C.Address, I2C.Requestor, sStr5);
            #endif
            I2C.State = I2C_STATE_Write_MustStart;
            I2C_Master_interrupt();
        }
    }
}

void I2C_Master_interrupt(void){
    unsigned char nChar;

    #if I2C_Debug > 8
        putch('.');
        LATBbits.LB3 = 1;
    #endif

    I2C_InterruptFlag = 0; 
        
    I2C.Tick = I2C_RecoveryTicks;
    
    switch(I2C.State){
    /* Master Write */
        // Initiate transfer => Send Start (this one is normally not called from the interrupt as there has been no events I2C before it)
        case I2C_STATE_Write_MustStart: 
            I2C.State = I2C_STATE_Write_StartSent;
            #if I2C_Debug > 8
                putch('\r');
                putch('\n');
                putch('W');
                putch('<');
                LATBbits.LB2 = 1;
                LATBbits.LB2 = 0;
            #endif
            I2C_sendStart();
            break;
        // Start Sent => Send Address
        case I2C_STATE_Write_StartSent:
            I2C.State = I2C_STATE_Write_AddressSent;
            #if I2C_Debug > 8
                putch((I2C.Address >> 4 ) + 48);
            #endif
            I2C_Buffer = I2C.Address; //Send addr should end in 0 as /Write bit
            break;
        // Address Sent => Send Requestor
        case I2C_STATE_Write_AddressSent: 
            if(I2C_NoAckStatus){
                // Error: No ACK is received, the slave we want is not there
                I2C.DiscardNoSlave = 1;
                #if I2C_Debug > 8
                    putch('!'); putch('>');
                #endif
                I2C.State = I2C_STATE_Write_Finish;
                I2C_sendStop();
                break;
            } 
            #if I2C_Debug > 8
                putch(I2C.Requestor + 48);
            #endif
            I2C.State = I2C_STATE_Write_SendData;            
            I2C_Buffer = I2C.Requestor;
            break;
        // Send Data
        case I2C_STATE_Write_SendData: 
            if(I2C_NoAckStatus){
                // Error: No ACK is received, the slave we want is not there anymore
                I2C.DiscardSlaveLost = 1;
                #if I2C_Debug > 8
                    putch('!'); putch('>');
                #endif
                I2C.State = I2C_STATE_Write_Finish;
                I2C_sendStop();
                break;
            } 
            nChar = ring_get(I2C.Output);
            #if I2C_Debug > 6
                if (nChar < 32) printf("(%02x)", nChar); else putch(nChar);
            #endif
            I2C_Buffer = nChar;
            if (nChar == I2C_EndOfMsg){
                I2C.State = I2C_STATE_Write_MustStop;
            }
            break;
        // Data Sent => Stop
        case I2C_STATE_Write_MustStop: 
            I2C.State = I2C_STATE_Write_Finish;
            #if I2C_Debug > 8
                putch('>');
            #endif
            I2C_sendStop();
            break;
        // Stop sent => Finish
        case I2C_STATE_Write_Finish: 
            I2C.State = I2C_STATE_Idle;
            break;
            
        /*** READ FROM SLAVES ***/
        // Initiate => Send start
        case I2C_STATE_Read_MustStart:
            I2C.State = I2C_STATE_Read_StartSent;
            #if I2C_Debug > 8
                putch('\r'); putch('\n'); putch('R'); putch('<'); LATBbits.LB2 = 1; LATBbits.LB2 = 0;
            #endif
            I2C_sendStart();
            break;
        // Start Sent => Send Address
        case I2C_STATE_Read_StartSent: 
            I2C.State = I2C_STATE_Read_AddressSent;
            // Bitwise Round Robin between slaves
            I2C_Buffer = I2C.Address | 0b00000001; //Send addr + Read bit
            #if I2C_Debug > 8
                putch((I2C.Address >> 4) + 48);
            #endif
            break;
        // Address Sent => Check Slave ACK and start receiving
        case I2C_STATE_Read_AddressSent: 
            if (I2C_NoAckStatus){
                // Error: No Slave connected, flag the bit as a zero
                I2C.Slaves &= ~I2C.Address;
                I2C.State   = I2C_STATE_Idle;
                #if I2C_Debug > 8
                    putch('!'); putch('>');
                #endif
                I2C_sendStop();
                break;
            }
            // Flag bit as connected
            I2C.Slaves |= I2C.Address;
            I2C.State = I2C_STATE_Read_ReceiveBytes;
            I2C_setReceive();
            #if I2C_Debug > 8
                putch('Z');
            #endif
            break;
        // Receive Bytes => Must Receive Data
        case I2C_STATE_Read_ReceiveBytes: 
            // Get the number of bytes to read
            I2C.SlaveLen = I2C_Buffer;
            #if I2C_Debug > 7
                printf("(n=%u)", I2C.SlaveLen);
            #endif

            if (I2C.SlaveLen && I2C.SlaveLen < ring_available(I2C.Input)){
                I2C.State = I2C_STATE_Read_MustReceiveData;
                #if I2C_Debug > 8
                    putch('K');
                #endif
                I2C_sendAck(I2C_ACK_OK);
            }
            else{
                // The slave reports it has no data for us
                I2C.State = I2C_STATE_Read_Finish;
                #if I2C_Debug > 8
                    putch('>');
                #endif
                I2C_sendStop();
            }
            break;
        case I2C_STATE_Read_MustReceiveData:
            I2C.State = I2C_STATE_Read_ReceiveData;
            #if I2C_Debug > 8
                putch('R');
            #endif
            I2C_setReceive();
            break;
        // Receiving Data
        case I2C_STATE_Read_ReceiveData: 
            nChar = I2C_Buffer;
            if (!ring_write(I2C.Input, nChar)){
                // We run out of space
                //I2C.State = I2C_STATE_Read_MustStop;
                #if I2C_Debug > 8
                    putch('E');
                #endif
                //I2C_sendAck(I2C_ACK_ERROR);
                I2C.State = I2C_STATE_Read_Finish;
                I2C_sendStop();

                break;
            }
            if (nChar == I2C_EndOfMsg){
                I2C.Execute = 1;
            }
            #if I2C_Debug > 6
                if (nChar < 32) printf("(%02x)", nChar); else putch(nChar);
            #endif

            I2C.SlaveLen--;
            if (I2C.SlaveLen){
                #if I2C_Debug > 8
                    putch('K');
                #endif
                I2C.State = I2C_STATE_Read_MustReceiveData;
                I2C_sendAck(I2C_ACK_OK);
            }
            else{
                // Done, lets finish
                #if I2C_Debug > 8
                    putch('>');
                #endif
                I2C.State = I2C_STATE_Read_Finish;
                I2C_sendStop();
            }
            
            break;
        // Send stop => Finish
        case I2C_STATE_Read_MustStop: 
            I2C.State = I2C_STATE_Read_Finish;
            #if I2C_Debug > 8
                putch('>');
            #endif
            I2C_sendStop();
            break;
        // Finish
        case I2C_STATE_Read_Finish:
            I2C.State = I2C_STATE_Idle;
            break;
    }
    #if I2C_Debug > 8
        LATBbits.LB3 = 0;
    #endif
}


/*** SLAVE FUNCTIONS ***/

inline void I2C_Slave_init(void){
    I2C.Input  = ring_new(I2C_sizeInput);
    I2C.Output = ring_new(I2C_sizeOutput);

    if (!I2C.Input || !I2C.Output){
        I2C.State = 254; // Not enough heap for Slave buffers
        return;
    }

    System.Buffers[1] = I2C.Output;
    
    I2C.Slave   = 1;           // Slave
    I2C.Execute = 0;
    I2C.State   = 0;           // State machine
    
    I2C_Pin_SDA          = 1;  // Input
    I2C_Pin_SCL          = 1;  // Input
    I2C_Status           = I2C_Cfg_Slave_Status;
    I2C_Slave_Address    = I2C.Address; // My Slave Address
    I2C_Control_2        = I2C_Cfg_Slave_Control_2;
    I2C_InterruptEnabled = 1;
    I2C_InterruptFlag    = 0;
    I2C_Control_1        = I2C_Cfg_Slave_Control_1;
    SSPCON2bits.SEN      = 0;
}


inline void I2C_Slave_service(void){
    unsigned char nChar;
    unsigned char idTo;
    unsigned char idFrom;
    unsigned char idBuffer = 0;
    
    if (I2C.Execute){
        nChar = ring_findChr(I2C.Input, I2C_EndOfMsg, 0);
        if (nChar < 3){
            nChar++;
            while (nChar--) ring_get(I2C.Input);
            printReply(0, 2, "I2C", txtErrorCorrupt);
        }
        else if (nChar == 255){
            I2C.Execute = 0;
        }
        else {
            idTo   = ring_get(I2C.Input);
            idFrom = ring_get(I2C.Input);
            nChar--;
            nChar--;
            if (nChar >= sizeof(bufCommand)){
                printReply(0, 2, "I2C", txtErrorTooBig);
                if (idFrom){
                    ring_write(I2C.Output, idFrom);
                    ring_write(I2C.Output, idTo);
                    printReply(1, 2, "I2C", txtErrorTooBig);
                    ring_write(I2C.Output, I2C_EndOfMsg);
                }
            }
            else{
                idBuffer = (idTo & 0x0e) >> 1;
                
                ring_str(I2C.Input, bufCommand,  nChar, 0);
                ring_get(I2C.Input); // Discard EoM
                
                if (bufCommand[0] == '$'){
                    // Command received
                    printReply(0, 8, "I2C", bufCommand);
                    ring_write(I2C.Output, idFrom);
                    ring_write(I2C.Output, idTo);
                    APP_executeCommand(1, &bufCommand[1]);
                    ring_write(I2C.Output, I2C_EndOfMsg);
                }
                else if(idBuffer == 0){
                    // Message received for USB
                    printReply(0, 3, "I2C", bufCommand);
                }
                else if(System.Buffers[idBuffer]){
                    // Message received for another buffer
                    ring_append(System.Buffers[idBuffer], bufCommand);
                }
                
            }
        }


    }
};
                    
inline void I2C_Slave_interrupt(void){
    unsigned char nChar;
    // Note: Never return without releasing the clock, as it is held automatically
    #if I2C_Debug > 8
        putch('.');
    #endif
    if (SSPSTATbits.P){
        nChar = SSPBUF; // Read the previous value to clear the buffer
        #if I2C_Debug > 8
            printf("(%02x)", nChar);
        #endif
    }
    if (SSPSTATbits.S){
        nChar = SSPBUF; // Read the previous value to clear the buffer
        // Payload transfers
        if (SSPCON1bits.SSPOV){ //If overflow or collision
            #if I2C_Debug > 8
                putch('E');
            #endif
            SSPCON1bits.SSPOV = 0; // Clear the overflow flag
            
            // I2C.State = 219;// todo!!!
        }

        if (SSPCON1bits.WCOL){ //If overflow or collision
            #if I2C_Debug > 8
                putch('e');
            #endif
            
            SSPCON1bits.WCOL  = 0; // Clear the collision bit
            // I2C.State = 219;// todo!!!
        }

        if(!SSPSTATbits.DA){
            // ADDRESS
            #if I2C_Debug > 8
                putch('A');
            #endif
            
            // check General Call (TODO!!!!)
            #if I2C_Debug > 8
                printf("[%02x]", nChar);
            #endif
            if (SSPSTATbits.RW){ 
                // Address + Read: Send the pending output buffer length
                SSPBUF = ring_strlen(I2C.Output);
                #if I2C_Debug > 8
                    putch('L');
                #endif
            }
            else{
                // Address + Write
                if (!ring_is_empty(I2C.Input) && ring_tail(I2C.Input) != I2C_EndOfMsg){
                    // Add EOM to ensure proper msg separation in the event of corruption on the line
                    ring_write(I2C.Input, I2C_EndOfMsg);
                }

                // Append the address
                ring_write(I2C.Input, nChar);
            }
        }
        else {
            // DATA
            if(!SSPSTATbits.RW){ 
                // WRITE
                #if I2C_Debug > 8
                    if (nChar < 32) printf("(%02x)", nChar); else putch(nChar);
                #endif

                ring_write(I2C.Input, nChar);
                if (nChar == I2C_EndOfMsg){
                    I2C.Execute = 1;
                }
                /*
                if (I2C.InputPos >= I2C_sizeInput){
                    // Error: Buffer overflow
                    printReply(4, txtI2C, I2C.Input);
                    I2C.InputPos = 0;
                    I2C.State = 218;
                }
                I2C.Input[I2C.InputPos] = 0;
                 * */
            }
            else if(SSPSTATbits.RW){
                #if I2C_Debug > 8
                    putch('R');
                #endif
                // READ
                //If last byte was Data + Read
                SSPSTATbits.BF = 0;
                SSPBUF = ring_get(I2C.Output);
            }        
        }
    }

    I2C_ClockPolarity = 1; // Release clock
    I2C_InterruptFlag = 0;
}


/*** Supporting functions ***/

inline void I2C_tick(void){
    if (I2C.Tick){
        I2C.Tick--;
    }
}

unsigned char I2C_send(unsigned char addrFrom, unsigned char idFrom, unsigned char addrTo, unsigned char idTo, const unsigned char *pMsg){
    // Address 0x00 is General Call (Master -> All Slaves)
    // Address 0x10 Is Master Receives (Slave -> Master)
    // Others are Master -> Slave

    if (!I2C_Enabled){
        return 253;
    }
    
    if (pMsg[0] == 0){
        // Error: Trying to send empty packet
        return 252;
    }
    if (addrTo != I2C_Address_Master && addrTo & I2C.Slaves == 0){
        // Sending to a slave, check if it is connected
        return 251;
    }
    
    unsigned char nLen = strlen(pMsg) + 3;
    if (nLen > ring_available(I2C.Output)){
        return 250;
    }
    
    if (!addrFrom){
        addrFrom = I2C.Slave ? I2C.Address : I2C_Address_Master;
    }

    addrFrom |= (idFrom << 1);
    addrTo   |= (idTo << 1);
    
    ring_write(I2C.Output, addrTo);
    ring_write(I2C.Output, addrFrom);
    ring_append(I2C.Output, pMsg);
    ring_write(I2C.Output, I2C_EndOfMsg);
    I2C.Tick = 0;
    
    return 0;
}


void I2C_discardMsg(const unsigned char *pMsg){
    unsigned char nChar;
    
    do { 
        if (!ring_read(I2C.Output, &nChar)){
            break;
        }
    }
    while (nChar != I2C_EndOfMsg);
    
    if (pMsg){
        printReply((I2C.Requestor & 0x0e) >> 1, 2, "I2C", pMsg);
    }
    
    I2C.Requestor = 0;
    I2C.Address   = 0;
}
    


void I2C_dump(void){
    putch('\r');
    putch('\n');
    putch('\t');
    putch(SSPSTATbits.S  ? 'S' : 's');
    putch(SSPSTATbits.P  ? 'P' : 'p');
    putch(SSPSTATbits.DA ? 'D' : 'A');
    putch(SSPSTATbits.RW ? 'R' : 'W');
    putch(SSPCON1bits.SSPOV ? 'V' : 'v');
    putch(SSPCON1bits.CKP ? 'K' : 'k');
    putch('\r');
    putch('\n');
    /*
    byte2binstr(sStr1, SSPSTAT);
    byte2binstr(sStr2, SSPCON1);
    byte2binstr(sStr3, SSPCON2);
    print(txtCrLf);
    printf("SM=%u STAT=%s CON1=%s CON2=%s\r\n", I2C.State, sStr1, sStr2, sStr3);
    printf("\t%-5s ", SSPSTATbits.S ? "START": (SSPSTATbits.P  ? "STOP" : "----"));
    printf("%7s ",    SSPSTATbits.DA ? "Data" : "Address");
    printf("%5s ",    SSPSTATbits.RW ? "Read" : "Write");
    
    printf("S =%u ",   SSPSTATbits.S);
    printf("P =%u ",   SSPSTATbits.P);
    printf("BF=%u ",   SSPSTATbits.BF);
    printf("DA=%u ",   SSPSTATbits.DA);
    printf("RW=%u ",   SSPSTATbits.RW);
    printf("CKP=%u ",  SSPCON1bits.CKP);
    printf("POV=%u ",  SSPCON1bits.SSPOV);
    printf("WCOL=%u\r\n",  SSPCON1bits.WCOL);
    */
    
}

inline unsigned char I2C_checkCmd(unsigned char idBuffer, unsigned char *pCommand, unsigned char *pArgs){
    if (strequal(pCommand, "cfg i2c")){
        I2C_cmd_cfg(idBuffer, pArgs);
        return 1;
    }
    if (strequal(pCommand, "i2c")){
        I2C_cmd(idBuffer, pArgs);
        return 1;
    }
    return 0;
}

void I2C_cmd_cfg(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    bool bShowStatus = true;
    unsigned char *pArg   = NULL;
   
    sReply[0] = 0x00;

    if (pArgs && *pArgs){
        pArg    = strtok(pArgs, txtWhitespace);
        if (pArg){
            str2lower(pArg);
        }
    }
       
    if (pArg == NULL){
        
    }
    else if (strequal(pArg, "in")){
        ring_dump(I2C.Input, sReply);
        bShowStatus = false;
    }
    else if (strequal(pArg, "out")){
        ring_dump(I2C.Output, sReply);
        bShowStatus = false;
    }
    else if (strequal(pArg, "on")){
        ring_clear(I2C.Input);
        ring_clear(I2C.Output);
        I2C_Enabled = 1;
        strcpy(sReply, txtOn);
    }
    else if (strequal(pArg, "off")){
        I2C_Enabled = 0;
        strcpy(sReply, txtOff);
    }
    
    if (bShowStatus){
        byte2binstr(sStr1, SSPSTAT);
        byte2binstr(sStr2, SSPCON1);
        byte2binstr(sStr3, SSPCON2);
        byte2binstr(sStr4, I2C.Slaves);

        sprintf(&sReply[strlen(sReply)], "S=%u I=%u O=%u Sl=%u Ss=%s STAT=%s CON1=%s CON2=%s", 
            I2C.State,
            ring_strlen(I2C.Input),
            ring_strlen(I2C.Output),  
            (I2C.Address >> 4),
            sStr4,
            sStr1,
            sStr2,
            sStr3
        );
    }

    printReply(idBuffer, bOK, "I2C", sReply);
}

void I2C_cmd(unsigned char idBuffer, unsigned char *pArgs){
    bool bOK = true;
    unsigned char nResult;
    unsigned char nAddress = 0;
    unsigned char idTo     = 0;
    unsigned char *pArg    = NULL;
    unsigned char *pTxt    = NULL;
   
    sReply[0] = 0x00;

    // Split the two mandatory arguments
    if (pArgs && *pArgs){
        pArg    = strtok(pArgs, txtWhitespace);
        if (pArg){
            str2lower(pArg);
            pTxt = pArg + strlen(pArg) + 1;
            if (!*pTxt){
                pTxt = NULL;
            }
        }
    }

    if (pArg == NULL || pTxt == NULL){
        bOK = false;
        strcpy(sReply, txtErrorMissingArgument);
    }
    else{
        switch (pArg[0]){
            #ifdef DEVICE_I2C_MASTER
                case 'p': 
                    nAddress = CFG_I2C_ADDRESS_PUNCHER; 
                    break;
                case 'r': 
                    nAddress = CFG_I2C_ADDRESS_READER;  
                    break;
                case 'c': 
                    nAddress = CFG_I2C_ADDRESS_CONSOLE; 
                    break;
                case 'm': 
                    nAddress = CFG_I2C_ADDRESS_MONITOR;    
                    break;
            #else
                case 'm': 
                    nAddress = I2C_Address_Master;    
                    break;
            #endif
        }
        if (nAddress){
            if (pArg[1] >= '0' && pArg[1] < '8'){
                idTo = pArg[1] - 48;
            }
            if (pTxt[0] == '$'){
                // Commands must always be directed to port 0
                idTo = 0;
            }
            
            nResult = I2C_send(0, idBuffer, nAddress, idTo, pTxt);
            if (nResult){
                bOK = false;
                sprintf(sReply, "%u", nResult);
            }
        }
        else{
            bOK = false;
            strcat(sReply, txtErrorInvalidArgument);
        }     
    }

    printReply(idBuffer, bOK, "I2C", sReply);
}

