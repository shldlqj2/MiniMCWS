/*
 * hw_sensor.h
 *
 * Created: 2026-05-20 오후 12:44:12
 *  Author: shldl
 */ 


#ifndef HW_SENSOR_H_
#define HW_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>

#define TEMP_FAULT_THRESHOLD    50		//온도 기준 50도

/*
원래 빛이 너무 밝으면 에러가 나게하려했는데 jkit-128-1에 들어간 cds 센서가 너무 민감해서(저항이 낮아서) (실내 형광등에도 거의 최대치에 가까운 값) 어두울때 에러 발생하도록 변경
*/

#define LIGHT_FAULT_THRESHOLD   500		//광량 기준 500(너무 어두움)

void sensor_init(void);

void sensor_temp_update(void);			//온도 측정 상태 진행
void sensor_light_update(void);			//조도 측정 상태 진행

int16_t sensor_getTemp(void);
bool sensor_isTempFault(void);

uint16_t sensor_getLight(void);
bool sensor_isLightFault(void);

#endif /* HW_SENSOR_H_ */