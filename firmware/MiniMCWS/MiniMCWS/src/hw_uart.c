/*
 * hw_uart.c
 *
 * Created: 2026-07-02 오후 6:05:57
 *  Author: shldl
 */ 

/*
실제로는 전투기라면 uart 통신이 아닌 무선 통신을 하겠지만, 현재 상황에서는 (jkit-128-1 보드) uart로 대체한다.
어디까지나 학습성 프로젝트의 일환이므로 이대로 진행한다.
*/


#include "hw_uart.h"
#include "hw_timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define BAUD 38400
#define UBRR_VAL ((F_CPU/16/BAUD)-1)

#define SYNC_BYTE 0xAA
#define FRAME_HEARTBEAT 0x01
#define FRAME_TELEMETRY 0x02

#define RX_STATE_SYNC 0
#define RX_STATE_TYPE 1
#define RX_STATE_SEQ 2
#define RX_STATE_CMD 3
#define RX_STATE_CRC 4

static volatile uint32_t last_heartbeat_tick = 0;
static volatile bool new_packet_received = false;

static uint8_t rx_state = RX_STATE_SYNC;
static uint8_t rx_buf[5];
static uint8_t injected_faults = 0;



//crc8을 통해 에러 검출

static uint8_t _calc_crc8(uint8_t *data, uint8_t len) {
	uint8_t crc = 0x00;
	for (uint8_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x80) {
				crc = (crc << 1) ^ 0x07;
				} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

void uart_init(void) {
	/*
	통신 속도 설정.
	atmega128은 8bit이므로 16비트짜리 큰 데이터를 2개에 나누어 담는 데이터 쪼개기 과정이다.
	예를들어 0x1234라면 다음과 같은 코드를 사용하면
	UBRR0H에는 8비트 오른쪽 시프트되어 34가 버려진 0x12가 들어가고
	UBRR0L에는 0x34(하위8bit)만 들어가게 된다.
	*/
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)UBRR_VAL;
	
	/*
	송수신 설정
	RXEN(수신) TXEN(송신) RXCIE(송수신 인터럽트)
	*/
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	
	/*
	ATmega128 데이터 시트에 따르면
	UPM01 =1, UPM00=1 로주면 짝수 패리티 사용이다.
	즉, 8개의 비트를 송신할때 1의 개수가 짝수개가 되도록 꼬리에 비트를 하나 더 붙인다는 의미
	UCSZ는 00~02까지 있는데 0 1 1은 8비트의 데이터를 사용하겠다는 의미
	*/
	UCSR0C = (1 << UPM01) | (0 << UPM00) | (1 << UCSZ01) | (1 << UCSZ00);
}

/*
RTOS환경임에도 송신부에 while을 사용한 이유는 다음과 같다.
1. 통신속도 115200bps의 속도에서 8bit를 송신하는데에는 86us정도 밖에 걸리지 않는다.
통신에 사용할 10바이트를 전부 보내는데는 약 1ms 밖에 걸리지 않는다.
2. 무한루프에 걸릴 가능성이 없다.
uart는 일방향성 통신이므로 그냥 전부 보내고 끝이다 (Ack를 받지 않음)
따라서 그냥 데이터를 보내고 알아서 flag를 1로 만들기 때문에 무한루프에 걸리는 일은 없다.
*/
void uart_putc(uint8_t c) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void uart_puts(const char* str) {
	while (*str) {
		uart_putc(*str++);
	}
}

/*
fsm상태, 온도, 조도, 에러코드, 패킷 번호,
*/
void uart_send_packet(uint8_t mode, int16_t temp, uint16_t light, uint8_t error_code, uint8_t seq) {
	uint8_t packet[10];

	packet[0] = SYNC_BYTE;
	packet[1] = FRAME_TELEMETRY;
	packet[2] = seq;
	packet[3] = mode;
	
	packet[4] = (uint8_t)(temp >> 8);
	packet[5] = (uint8_t)(temp & 0xFF);
	
	packet[6] = (uint8_t)(light >> 8);
	packet[7] = (uint8_t)(light & 0xFF);
	
	packet[8] = error_code;
	
	packet[9] = _calc_crc8(packet, 9);



	for (uint8_t i = 0; i < 10; i++) {
		uart_putc(packet[i]);
	}
}

ISR(USART0_RX_vect) {
	uint8_t status = UCSR0A;
	uint8_t data = UDR0;
	
	// 패리티 에러 확인 (통신 중 노이즈로 데이터 깨짐)
	if (status & (1 << UPE0)) {
		rx_state = RX_STATE_SYNC;
		return;
	}
	
	// 상태 기계: 한 단계씩 넘어간다
	switch (rx_state) {
		case RX_STATE_SYNC:
		if (data == SYNC_BYTE) {
			rx_buf[0] = data;
			rx_state = RX_STATE_TYPE;
		}
		break;

		case RX_STATE_TYPE:
		rx_buf[1] = data;
		rx_state = RX_STATE_SEQ;
		break;

		case RX_STATE_SEQ:
		rx_buf[2] = data;
		rx_state = RX_STATE_CMD;
		break;

		case RX_STATE_CMD:
		rx_buf[3] = data;
		rx_state = RX_STATE_CRC;
		break;

		case RX_STATE_CRC:
		rx_buf[4] = data;
		
		// 패킷 타입이 하트비트고, 내가 계산한 CRC와 날아온 CRC가 같으면 정상 수신 완료!
		if (rx_buf[1] == FRAME_HEARTBEAT && _calc_crc8(rx_buf, 4) == rx_buf[4]) {
			last_heartbeat_tick = get_timer_tick();  // 현재 시각 업데이트
			injected_faults = rx_buf[3];             // GCS가 보낸 고장 명령 저장
			new_packet_received = true;              // 메인 루프에 알림
		}
		// 패킷 처리가 끝났거나 실패했으니 다시 처음 상태로 돌아감
		rx_state = RX_STATE_SYNC;
		break;

		default:
		rx_state = RX_STATE_SYNC;
		break;
	}
}

void uart_send_telemetry(uint8_t mode, int16_t temp, uint16_t light, uint8_t error_code) {
	static uint8_t seq = 0;
	uart_send_packet(mode, temp, light, error_code, seq++);
}

uint8_t uart_get_injected_faults(void) {
	return injected_faults;
}

bool uart_get_packet_status(void) {
	return new_packet_received;
}

void uart_clear_packet_status(void) {
	new_packet_received = false;
}

uint32_t uart_get_last_heartbeat_tick(void) {
	return last_heartbeat_tick;
}
