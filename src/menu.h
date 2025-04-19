#ifndef MENU_H
#define MENU_H

/**
 * Structure to represent a program with a label and function pointer
 */
struct Program {
  char label[15];
  void (*init)(void);
  void (*destructor)(void);
  void (*main)(void);
  void (*lp_interrupt)(void);
  void (*hp_interrupt)(void);
};

volatile struct Program *activeProgram;

/**
 * Initialize the menu state
 */
void initMenu(void);

/**
 * Register a new program in the menu
 * @param label The label for the menu item
 * @param program Function pointer to the program
 * @param init Function pointer to the program init
 * @param destructor Function pointer to the program destructor
 * @param lp_interrupt Function pointer to the low priority interrupt
 * @param hp_interrupt Function pointer to the high priority interrupt
 */
void registerProgram(char label[14], void (*init)(void),
                     void (*destructor)(void), void (*main)(void),
                     void (*lp_interrupt)(void), void (*hp_interrupt)(void));

void returnToMenu(void);

/**
 * Confirm and set the selected program as active
 */
void confirmProgram(void);

void launchProgram(int index);

/**
 * Move to the next menu row
 */
void nextRow(void);

/**
 * Move to the previous menu row
 */
void previousRow(void);

/**
 * Main menu handling program
 */
void menuProgram(void);

void runProgram(void);

#endif /* MENU_H */
