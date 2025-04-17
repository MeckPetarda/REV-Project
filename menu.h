#ifndef MENU_H
#define MENU_H

/**
 * Structure to represent a subroutine with a label and function pointer
 */
struct Subroutine {
  char label[15];
  void (*init_subroutine)(void);
  void (*subroutine)(void);
  void (*lp_interrupt)(void);
  void (*hp_interrupt)(void);
};

volatile struct Subroutine *activeSubroutine;

/**
 * Initialize the menu state
 */
void initMenu(void);

/**
 * Register a new subroutine in the menu
 * @param label The label for the menu item
 * @param subroutine Function pointer to the subroutine
 * @param lp_interrupt Function pointer to the low priority interrupt
 * @param hp_interrupt Function pointer to the high priority interrupt
 */
void registerSubroutine(char label[14], void (*init_subroutine)(void),
                        void (*subroutine)(void), void (*lp_interrupt)(void),
                        void (*hp_interrupt)(void));

void returnToMenu(void);

/**
 * Confirm and set the selected subroutine as active
 */
void confirmSubroutine(void);

void launchSubroutine(int index);

/**
 * Move to the next menu row
 */
void nextRow(void);

/**
 * Move to the previous menu row
 */
void previousRow(void);

/**
 * Main menu handling subroutine
 */
void menuSubroutine(void);

void runSubroutine(void);

#endif /* MENU_H */
