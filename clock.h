#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

void tick(void);
uint32_t get_ticks(void);
void systick_init(void);
void systick_deinit(void);
uint32_t clock_get_apb1(void);
uint32_t clock_get_apb2(void);
uint32_t clock_get_ahb(void);

void clock_setup(void);
void clock_delay(int t);

#endif 
