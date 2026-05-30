/*
 * app_fsm.h
 *
 * Created: 2026-05-19 오후 12:02:11
 *  Author: shldl
 */ 


#ifndef APP_FSM_H_
#define APP_FSM_H_

typedef enum{
	FSMState_SAFE,
	FSMState_ARMED,
	FSMState_FIRE,
	FSMState_SAFE_LOCK
	
	} FSMState_t;

void FSM_init(void);
void FSM_update(void);
FSMState_t get_FSM_State(FSMState_t state);

#endif /* APP_FSM_H_ */