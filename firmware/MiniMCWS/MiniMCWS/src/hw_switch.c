/*
 * hw_switch.c
 *
 * Created: 2026-05-30 오후 5:17:59
 *  Author: shldl
 */ 
#include <hw_switch.h>
#include <hw_timer.h>
#include <avr/io.h>
#include <avr/iom128.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

#define LONGPRESS_TICK  100
#define DEBOUNCE_TICK 2

static volatile uint32_t sw1_pressed_tick=0;
static volatile uint32_t sw2_pressed_tick=0;

static volatile bool sw1_active = false;
static volatile bool sw2_active = false;

static SWPress_t sw1_press = PRESS_NONE;
static SWPress_t sw2_press = PRESS_NONE;

void switch_init(void){
	DDRE &= ~((1<<PINE4) | (1<<PINE5));

	EICRB |= (1<<ISC41) | (1<<ISC51);	//엣지트리거 -> sw가 눌렸을때와 떼어졌을 때 모두 감지 (Why? 디바운싱과 몇초 눌렀는 지 확인하기 위해서)
	EIMSK |= (1<<INT4) | (1<<INT5);		//회로도상 sw1은 INT4 sw2는 INT5이다.
}

ISR(INT4_vect){
	if(!sw1_active){
		sw1_pressed_tick=get_timer_tick();
		sw1_active=true;
	}
}

ISR(INT5_vect){
	if(!sw2_active){
		sw2_pressed_tick=get_timer_tick();
		sw2_active=true;
	}
}

void switch_update(void){
	uint32_t curr_tick=get_timer_tick();
	if (sw1_active){
		if (PINE & (1 << PINE4)) { // 스위치를 뗐을 때 (High)
			uint32_t duration = curr_tick - sw1_pressed_tick;
			if (DEBOUNCE_TICK <= duration && duration < LONGPRESS_TICK) {
				sw1_press = PRESS_SHORT;
				} else if (LONGPRESS_TICK <= duration) {
				sw1_press = PRESS_LONG;
			}
			sw1_active = false;
		}
	}
	
	if (sw2_active) {
		if (PINE & (1 << PINE5)) { // 스위치를 뗐을 때 (High)
			uint32_t duration = curr_tick - sw2_pressed_tick;
			if (duration >= DEBOUNCE_TICK) {
				sw2_press = PRESS_SHORT;
			}
			sw2_active = false;
		}
	}
}
SWPress_t switch_getPress(uint8_t sw){
	if (sw==1) return sw1_press;
	else if (sw==2) return sw2_press;
	return PRESS_NONE;
}
void switch_clearPress(uint8_t sw){
	if (sw==1) sw1_press=PRESS_NONE;
	else if (sw==2) sw2_press=PRESS_NONE;
}