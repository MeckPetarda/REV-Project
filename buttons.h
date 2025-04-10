#ifndef BUTTONS_H
#define BUTTONS_H

// Structure to hold button states
typedef struct {
    char btn1_re;      // Rising edge detection
    char btn2_re;
    char btn3_re;
    char btn4_re;

    char btn1_fe;      // Falling edge detection
    char btn2_fe;
    char btn3_fe;
    char btn4_fe;

    char btn1_he;      // Held state detection
    char btn2_he;
    char btn3_he;
    char btn4_he;

    char btn1_state;   // Current shift register state
    char btn2_state;
    char btn3_state;
    char btn4_state;
} button_states_t;

// External declaration of shared button states
extern volatile button_states_t button_states;

// Function prototypes
void buttons_interrupt(void);
void buttons_init(void);

#endif  // BUTTONS_H
