#include <xc.h>

#define LED1 LATDbits.LATD2
#define LED2 LATDbits.LATD3
#define LED3 LATCbits.LATC4
#define LED4 LATDbits.LATD4
#define LED5 LATDbits.LATD5
#define LED6 LATDbits.LATD6

void led_init(void) {  
  TRISDbits.TRISD2 = 0;
  TRISDbits.TRISD3 = 0;
  TRISCbits.TRISC4 = 0;
  TRISDbits.TRISD4 = 0;
  TRISDbits.TRISD5 = 0;
  TRISDbits.TRISD6 = 0;
  
  LED1 = 1;
  LED2 = 1;
  LED3 = 1;
  LED4 = 1;
  LED5 = 1;
  LED6 = 1;
}

void drive_led(char in){
    LED1 = in & 1;          
    LED2 = in & 2 ? 1 : 0;     
    LED3 = in & 4 ? 1 : 0;     
    LED4 = in & 8 ? 1 : 0;     
    LED5 = in & 16 ? 1 : 0;    
    LED6 = in & 32 ? 1 : 0;   
}