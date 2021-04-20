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
#include <string.h>

typedef char songname_t[32];
typedef char songbyte_t[512];
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

void mp3_reader_task(void *p);
void mp3_player_task(void *p);

int main(void) {
  // Adds the ability for CLI commands
  sj2_cli__init();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(2, sizeof(songbyte_t));

  xTaskCreate(mp3_reader_task, "mp3_reader", 1024, NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "mp3_player", 1024, NULL, PRIORITY_LOW, NULL);

  vTaskStartScheduler();
  return 0;
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
      for (int i = 0; i < 512; i++) {
        printf("%x", chunk[i]);
      }
    }
  }
}