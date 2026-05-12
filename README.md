# MiniMCWS

---

#### 프로젝트 명: Mini-MCWS

##### 개발 보드: JKIT-128-1 (ATmega128-16AU @ 16 MHz)

##### 개발 언어: 임베디드 C, 지상관제/시험: Python

---

#### 목표

JKIT-128-1 보드에 내장된 LED, 스위치, FND, 부저, 온도센서, 광센서, UART를 이용하여

- 무장/센서 제어를 모델링한 FSM 모사
- 타이머 기반 실시간 제어 루프 & Fail-Safe로직
- UART통신 무결성(패킷 + CRC/패리티)
- Python 기반 간이 SIL + Fault Injection & 로깅
  을 구현해 보고자 한다.

---

#### 기능 요구사항

##### FSM

1. 시스템은 SAFE/SAFE_LOCK/ARMED/FIRE 4가지 상태를 가진다.
2. 전원 인가 후 시스템은 초기 SAFE 상태를 가진다.
3. Safe 상태에서는 FND 에 0x00을 표기하고, LED를 전부 끈다.
4. SafeLock 상태에서는 FND에 0x99를 표기하고, LED를 전부 끈다. 짧은 Beep음을 3번 연속해서 울린다.
5. Armed 상태에서는 FND에 0x01을 표기하고, Armed와 연관된 LED를 켠다.
6. Fire 상태에서는 FND에 0x02를 표기하고 Fire와 연관된 LED를 점멸한다. 또한 Buzzer를 울린다.
7. Fire 상태에서는 1초 경과 후 Armed 상태로 돌아간다.
8. Safe상태에서 Sw1을 단클릭(1000ms미만)해 Armed 상태로 넘어간다.
9. Armed상태에서 Sw1을 장클릭(1000ms이상)해 Safe 상태로 넘어간다.
10. SafeLock 상태에서는 Sw1을 장클릭해 센서/타임아웃 검사 후 Safe로 복귀한다.
11. Armed 상태에서 Sw2를 통해 Fire 상태로 전환한다.
12. 온도 임계값(50℃) 초과 시 SAFE_LOCK 상태로 변경되어야 한다.
13. 통신 타임아웃(200ms) 시 SAFE_LOCK 상태로 변경되어야 한다.
14. FSM은 어떤 상태에서도 센서/통신 Fault 발생 시 즉시 SAFE_LOCK 상태로 전이해야 한다. (SAFE_LOCK 우선)
15. 모든 과정은 Timer Tick(10ms 주기로 동작한다.)

##### 통신

1. 통신은 CRC-8과 Parity를 이용해 무결성을 검증한다.
2. 패킷은 MIL-STD-1553 구조를 모사한 UART 프로토콜을 사용한다.
3. GCS는 20ms 주기로 Heartbeat 패킷을 전송해야 한다.
4. MCU는 마지막 정상 Heartbeat 수신 후 200ms 이상 경과하면 통신 타임아웃으로 판단한다.
5. CRC-8은 다항식 $$x⁸ + x² + x + 1(0x07)$$을 사용한다.

##### Python GCS

1. 통신내용을 자동 로깅한다.
2. CRC오류 패킷을 보내 MCU에서 무시하는 지 확인한다.
3. 임의로 센서 Fault 상태를 입력해 비정상 센서 상태를 주입한다.
4. 일정시간 HeartBeat전송을 일시 중단해 MCU에서 Timeout후 SafeLock상태로 진입하도록 한다.
5. GCS는 timestamp, mode, temp, light, error_code, crc_ok, packet_seq를 CSV로 로깅해야 한다.
6. MCU로부터 수신한 유효 패킷마다 1행씩 로깅해야 한다.
7. GCS는 콘솔 메뉴를 통해 CRC Fault, 센서 Fault, 통신 단절 시나리오를 선택하여 실행할 수 있어야 한다.
8. GCS는 Fault Injection 시 SAFE_LOCK 상태 진입까지의 시간을 ms 단위로 계산해 CSV에 기록해야 한다.

##### 비기능 요구사항

1. SAFE_LOCK 진입은 Fault 감지 후 40ms 이내에 완료되어야 한다.
2. 패킷 손실률은 1% 미만이어야 한다.
3. 제어루프는 10ms 주기로 실행되어야하며, 오차는 ±1ms 이내여야 한다.
