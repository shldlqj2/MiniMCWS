/*
 * timer.h
 *
 * Created: 2026-05-19 오전 11:22:40
 *  Author: shldl
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <stdbool.h>

void timer1_init(void);
uint32_t get_timer_tick(void);
bool TIMER_HasTickElapsed(void);


#endif /* TIMER_H_ */