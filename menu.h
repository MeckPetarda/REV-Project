#ifndef MENU_H
#define MENU_H

/**
 * Structure to represent a subroutine with a label and function pointer
 */
struct Subroutine {
    char label[14];
    void (*subroutine)(void);
};

/**
 * Structure to maintain the menu state
 */
struct SMenuState {
    int selectedRow;
    int numberOfRows;
    int previousSelectedRow;
    struct Subroutine *activeSubroutine;
};

/**
 * Define types for the structures
 */
typedef struct Subroutine Subroutine;
typedef struct SMenuState SMenuState;

/**
 * External declarations for global variables
 */
extern volatile SMenuState menuState;
extern volatile Subroutine *subroutines[];

/**
 * Initialize the menu state
 */
void initMenu(void);

/**
 * Register a new subroutine in the menu
 * @param label The label for the menu item
 * @param subroutine Function pointer to the subroutine
 */
void registerSubroutine(char label[14], void (*subroutine)(void));

void returnToMenu(void);

/**
 * Confirm and set the selected subroutine as active
 */
void confirmSubroutine(void);

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

#endif /* MENU_H */
