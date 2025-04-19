#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>

//                                       CCPR1L CCP1CON<5:4>
// DC = 0     x = 0   = 0b00 0000 0000 -> 0x00      0b00
// DC = 0.25  x = 200 = 0b00 1100 1000 -> 0x32      0b00
// DC = 0.5   x = 400 = 0b01 1001 0000 -> 0x64      0b00
// DC = 0.75  x = 600 = 0b10 0101 1000 -> 0x96      0b00
// DC = 1     x = 800 = 0b11 0010 0000 -> 0xC8      0b00

#define NUMBER_OF_STATES 5

volatile int DCs[NUMBER_OF_STATES] = {0x00, 0x32, 0x64, 0x96, 0xC8};
volatile int index = 0;

static void update_screen() {
  char line1[17] = {0};

  int percent = index * 25;

  snprintf(line1, sizeof(line1), "POWER LEVEL %3d%c", percent, '%');
  lcd_show_string(1, line1, false);
}

static void init() {
  set_number_of_lines(DOUBLE_HEIGHT);

  index = 0;

  // init - PWM
  TRISDbits.RD6 = 1; // nastavim jako vstup pin P1B

  PSTR1CON |= 0b10; // steering na P1B
  PSTR1CON &= 0b10;

  CCP1CONbits.P1M = 0b00;     // PWM single
  CCP1CONbits.CCP1M = 0b1101; // PWM single

  CCPR1L = DCs[index];

  CCPTMRS0bits.C1TSEL = 0b01; // Timer 4
  PR4 = 199;                  // f = 10kHz
  T4CONbits.T4CKPS = 0b10;    // 1:4 Prescaler
  TMR4ON = 1;

  // Wait for te timer to overflow once
  TMR4IF = 0;
  while (!TMR4IF)
    ;

  TRISDbits.RD6 = 0; // nastavim jako vystup pin P1B

  update_screen();
}

static void destructor(void) {
  set_number_of_lines(TWO_ROWS);

  TMR4ON = 0;

  LED5 = 1;
}

static void main(void) {
  if (button_states.btn2_re) {
    if (index < NUMBER_OF_STATES - 1) {
      index++;
    } else {
      index = 0;
    }

    CCPR1L = DCs[index];
    CCP1CONbits.DC1B = 0b00;
    update_screen();

  } else if (button_states.btn4_re) {
    returnToMenu();
  }
}

void register_pwm_led(void) {
  registerProgram("PWM LED", &init, &destructor, &main, NULL, NULL);
}
