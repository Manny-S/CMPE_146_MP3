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

#include "event_groups.h"
#include "ff.h"
#include "ssp2.h"
#include <string.h>

typedef char songname_t[32];
typedef char songbyte_t[512];
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

// static const uint32_t play_pause = (1 << 19);
// TaskHandle_t MP3PlayPause = NULL;

const uint8_t READ = 0x03;
const uint8_t WRITE = 0x02;
const uint8_t SCI_MODE = 0x00;
const uint8_t SCI_STATUS = 0x1;
const uint8_t SCI_VOL = 0xB;
const uint8_t DUMB = 0xFF;

gpio_s CS;
gpio_s DREQ; // Data Request input
gpio_s RST;
gpio_s XDCS;

void mp3_reader_task(void *p);
void mp3_player_task(void *p);
void read_test(void);
void write_test(void);

int main(void) {
  // Adds the ability for CLI commands
  sj2_cli__init();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(2, sizeof(songbyte_t));

  CS = gpio__construct_as_output(4, 28);
  DREQ = gpio__construct_as_input(0, 6);
  RST = gpio__construct_as_output(0, 8);
  XDCS = gpio__construct_as_output(0, 26);

  gpio__reset(RST);
  gpio__set(RST);

  gpio__set(CS);
  gpio__set(XDCS);

  ssp2__initialize(1000);

  xTaskCreate(mp3_reader_task, "mp3_reader", 1024, NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "mp3_player", 1024, NULL, PRIORITY_LOW, NULL);
  // write_test();
  read_test();
  vTaskStartScheduler();
  return 0;
}
void write_test(void) {
  uint8_t byte1 = 0x48;
  uint8_t byte2 = 0x00;
  gpio__reset(CS);
  ssp2__exchange_byte(WRITE);
  ssp2__exchange_byte(SCI_MODE);
  ssp2__exchange_byte(byte1);
  ssp2__exchange_byte(byte2);
  gpio__set(CS);

  printf("WRITE: %x %x\n", byte1, byte2);
}

void read_test(void) {
  uint8_t byte1 = 0xFF;
  uint8_t byte2 = 0xFF;
  gpio__reset(CS);
  ssp2__exchange_byte(READ);
  ssp2__exchange_byte(SCI_MODE);
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
      for (int i = 0; i < 512;) {
        while (!gpio__get(DREQ)) {
          vTaskDelay(1);
        }
        gpio__reset(XDCS);
        for (int j = 0; j < 32;) {
          ssp2__exchange_byte(chunk[i + j]);
          printf("%x", chunk[i + j]);
          j++;
        }
        gpio__set(XDCS);
        i = i + 32;
      }
    }
  }
}

// void play_pause_button(void *p) {
//   bool play_status = false;
//   uint8_t alternative_status = 1;
//   while (1) {
//     vTaskDelay(100);
//     if (gpio1__get_level(play_pause)) {
//       while (gpio1__get_level(play_pause)) {
//         vTaskDelay(1);
//       }
//       play_status = true;
//     } else {
//       play_status = false;
//     }

//     if (play_status) {
//       if (alternative_status) {
//         vTaskResume(MP3PlayPause);
//         alternative_status--;
//       } else {
//         vTaskSuspend(MP3PlayPause);
//         alternative_status--;
//       }
//     }
//     vTaskDelay(1);
//   }
// }
