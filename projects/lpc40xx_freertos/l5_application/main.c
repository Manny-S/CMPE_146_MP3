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
QueueHandle_t Q_songname;

void mp3_reader_task(void *p);

int main(void) {
  // Adds the ability for CLI commands
  sj2_cli__init();

  Q_songname = xQueueCreate(1, sizeof(songname_t));

  xTaskCreate(mp3_reader_task, "mp3_reader", 1024, NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler();
  return 0;
}

void mp3_reader_task(void *p) {
  songname_t name;
  while (1) {
    if (xQueueReceive(Q_songname, &name[0], portMAX_DELAY)) {
      printf("Received song to play: %s\n", name);
    }
  }
}
