#pragma once

#include <stdbool.h>
#include <stdint.h>

bool LCD_init();

void LCD_write(uint8_t command, uint8_t data);

void write_command(uint8_t command,uint8_t data,uint8_t size);
void write_data(uint8_t data);

void LCD_clear_display();
void LCD_display_on();
