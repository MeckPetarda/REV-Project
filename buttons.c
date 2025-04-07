#include <xc.h>
#include "led.h"

#define BTN1 PORTCbits.RC0
#define BTN2 PORTAbits.RA4
#define BTN3 PORTAbits.RA3
#define BTN4 PORTAbits.RA2

#define RE 0b01111111
#define FE 0b10000000
#define HE 0b11111111

typedef struct {
  char btn1_re;
  char btn2_re;
  char btn3_re;
  char btn4_re;

  char btn1_fe;
  char btn2_fe;
  char btn3_fe;
  char btn4_fe;

  char btn1_he;
  char btn2_he;
  char btn3_he;
  char btn4_he;

  char btn1_state;
  char btn2_state;
  char btn3_state;
  char btn4_state;
} button_states_t;

volatile button_states_t button_states = {0};

void buttons_interrupt(void) {
  if (TMR2IF && TMR2IE) {      
    button_states.btn1_state <<= 1;
    button_states.btn1_state |= BTN1;

    button_states.btn2_state <<= 1;
    button_states.btn2_state |= BTN2;

    button_states.btn3_state <<= 1;
    button_states.btn3_state |= BTN3;

    button_states.btn4_state <<= 1;
    button_states.btn4_state |= BTN4;

    button_states.btn1_re = (button_states.btn1_state == RE);
    button_states.btn1_fe = (button_states.btn1_state == FE);
    button_states.btn1_he = (button_states.btn1_state == HE);

    button_states.btn2_re = (button_states.btn2_state == RE);
    button_states.btn2_fe = (button_states.btn2_state == FE);
    button_states.btn2_he = (button_states.btn2_state == HE);

    button_states.btn3_re = (button_states.btn3_state == RE);
    button_states.btn3_fe = (button_states.btn3_state == FE);
    button_states.btn3_he = (button_states.btn3_state == HE);

    button_states.btn4_re = (button_states.btn4_state == RE);
    button_states.btn4_fe = (button_states.btn4_state == FE);
    button_states.btn4_he = (button_states.btn4_state == HE);

    TMR2IF = 0;
  }
}

void buttons_init(void) {  
  ANSELA = 0x00;

  TRISCbits.TRISC0 = 1; // btn1
  TRISAbits.TRISA4 = 1; // btn2
  TRISAbits.TRISA3 = 1; // btn3
  TRISAbits.TRISA2 = 1; // btn4

  T2CONbits.T2CKPS = 0b11;    // 16 prescaler
  T2CONbits.T2OUTPS = 0b1111; // 1:16 postscaler

  PR2 = 250; // T2 period

  IPR1bits.TMR2IP = 1; // set TMR2 interrupt priority to LoW

  TMR2IE = 1;
  TMR2IF = 0;

  TMR2ON = 1;
  
  PEIE = 1;   // povoleni preruseni od periferii
  GIE = 1;    // globalni povoleni preruseni
}