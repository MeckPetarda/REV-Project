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

#include "buttons.h"
#include "lcd.h"
#include "led.h"
#include <stdbool.h>

#include "menu.h"
#include "sos.h"
#include "uart.h"

void __interrupt(low_priority) LP_ISR_HANDLER(void) {
  buttons_interrupt();

  if (activeSubroutine != NULL && activeSubroutine->lp_interrupt != NULL) {
    activeSubroutine->lp_interrupt();
  }
}

void __interrupt(high_priority) HP_ISR_HANDLER(void) {
  if (activeSubroutine != NULL && activeSubroutine->hp_interrupt != NULL) {
    activeSubroutine->hp_interrupt();
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

  lcd_init();
}

void main(void) {
  init();

  while (true) {
    runSubroutine();
  }
}
