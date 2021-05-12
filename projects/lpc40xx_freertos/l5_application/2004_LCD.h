#include "clock.h"
#include "delay.h"
#include "lpc40xx.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void LCD2004_init(void);

void LCD2004_print_char(char letter);

void LCD2004_print(int rows, int cols, char string[20]);

void LCD2004_clear(void);

void LCD2004_menu1(char name[20]);

void LCD2004_menu_play(char name[20]);

void LCD2004_menu_volume(char name[20]);

void LCD2004_menu_treb_bass(char name[20]);

void LCD2004_menu_treble(char name[20]);

void LCD2004_menu_bass(char name[20]);