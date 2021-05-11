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
uint8_t volume_level = 5;

gpio_s CS;
gpio_s DREQ; // Data Request input
gpio_s RST;
gpio_s XDCS;
gpio_s play_pause;
gpio_s Volume_up;
gpio_s Volume_down;

void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void read_reg(void);
void clockf_init(void);
void play_pause_button(void *p);
void volume_C(bool higher, bool initial);
void Volume_Control(void *p);

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
  Volume_up = gpio__construct_as_input(1, 10);
  Volume_down = gpio__construct_as_input(1, 14);

  gpio__reset(RST);
  gpio__set(RST);

  gpio__set(CS);
  gpio__set(XDCS);

  ssp2__initialize(spi_clock_khz);

  xTaskCreate(mp3_reader_task, "mp3_reader", 1024, NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "mp3_player", 1024, NULL, PRIORITY_LOW, NULL);
  xTaskCreate(play_pause_button, "play_pause", 1024, NULL, PRIORITY_LOW, NULL);

  song_list__populate();
  for (size_t song_number = 0; song_number < song_list__get_item_count();
       song_number++) {
    printf("Song %2d: %s\n", (1 + song_number),
           song_list__get_name_for_item(song_number));
  }

  xTaskCreate(Volume_Control, "Volume", 1024, NULL, PRIORITY_HIGH, NULL);
  clockf_init();
  read_reg();
  LCD2004_init();
  MP3PlayPause = xTaskGetHandle("mp3_player");
  vTaskStartScheduler();
  return 0;
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
        while (!f_eof(&file)) {
          f_read(&file, chunk, sizeof(songbyte_t), &readCount);
          xQueueSend(Q_songdata, &chunk, portMAX_DELAY);
        }
        f_close(&file);
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

void volume_C(bool higher, bool initial) {
  if (higher && volume_level < 8 && !initial) {
    volume_level++;
  } else if (!higher && volume_level > 1 && !initial) {
    volume_level--;
  }

  if (volume_level == 1) {
    // write_to_decoder(SCI_VOL, 0xFEFE);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 2) {
    // write_to_decoder(SCI_VOL, 0x4545);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 3) {
    // write_to_decoder(SCI_VOL, 0x4040);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 4) {
    // write_to_decoder(SCI_VOL, 0x3535);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 5) {
    // write_to_decoder(SCI_VOL, 0x3030);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 6) {
    // write_to_decoder(SCI_VOL, 0x2525);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 7) {
    // write_to_decoder(SCI_VOL, 0x2020);
    printf("volume level = %i", volume_level);
  }

  else if (volume_level == 8) {
    // write_to_decoder(SCI_VOL, 0x1010);
    printf("volume level = %i", volume_level);
  }
  vTaskDelay(1000);
}

void Volume_Control(void *p) {
  bool left_volume_s = false;
  bool right_volume_s = false;
  while (1) {
    vTaskDelay(100);
    if (gpio__get(Volume_up)) {
      while (gpio__get(Volume_up)) {
        vTaskDelay(1);
      }
      left_volume_s = true;
    } else if (gpio__get(Volume_down)) {
      while (gpio__get(Volume_down)) {
        vTaskDelay(1);
      }
      right_volume_s = true;
    }

    if (left_volume_s) {
      volume_C(true, false);
      left_volume_s = false;
    } else if (right_volume_s) {
      volume_C(false, false);
      right_volume_s = false;
    }
  }
}
