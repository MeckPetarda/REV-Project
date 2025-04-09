#include "config.h"

#include "lcd.h"
#include "sos.h"
#include "menu.h"
#include "buttons.h"
#include "led.h"
#include <stdbool.h>

// in ms
#define PBS 150
#define PBL 300
#define PBW 100

#define DASH 300
#define DOT 100

#define S 3, DOT, DOT, DOT
#define O 3, DASH, DASH, DASH

volatile int message[] = {S, O, S};

void sos_interrupt(void) {}

void register_sos(void) {
    registerSubroutine("SOS", &sos_init, &sos_main, NULL, &hp_interrupt);
}

static void hp_interrupt() {
    if (TMR1IE && TMR1IF) {
        LED1 ^= 1;
        
        TMR1 = 0;
        TMR1IF = 0;
    }
}

void sos_init() {
    T1CONbits.TMR1CS = 0b00;        // zdroj casovace Fosc/4
    T1CONbits.T1CKPS = 0b11;        // nastaveni delicky
    TMR1 = 0;
    TMR1ON = 1;                     // spusteni TMR1
    TMR1IE = 1;                     // povoleni preruseni pro TMR1
    TMR1IP = 1;
    
    lcd_show_string(1, "SOS SOS SOS SOS SOS");
    __delay_ms(40);
}

void sos_main(void) {
    if (button_states.btn4_re) {
        TMR1ON = 0;
        TMR1IF = 0;
        TMR1IE = 0;
        returnToMenu();
    }
}