
#ifndef LCD_H
#define LCD_H

#define DISPLAY_LENGTH 16
#include <stdbool.h>

enum DisplayModes { ONE_ROW, TWO_ROWS, DOUBLE_HEIGHT };

void lcd_init(void);
void lcd_show_string(char lineNum, char textData[], bool endWithNullByte);
static void lcd_send(unsigned char data);
void lcd_clear(void);
void lcd_reset(void);

void show_cursor(void);
void hide_cursor(void);

void start_i2c_comunication(void);
void stop_i2c_comunication(void);
void send_instruction(unsigned char instruction);

void write_text(int row, char position, char text[]);
void write_char(int row, char index, char character);
void set_cursor_position(int line, char position);

void set_number_of_lines(enum DisplayModes displayMode);

#endif /* LCD_H */
