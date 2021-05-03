#pragma once

#include <stdint.h>
#include <stdlib.h>

void ssp2__init(uint32_t max_clock_mhz);
uint8_t new_ssp2__exchange_byte_owner(uint8_t data_out);
uint8_t ssp2__exchange_byte(uint8_t data_out);
void ssp2__initialize(uint32_t max_clock_khz);
// my own functions
void clear_bits_setup(void);
void ssp2_power_on(void);
void ssp_enable(void);
// functions for part 1
void adesto_cs(void);
void adesto_ds(void);