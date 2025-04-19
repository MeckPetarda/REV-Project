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

volatile int numberOfRows = 0;
volatile int selectedRow = 0;
volatile int previousSelectedRow = 0;

volatile struct Subroutine *activeSubroutine;

struct Subroutine subroutines[SUBROUTINE_LIMIT]; // Array of actual structures

void initMenu(void) {
  numberOfRows = 0;
  selectedRow = 0;
  previousSelectedRow = -1;

  activeSubroutine = NULL;
}

void registerSubroutine(char label[14], void (*init_subroutine)(void),
                        void (*subroutine)(void), void (*lp_interrupt)(void),
                        void (*hp_interrupt)(void)) {
  if (numberOfRows >= SUBROUTINE_LIMIT)
    return;

  // Directly modify the permanent structure
  strncpy(subroutines[numberOfRows].label, label, 14);
  subroutines[numberOfRows].label[14] = '\0';

  subroutines[numberOfRows].init_subroutine = init_subroutine;
  subroutines[numberOfRows].subroutine = subroutine;
  subroutines[numberOfRows].lp_interrupt = lp_interrupt;
  subroutines[numberOfRows].hp_interrupt = hp_interrupt;

  numberOfRows++;
}

void returnToMenu() {
  selectedRow = 0;
  previousSelectedRow = -1;

  activeSubroutine = NULL;
}

void launchSubroutine(int index) {
  if (index >= 0 && index < numberOfRows) {
    activeSubroutine = &subroutines[index];
    activeSubroutine->init_subroutine();
  }
}

void confirmSubroutine(void) { launchSubroutine(selectedRow); }

void nextRow(void) {
  if (selectedRow + 1 < numberOfRows) {
    selectedRow++;
  }
}

void previousRow(void) {
  if (selectedRow - 1 >= 0) {
    selectedRow--;
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
  if (selectedRow == previousSelectedRow)
    return;

  char line1[18] = {0}; // Buffer for first LCD line
  char line2[18] = {0}; // Buffer for second LCD line

  // Handle different display scenarios based on number of items and selection
  if (numberOfRows == 0) {
    // No items registeredg
    snprintf(line1, sizeof(line1), "No menu items");
    line2[0] = '\0'; // Empty string
  } else if (numberOfRows == 1) {
    // Only one item
    snprintf(line1, sizeof(line1), "> %s", subroutines[0].label);
    line2[0] = '\0'; // Empty string
  } else {
    // Multiple items - determine which to show
    int topRow = selectedRow;
    // If at last row, show previous item on top
    if (topRow == numberOfRows - 1) {
      topRow--;
    }

    // Format lines with selection indicator
    snprintf(line1, sizeof(line1), "%c %s", (topRow == selectedRow ? '>' : ' '),
             subroutines[topRow].label);

    // Only show second line if there's an item to display
    if (topRow + 1 < numberOfRows) {
      snprintf(line2, sizeof(line2), "%c %s",
               (topRow + 1 == selectedRow ? '>' : ' '),
               subroutines[topRow + 1].label);
    } else {
      line2[0] = '\0'; // Empty string
    }
  }

  line1[17] = '\0';
  line2[17] = '\0';

  // Update the display
  lcd_show_string(1, line1, false);

  // Only update second line if we have content or need to clear it
  if (line2[0] != '\0' || previousSelectedRow != -1) {
    lcd_show_string(2, line2, false);
  }

  previousSelectedRow = selectedRow;
}

void runSubroutine(void) {
  if (activeSubroutine == NULL) {
    menuSubroutine();
  } else {
    activeSubroutine->subroutine();
  }
}
