/* 
 * File:   service_i2c.h
 * Author: Javier
 *
 * Created on March 1, 2017, 12:30 AM
 */

#include <xc.h>
#include <string.h>

#include "app_globals.h"
#include "app_io.h"
#include "app_main.h"

#define LIB_I2C

typedef struct {
    union {
        unsigned char Status;
        struct {
            unsigned Enabled:1;         // 0 = Off / 1 = On
            unsigned Slave:1;           // 0 = Master / 1 = Slave
            unsigned Execute:1;         // Slv: Must execute command / Master: Must pass message
            unsigned DiscardNoSlave:1;  // Must discard Msg at Output Buffer
            unsigned DiscardSlaveLost:1;// Must discard Msg at Output Buffer
            unsigned :3;
        };
    };
    unsigned int  Tick;     
    unsigned char Slaves;      // Bitmap of Slave statuses, it is coincident to addresses (They just use 1 bit)
    unsigned char SlaveLen;    // Number of bytes pending receiving from slave
    unsigned char State;       // State Machine: 0 = Idle ... (enum I2C_State)
    unsigned char Address;     // Slave address 7 bits
    unsigned char Requestor;   // Id of the buffer who requested the command and will get the replies
    Ring_t        *Output;     // Output ring buffer
    Ring_t        *Input;      // Input ring buffer
} i2c_t;

extern i2c_t I2C;

// Configs
#define I2C_Debug                0       // 0 to 9 Verbosity
#define I2C_Cfg_BaudRate         50      // Desired Baud Rate in kbps

#ifdef CFG_BOARD_XTAL_FREQ
    #define I2C_Cfg_BoardXtalFreq CFG_BOARD_XTAL_FREQ
#elif
    #define I2C_Cfg_BoardXtalFreq 20000      // Xtal Freq in KHz
#endif

// These are defaults, for convenience they can be defined at globals
#ifndef I2C_sizeInput
    #define I2C_sizeInput        16
#endif
#ifndef I2C_sizeOutput
    #define I2C_sizeOutput       16
#endif

// Slave pooling: Important, changing the addresses scheme, implies brutal changes to I2C.Slaves (Statuses))
#define I2C_RecoveryTicks    250       // ms between Slaves pool contacts and for recovery from errors
#define I2C_SeparationTicks  2         // ms between a Write and the next read
#define I2C_Pool_Min         0x10      // First Slave Address in pool
#define I2C_Pool_Max         0x80      // Last Slave Address in pool
#define I2C_Pool_next(a)     (a << 1)  // Delta function used when pooling slaves

// I2C Control registers, you should not need to change them
#define I2C_Cfg_Master_Control_1 0b00101000  // SSPEN = 1 Enable /CKP = 0 /SSPM[4] = 1000 I2C Master Mode
#define I2C_Cfg_Master_Control_2 0b00000000  // 
#define I2C_Cfg_Master_Status    0b10000000  // Disable slew rate

#define I2C_Cfg_Slave_Control_1  0b00110110  // SSPEN = 1 Enable /CKP = 1 /SSPM[4] = 0110 I2C Slave Mode
#define I2C_Cfg_Slave_Control_2  0b00000001  // SEN = 1
#define I2C_Cfg_Slave_Status     0b10000000  // Disable slew rate

// HAL
#define I2C_Enabled              SSPCON1bits.SSPEN
#define I2C_Status               SSPSTAT
#define I2C_Control_1            SSPCON1
#define I2C_Control_2            SSPCON2
#define I2C_ClockPolarity        SSPCON1bits.CKP
#define I2C_Pin_SDA              TRISBbits.RB0
#define I2C_Pin_SCL              TRISBbits.RB1
#define I2C_Slave_Address        SSPADD
#define I2C_Master_BaudRate      SSPADD
#define I2C_Buffer               SSPBUF
#define I2C_InterruptEnabled     PIE1bits.SSPIE
#define I2C_InterruptFlag        PIR1bits.SSPIF
#define I2C_NoAckStatus          SSPCON2bits.ACKSTAT
#define I2C_NoAckData            SSPCON2bits.ACKDT
#define I2C_EndOfMsg             0xff

#define I2C_ACK_OK               0
#define I2C_ACK_ERROR            1

#define I2C_sendStart()         {SSPCON2bits.SEN   = 1;}
#define I2C_sendRestart()       {SSPCON2bits.RSEN  = 1;}
#define I2C_sendStop()          {SSPCON2bits.PEN   = 1;}
#define I2C_sendAck(a)          {SSPCON2bits.ACKDT = a; SSPCON2bits.ACKEN = 1;}
#define I2C_setReceive()        {SSPCON2bits.RCEN  = 1;}
#define I2C_write(a)            {SSPBUF = a;}
#define I2C_read()               SSPBUF

#define I2C_Master_busy        ((SSPSTAT & 0b00000100) || (SSPCON2 &  0b00011111))
#define I2C_Slave_busy          (SSPCON2 & 0b00011111)

// State Machine (really used only by master)
enum I2C_State {
    I2C_STATE_Idle,
    I2C_STATE_Write_MustStart,
    I2C_STATE_Write_StartSent,
    I2C_STATE_Write_AddressSent,
    I2C_STATE_Write_SendData,
    I2C_STATE_Write_MustStop,
    I2C_STATE_Write_Finish,
    I2C_STATE_Read_MustStart,
    I2C_STATE_Read_StartSent,
    I2C_STATE_Read_AddressSent,
    I2C_STATE_Read_ReceiveBytes,
    I2C_STATE_Read_MustReceiveData,
    I2C_STATE_Read_ReceiveData,
    I2C_STATE_Read_MustStop,
    I2C_STATE_Read_Finish,
    I2C_STATE_Slave_Started,
    I2C_STATE_Slave_Address,
    I2C_STATE_Slave_Report,
    I2C_STATE_Slave_Data,
    I2C_STATE_Slave_Stopped,
    I2C_STATE_Slave_Error,
};        

// Public Prototypes
inline void   I2C_Master_init(void);
inline void   I2C_Master_service(void);
void          I2C_Master_interrupt(void);
inline void   I2C_Slave_init(void);
inline void   I2C_Slave_service(void);
inline void   I2C_Slave_interrupt(void);
inline void   I2C_tick(void);
unsigned char I2C_send(unsigned char idBuffer, unsigned char addr, const unsigned char *pCommand);
void          I2C_reportResult(unsigned char idBuffer, unsigned char *pStr);
void          I2C_discardMsg(const unsigned char *pMsg);
void          I2C_cmd(Ring_t * pBuffer, unsigned char *pArgs);

