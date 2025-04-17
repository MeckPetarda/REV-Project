#include "config.h"

#include "buttons.h"
#include "lcd.h"
#include "led.h"
#include "menu.h"
#include <stdbool.h>
#include <stdio.h>

char msg[64];
volatile int head_index = 0;
volatile int uart_index = 0;

static void uart_init();
static void uart_main(void);
static void lp_interrupt();
static void hp_interrupt();

void putch(char data);
int getch(void);

void register_uart(void) {
  registerSubroutine("UART", &uart_init, &uart_main, &lp_interrupt,
                     &hp_interrupt);
}

static void slice_str(const char *str, char *buffer, size_t start, size_t end) {
  size_t j = 0;
  for (size_t i = start; i <= end; ++i) {
    buffer[j++] = str[i];
    
    if (str[i] == '\0') return;
  }
  buffer[j] = 0;
}

void update_screen() {
  char line1[17] = {0};

  slice_str(msg, line1, head_index, head_index + DISPLAY_LENGTH - 1);

  lcd_show_string(1, line1);

  char line2[17] = {0};
  snprintf(line2, sizeof(line2), "UART  <%c%c> Cl Bk",
           '0' + (char)((head_index - head_index % 10) / 10),
           '0' + (char)(head_index % 10));
  lcd_show_string(2, line2);
}

static void clear_string() {
  uart_index = 0;
  head_index = 0;
  msg[0] = '\0';
  
  update_screen();
}

static void lp_interrupt() {}

static void hp_interrupt() {
  if (RC1IF & RC1IE) {
    char c = (char)getch();

    int maxLength = sizeof(msg) / sizeof(msg[0]);

    if (uart_index >= maxLength) {
      printf("\nMessage too long!, resetting\n");
      msg[1] = '\0';
      uart_index = 0;
      head_index = 0;
    }

    msg[uart_index] = c;
    uart_index++;

    if (uart_index > DISPLAY_LENGTH) {
      head_index = uart_index - DISPLAY_LENGTH;

      if (head_index < 0)
        head_index = 0;
    }

    msg[uart_index] = '\0';

    update_screen();
  }
}

void awaitCommand(void) { printf("\nUART > "); }

static void uart_init() {
  uart_index = 0;
  head_index = 0;
  msg[0] = '\0';

  // TRISD = 0x00;           // PORTD jako vystup
  TRISCbits.TRISC6 = 1; // TX pin jako vstup
  TRISCbits.TRISC7 = 1; // rx pin jako vstup

  ANSELC = 0x00; // Configure PORTC pins as digital

  // Configure interrupt priority
  RCONbits.IPEN = 1; // Enable priority levels

  RC1IE = 1; // zap  preruseni od RCREG

  // Configure baudrate
  SPBRG1 = 51; // (32_000_000 / (64 * 9600)) - 1

  TXSTA1bits.SYNC = 0; // EUSART Mode Select bit - ASYNC
  RCSTA1bits.SPEN = 1; // Serial Port Enable bit
  TXSTA1bits.TXEN = 1; // Transmit Enable bit
  RCSTA1bits.CREN = 1; // Continuous Receive Enable bit

  lcd_show_string(1, "                ");
  update_screen();

  awaitCommand();
}

static void uart_main(void) {
  if (button_states.btn1_re) {
    if (head_index != 0) {
      head_index--;
      update_screen();
    }
  } else if (button_states.btn2_re) {
    if (head_index < uart_index - DISPLAY_LENGTH) {
      head_index++;
      update_screen();
    }
  }
  else if (button_states.btn3_re) {
    clear_string();
  }
  else if (button_states.btn4_re) {
    RC1IE = 0;

    RCSTA1bits.SPEN = 0; // Serial Port Enable bit
    TXSTA1bits.TXEN = 0; // Transmit Enable bit
    RCSTA1bits.CREN = 0; // Continuous Receive Enable bit

    returnToMenu();
  }
}

void putch(char data) {
  while (!TX1IF)
    ;
  TXREG1 = data;
}

int getch(void) {
  if (!RC1IF)
    return 0;
  return RCREG1;
}
