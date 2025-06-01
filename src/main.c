#include "config.h"
/**
    257640
    0:GPIO-Blikani SOS
    1:UART-Vypis zpravy na displej
    2:PWM-BTN2 p?epíná jas LED mezi p?ti úrovni
    3:ADC-Vypis pot1 a pot2 na displej ve V
    4:DAC-Orezana sinusovka
    5:GAME-Závody POT
    6:Prehravac hudby ? pwm
**/

#include "per/buttons.h"
#include "per/lcd.h"
#include "per/led.h"
#include <stdbool.h>

#include "menu.h"
#include "programs/pot.h"
#include "programs/pwm_led.h"
#include "programs/race.h"
#include "programs/sos.h"
#include "programs/uart.h"
#include "programs/dac.h"
#include "programs/hw.h"

void __interrupt(low_priority) LP_ISR_HANDLER(void) {
  if (activeProgram != NULL && activeProgram->lp_interrupt != NULL) {
    activeProgram->lp_interrupt();
  }
}

void __interrupt(high_priority) HP_ISR_HANDLER(void) {
  buttons_interrupt();

  if (activeProgram != NULL && activeProgram->hp_interrupt != NULL) {
    activeProgram->hp_interrupt();
  }
}

void init(void) {
  PEIE = 1; // povoleni preruseni od periferii
  GIE = 1;  // globalni povoleni preruseni
  IPEN = 1;

  buttons_init();
  led_init();

  initMenu();

  register_sos();
  register_uart();
  register_pwm_led();
  register_pot();
  register_dac();
  register_race();
  register_hw();

  lcd_init();
}

void main(void) {
  init();

  while (true) {
    runProgram();
  }
}
