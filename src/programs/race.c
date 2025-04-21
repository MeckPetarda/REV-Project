#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>

volatile int ms = 0;
volatile int duration = 1000;
volatile int countdown = 3;

volatile long pot1 = 0;

enum State { COUNTDOWN, GAME, LOSS };
enum Position { TOP, BOTTOM };

char line1[17] = {0};
char line2[17] = {0};

volatile enum State state;
volatile enum Position position;

static void update_screen() {
  switch (position) {
  case TOP:
    write_char(1, 0, '>');
    write_char(2, 0, ' ');

    break;
  case BOTTOM:
    write_char(1, 0, ' ');
    write_char(2, 0, '>');

    break;
  }
}

static void hp_interrupt() {
  if (TMR1IE && TMR1IF) {
    TMR1 = 0xFFFF - 1000;

    if (ms < duration) {
      ms++;
      TMR1IF = 0;
      return;
    }

    if (state == COUNTDOWN) {
      if (countdown == 3) {
        lcd_show_string(1, " Starting in    ", false);
      }
      if (countdown > 0) {
        write_char(1, 13, '0' + countdown);
      } else if (countdown == 0) {
        snprintf(line1, sizeof(line1), "    !! GO !!    ");
        lcd_show_string(1, line1, false);
      } else {
        state = GAME;
        GODONE = 0;

        set_number_of_lines(TWO_ROWS);
        lcd_clear();

        update_screen();
      }

      countdown--;

    } else if (state == GAME) {
      update_screen();
    }

    ms = 0;

    LED1 ^= 1;

    TMR1IF = 0;
  }
}

static void lp_interrupt() {
  if (ADIE && ADIF) {
    LED2 ^= 1;

    enum Position newPosition;

    pot1 = (ADRESH << 2) + (ADRESL >> 6);

    if (pot1 < 501) {
      newPosition = BOTTOM;
    } else if (pot1 > 530) {
      newPosition = TOP;
    }

    if (position != newPosition) {
      position = newPosition;

      update_screen();
    }

    ADIF = 0;
  }
}

static void init() {
  set_number_of_lines(DOUBLE_HEIGHT);

  T1CONbits.TMR1CS = 0b00; // zdroj casovace Fosc/4
  T1CONbits.T1CKPS = 0b11; // 1:16
  TMR1 = 0xFFFF;
  TMR1ON = 1;
  TMR1IE = 1;
  TMR1IP = 1;

  ANSELAbits.ANSA5 = 0;

  ADIE = 1; // Enable A/D interrupt
  ADIP = 0; // A/D hight priority
  ADIF = 0;
  GODONE = 1;

  ADCON2bits.ADFM = 0;     // left justified
  ADCON2bits.ADCS = 0b110; // Fosc/64
  ADCON2bits.ACQT = 0b110; // 16 Tad
  ADCON0bits.ADON = 1;     // ADC zapnout
  ADCON0bits.CHS = 5;      // kanal AN5

  ms = 1000;
  state = COUNTDOWN;
  position = TOP;
  countdown = 3;
  duration = 1000;

  lcd_show_string(1, "                ", false);
}

static void destructor(void) {
  set_number_of_lines(TWO_ROWS);

  TMR1ON = 0;
  TMR1IF = 0;
  TMR1IE = 0;
  drive_led(0b111111);
}

static void main(void) {
  if (state == GAME && GODONE == 0) {
    GODONE = 1;
  }
}

void register_race(void) {
  registerProgram("RACE", &init, &destructor, &main, &lp_interrupt,
                  &hp_interrupt);
}
