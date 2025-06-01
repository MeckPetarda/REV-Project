#ifndef UART_COMMON_H
#define UART_COMMON_H

// Common UART initialization and cleanup functions
void uart_common_init(void);
void uart_common_deinit(void);

// Standard UART functions used by printf/scanf
void putch(char data);
int getch(void);

#endif // UART_COMMON_H
