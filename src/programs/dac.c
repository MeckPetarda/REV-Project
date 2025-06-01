#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include "../per/uart_common.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

// DAC SPI definitions (from reference)
#define DAC_SS LATBbits.LATB3       // DAC slave select pin
#define DAC_CH1 0b00110000          // channel 1/B

// Sine wave lookup table (0-127 values for 8-bit DAC)
// 128 samples for HALF sine wave cycle - using symmetry to save memory
#define SINE_TABLE_SIZE 128
#define FULL_CYCLE_SIZE 256  // Virtual full cycle size
#define DAC_MAX 255
#define DAC_MIN 0
#define DAC_MEAN 127

volatile unsigned char sine_table[SINE_TABLE_SIZE];  // Only store half cycle
volatile int sine_index = 0;

volatile long pot1 = 0;  // Lower clipping control
volatile long pot2 = 0;  // Upper clipping control
volatile bool pot_updated = false;

// Clipping bounds (0-127 range for each pot)
volatile unsigned char lower_clip = DAC_MIN;   // POT1 controls clipping from min to mean
volatile unsigned char upper_clip = DAC_MAX;   // POT2 controls clipping from max to mean

// UART transmission variables
volatile int uart_counter = 0;
volatile bool send_uart_data = false;
#define UART_SEND_INTERVAL 3  // Send data every 3 sine samples for good visualization

// ADC reading from RB5
volatile unsigned int adc_reading = 0;
volatile unsigned int scaled_adc_reading = 0;  // Software-scaled version

// Scaling factor to compensate for hardware mismatch
// Adjust this based on your measured ratio (currently 2.5x)
#define ADC_SCALE_FACTOR_NUM 10  // Numerator  
#define ADC_SCALE_FACTOR_DEN 25  // Denominator (10/25 = 0.4, inverse of 2.5)

static void generate_sine_table() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // Generate only the first half of sine wave (0 to π)
        // Store as positive values (127 to 255) for the first half
        double angle = (3.14159265359 * i) / SINE_TABLE_SIZE;  // 0 to π
        sine_table[i] = (unsigned char)(127.5 + 127.5 * sin(angle));
    }
}

static unsigned char get_sine_value(int index) {
    // Use symmetry to get full sine wave from half-wave table
    if (index < SINE_TABLE_SIZE) {
        // First half: 0 to π (positive half of sine wave)
        return sine_table[index];
    } else {
        // Second half: π to 2π (negative half of sine wave)
        // Mirror the index and flip the sign
        int mirror_index = index - SINE_TABLE_SIZE;  // 0 to 127 for second half
        unsigned char positive_value = sine_table[mirror_index];
        
        // Flip the sign around the center (127) with proper bounds checking
        int amplitude = (int)positive_value - (int)DAC_MEAN;  // Get signed amplitude
        int negative_result = (int)DAC_MEAN - amplitude;      // Flip it
        
        // Clamp to valid DAC range to prevent underflow/overflow
        if (negative_result < DAC_MIN) {
            return DAC_MIN;
        } else if (negative_result > DAC_MAX) {
            return DAC_MAX;
        } else {
            return (unsigned char)negative_result;
        }
    }
}

static void update_clipping_bounds() {
    // POT1 (0-1023) controls lower clipping from DAC_MIN to DAC_MEAN
    lower_clip = DAC_MIN + (pot1 * (DAC_MEAN - DAC_MIN)) / 1023;
    
    // POT2 (0-1023) controls upper clipping from DAC_MAX to DAC_MEAN  
    upper_clip = DAC_MAX - (pot2 * (DAC_MAX - DAC_MEAN)) / 1023;
}

static void update_screen() {
    char line1[17] = {0};
    char line2[17] = {0};

    // Display the clipping bounds and scaled ADC reading
    snprintf(line1, sizeof(line1), "LOW %3d HIGH %3d", lower_clip, upper_clip);
    snprintf(line2, sizeof(line2), "ADC:%3d SC:%3d", adc_reading, scaled_adc_reading);
    
    lcd_show_string(1, line1, false);
    lcd_show_string(2, line2, false);
}

static unsigned char apply_clipping(unsigned char sine_value) {
    // Apply clipping based on the current bounds
    if (sine_value < lower_clip) {
        return lower_clip;
    } else if (sine_value > upper_clip) {
        return upper_clip;
    }
    return sine_value;
}

/* SPI write function (from reference) - writes two bytes in sequence */
static void SPIWrite(unsigned char channel, unsigned char data) {
    unsigned char msb, lsb, flush;
    
    // Set DAC gain to 1x (bit 13 = 1) and ensure proper formatting
    // Format: [AB x GA SH D7 D6 D5 D4] [D3 D2 D1 D0 x x x x]
    // Where AB=1 for B, AB=0 for A, GA=1 for 1x gain, SH=1 for active mode
    msb = (channel | 0x10 | (data >> 4)); // channel + gain=1x + upper 4 bits of data
    lsb = (data << 4) & 0xF0;             // lower 4 bits of data, shifted left
    
    DAC_SS = 0;                    // slave select active
    PIR1bits.SSPIF = 0;            // clear SPI flag
    SSP1BUF = msb;                 // write to buffer
    while (PIR1bits.SSPIF == 0)
        NOP();                     // wait until SPI sends first byte

    PIR1bits.SSPIF = 0;            // clear SPI flag
    SSP1BUF = lsb;                 // write to buffer
    while (PIR1bits.SSPIF == 0)
        NOP();                     // wait until SPI sends second byte

    DAC_SS = 1;                    // deactivate slave select
    flush = SSP1BUF;               // read buffer to clear
}

static void hp_interrupt() {
    // Timer interrupt for sine wave generation
    if (TMR1IE && TMR1IF) {
        // Get the current sine value using symmetry
        unsigned char raw_sine = get_sine_value(sine_index);
        
        // Apply clipping
        unsigned char clipped_sine = apply_clipping(raw_sine);
        
        // Output to external DAC via SPI (using channel 1)
        SPIWrite(DAC_CH1, clipped_sine);

        // UART transmission counter
        uart_counter++;
        if (uart_counter >= UART_SEND_INTERVAL) {
            uart_counter = 0;
            send_uart_data = true;
        }
        
        // Move to next sine table entry (full cycle)
        sine_index++;
        if (sine_index >= FULL_CYCLE_SIZE) {
            sine_index = 0;
        }
        
        TMR1IF = 0;
    }
}

static void lp_interrupt() {
    // ADC interrupt for reading potentiometers and RB5
    if (ADIE && ADIF) {
        unsigned int tmp;
        
        // Read ADC result - using 10-bit resolution properly
        if (ADCON2bits.ADFM == 1) {
            // Right justified: combine ADRESH and ADRESL properly for 10-bit result
            tmp = ((unsigned int)ADRESH << 8) | ADRESL;
        } else {
            // Left justified: shift right to get 10-bit result
            tmp = ((unsigned int)ADRESH << 2) | (ADRESL >> 6);
        }

        if (ADCON0bits.CHS == 4) {
            // Reading POT1 (AN4)
            ADCON0bits.CHS = 5;  // Switch to POT2
            
            if (tmp != pot1) {
                pot1 = tmp;
                pot_updated = true;
            }
        } else if (ADCON0bits.CHS == 5) {
            // Reading POT2 (AN5)
            ADCON0bits.CHS = 13;  // Switch to RB5 (AN13)
            
            if (tmp != pot2) {
                pot2 = tmp;
                pot_updated = true;
            }
        } else if (ADCON0bits.CHS == 13) {
            // Reading RB5 (AN13) - the digitized DAC output
            ADCON0bits.CHS = 4;  // Switch back to POT1
            
            adc_reading = tmp;
            // Apply software scaling to compensate for hardware mismatch
            scaled_adc_reading = (adc_reading * ADC_SCALE_FACTOR_NUM) / ADC_SCALE_FACTOR_DEN;
        }

        ADIF = 0;
    }
}

static void init() {
    // Generate sine lookup table
    generate_sine_table();
    
    // Initialize variables
    sine_index = 0;
    pot1 = 512;  // Mid-range default
    pot2 = 512;  // Mid-range default
    pot_updated = true;
    uart_counter = 0;
    send_uart_data = false;
    adc_reading = 0;
    scaled_adc_reading = 0;
    
    // Setup UART
    uart_common_init();
    
    // Configure SPI pins and module (from reference)
    ANSELC = 0x00;                // turn off analog functions on PORTC
    TRISCbits.TRISC3 = 0;         // SCK as output
    TRISCbits.TRISC5 = 0;         // SDO as output
    TRISBbits.TRISB3 = 0;         // SS as output
    LATBbits.LATB3 = 1;           // DAC SS off (inactive high)
    
    // Configure SPI module (from reference)
    SSP1CON1bits.SSPM = 0b0010;   // SPI master mode, clock = FOSC/64
    SSP1STATbits.CKE = 1;         // Clock edge select
    SSP1CON1bits.SSPEN = 1;       // Enable SPI
    
    // Setup ADC for RB5 (AN13), POT1 (AN4), and POT2 (AN5)
    ANSELBbits.ANSB5 = 1;         // RB5 as analog input (from reference)
    ANSELAbits.ANSA5 = 0;         // POT1 analog
    ANSELEbits.ANSE0 = 0;         // POT2 analog
    
    ADIE = 1;                     // Enable A/D interrupt
    ADIP = 0;                     // A/D low priority
    ADIF = 0;
    
    ADCON2bits.ADFM = 1;          // Right justified for proper 10-bit reading
    ADCON2bits.ADCS = 0b110;      // Fosc/64 (from reference)
    ADCON2bits.ACQT = 0b110;      // 16 Tad (from reference)
    ADCON0bits.ADON = 1;          // ADC on
    ADCON0bits.CHS = 13;          // Start with AN13 (RB5)
    
    // Setup Timer1 for sine wave generation with higher resolution
    T1CONbits.TMR1CS = 0b00;      // Clock source Fosc/4
    T1CONbits.T1CKPS = 0b01;      // 1:2 prescaler (faster for higher resolution)
    TMR1 = 0xFFFF - 400;          // Shorter period for smoother sine wave
    TMR1ON = 1;
    TMR1IE = 1;
    TMR1IP = 1;                   // High priority
    
    // Initial screen update
    update_clipping_bounds();
    update_screen();
    
    // Send initial UART message
    printf("\nDAC Sine Wave Generator Started\n");
    printf("Sending ADC readings for Tauno Serial Plotter\n");
}

static void destructor(void) {
    // Stop Timer1
    TMR1ON = 0;
    TMR1IF = 0;
    TMR1IE = 0;

    // Stop ADC
    ADIE = 0;
    ADIF = 0;
    ADCON0bits.ADON = 0;

    // Reset analog configuration
    ANSELAbits.ANSA5 = 1;
    ANSELEbits.ANSE0 = 1;
    ANSELBbits.ANSB5 = 0;         // Turn off RB5 analog

    // Disable SPI
    SSP1CON1bits.SSPEN = 0;

    // Cleanup UART
    uart_common_deinit();
}

static void main(void) {
    // Return to menu on button 4 - check this FIRST to ensure responsiveness
    if (button_states.btn4_re || button_states.btn4_he) {
        returnToMenu();
        return;
    }
    
    // Send UART data if flagged (send digitized signal from RB5)
    if (send_uart_data) {
        // Send scaled ADC reading for Tauno Serial Plotter
        // This should now match the DAC output values better
        printf("%u\n", scaled_adc_reading);
        
        send_uart_data = false;
    }
    
    // Update clipping bounds and screen if potentiometers changed
    if (pot_updated) {
        update_clipping_bounds();
        update_screen();
        pot_updated = false;
    }
    
    // Start ADC conversion if not running
    if (GODONE == 0) {
        GODONE = 1;
    }
}

void register_dac(void) {
    registerProgram("DAC", &init, &destructor, &main, &lp_interrupt, &hp_interrupt);
}
