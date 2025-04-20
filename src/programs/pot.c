#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>

volatile long pot1 = 0;
volatile long pot2 = 0;

static void update_screen() {
  char line1[17] = {0};
  char line2[17] = {0};

  snprintf(line1, sizeof(line1), "POT1 %4ld", pot1);
  lcd_show_string(1, line1, false);

  snprintf(line2, sizeof(line2), "POT2 %4ld", pot2);
  lcd_show_string(2, line2, false);
}

static void lp_interrupt() {
  if (ADIE && ADIF) {

    long tmp = (ADRESH << 2) + (ADRESL >> 6);

    if (ADCON0bits.CHS == 4) {
      ADCON0bits.CHS = 5;

      if (tmp != pot2) {
        pot2 = tmp;
        update_screen();
      }
    } else if (ADCON0bits.CHS == 5) {
      ADCON0bits.CHS = 4;

      if (tmp != pot1) {
        pot1 = tmp;
        update_screen();
      }
    }

    ADIF = 0;
  }
}

// RA5/AN4 - POT1
// RE0/AN5 - POT2

static void init() {
  ANSELAbits.ANSA5 = 0;
  ANSELEbits.ANSE0 = 0;

  ADIE = 1; // Enable A/D interrupt
  ADIP = 0; // A/D hight priority
  ADIF = 0;

  pot1 = -1;
  pot2 = -1;

  ADCON2bits.ADFM = 0;     // left justified
  ADCON2bits.ADCS = 0b110; // Fosc/64
  ADCON2bits.ACQT = 0b110; // 16 Tad
  ADCON0bits.ADON = 1;     // ADC zapnout
  ADCON0bits.CHS = 5;      // kanal AN5
}

static void destructor(void) {
  ADIE = 0;
  ADIF = 0;

  ANSELAbits.ANSA5 = 1;
  ANSELEbits.ANSE0 = 1;
}

static void main(void) {
  if (button_states.btn4_re || button_states.btn4_he) {
    returnToMenu();
  }

  GODONE = 1;
}

void register_pot(void) {
  registerProgram("POT", &init, &destructor, &main, &lp_interrupt, NULL);
}
