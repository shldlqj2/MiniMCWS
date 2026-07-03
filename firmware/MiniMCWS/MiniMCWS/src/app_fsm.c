/*
 * app_fsm.c
 *
 * Created: 2026-05-20 오후 12:29:42
 *  Author: shldl
 */ 
#include <app_fsm.h>
#include <hw_sensor.h>
#include <hw_switch.h>
#include <hw_sensor.h>
#include <hw_uart.h>
#include <hw_timer.h>
#include <hw_output.h>
#include <stdbool.h>
#include <stdint.h>

static FSMState_t currentState = FSMState_SAFE;
static uint32_t state_entered_tick = 0; //Fire상태 1초 확인하거나 어찌됐든 진입 시점 보기 위해

void FSM_init(void){
	currentState=FSMState_SAFE;
	state_entered_tick=get_timer_tick();
}

static void check_faults(void){
	if(currentState==FSMState_SAFE_LOCK) return;
	uint8_t injected = uart_get_injected_faults();
	
	bool temp_fault = sensor_isTempFault() || (injected & 0x01);
	bool light_fault = sensor_isLightFault()|| (injected & 0x02);
	
	
	uint32_t current_tick=get_timer_tick();
	bool comm_fault = (current_tick - uart_get_last_heartbeat_tick())>=20; //(200ms 무응답시 에러)
	//bool comm_fault=false;
	
	if (temp_fault || light_fault || comm_fault){
		currentState=FSMState_SAFE_LOCK;
		state_entered_tick=get_timer_tick();
	}
}

void FSM_update(void){
	uint32_t current_tick = get_timer_tick();
	
	check_faults();
	
	switch (currentState){
		case FSMState_SAFE:
			fnd_set(0x00);
			led_set(0x00);
			buzzer_set(false);
		
			if (switch_getPress(1)==PRESS_SHORT){		
				switch_clearPress(1);//이후 armed관련 로직 실행 후 문제 없으면 현재 상태 armed로 변경
				switch_clearPress(2);//이전에 sw2눌렀을때 값이 기억되고 있는 문제가 있어 사용 armed로 전환하자마자 fire되는 문제
				currentState=FSMState_ARMED;
				state_entered_tick=get_timer_tick();
			}
			break;
		case FSMState_ARMED:
			fnd_set(0x01);
			led_set(0xFF);
			buzzer_set(false);
			if (switch_getPress(2)==PRESS_SHORT){
				switch_clearPress(2);
				currentState=FSMState_FIRE;
				state_entered_tick=get_timer_tick();
			}else if (switch_getPress(1)==PRESS_LONG){
				switch_clearPress(1);
				currentState=FSMState_SAFE;
				state_entered_tick=get_timer_tick();
			}
			break;
		case FSMState_FIRE:
			//fire된지 1초 지났으면 다시 ARMED로 전환
			fnd_set(0x02);
			if((current_tick-state_entered_tick)%20 < 10){
				led_set(0xFF);
			}else{
				led_set(0x00);
			}
			buzzer_set(true);
			if ((current_tick - state_entered_tick) >= 100) { // 100틱 = 1000ms = 1초
				currentState = FSMState_ARMED;
				state_entered_tick = current_tick;
				/*
				fire도중 sw누름으로 인해 저장된 상태 초기화(안하면 연속발사가 되거나, 바로 safe상태로 돌아가는 문제 발생)
				*/
				switch_clearPress(1);
				switch_clearPress(2);
			}
			break;
		case FSMState_SAFE_LOCK:
			//스위치 입력 받은 뒤 센서,통신 점검 후 SAFE전환
			fnd_set(0x99);
			led_set(0x00);
			
			uint32_t elapsed = current_tick - state_entered_tick;
			if (elapsed < 60) {
				if ((elapsed % 20) < 10) buzzer_set(true);
				else buzzer_set(false);
				} else {
				buzzer_set(false);
			}

			if (switch_getPress(1) == PRESS_LONG) {
				switch_clearPress(1);
				
				bool temp_fault = sensor_isTempFault();
				bool light_fault = sensor_isLightFault();
				bool comm_timeout = (current_tick - uart_get_last_heartbeat_tick()) >= 20;
				//bool comm_timeout=false;

				if (!temp_fault && !comm_timeout && !light_fault) {
					currentState = FSMState_SAFE;
					state_entered_tick = current_tick;
					
				}
			}
			break;
		default:
			currentState=FSMState_SAFE;
			state_entered_tick=get_timer_tick();
			break;
	}
}
FSMState_t FSM_getState(void){
	return currentState;
}