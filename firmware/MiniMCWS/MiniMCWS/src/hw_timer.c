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

static volatile uint32_t count=0;
bool g_tick_flag=false;

ISR(TIMER1_COMPA_vect){
	count+=1;
	g_tick_flag=true;
}

void timer1_init(void){
	TCCR1B |= (1<<WGM12) | (1<<CS11) | (1<<CS10);
	OCR1A = 2499;
	TIMSK |= (1 << OCIE1A);
	sei();
}

uint32_t get_timer_tick(void){
	return count;
}

bool TIMER_HasTickElapsed(void){
	if (g_tick_flag){
		g_tick_flag=false;
		return true;
	}
	return false;
}