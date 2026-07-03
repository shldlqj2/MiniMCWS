/*
 * hw_switch.h
 *
 * Created: 2026-05-30 오후 5:14:06
 *  Author: shldl
 */ 


#ifndef HW_SWITCH_H_
#define HW_SWITCH_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum{
	PRESS_NONE,
	PRESS_SHORT,
	PRESS_LONG
} SWPress_t;
void switch_init(void);
void switch_update(void);
SWPress_t switch_getPress(uint8_t sw);
void switch_clearPress(uint8_t sw);



#endif /* HW_SWITCH_H_ */