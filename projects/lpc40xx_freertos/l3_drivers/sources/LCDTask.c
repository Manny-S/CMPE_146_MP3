#include "LCDTask.h"
#include "ssp2_lab.h"

bool LCD_init() {
  LCD_configure_spi();
  LCD_display_on();
  LCD_clear_display();
  delay_ms(100);
  return true;
}

void LCD_display_on() {
  LCD_write(0xAE, 0);
  LCD_write(0xD5, 0x80);
  LCD_write(0xA8, 0x3F);
  LCD_write(0x40, 0);
  LCD_write(0X8D, 0x14);
  LCD_write(0xA1, 0);
  LCD_write(0xC8, 0);
  LCD_write(0xDA, 0x12);
  LCD_write(0x81, 0xCF);
  LCD_write(0xD9, 0XF1);

  LCD_write(0xD8, 0x40);
  LCD_write(0xA4, 0);
  LCD_write(0xA6, 0x0);
  LCD_write(0xAF, 0x0);
  LCD_write(0x20, 0X00);
  LCD_write(0xA4, 0);
}

void LCD_clear_display() {
  uint8_t c_data[] = {0x00, 0x7F};
  uint8_t p_data[] = {0x00, 0x07};
  write_command(0x21, c_data, 2);
  write_command(0x22, p_data, 2);

  for (int i = 0; i < 1024; i++) {
    write_data(0x00);
  }
}

void LCD_write(uint8_t command, uint8_t data) {
  adesto_cs();
  data_ds();
  new_ssp2__exchange_byte_owner(command);
  if (data != 0) {
    new_ssp2__exchange_byte_owner(data);
  }
  adesto_ds();
}

void write_command(uint8_t command, uint8_t data, uint8_t size) {
  adesto_cs();
  data_ds();
  new_ssp2__exchange_byte_owner(command);
  for (uint8_t *index = 0; index < size; index++) {
    new_ssp2__exchange_byte_owner(data[index]);
  }
  adesto_ds();
}
void write_data(uint8_t data) {
  adesto_cs();
  data_cs();
  new_ssp2__exchange_byte_owner(data);
  data_ds();
  adesto_ds();
}
