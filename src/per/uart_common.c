#include "uart_common.h"
#include "../config.h"

void uart_common_init(void) {
  TRISCbits.TRISC6 = 1; // TX pin jako vstup
  TRISCbits.TRISC7 = 1; // rx pin jako vstup

  ANSELC = 0x00; // Configure PORTC pins as digital

  // Configure interrupt priority
  RCONbits.IPEN = 1; // Enable priority levels

  RC1IE = 1; // zap  preruseni od RCREG

  // Configure baudrate
  SPBRG1 = 51; // (32_000_000 / (64 * 9600)) - 1

  TXSTA1bits.SYNC = 0; // EUSART Mode Select bit - ASYNC
  RCSTA1bits.SPEN = 1; // Serial Port Enable bit
  TXSTA1bits.TXEN = 1; // Transmit Enable bit
  RCSTA1bits.CREN = 1; // Continuous Receive Enable bit
}

void uart_common_deinit(void) {
  RC1IE = 0;

  RCSTA1bits.SPEN = 0; // Serial Port Enable bit
  TXSTA1bits.TXEN = 0; // Transmit Enable bit
  RCSTA1bits.CREN = 0; // Continuous Receive Enable bit
}

void putch(char data) {
  while (!TX1IF)
    ;
  TXREG1 = data;
}

int getch(void) {
  if (!RC1IF)
    return 0;
  return RCREG1;
}
