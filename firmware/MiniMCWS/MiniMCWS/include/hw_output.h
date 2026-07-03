/*
 * hw_output.h
 *
 * Created: 2026-07-02 오후 5:04:04
 *  Author: shldl
 */ 


#ifndef HW_OUTPUT_H_
#define HW_OUTPUT_H_

#include <stdint.h>
#include <stdbool.h>

void output_init(void);
void led_set(uint8_t val);
void led_toggle(void);
void fnd_set(uint8_t val);
void fnd_update(void);
void buzzer_set(bool on);

#endif /* HW_OUTPUT_H_ */