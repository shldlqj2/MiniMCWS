/*
 * hw_sensor.c
 *
 * Created: 2026-05-31 오후 4:37:38
 *  Author: shldl
 */ 
#include <avr/io.h>
#include <hw_timer.h>
#include <stdint.h>
#include <stdbool.h>
#include <hw_sensor.h>

#define F_CPU 16000000UL
#define F_SCK 40000UL

#define ATS75_ADDR_W   0x98		//ats75의 slave 주소 = 1001100, i2c 프로토콜에서는 이 7비트를 왼쪽으로 shift해서 8비트로 전송한다
#define ATS75_ADDR_R   0x99		//ats75의 주소 | 0 = Write, | 1 = Read이다. 따라서 0x98=write, 0x99=read
#define ATS75_REG_TEMP    0x00	//ast75의 Command Register 0x00=Temp(온도값 저장), 0x01=Config(해상도/모드 설정)
#define ATS75_REG_CONFIG  0x01

#define SENSOR_TWI_TIMEOUT_TICKS 2


/*
	병렬처리를 위해서 상태정의
	병렬처리를 왜 구현하는가? 원래 twi는 while문을 통해서 twi값 반환을 기다려야함
	물론 보통은 매우 짧은 시간이니까 거의 실시간 일거라고 생각은 됨
	그러나, 문제가 발생해서 while문에 오래 갇혀있을 수 있음(물론 timeout을 넣어도 되긴 할테지만)
*/

typedef enum{
	TEMP_PHASE_START,
	TEMP_PHASE_WAIT_START,
	TEMP_PHASE_WAIT_ADDR_W,
	TEMP_PHASE_WAIT_REG,
	TEMP_PHASE_WAIT_RESTART,
	TEMP_PHASE_WAIT_ADDR_R,
	TEMP_PHASE_WAIT_HIGH,
	TEMP_PHASE_WAIT_LOW
	}TempState_t;
	
typedef enum{
	LIGHT_PHASE_START,
	LIGHT_PHASE_WAIT_START
	}LightState_t;

static TempState_t temp_state=TEMP_PHASE_START;
static LightState_t light_state=LIGHT_PHASE_START;

static uint32_t temp_phase_tick=0;
static uint32_t light_phase_tick=0;

static int16_t cached_temp=0;
static uint16_t cached_light=1023;

static bool temp_timeout_fault=false;
static bool light_timeout_fault=false;

static void _twi_init(void);
static void _adc_init(void);

static void _twi_issue_start(void);
static void _twi_issue_stop(void);
static void _twi_issue_write(uint8_t data);
static void _twi_issue_read_ack(void);
static void _twi_issue_read_nack(void);

static bool _twi_ready(void);
static bool _adc_ready(void);

static bool _temp_phase_timed_out(void);
static bool _light_phase_timed_out(void);


static void _twi_temp_reset(void);

void sensor_init(void){
	_twi_init();
	_adc_init();
	
	_twi_issue_start();
	temp_state=TEMP_PHASE_START;
	temp_phase_tick=get_timer_tick();
	
	ADCSRA |= (1<<ADSC);
	light_state=LIGHT_PHASE_START;
	light_phase_tick=get_timer_tick();
	
}

void sensor_temp_update(void){
	uint32_t now = get_timer_tick();
	switch(temp_state){
		case TEMP_PHASE_START:
			_twi_issue_start();
			temp_state=TEMP_PHASE_WAIT_START;
			temp_phase_tick=now;
			break;
		case TEMP_PHASE_WAIT_START:
			if(_twi_ready()){
				_twi_issue_write(ATS75_ADDR_W);
				temp_state=TEMP_PHASE_WAIT_ADDR_W;
				temp_phase_tick=now;
			}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;
		case TEMP_PHASE_WAIT_ADDR_W:
			if(_twi_ready()){
				_twi_issue_write(ATS75_REG_TEMP);
				temp_state=TEMP_PHASE_WAIT_REG;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;
		case TEMP_PHASE_WAIT_REG:
			if(_twi_ready()){
				_twi_issue_start();
				temp_state=TEMP_PHASE_WAIT_RESTART;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;	
		case TEMP_PHASE_WAIT_RESTART:
			if(_twi_ready()){
				_twi_issue_write(ATS75_ADDR_R);
				temp_state=TEMP_PHASE_WAIT_ADDR_R;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;	
		case TEMP_PHASE_WAIT_ADDR_R:
			if(_twi_ready()){
				_twi_issue_read_ack();
				temp_state=TEMP_PHASE_WAIT_HIGH;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;
		case TEMP_PHASE_WAIT_HIGH:
			if(_twi_ready()){
				cached_temp = (int16_t)(int8_t)TWDR;
				_twi_issue_read_nack();
				temp_state=TEMP_PHASE_WAIT_LOW;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;
		case TEMP_PHASE_WAIT_LOW:
			if(_twi_ready()){
				_twi_issue_stop();
				temp_timeout_fault=false;
				temp_state=TEMP_PHASE_START;
				temp_phase_tick=now;
				}else if(_temp_phase_timed_out()){
				temp_timeout_fault=true;
				_twi_temp_reset();
				temp_state=TEMP_PHASE_START;
			}
			break;
	}
	
}


void sensor_light_update(void){
	uint32_t now=get_timer_tick();
	
	switch(light_state){
		case LIGHT_PHASE_START:
			ADCSRA |= (1<<ADSC);

			light_state=LIGHT_PHASE_WAIT_START;
			light_phase_tick=now;
			break;
		case  LIGHT_PHASE_WAIT_START:
			if(_adc_ready()){
				uint8_t low = ADCL;
				uint8_t high = ADCH;
				cached_light=((uint16_t)high << 8) | low;
				
				light_timeout_fault=false;
				light_state=LIGHT_PHASE_START;
				light_phase_tick=now;
				
			}else if(_light_phase_timed_out()){
				light_timeout_fault=true;
				light_state=LIGHT_PHASE_START;
			}
			break;
	}
}

int16_t sensor_getTemp(void) { return cached_temp; }
bool sensor_isTempFault(void) { return temp_timeout_fault || (cached_temp >= TEMP_FAULT_THRESHOLD); }
	
uint16_t sensor_getLight(void) { return cached_light; }
bool sensor_isLightFault(void) { return light_timeout_fault || (cached_light <= LIGHT_FAULT_THRESHOLD); }
	
static void _twi_init(void){
	PORTD |= 3;
	SFIOR &= ~(1<<PUD);
	
	TWBR=(uint16_t)((F_CPU / F_SCK - 16)/2);
	TWSR&=0xFC;
	TWCR=(1<<TWEN);
}
/*
	처음에는 twi_reset을 그냥 TWCR을 강제 종료 후 키려고 했다.
	근데 이렇게하면 I2C 버스의 lock-up이 발생할 수 있다고 한다.
	타임아웃이 발생하는 가장 흔한 원인은
	슬레이브가 0을 전송하던 찰나에 마스터가 통신을 중단해버리는 경우이다.
	센서(슬레이브)는 마스터가 다음 SCL클럭을 줄 때까지 SDA 선을 LOW로 유지한다.
	이 상태에서 TWCR을 리셋한다고 해도 SDA는 LOW상태로 유지 되어 버린다.
	그렇기 때문에 SCL핀을 수동으로 펄스를 내보내 센서를 강제로 밀어내야한다.
	
	그럼 어떻게 이걸 처리할 것이냐?
	PD0에 신호를 최대 9번 주는거다
	왜 9번이냐? I2C통신은 8bit 데이터 + 1bit대답(ACK/NACK) 총 9개의 bit로 이루어진다.
	근데 timeout이 발생한다면 아마 마지막 ACK/NACK를 받기 전에 끝난상태로 유지될 것이다.
	그러니 나머지 값들을 임의로 넣어서 해결해줘야한다.
	매번 값을 넣기전에 SDA의 값이 High(다음 통신 받을 준비가 되면)인지 확인하고 High면 초기화 하는거다.
*/
static void _twi_temp_reset(void){
	TWCR=0; //하드웨어 강제 종료(i2c)
	DDRD |= (1 << PD0);
	DDRD &= ~(1 << PD1);
	
	for (int i=0; i<9; i++){
		if(PIND & (1<<PD1)){
			break;	//센서의 SDA가 HIGH면 즉시 탈출
		}
		PORTD &= ~(1<<PD0);
		for(volatile int j =0; j<9; j++){} //약 4us의 아주 짧은 딜레이 (SCL을 LOW로 만든게 적용될때 까지 대기하기위함)
		
		PORTD |= (1<<PD0);
		for(volatile int j =0; j<9; j++){} //약 4us의 아주 짧은 딜레이 (SCL을 HIGH로 만든게 적용될때 까지 대기하기위함)
		
	}
	
	_twi_init();
}

static void _adc_init(void){
	ADMUX=0x00;
	ADCSRA=0x87;
}

static void _twi_issue_start(void){
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); 
}

static void _twi_issue_stop(void) {
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

static void _twi_issue_write(uint8_t data) {
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
}

static void _twi_issue_read_ack(void) {
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
}

static void _twi_issue_read_nack(void) {
	TWCR = (1 << TWINT) | (1 << TWEN);
}

static bool _twi_ready(void) {
	return (TWCR & (1 << TWINT)) != 0;
}

static bool _adc_ready(void) {
	return (ADCSRA & (1 << ADSC)) == 0;
}

static bool _temp_phase_timed_out(void) {
	return (get_timer_tick() - temp_phase_tick) >= SENSOR_TWI_TIMEOUT_TICKS;
}

static bool _light_phase_timed_out(void) {
	return (get_timer_tick() - light_phase_tick) >= SENSOR_TWI_TIMEOUT_TICKS;
}