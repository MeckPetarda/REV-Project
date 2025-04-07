// REV INTERRUPT
#pragma config FOSC = HSMP // Oscillator Selection bits (HS oscillator (medium power 4-16 MHz))
#pragma config PLLCFG = ON // 4X PLL Enable (Oscillator multiplied by 4)
#pragma config WDTEN = OFF // Watchdog Timer Enable bits (Watch dog timer is  always disabled. SWDTEN has no effect.)

#define _XTAL_FREQ 32E6 // definice fosc pro knihovnu
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

#include <xc.h>
#include <stdbool.h>

#include "buttons.h"
#include "led.h"
#include "lcd.h"
#include "menu.h"

#include "sos.h"

void __interrupt(low_priority) T2_ISR_HANDLER(void) {
    buttons_interrupt();
}

void test(void) {}

void init(void) {
    buttons_init();
    led_init();

    
    initMenu();
    
    registerSubroutine("Menu", &menuSubroutine);
    confirmSubroutine();
    
    register_sos();
    
    lcd_init();
}

void main(void) {
    init();
    
    while (true) {
        if (menuState.activeSubroutine != NULL) {
            menuState.activeSubroutine->subroutine();          
        }
    }
}
