#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include "../per/uart_common.h"
#include <stdbool.h>
#include <stdio.h>

enum Mode { CURSOR, SCROLL };

volatile enum Mode mode = SCROLL;
volatile bool cmdMode = false;

char msg[64];
volatile int cursor_index = 0;
volatile int head_index = 0;
volatile int uart_index = 0;

static void awaitCommand();

static void slice_str(const char *str, char *buffer, size_t start, size_t end) {
  size_t j = 0;
  for (size_t i = start; i <= end; ++i) {
    buffer[j++] = str[i];

    if (str[i] == '\0')
      return;
  }
  buffer[j] = 0;
}

void update_screen() {
  char line1[17] = {0};

  slice_str(msg, line1, head_index, head_index + DISPLAY_LENGTH - 1);

  char line2[17] = {0};

  int used_index;

  if (mode == CURSOR) {
    used_index = cursor_index;
  } else if (mode == SCROLL) {
    used_index = head_index;
  }

  if (cmdMode == true) {
    snprintf(line2, sizeof(line2), "CMD  Ms Cl    Cl");
  } else {
    snprintf(line2, sizeof(line2), "%s <%c%c> Cm Bk",
             mode == CURSOR ? "CURSR" : "SCRLL",
             '0' + (char)((used_index - used_index % 10) / 10),
             '0' + (char)(used_index % 10));
  }
  lcd_show_string(2, line2, false);

  lcd_show_string(1, line1, true);
  set_cursor_position(1, (char)cursor_index);
}

static void clear_string() {
  uart_index = 0;
  head_index = 0;
  cursor_index = 0;
  msg[0] = '\0';

  awaitCommand();

  lcd_show_string(1, "                ", false);

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
  cursor_index = 0;
  msg[0] = '\0';
  mode = SCROLL;

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

  lcd_show_string(1, "                ", false);
  update_screen();

  awaitCommand();
}

static void destructor() {
  RC1IE = 0;

  hide_cursor();

  RCSTA1bits.SPEN = 0; // Serial Port Enable bit
  TXSTA1bits.TXEN = 0; // Transmit Enable bit
  RCSTA1bits.CREN = 0; // Continuous Receive Enable bit
}

static void uart_main(void) {
  // Cmd key is pressed
  if (cmdMode == false && button_states.btn3_re) {
    cmdMode = true;
    update_screen();
  } else if (cmdMode == true && button_states.btn3_fe) {
    cmdMode = false;
    update_screen();
  }

  if (cmdMode == true) {
    if (button_states.btn1_re) {
      if (mode == CURSOR) {
        mode = SCROLL;
        hide_cursor();
      } else if (mode == SCROLL) {
        mode = CURSOR;
        cursor_index = 0;
        show_cursor();
      }
      cmdMode = false;
      update_screen();
    } else if (button_states.btn2_re) {
      clear_string();

      cmdMode = false;
      update_screen();
    }
  } else {
    if (button_states.btn1_re) {
      if (mode == CURSOR) {
        if (cursor_index != 0) {
          cursor_index--;
          update_screen();
        } else if (head_index != 0) {
          head_index--;
          update_screen();
        }
      } else if (mode == SCROLL) {
        if (head_index != 0) {
          head_index--;
          update_screen();
        }
      }
    } else if (button_states.btn2_re) {

      if (mode == CURSOR) {
        if (cursor_index < DISPLAY_LENGTH - 1) {
          cursor_index++;
          update_screen();
        } else if (head_index < uart_index - DISPLAY_LENGTH) {
          head_index++;
          update_screen();
        }
      } else if (mode == SCROLL) {
        if (head_index < uart_index - DISPLAY_LENGTH) {
          head_index++;
          update_screen();
        }
      }

    } else if (button_states.btn4_re) {

      returnToMenu();
    }
  }
}

void register_uart(void) {
  registerProgram("UART", &uart_init, NULL, &uart_main, &lp_interrupt,
                  &hp_interrupt);
}
