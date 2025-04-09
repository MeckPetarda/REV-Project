#ifndef CONFIG_H
#define	CONFIG_H

#include <xc.h>

#pragma config FOSC = HSMP // Oscillator Selection bits (HS oscillator (medium power 4-16 MHz))
#pragma config PLLCFG = ON // 4X PLL Enable (Oscillator multiplied by 4)
#pragma config WDTEN = OFF // Watchdog Timer Enable bits (Watch dog timer is  always disabled. SWDTEN has no effect.

#define _XTAL_FREQ 32E6 // definice fosc pro knihovnu

#endif	/* CONFIG_H */

