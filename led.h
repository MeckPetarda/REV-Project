#ifndef LEDS_H
#define LEDS_H

#include <xc.h>

// LED pin definitions
#define LED1 LATDbits.LATD2
#define LED2 LATDbits.LATD3
#define LED3 LATCbits.LATC4
#define LED4 LATDbits.LATD4
#define LED5 LATDbits.LATD5
#define LED6 LATDbits.LATD6

// Function prototypes
void led_init(void);
void drive_led(char in);

#endif  // LEDS_H