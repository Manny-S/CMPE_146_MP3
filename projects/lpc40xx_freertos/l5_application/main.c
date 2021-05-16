#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "semphr.h"

#include "gpio.h"
#include "queue.h"
#include "task.h"
#include <stdlib.h>

#include "2004_LCD.h"
#include "event_groups.h"
#include "ff.h"
#include "song_list.h"
#include "ssp2.h"
#include <string.h>

typedef char songname_t[32];
typedef char songbyte_t[512];
QueueHandle_t Q_songname;
bool new_song = false;
QueueHandle_t Q_songdata;

TaskHandle_t MP3PlayPause = NULL;

const uint32_t spi_clock_khz = 1000;

const uint8_t READ = 0x03;
const uint8_t WRITE = 0x02;
const uint8_t SCI_MODE = 0x00;
const uint8_t SCI_STATUS = 0x1;
const uint8_t SCI_CLOCKF = 0x3;
const uint8_t SCI_VOL = 0xB;
const uint8_t DUMB = 0xFF;
uint8_t v_level = 5;

int menu_level = 0;

gpio_s CS;
gpio_s DREQ; // Data Request input
gpio_s RST;
gpio_s XDCS;

// buttons
gpio_s play_pause;
gpio_s button1;
gpio_s button2;
gpio_s button3;

void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void read_reg(void);
void clockf_init(void);
void decoder_write(uint8_t address, uint16_t data);
void play_pause_button(void *p);
void adjust_volume(bool higher);
void volume_task(void *p);
void menu(void *p);

int main(void) {
  // Adds the ability for CLI commands
  sj2_cli__init();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songbyte_t));

  CS = gpio__construct_as_output(4, 28);
  DREQ = gpio__construct_as_input(0, 6);
  RST = gpio__construct_as_output(0, 8);
  XDCS = gpio__construct_as_output(0, 26);

  play_pause = gpio__construct_as_input(1, 19);
  button1 = gpio__construct_as_input(1, 15);
  button2 = gpio__construct_as_input(0, 30);
  button3 = gpio__construct_as_input(0, 29);

  gpio__reset(RST);
  gpio__set(RST);

  gpio__set(CS);
  gpio__set(XDCS);

  ssp2__initialize(spi_clock_khz);

  xTaskCreate(mp3_reader_task, "mp3_reader", 1024, NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "mp3_player", 1024, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(play_pause_button, "play_pause", 1024, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(volume_task, "volume", 1024, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(menu, "menu", 1024, NULL, PRIORITY_LOW, NULL);

  song_list__populate();
  for (size_t song_number = 0; song_number < song_list__get_item_count();
       song_number++) {
    printf("Song %2d: %s\n", (1 + song_number),
           song_list__get_name_for_item(song_number));
  }
  clockf_init();
  LCD2004_init();

  MP3PlayPause = xTaskGetHandle("mp3_player");
  vTaskStartScheduler();
  return 0;
}

void menu(void *p) {
  int song_index = 0;
  int song_index_max = song_list__get_item_count() - 1;
  char *song_name = song_list__get_name_for_item(song_index);
  LCD2004_menu1(song_name);
  while (1) {
    switch (menu_level) {
    case 0: // main menu
      if (gpio__get(button1) && song_index < song_index_max) {
        song_index++;
        song_name = song_list__get_name_for_item(song_index);
        LCD2004_menu1(song_name);
        vTaskDelay(100);
      }
      if (gpio__get(button2) && song_index > 0) {
        song_index--;
        song_name = song_list__get_name_for_item(song_index);
        LCD2004_menu1(song_name);
        vTaskDelay(100);
      }
      if (gpio__get(button3)) {
        new_song = true;
        menu_level++;
        LCD2004_menu_play(song_name);
        xQueueSend(Q_songname, song_name, 10);
        vTaskDelay(100);
      }
      break;

    case 1: // play menu
      if (gpio__get(button3)) {
        menu_level--;
        new_song = true;
        xQueueSend(Q_songname, "", 10);
        LCD2004_menu1(song_name);
        vTaskDelay(100);
      }
      if (gpio__get(button2)) {
        menu_level++;
        LCD2004_menu_volume(song_name);
        vTaskDelay(100);
      }
      break;

    case 2: // volume menu
      if (gpio__get(button3)) {
        menu_level--;
        LCD2004_menu_play(song_name);
        vTaskDelay(100);
      }
      break;

    default:
      menu_level = 0;
      break;
    }
    vTaskDelay(1);
  }
}

void clockf_init(void) {
  uint8_t byte1 = 0x88;
  uint8_t byte2 = 0x00;
  gpio__reset(CS);
  ssp2__exchange_byte(WRITE);
  ssp2__exchange_byte(SCI_CLOCKF);
  ssp2__exchange_byte(byte1);
  ssp2__exchange_byte(byte2);
  gpio__set(CS);
}

void decoder_write(uint8_t address, uint16_t data) {
  gpio__reset(CS);
  ssp2__exchange_byte(WRITE);
  ssp2__exchange_byte(address);
  ssp2__exchange_byte((data >> 8) & 0xFF);
  ssp2__exchange_byte((data >> 0) & 0xFF);
  gpio__set(CS);
}

void read_reg(void) {
  uint8_t byte1 = 0xFF;
  uint8_t byte2 = 0xFF;
  gpio__reset(CS);
  ssp2__exchange_byte(READ);
  ssp2__exchange_byte(SCI_CLOCKF);
  byte1 = ssp2__exchange_byte(DUMB);
  byte2 = ssp2__exchange_byte(DUMB);
  gpio__set(CS);

  printf("READ: %x %x\n", byte1, byte2);
}

void mp3_reader_task(void *p) {
  songname_t name;
  songbyte_t chunk;

  FIL file;
  FRESULT result;
  UINT readCount;

  while (1) {
    if (xQueueReceive(Q_songname, &name[0], portMAX_DELAY)) {
      printf("Received song to play: %s\n", name);

      result = f_open(&file, name, FA_READ);
      if (FR_OK == result) {
        new_song = false;
        while (!f_eof(&file) && new_song != true) {
          f_read(&file, chunk, sizeof(songbyte_t), &readCount);
          xQueueSend(Q_songdata, &chunk, portMAX_DELAY);
        }
        f_close(&file);
        vTaskDelay(10);
      } else {
        printf("ERROR: File not found\n");
      }
    }
  }
}

void mp3_player_task(void *p) {
  songbyte_t chunk;

  while (1) {
    if (xQueueReceive(Q_songdata, &chunk, portMAX_DELAY)) {
      gpio__reset(XDCS);
      for (int i = 0; i < 512; i++) {
        while (!gpio__get(DREQ)) {
          vTaskDelay(1);
        }

        ssp2__exchange_byte(chunk[i]);
      }
      gpio__set(XDCS);
    }
  }
}

void play_pause_button(void *p) {
  bool play_status = false;
  uint8_t alternative_status = 1;
  while (1) {
    vTaskDelay(100);
    if (gpio__get(play_pause)) {
      while (gpio__get(play_pause)) {
        vTaskDelay(1);
      }
      play_status = true;
    } else {
      play_status = false;
    }

    if (play_status) {
      if (alternative_status) {
        vTaskResume(MP3PlayPause);
        alternative_status--;
      } else {
        vTaskSuspend(MP3PlayPause);
        alternative_status++;
      }
    }
    vTaskDelay(1);
  }
}

void adjust_volume(bool higher) {
  if (higher && v_level < 8 && !false) {
    v_level++;
  } else if (!higher && v_level > 1 && !false) {
    v_level--;
  }

  switch (v_level) {
  case 1:
    decoder_write(SCI_VOL, 0xFEFE);
    break;

  case 2:
    decoder_write(SCI_VOL, 0x4545);
    break;

  case 3:
    decoder_write(SCI_VOL, 0x4040);
    break;

  case 4:
    decoder_write(SCI_VOL, 0x3535);
    break;

  case 5:
    decoder_write(SCI_VOL, 0x3030);
    break;

  case 6:
    decoder_write(SCI_VOL, 0x2525);
    break;

  case 7:
    decoder_write(SCI_VOL, 0x2020);
    break;

  case 8:
    decoder_write(SCI_VOL, 0x1010);
    break;

  default:
    decoder_write(SCI_VOL, 0x3535);
    // printf("volume  = %i", v_level);
  }
  vTaskDelay(1000);
}

void volume_task(void *p) {
  bool v_increase = false;
  bool v_decrease = false;
  while (1) {
    vTaskDelay(100);
    if (gpio__get(button2) && menu_level == 2) {
      while (gpio__get(button2)) {
        vTaskDelay(1);
      }
      v_increase = true;
    } else if (gpio__get(button1) && menu_level == 2) {
      while (gpio__get(button1)) {
        vTaskDelay(1);
      }
      v_decrease = true;
    }

    if (v_increase) {
      adjust_volume(true);
      v_increase = false;
    } else if (v_decrease) {
      adjust_volume(false);
      v_decrease = false;
    }
  }
}
