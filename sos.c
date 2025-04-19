#include "config.h"

#include "buttons.h"
#include "lcd.h"
#include "led.h"
#include "menu.h"
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

static void sos_init();
static void sos_main(void);
static void hp_interrupt();

void register_sos(void) {
  registerSubroutine("SOS", &sos_init, &sos_main, NULL, &hp_interrupt);
}

static void hp_interrupt() {
  if (TMR1IE && TMR1IF) {
    TMR1 = 0xFFFF - 1000;

    // Increment timer count
    if (ms < duration) {
      ms++;
      TMR1IF = 0;
      return;
    }

    // Reset timer count for next interval
    ms = 0;

    // Get number of dot/dash elements in current character
    int elementsInChar = message[index];

    // If we've finished processing all elements in the current character
    if (step > elementsInChar * 2) {
      // Reset step counter
      step = 1;
      // Move to the next character (skip count + all elements)
      index += elementsInChar + 1;

      // If we've reached the end of the message, loop back to start
      if (index >= sizeof(message) / sizeof(message[0])) {
        index = 0;
        // Add extra pause between message repetitions
        duration = PBW;
      } else {
        // Pause between letters
        duration = PBL;
      }
    } else {
      // Toggle LED for beeps (odd steps) and set duration
      if (step % 2 == 1) {
        drive_led(0b000000); // Turn on LED for beep
        // Get duration from message array (offset by 1 to skip count)
        duration = message[index + (step + 1) / 2];
      } else {
        drive_led(0b111111); // Turn off LED for pause
        duration = PBB;      // Standard pause between beeps
      }

      // Move to next step
      step++;
    }

    TMR1IF = 0;
  }
}

static void sos_init() {
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

static void sos_main(void) {
  if (button_states.btn4_re) {
    TMR1ON = 0;
    TMR1IF = 0;
    TMR1IE = 0;
    drive_led(0b111111);
    returnToMenu();
  }
}
