#include "../config.h"

#include "../buttons.h"
#include "../lcd.h"
#include "../led.h"
#include "../menu.h"
#include <stdbool.h>
#include <stdio.h>

// in ms
// Pause between beeps
#define PBB 250
// Pause between letters
#define PBL 500
// Pause between words
#define PBW 2000

#define DASH 400
#define DOT 100

#define S 3, DOT, DOT, DOT
#define O 3, DASH, DASH, DASH

volatile int message[] = {S, O, S};

volatile int ms = 0;
volatile int duration = 1000;
volatile int index = 0;
volatile int step = 0;

static void hp_interrupt() {
  if (TMR1IE && TMR1IF) {
    TMR1 = 0xFFFF - 1000;

    if (ms < duration) {
      ms++;
      TMR1IF = 0;
      return;
    }

    ms = 0;

    int elementsInChar = message[index];

    if (step > elementsInChar * 2) {
      step = 1;
      index += elementsInChar + 1;

      if (index >= sizeof(message) / sizeof(message[0])) {
        index = 0;
        duration = PBW;
      } else {
        duration = PBL;
      }
    } else {
      if (step % 2 == 1) {
        drive_led(0b000000);
        duration = message[index + (step + 1) / 2];
      } else {
        drive_led(0b111111);
        duration = PBB;
      }

      step++;
    }

    TMR1IF = 0;
  }
}

static void init() {
  T1CONbits.TMR1CS = 0b00; // zdroj casovace Fosc/4
  T1CONbits.T1CKPS = 0b11; // 1:16
  TMR1 = 0;
  TMR1ON = 1;
  TMR1IE = 1;
  TMR1IP = 1;

  index = 0;
  step = 0;

  lcd_show_string(1, "SOS SOS SOS SOS SOS", false);
}

static void destructor(void) {
  TMR1ON = 0;
  TMR1IF = 0;
  TMR1IE = 0;
  drive_led(0b111111);
}

static void main(void) {
  if (button_states.btn4_re) {
    returnToMenu();
  }
}

void register_sos(void) {
  registerProgram("SOS", &init, &destructor, &main, NULL, &hp_interrupt);
}
