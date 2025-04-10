
#ifndef LCD_H
#define LCD_H

#define DISPLAY_LENGTH 16

void lcd_init(void);
void lcd_show_string(char line, char a[]);
static void lcd_send(unsigned char data);
void lcd_clear(void);
void lcd_reset(void);

#endif /* LCD_H */
