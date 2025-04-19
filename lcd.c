#include "config.h"

#include "lcd.h"
#include <stdbool.h>
#include <stdio.h>

#define RST LATAbits.LATA0

void lcd_init(void) {

  ANSELDbits.ANSD0 = 0; // turn off analog for D0
  ANSELDbits.ANSD1 = 0; // turn off analog for D1

  TRISDbits.TRISD0 = 1; // pin RD0 as input
  TRISDbits.TRISD1 = 1; // pin RD1 as input
  TRISAbits.TRISA0 = 0; // pin RA0 as output

  LATAbits.LATA0 = 1;

  // I2C Master mode, clock = FOSC / (4 * (SSPxADD+1))
  SSP2CON1bits.SSPM = 0b1000;

  SSP2ADD = 19; // Baud rate clock divider bits
                // clock period = 2.5E-6 sec

  // Synchronous Serial Port Enable bit
  SSP2CON1bits.SSPEN = 1;

  __delay_ms(5);

  start_i2c_comunication();

  send_instruction(0x39); // function set
  send_instruction(0x39); // function set - datasheet says to do it twice
  send_instruction(0x17); // internal OSC freq
  send_instruction(0x7A); // contrast set CS3..0
  send_instruction(0x5E); // Power/ICON control/Contrast set CS5..4
  send_instruction(0x6B); // Follower conrol
  send_instruction(0x0C); // Display ON, Cursor OFF, Blink OFF
  send_instruction(0x01); // Clear display
  send_instruction(0x06);
  send_instruction(0x02); // Return home

  stop_i2c_comunication();

  __delay_ms(5);
}

void set_cursor_position(int line, unsigned int index) {
  unsigned int position = 0;

  if (line == 1) {
    position = 0x00 + index;
  } else if (line == 2) {
    position = 0x40 + index;
  }

  start_i2c_comunication();

  send_instruction(0b10000000 | position); // Return home

  stop_i2c_comunication();
}

void shift_cursor_left() {
  start_i2c_comunication();

  send_instruction(0x38);       // function set
  send_instruction(0b10001111); // Return home
  send_instruction(0b00010000); // function set
  send_instruction(0b00010000); // function set
  send_instruction(0x39);       // function set

  stop_i2c_comunication();
}

void shift_cursor_right() {

  start_i2c_comunication();

  send_instruction(0x38);       // function set
  send_instruction(0b00010100); // function set
  send_instruction(0x39);       // function set

  stop_i2c_comunication();
}

void show_cursor() {
  start_i2c_comunication();

  send_instruction(0b00001110);

  stop_i2c_comunication();
}

void hide_cursor() {
  start_i2c_comunication();

  send_instruction(0b00001100);

  stop_i2c_comunication();
}

void lcd_show_string(char lineNum, char textData[], bool endWithNullByte) {
  unsigned char i;
  i = 0;

  start_i2c_comunication();

  if (lineNum == 1) {
    // set DDRAM addres to 0x00 (first line, first char)
    send_instruction(0x80);
  } else if (lineNum == 2) {
    // set DDRAM addres to 0x40 (second line, first char)
    send_instruction(0xC0);
  }

  // 0x40 = 0b01000000 -> Co = 0, RS = 1
  // This is the last chunk of this message, what follows will only be raw data
  // that will be passed to the DR (data register) and shown on the LCD display.
  //
  // After the data is sent a STOP signal will follow ending the message
  lcd_send(0x40);

  for (i = 0; i < 16; i++) {
    if (endWithNullByte && textData[i] == '\0') {
      break;
    }

    lcd_send(textData[i]);
  }

  stop_i2c_comunication();

  __delay_ms(40);
}

static void lcd_send(unsigned char data) {
  SSP2BUF = data;
  // Send data and wait until the LCD is ready to recieve again
  while (SSP2STATbits.BF)
    ;
  while (!SSP2IF)
    ;
  SSP2IF = 0;
}

void lcd_clear(void) {
  start_i2c_comunication();

  send_instruction(0x01); // Clear display

  stop_i2c_comunication();
}

void lcd_reset(void) {
  LATAbits.LATA0 = 0;
  __delay_us(100);
  LATAbits.LATA0 = 1;
}

void start_i2c_comunication(void) {
  // Send START signal
  SSP2CON2bits.SEN = 1;
  while (SSP2CON2bits.SEN)
    ;
  SSP2IF = 0;

  // LCD addr:  0111110 (0x3E)
  // LCD write: 01111100 (0x7C)
  // LSb = 0 of ADD sets slave to recieve mode (R/W bit in datasheet)
  lcd_send(0x7C);
}

void stop_i2c_comunication(void) {
  // Send STOP signal
  SSP2CON2bits.PEN = 1;
  while (SSP2CON2bits.PEN)
    ;
}

void send_instruction(unsigned char instruction) {
  lcd_send(0x80); // Control byte
  lcd_send(instruction);
}
