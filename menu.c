#include "config.h"

#include "buttons.h"
#include "lcd.h"
#include "led.h"
#include "menu.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PROGRAM_LIMIT 5

volatile int numberOfRows = 0;
volatile int selectedRow = 0;
volatile int previousSelectedRow = 0;

volatile struct Program *activeProgram;

struct Program programs[PROGRAM_LIMIT]; // Array of actual structures

void initMenu(void) {
  numberOfRows = 0;
  selectedRow = 0;
  previousSelectedRow = -1;

  activeProgram = NULL;
}

void registerProgram(char label[14], void (*init)(void),
                     void (*destructor)(void), void (*main)(void),
                     void (*lp_interrupt)(void), void (*hp_interrupt)(void)) {
  if (numberOfRows >= PROGRAM_LIMIT)
    return;

  // Directly modify the permanent structure
  strncpy(programs[numberOfRows].label, label, 14);
  programs[numberOfRows].label[14] = '\0';

  programs[numberOfRows].init = init;
  programs[numberOfRows].main = main;
  programs[numberOfRows].destructor = destructor;
  programs[numberOfRows].lp_interrupt = lp_interrupt;
  programs[numberOfRows].hp_interrupt = hp_interrupt;

  numberOfRows++;
}

void returnToMenu() {
  selectedRow = 0;
  previousSelectedRow = -1;

  if (activeProgram != NULL && activeProgram->destructor != NULL) {
    activeProgram->destructor();
  }

  activeProgram = NULL;
}

void launchProgram(int index) {
  if (index >= 0 && index < numberOfRows) {
    activeProgram = &programs[index];
    activeProgram->init();
  }
}

void confirmProgram(void) { launchProgram(selectedRow); }

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

void menuProgram(void) {
  // Handle button presses
  if (button_states.btn1_re)
    previousRow();
  else if (button_states.btn2_re)
    nextRow();
  else if (button_states.btn3_re)
    confirmProgram();

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
    snprintf(line1, sizeof(line1), "> %s", programs[0].label);
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
             programs[topRow].label);

    // Only show second line if there's an item to display
    if (topRow + 1 < numberOfRows) {
      snprintf(line2, sizeof(line2), "%c %s",
               (topRow + 1 == selectedRow ? '>' : ' '),
               programs[topRow + 1].label);
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

void runProgram(void) {
  if (activeProgram == NULL) {
    menuProgram();
  } else {
    activeProgram->main();
  }
}
