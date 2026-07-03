/*
 * hw_output.c
 *
 * Created: 2026-07-02 오후 5:04:04
 *  Author: shldl
 */ 
#include "hw_output.h"
#include <avr/io.h>

static const uint8_t fnd_font[] = {
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x27, 0x7F, 0x67,
	0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
};

static uint8_t fnd_digits[4] = {0, 0, 0, 0};
static uint8_t fnd_idx = 0;


void output_init(void){
	//LED 초기화
	DDRA = 0xFF;
	PORTA = 0x00;
	
	//FND 초기화
	DDRC=0xFF;
	DDRG |= 0x0F;
	PORTC = 0x00;
	PORTG &= ~0x0F;
	
	DDRB |= (1<<PB4);
	PORTB &= ~(1<<PB4);
	
	TCCR0 = (1 << WGM01) | (1 << WGM00) | (1 << COM01) | (1 << CS02);
	OCR0 = 0;
}
void led_set(uint8_t val){
	PORTA=val;
}
void led_toggle(void){
	PORTA ^= 0xFF; //xor로 전부 반전
}
void fnd_set(uint8_t val){
	fnd_digits[0] = val & 0x0F;
	fnd_digits[1] = (val >> 4) & 0x0F;
	fnd_digits[2] = 0;
	fnd_digits[3] = 0;
}
void fnd_update(void){
	PORTG &= ~0x0F;
	PORTC = fnd_font[fnd_digits[fnd_idx]];
	PORTG |= (1<<fnd_idx);
	fnd_idx++;
	if (fnd_idx>1){
		fnd_idx=0;
	}
	
}
void buzzer_set(bool on){
	if (on) {

		TCCR0 |= (1 << COM01);
		OCR0 = 128;
		
		} else {
		TCCR0 &= ~(1 << COM01);
		PORTB &= ~(1 << PB4);
	}
}