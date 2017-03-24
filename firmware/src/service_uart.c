#define LIB_UART

#include <xc.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>


#include "app_globals.h"
#include "lib_helpers.h"
#include "app_io.h"


void UART_init(void);
void UART_service(void);
void UART_cmd(void);


void UART_init(void){
    // UART
	TRISCbits.TRISC6  = 1; // TX pin

	BAUDCONbits.WUE   = 0; // Wake-up Enable bit
	BAUDCONbits.ABDEN = 0; // Auto-Baud Detect Enable bit
	BAUDCONbits.BRG16 = 1; // 16-Bit Baud Rate Register Enable bit
	BAUDCONbits.TXCKP = 0; // Inverted
	SPBRGH = 230;          // 50 BAUDS
	SPBRG  = 255;          // 


	TXSTAbits.SYNC = 0; // EUSART Mode Select bit
	TXSTAbits.BRGH = 0; // High Baud Rate Select bit
	TXSTAbits.TX9  = 0; // 9-Bit Transmit Enable bit
	TXSTAbits.TXEN = 1; // Transmit Enable bit
	
	RCSTAbits.RX9  = 0; // 9-Bit Receive Enable bit
	RCSTAbits.CREN = 0; // Continuous Receive Enable bit
	RCSTAbits.SPEN = 1; // Serial Port Enable bit
}

void UART_service(void){
    
    if (PIR1bits.TXIF){
       // TXREG = MasterSerialOut;
    }
}


void UART_cmd(void){
    
}