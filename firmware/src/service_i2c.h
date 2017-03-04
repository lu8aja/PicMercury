/* 
 * File:   service_i2c.h
 * Author: Javier
 *
 * Created on March 1, 2017, 12:30 AM
 */

#include <xc.h>
#include <string.h>

#define LIB_I2C

#define I2C_BAUD 100      // Desired Baud Rate in kbps
#define I2C_FOSC 20000    // Oscillator Clock in kHz
#define I2C_InterruptFlag PIR1bits.SSPIF

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

extern i2c_t I2C;


inline void   I2C_Master_init(void);
void          I2C_Master_interrupt(void);
unsigned char I2C_Master_send(unsigned char addr, const unsigned char *buff, const unsigned char bAutoIdle);

inline void   I2C_Slave_init(void);
inline void   I2C_Slave_interrupt(void);
inline void   I2C_Slave_service(void);

void I2C_cmd(unsigned char *pArgs);

