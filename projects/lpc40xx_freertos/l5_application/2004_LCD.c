#include "2004_LCD.h"

void LCD2004_init(void) {
  uart__init(UART__2, clock__get_peripheral_clock_hz(), 9600);

  const uint32_t uart_pin_func = 0b010;

  LPC_IOCON->P2_8 |= uart_pin_func;

  LCD2004_clear();
}

void LCD2004_print_char(char letter) { uart__polled_put(UART__2, letter); }

void LCD2004_print(char string[80]) {
  for (int i = 0; i < strlen(string); i++) {
    LCD2004_print_char(string[i]);
  }
}

void LCD2004_clear(void) {
  uart__polled_put(UART__2, '|');
  uart__polled_put(UART__2, '-');
}
