#include "config.h"

#include "buttons.h"
#include "lcd.h"
#include "led.h"
#include "menu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SUBROUTINE_LIMIT 5

volatile SMenuState menuState;
Subroutine subroutineEntries[SUBROUTINE_LIMIT]; // Array of actual structures
volatile Subroutine *subroutines[SUBROUTINE_LIMIT]; // Array of pointers

void initMenu(void) {
  menuState.numberOfRows = 0;
  menuState.selectedRow = 0;
  menuState.previousSelectedRow = -1;

  menuState.activeSubroutine = NULL;
}

void registerSubroutine(char label[14], void (*init_subroutine)(void),
                        void (*subroutine)(void), void (*lp_interrupt)(void),
                        void (*hp_interrupt)(void)) {
  if (menuState.numberOfRows >= SUBROUTINE_LIMIT)
    return;

  // Directly modify the permanent structure
  strncpy(subroutineEntries[menuState.numberOfRows].label, label, 14);
  subroutineEntries[menuState.numberOfRows].label[14] = '\0';

  subroutineEntries[menuState.numberOfRows].init_subroutine = init_subroutine;
  subroutineEntries[menuState.numberOfRows].subroutine = subroutine;
  subroutineEntries[menuState.numberOfRows].lp_interrupt = lp_interrupt;
  subroutineEntries[menuState.numberOfRows].hp_interrupt = hp_interrupt;

  // Store pointer to the permanent structure
  subroutines[menuState.numberOfRows] =
      &subroutineEntries[menuState.numberOfRows];

  menuState.numberOfRows++;
}

void returnToMenu() {
  menuState.selectedRow = 0;
  menuState.previousSelectedRow = -1;

  menuState.activeSubroutine = NULL;
}

void launchSubroutine(int index) {
  if (index >= 0 && index < menuState.numberOfRows) {
    menuState.activeSubroutine = subroutines[index];
    menuState.activeSubroutine->init_subroutine();
  }
}

void confirmSubroutine(void) { launchSubroutine(menuState.selectedRow); }

void nextRow(void) {
  if (menuState.selectedRow + 1 < menuState.numberOfRows) {
    menuState.selectedRow++;
  }
}

void previousRow(void) {
  if (menuState.selectedRow - 1 >= 0) {
    menuState.selectedRow--;
  }
}

void menuSubroutine(void) {
  // Handle button presses
  if (button_states.btn1_re)
    previousRow();
  else if (button_states.btn2_re)
    nextRow();
  else if (button_states.btn3_re)
    confirmSubroutine();

  // Only update the display if the selection has changed
  if (menuState.selectedRow == menuState.previousSelectedRow)
    return;

  char line1[18] = {0}; // Buffer for first LCD line
  char line2[18] = {0}; // Buffer for second LCD line

  // Handle different display scenarios based on number of items and selection
  if (menuState.numberOfRows == 0) {
    // No items registered
    snprintf(line1, sizeof(line1), "No menu items");
    line2[0] = '\0'; // Empty string
  } else if (menuState.numberOfRows == 1) {
    // Only one item
    snprintf(line1, sizeof(line1), "> %s", subroutines[0]->label);
    line2[0] = '\0'; // Empty string
  } else {
    // Multiple items - determine which to show
    int topRow = menuState.selectedRow;
    // If at last row, show previous item on top
    if (topRow == menuState.numberOfRows - 1) {
      topRow--;
    }

    // Format lines with selection indicator
    snprintf(line1, sizeof(line1), "%c %s",
             (topRow == menuState.selectedRow ? '>' : ' '),
             subroutines[topRow]->label);

    // Only show second line if there's an item to display
    if (topRow + 1 < menuState.numberOfRows) {
      snprintf(line2, sizeof(line2), "%c %s",
               (topRow + 1 == menuState.selectedRow ? '>' : ' '),
               subroutines[topRow + 1]->label);
    } else {
      line2[0] = '\0'; // Empty string
    }
  }

  line1[17] = '\0';
  line2[17] = '\0';

  // Update the display
  lcd_show_string(1, line1);

  // Only update second line if we have content or need to clear it
  if (line2[0] != '\0' || menuState.previousSelectedRow != -1) {
    lcd_show_string(2, line2);
  }

  menuState.previousSelectedRow = menuState.selectedRow;
}

void runSubroutine(void) {
  if (menuState.activeSubroutine == NULL) {
    menuSubroutine();
  } else {
    menuState.activeSubroutine->subroutine();
  }
}
