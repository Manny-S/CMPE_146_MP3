#include "2004_LCD.h"
#include "song_list.h"

void LCD2004_init(void) {
  uart__init(UART__2, clock__get_peripheral_clock_hz(), 9600);

  const uint32_t uart_pin_func = 0b010;

  LPC_IOCON->P2_8 |= uart_pin_func;

  LCD2004_clear();
}

void LCD2004_print_char(char letter) { uart__polled_put(UART__2, letter); }

void LCD2004_print(int rows, int cols, char string[20]) {
  switch (rows) {
  case 0:
    rows = 0;
    break;
  case 1:
    rows = 64;
    break;
  case 2:
    rows = 20;
    break;
  case 3:
    rows = 84;
    break;
  default:
    rows = 0;
    break;
  }
  uart__polled_put(UART__2, 254);
  uart__polled_put(UART__2, 128 + rows + cols);
  for (int i = 0; i < strlen(string); i++) {
    LCD2004_print_char(string[i]);
  }
}

void LCD2004_clear(void) {
  uart__polled_put(UART__2, '|');
  uart__polled_put(UART__2, '-');
}

void LCD2004_menu1(char name[20]) {
  LCD2004_clear();
  LCD2004_print(0, 0, "---Select Song---");
  LCD2004_print(1, 0, name);
  LCD2004_print(3, 0, "Select  <  >");
}
