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

void sleep(void *p);

int main(void) {
  // Adds the ability for CLI commands
  sj2_cli__init();
  printf("Hello\n");

  xTaskCreate(sleep, "sleep", 500, NULL, 0, NULL);

  vTaskStartScheduler();
  return 0;
}

void sleep(void *p) {
  while (1) {
    // do nothing then sleep
    vTaskDelay(1000);
  }
}