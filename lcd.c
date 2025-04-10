#include "config.h"

#include "lcd.h"
#include <stdio.h>

void lcd_init(void) {

  ANSELDbits.ANSD0 = 0;
  ANSELDbits.ANSD1 = 0;

  TRISDbits.TRISD0 = 1; // pin as input
  TRISDbits.TRISD1 = 1; // pin as input
  TRISAbits.TRISA0 = 0; // pin as input
  LATAbits.LATA0 = 1;

  SSP2CON1bits.SSPM = 0b1000; // I2C Master mode
  SSP2ADD = 19;
  SSP2CON1bits.SSPEN = 1; // enable

  __delay_ms(5);

  SSP2CON2bits.SEN = 1;
  while (SSP2CON2bits.SEN)
    ;
  SSP2IF = 0;

  // LCD addr:  0111110 (0x3E)
  // LCD write: 01111100 (0x7C)

  lcd_send(0x7C); // device addr write
  lcd_send(0x80); // Control byte
  lcd_send(0x38); // function set
  lcd_send(0x80);
  lcd_send(0x39); // function set
  lcd_send(0x80);
  lcd_send(0x17); // internal OSC freq
  lcd_send(0x80);
  lcd_send(0x7A); // contrast set
  lcd_send(0x80);
  lcd_send(0x5E); // Power/ICON control/Contrast set
  lcd_send(0x80);
  lcd_send(0x6B); // Follower conrol
  lcd_send(0x80);
  lcd_send(0x0C); // Display ON/OFF
  lcd_send(0x80);
  lcd_send(0x01); // Clear display
  lcd_send(0x80);
  lcd_send(0x06);
  lcd_send(0x80);
  lcd_send(0x02);

  SSP2CON2bits.PEN = 1;
  while (SSP2CON2bits.PEN)
    ;

  __delay_ms(5);
}

void lcd_show_string(char lineNum, char textData[]) {
  unsigned char i;
  i = 0;

  SSP2CON2bits.SEN = 1;
  while (SSP2CON2bits.SEN)
    ;
  SSP2IF = 0;

  lcd_send(0x7c);

  lcd_send(0x80);

  if (lineNum == 1) {
    lcd_send(0x80);
  } else if (lineNum == 2) {
    lcd_send(0xC0);
  }

  lcd_send(0x40);

  for (i = 0; i < 16; i++) {
    lcd_send(textData[i]);
  }

  SSP2CON2bits.PEN = 1;
  while (SSP2CON2bits.PEN)
    ;

  __delay_ms(40);
}

static void lcd_send(unsigned char data) {

  SSP2BUF = data;
  while (SSP2STATbits.BF)
    ;
  while (!SSP2IF)
    ;
  SSP2IF = 0;
}

void lcd_clear(void) {
  SSP2CON2bits.SEN = 1;
  while (SSP2CON2bits.SEN)
    ;
  SSP2IF = 0;

  lcd_send(0x7C); // device addr write
  lcd_send(0x80); // Control byte
  lcd_send(0x01); // Clear display

  SSP2CON2bits.PEN = 1;
  while (SSP2CON2bits.PEN)
    ;
}

void lcd_reset(void) {
  LATAbits.LATA0 = 0;
  __delay_us(100);
  LATAbits.LATA0 = 1;
}
