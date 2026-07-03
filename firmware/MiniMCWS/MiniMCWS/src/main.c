/*
 * main.c
 *
 * Created: 5/13/2026 8:24:49 PM
 *  Author: shldl
 */ 

#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "hw_timer.h"
#include "hw_switch.h"
#include "hw_sensor.h"
#include "hw_output.h"
#include "hw_uart.h"
#include "app_fsm.h"

int main(void)
{
	//초기화
	cli();
	
	timer1_init();
	switch_init();
	sensor_init();
	output_init();
	uart_init();
	FSM_init();
	
	sei();
	
    while(1)
    {
        if(TIMER_HasTickElapsed()){	//타이머 틱 확인 10ms마다 실행
			sensor_temp_update();
			sensor_light_update();
			switch_update();
			
			FSM_update();
			{
				int16_t temp=sensor_getTemp();
				uint16_t light=sensor_getLight();
				uint16_t error_code=0;
				
				if (sensor_isTempFault()) error_code |= (1<<0);
				if (sensor_isLightFault()) error_code |= (1<<1);
				if (FSM_getState() == FSMState_SAFE_LOCK) error_code |= (1<<2);
				
				uart_send_telemetry((uint8_t)FSM_getState(), temp, light, error_code);
			}
			
			fnd_update();
			
		}
    }
}