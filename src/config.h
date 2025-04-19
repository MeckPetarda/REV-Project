#ifndef CONFIG_H
#define CONFIG_H

#include <xc.h>

// Oscillator Selection bits (HS oscillator (medium power 4-16 MHz))
#pragma config FOSC = HSMP
// 4X PLL Enable (Oscillator multiplied by 4)
#pragma config PLLCFG = ON
// Watchdog Timer disabled. SWDTEN has no effect.
#pragma config WDTEN = OFF

// System clock definition for xc8
#define _XTAL_FREQ 32E6

#endif /* CONFIG_H */
