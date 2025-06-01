#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>
#include "notes.h"

volatile unsigned int hw_ms = 0;
volatile unsigned int hw_duration = 0;
volatile unsigned int hw_noteIndex = 0;
volatile int currentSong = 0; // 0 = melody1, 1 = melody2
volatile int numberOfNotes1;
volatile int numberOfNotes2;
volatile int *currentMelody;
volatile int currentNumberOfNotes;
volatile int wholenote = (60000 * 4) / 280; // Tempo for song timing

static void update_screen() {
    char line1[17] = {0};
    char line2[17] = {0};
    
    snprintf(line1, sizeof(line1), "Song: %d Note:%3d", currentSong + 1, hw_noteIndex + 1);
    snprintf(line2, sizeof(line2), "Freq: %4d Hz", 
             (currentMelody[2 * hw_noteIndex] == REST) ? 0 : currentMelody[2 * hw_noteIndex] * 2);
    
    lcd_show_string(1, line1, false);
    lcd_show_string(2, line2, false);
}

static void set_pwm_frequency(int frequency) {
    if (frequency == REST || frequency == 0) {
        // Turn off PWM for rest notes
        CCP2CONbits.CCP2M = 0b0000; // PWM mode off
        return;
    }
    
    // Double the frequency as required by assignment
    frequency *= 2;
    
    // Calculate PR4 for desired frequency
    // PWM frequency = FOSC / (4 * (PR4 + 1) * TMR4_prescaler)
    // Rearranged: PR4 = (FOSC / (4 * frequency * prescaler)) - 1
    
    // Use prescaler 1:4 for better frequency range
    T4CONbits.T4CKPS = 0b01; // 1:4 prescaler
    int prescaler = 4;
    
    // Calculate PR4
    int pr2_calc = (_XTAL_FREQ / (4 * frequency * prescaler)) - 1;
    
    // Clamp PR4 to valid range (0-255)
    if (pr2_calc > 255) {
        pr2_calc = 255;
    } else if (pr2_calc < 1) {
        pr2_calc = 1;
    }
    
    PR4 = pr2_calc;
    
    // Set duty cycle to 50%
    int duty_cycle = (pr2_calc + 1) * 2; // 50% duty cycle
    CCPR2L = duty_cycle >> 2;
    CCP2CONbits.DC2B = duty_cycle & 0b11;
    
    // Enable PWM mode
    CCP2CONbits.CCP2M = 0b1100; // PWM mode
}

static void switch_song() {
    currentSong = (currentSong + 1) % 2;
    
    if (currentSong == 0) {
        currentMelody = melody1;
        currentNumberOfNotes = numberOfNotes1;
    } else {
        currentMelody = melody2;
        currentNumberOfNotes = numberOfNotes2;
    }
    
    // Reset playback
    hw_noteIndex = 0;
    hw_ms = 0;
    
    update_screen();
}

static void hp_interrupt() {
    if (TMR1IE && TMR1IF) {
        TMR1 = 0xFFFF - 1000; // 1ms intervals
        
        if (hw_ms < hw_duration) {
            hw_ms++;
            TMR1IF = 0;
            return;
        }
        
        // Time to play next note
        if (hw_noteIndex >= currentNumberOfNotes) {
            hw_noteIndex = 0; // Loop the song
        }
        
        // Update LED display with current note index
        drive_led((char)hw_noteIndex);
        
        // Get frequency and duration from current melody
        int frequency = currentMelody[2 * hw_noteIndex];
        int noteDuration = currentMelody[2 * hw_noteIndex + 1];
        
        // Set PWM frequency
        set_pwm_frequency(frequency);
        
        // Calculate note duration
        if (noteDuration > 1) {
            hw_duration = wholenote / noteDuration;
        } else if (noteDuration < 0) {
            // Dotted note (1.5x duration)
            hw_duration = wholenote / (-noteDuration);
            hw_duration += hw_duration / 2;
        } else {
            hw_duration = wholenote; // Whole note
        }
        
        // Update display
        update_screen();
        
        hw_ms = 0;
        hw_noteIndex++;
        
        TMR1IF = 0;
    }
}

static void lp_interrupt() {}

static void init() {
    // Initialize song arrays
    numberOfNotes1 = (sizeof(melody1) / sizeof(melody1[0])) / 2;
    numberOfNotes2 = (sizeof(melody2) / sizeof(melody2[0])) / 2;
    
    // Start with song 1
    currentSong = 0;
    currentMelody = melody1;
    currentNumberOfNotes = numberOfNotes1;
    hw_noteIndex = 0;
    hw_ms = 0;
    hw_duration = wholenote / 4; // Start with quarter note duration
    
    // Configure RE1 as PWM output (CCP3)
    TRISEbits.TRISE1 = 1; // Set as input initially
    ANSELEbits.ANSE1 = 0; // Digital mode
    
    // Configure PWM module CCP3
    CCP3CONbits.CCP3M = 0b0000; // PWM mode off initially

    PSTR3CON |= 0b10; // steering set P3B
    PSTR3CON &= 0b10;

    CCP3CONbits.P3M = 0b00;
    CCP3CONbits.CCP3M = 0b1100;

    CCPTMRS0bits.C3TSEL = 0b01; // Timer 4
    
    // Configure Timer4 for PWM
    T4CONbits.T4CKPS = 0b01; // 1:4 prescaler
    T4CONbits.T4OUTPS = 0b0000; // 1:1 postscaler
    PR4 = 255; // Initial period
    TMR4ON = 1; // Enable Timer4
    
    // Wait for Timer4 to overflow once
    TMR4IF = 0;
    while (!TMR4IF);
    
    // Now set RB5 as output
    TRISBbits.TRISB5 = 0;
    
    // Configure Timer1 for note timing
    T1CONbits.TMR1CS = 0b00; // Clock source Fosc/4
    T1CONbits.T1CKPS = 0b11; // 1:8 prescaler
    TMR1 = 0xFFFF - 1000; // 1ms intervals
    TMR1ON = 1;
    TMR1IE = 1;
    TMR1IP = 1; // High priority
    
    // Initial display update
    update_screen();
    
    // Start playing first note
    set_pwm_frequency(currentMelody[0]);
}

static void destructor(void) {
    // Stop Timer1
    TMR1ON = 0;
    TMR1IF = 0;
    TMR1IE = 0;
    
    // Stop Timer4 and PWM
    TMR4ON = 0;
    CCP2CONbits.CCP2M = 0b0000; // PWM mode off
    
    // Reset RB5 configuration
    TRISBbits.TRISB5 = 1;
    ANSELBbits.ANSB5 = 1;
    
    // Turn off all LEDs
    drive_led(0b111111);
}

static void main(void) {
    // BTN2 switches songs
    if (button_states.btn2_re) {
        switch_song();
    }
    
    // BTN4 returns to menu
    if (button_states.btn4_re || button_states.btn4_he) {
        returnToMenu();
        return;
    }
}

void register_hw(void) {
    registerProgram("HW", &init, &destructor, &main, &lp_interrupt, &hp_interrupt);
}
