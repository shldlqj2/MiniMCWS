/*
 * timer.c
 *
 * Created: 2026-05-19 오전 11:22:49
 *  Author: shldl
 */ 
#include <hw_timer.h>
#include <avr/io.h>
#include <avr/iom128.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#define F_CPU = 16000000L

static volatile uint32_t count=0;
bool tickFlag=false;

ISR(TIMER1_COMPA_vect){
	count+=1;
	tickFlag=true;
}

void timer1_init(void){
	TCCR1B |= (1<<WGM12) | (1<<CS11) | (1<<CS10);	//64분주
	OCR1A = 2499;									//1초에 250000번 cnt -> 10ms는 1/100초이므로 2500번 cnt시 10ms 단, 0부터 시작이므로 2499
	TIMSK |= (1 << OCIE1A);
	sei();
}

uint32_t get_timer_tick(void){
	return count;
}

bool TIMER_HasTickElapsed(void){
	if (tickFlag){
		tickFlag=false;
		return true;
	}
	return false;
}