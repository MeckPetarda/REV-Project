#include <xc.h>

#include "lcd.h"
#include "sos.h"
#include "menu.h"
#include "buttons.h"
#include <stdbool.h>

#define _XTAL_FREQ 32E6

void register_sos(void) {
    registerSubroutine("SOS", &sos_main);
}

void sos_init() {}

void sos_main(void) {
    sos_init();
    
    __delay_ms(40);
    lcd_show_string(1, "SOS SOS SOS SOS SOS");
    
    while (true) {
        if (button_states.btn4_re) {
            returnToMenu();
            break;
        }
    }
    
    
}