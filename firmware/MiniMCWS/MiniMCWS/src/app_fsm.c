/*
 * app_fsm.c
 *
 * Created: 2026-05-20 오후 12:29:42
 *  Author: shldl
 */ 
#include <app_fsm.h>

FSMState_t currentState;

void FSM_init(void){
	currentState=FSMState_SAFE;
}
void FSM_update(FSMState_t state){
	currentState=state;
}
FSMState_t get_FSM_State(void){
	return currentState;
}