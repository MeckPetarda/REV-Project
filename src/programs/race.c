#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

volatile int ms = 0;
volatile int duration = 1000;
volatile int countdown = 3;

volatile int lives = 3;
volatile int score = 0;
volatile int gameOverTimer = 0;

volatile long pot1 = 0;

enum State { COUNTDOWN, GAME, GAME_OVER };
enum Position { TOP, BOTTOM };

// Obstacle array - each element represents a column position
// 0 = no obstacle, 1 = obstacle on top row, 2 = obstacle on bottom row
volatile char obstacles[16] = {0};
volatile char lastObstacleRow = 0; // Track last obstacle position for fairness
volatile char needsFreeSpace = 0; // Flag to enforce free space before row switch

char line1[17] = {0};
char line2[17] = {0};

volatile enum State state;
volatile enum Position position;

static void update_lives_display() {
    char led_pattern = 0b111111;
    
    if (lives >= 1) led_pattern &= ~0b000001; // Turn on LED1
    if (lives >= 2) led_pattern &= ~0b000010; // Turn on LED2
    if (lives >= 3) led_pattern &= ~0b000100; // Turn on LED3
    
    drive_led(led_pattern);
}

static void shift_obstacles() {
    // Shift all obstacles one position to the left
    for (int i = 0; i < 15; i++) {
        obstacles[i] = obstacles[i + 1];
    }
    obstacles[15] = 0; // Clear the rightmost position
}

static void generate_obstacle() {
    // If we need to provide free space for row switching, do it now
    if (needsFreeSpace) {
        obstacles[15] = 0; // Mandatory free space
        needsFreeSpace = 0;
        lastObstacleRow = 0;
        return;
    }
    
    // Generate obstacle with high frequency but fair rules
    int obstacle_chance = rand() % 10; // 0-9 for fine-tuned probability
    
    if (obstacle_chance < 2) {
        // 20% chance: No obstacle
        obstacles[15] = 0;
        lastObstacleRow = 0;
    } else if (obstacle_chance < 6) {
        // 40% chance: Top row obstacle
        if (lastObstacleRow == 2) {
            // Last was bottom, need free space before switching to top
            needsFreeSpace = 1;
            obstacles[15] = 0;
            lastObstacleRow = 0;
        } else {
            obstacles[15] = 1;
            lastObstacleRow = 1;
        }
    } else {
        // 40% chance: Bottom row obstacle  
        if (lastObstacleRow == 1) {
            // Last was top, need free space before switching to bottom
            needsFreeSpace = 1;
            obstacles[15] = 0;
            lastObstacleRow = 0;
        } else {
            obstacles[15] = 2;
            lastObstacleRow = 2;
        }
    }
}

static void check_collision() {
    // Check if car (at position 0) collides with obstacle
    if (obstacles[0] != 0) {
        // There's an obstacle at position 0
        if ((position == TOP && obstacles[0] == 1) || 
            (position == BOTTOM && obstacles[0] == 2)) {
            // Collision detected!
            lives--;
            obstacles[0] = 0; // Remove the obstacle
            update_lives_display();
            
            if (lives <= 0) {
                state = GAME_OVER;
                gameOverTimer = 0;
                duration = 100; // Faster timer for game over display
            }
        }
    }
}

static void update_game_screen() {
    // Clear the lines
    for (int i = 0; i < 16; i++) {
        line1[i] = ' ';
        line2[i] = ' ';
    }
    line1[16] = '\0';
    line2[16] = '\0';
    
    // Draw the car (always at position 0)
    if (position == TOP) {
        line1[0] = '>';
        line2[0] = ' ';
    } else {
        line1[0] = ' ';
        line2[0] = '>';
    }
    
    // Draw obstacles
    for (int i = 0; i < 16; i++) {
        if (obstacles[i] == 1) {
            line1[i] = '#'; // Obstacle on top row
        } else if (obstacles[i] == 2) {
            line2[i] = '#'; // Obstacle on bottom row
        }
    }
    
    // Make sure car is always visible (override obstacle at position 0)
    if (position == TOP) {
        line1[0] = '>';
    } else {
        line2[0] = '>';
    }
    
    lcd_show_string(1, line1, false);
    lcd_show_string(2, line2, false);
}

static void display_game_over() {
    char score_line[17] = {0};
    
    lcd_show_string(1, "   GAME OVER    ", false);
    snprintf(score_line, sizeof(score_line), "Score: %d", score);
    lcd_show_string(2, score_line, false);
}

static void hp_interrupt() {
    if (TMR1IE && TMR1IF) {
        TMR1 = 0xFFFF - 1000;

        if (ms < duration) {
            ms++;
            TMR1IF = 0;
            return;
        }

        ms = 0;

        if (state == COUNTDOWN) {
            if (countdown == 3) {
                lcd_show_string(1, " Starting in    ", false);
            }
            if (countdown > 0) {
                write_char(1, 13, (char)('0' + countdown));
            } else if (countdown == 0) {
                snprintf(line1, sizeof(line1), "    !! GO !!    ");
                lcd_show_string(1, line1, false);
            } else {
                state = GAME;
                duration = 300; // Game speed - obstacles move every 300ms
                
                set_number_of_lines(TWO_ROWS);
                lcd_clear();
                
                // Initialize game state
                score = 0;
                lives = 3;
                for (int i = 0; i < 16; i++) {
                    obstacles[i] = 0;
                }
                update_lives_display();
                update_game_screen();
            }
            
            countdown--;

        } else if (state == GAME) {
            // Move obstacles and generate new ones
            shift_obstacles();
            generate_obstacle();
            score++; // Increase score (distance traveled)
            
            // Check for collisions
            check_collision();
            
            // Update display
            update_game_screen();
            
        } else if (state == GAME_OVER) {
            gameOverTimer++;
            
            if (gameOverTimer == 1) {
                // First time entering game over state
                drive_led(0b111111); // Turn off all LEDs (active low)
                display_game_over();
            } else if (gameOverTimer >= 20) { // 2 seconds at 100ms intervals
                // Return to menu after 2 seconds
                returnToMenu();
            }
        }

        TMR1IF = 0;
    }
}

static void lp_interrupt() {
    if (ADIE && ADIF) {
        enum Position newPosition;

        pot1 = (ADRESH << 2) + (ADRESL >> 6);

        if (pot1 < 501) {
            newPosition = BOTTOM;
        } else if (pot1 > 530) {
            newPosition = TOP;
        } else {
            newPosition = position; // Keep current position in dead zone
        }

        if (position != newPosition && state == GAME) {
            position = newPosition;
            
            // Check for collision immediately after player movement
            check_collision();
            
            // Only update screen if still alive (collision might have changed game state)
            if (state == GAME) {
                update_game_screen();
            }
        }

        ADIF = 0;
    }
}

static void init() {
    set_number_of_lines(DOUBLE_HEIGHT);

    // Timer setup
    T1CONbits.TMR1CS = 0b00; // clock source Fosc/4
    T1CONbits.T1CKPS = 0b11; // 1:16 prescaler
    TMR1 = 0xFFFF;
    TMR1ON = 1;
    TMR1IE = 1;
    TMR1IP = 1;

    // ADC setup for POT2
    ANSELAbits.ANSA5 = 0; // Set as analog input

    ADIE = 1; // Enable A/D interrupt
    ADIP = 0; // A/D low priority
    ADIF = 0;

    ADCON2bits.ADFM = 0;     // left justified
    ADCON2bits.ADCS = 0b110; // Fosc/64
    ADCON2bits.ACQT = 0b110; // 16 Tad
    ADCON0bits.ADON = 1;     // ADC on
    ADCON0bits.CHS = 5;      // channel AN5 (POT2)

    // Initialize random number generator
    srand(526);
    
    // Initialize game state
    ms = 1000;
    state = COUNTDOWN;
    position = TOP;
    countdown = 3;
    duration = 1000;
    lives = 3;
    score = 0;
    gameOverTimer = 0;
    lastObstacleRow = 0;
    needsFreeSpace = 0;
    
    // Clear obstacles
    for (int i = 0; i < 16; i++) {
        obstacles[i] = 0;
    }

    lcd_show_string(1, "                ", false);
    
    // Start ADC conversion
    GODONE = 1;
}

static void destructor(void) {
    set_number_of_lines(TWO_ROWS);

    // Stop timer
    TMR1ON = 0;
    TMR1IF = 0;
    TMR1IE = 0;

    // Stop ADC
    ADIE = 0;
    ADIF = 0;
    ADCON0bits.ADON = 0;

    // Reset analog configuration
    ANSELAbits.ANSA5 = 1;

    // Turn on all LEDs (default state)
    drive_led(0b111111);
}

static void main(void) {
    if (state == GAME && GODONE == 0) {
        GODONE = 1; // Keep ADC running during game
    }
    
    // Button 4 returns to menu (only during countdown or game over)
    if ((state == COUNTDOWN || state == GAME_OVER) && button_states.btn4_re) {
        returnToMenu();
    }
}

void register_race(void) {
    registerProgram("RACE", &init, &destructor, &main, &lp_interrupt, &hp_interrupt);
}
