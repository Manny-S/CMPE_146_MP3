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