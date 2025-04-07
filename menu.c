#include <xc.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "buttons.h"
#include "led.h"
#include "lcd.h"
#include "menu.h"

#define SUBROUTINE_LIMIT 5
#define _XTAL_FREQ 32E6

volatile SMenuState menuState;
Subroutine subroutineEntries[SUBROUTINE_LIMIT];  // Array of actual structures
volatile Subroutine *subroutines[SUBROUTINE_LIMIT];  // Array of pointers


void initMenu(void) {
    menuState.numberOfRows = 0;
    menuState.selectedRow = 0;
    menuState.previousSelectedRow = -1;
    menuState.activeSubroutine = NULL;
}

void registerSubroutine(char label[14], void (*subroutine)(void)) {
    if (menuState.numberOfRows >= SUBROUTINE_LIMIT) return;
    
    // Directly modify the permanent structure
    strncpy(subroutineEntries[menuState.numberOfRows].label, label, 13);
    subroutineEntries[menuState.numberOfRows].label[13] = '\0';
    subroutineEntries[menuState.numberOfRows].subroutine = subroutine;
    
    // Store pointer to the permanent structure
    subroutines[menuState.numberOfRows] = &subroutineEntries[menuState.numberOfRows];
    
    menuState.numberOfRows++;
}

void returnToMenu() {
    menuState.selectedRow = 0;
    menuState.previousSelectedRow = -1;
    
    confirmSubroutine();
}

void confirmSubroutine(void) {
    if (menuState.numberOfRows == 0) return;
    
    menuState.activeSubroutine = subroutines[menuState.selectedRow];
}

void nextRow(void) {
    menuState.selectedRow++;
}

void previousRow(void) {
    menuState.selectedRow--;
}

volatile char text[32] = "";             // retezec zatim prazdny

void menuSubroutine(void) {
    drive_led(button_states.btn1_state);
        
    if (button_states.btn1_re) previousRow();
    else if (button_states.btn2_re) nextRow();
    else if (button_states.btn3_re) confirmSubroutine();
    
    if (menuState.selectedRow == menuState.previousSelectedRow) return;
    
    sprintf(text, "ADC = %d : %d             ", menuState.selectedRow, menuState.numberOfRows);
    
    __delay_ms(40);
    lcd_show_string(1, text);
    __delay_ms(40);
    sprintf(text, "> %s                 ", subroutines[menuState.selectedRow]->label);
    
    
    lcd_show_string(2, text);
    
    menuState.previousSelectedRow = menuState.selectedRow;
}