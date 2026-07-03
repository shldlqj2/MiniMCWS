import serial      
import time        
import struct      
import csv         
import threading   
import sys         
import datetime  

SYNC_BYTE=0xAA
FRAME_HEARTBEAT = 0x01
FRAME_TELEMETRY = 0x02

def calc_crc8(data):
    crc=0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc=((crc << 1)^0x07) & 0xFF
            else:
                crc= (crc<<1) & 0xFF
    return crc


class GCS:
    def __init__(self, port='COM4', baud=38400):
        try:
            self.ser=serial.Serial(port,baud,parity=serial.PARITY_EVEN, timeout=0.01)
            print(f"{port} 통신 성공, 속도 = {baud}")
        except serial.SerialException:
            print(f"통신 실패. 연결 된 장치 없음")
            self.ser=None
        
        self.running = True
        self.seq=0
        self.cmd=0
        self.inject_crc_error=False
        self.pause_heartbeat=False

        self.csv_file = open(f"gcs_log_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv","w",newline='')
        self.csv_writer=csv.writer(self.csv_file)
        self.csv_writer.writerow(['timestamp','mode','temp','light','error_code','packet_seq','safe_lock_ms'])

        self.fault_injection_time=None
        self.fault_injection_active=False

        self.tx_thread=threading.Thread(target=self.tx_loop)
        self.rx_thread = threading.Thread(target=self.rx_loop)
        self.tx_thread.start() 
        self.rx_thread.start() 

    def tx_loop(self):
        while self.running:
            if not self.pause_heartbeat:
                packet = bytearray([SYNC_BYTE, FRAME_HEARTBEAT, self.seq, self.cmd])
                crc = calc_crc8(packet) 
                
                #CRC에러 주입시
                if self.inject_crc_error:
                    crc ^= 0xFF  
                
                
                packet.append(crc) 
                
                if self.ser:
                    self.ser.write(packet) 
                self.seq = (self.seq + 1) & 0xFF 
            
            time.sleep(0.02)
    
    def rx_loop(self):
        buf = bytearray() 
        while self.running:
            if self.ser:
                data = self.ser.read(max(1, self.ser.in_waiting))
                if data:
                    buf.extend(data)
                    while len(buf) >= 10:
                        if buf[0] == SYNC_BYTE: 
                            packet = buf[:10] 
                            calc_crc = calc_crc8(packet[:9]) 
                            
                            if packet[1] == FRAME_TELEMETRY and calc_crc == packet[9]:
                                seq = packet[2]
                                mode = packet[3]
                                temp = struct.unpack(">h", packet[4:6])[0]  
                                light = struct.unpack(">H", packet[6:8])[0] 
                                err = packet[8]
                                crc_ok = 1
                                
                                ts = datetime.datetime.now().isoformat()
                                safe_lock_ms = ""
                                self.csv_file.flush()
                                
                                if self.fault_injection_active and mode == 3:
                                    elapsed = time.time() - self.fault_injection_time
                                    print(f"\n에러 injection방어 성공! 걸린 시간: {elapsed*1000:.2f} ms")
                                    safe_lock_ms = f"{elapsed*1000:.2f}"
                                    self.fault_injection_active = False
                                
                                self.csv_writer.writerow([ts, mode, temp, light, err, seq, safe_lock_ms])
                                
                            else:
                                print(f"\nCRC에러 발생") 
                            
                            buf = buf[10:]
                        else:

                            buf.pop(0)
            else:
                time.sleep(0.01)

    def menu(self):
        while self.running:
            print("\n--- 통신 메뉴 ---")
            print("1. CRC 에러 주입")
            print("2. 온도 센서 에러 주입")
            print("3. 통신 중지 주입(HeartBeat 보내기 중지)")
            print("4. 주입된 에러 초기화")
            print("5. 종료")
            choice = input("선택: ")
            
            if choice == '1':
                print("CRC에러 발생")
                self.inject_crc_error = True
            elif choice == '2':
                print("온도센서 에러 주입")
                self.cmd |= 0x01 
                self.fault_injection_time = time.time() 
                self.fault_injection_active = True
            elif choice == '3':
                print("heartbeat 전송 중단")
                self.pause_heartbeat = True 
                self.fault_injection_time = time.time()
                self.fault_injection_active = True
                time.sleep(0.3) 
                self.pause_heartbeat = False 
                print("다시 heartbeat전송 시작")
            elif choice == '4':
                print("주입된 에러 초기화")
                self.cmd = 0x00
                self.inject_crc_error = False
                self.pause_heartbeat = False
            elif choice == '5':
                self.running = False 
                break
            else:
                print("잘못된 입력")

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else 'COM4'
    gcs = GCS(port=port)
    gcs.menu() 
    gcs.csv_file.close() 
    print("종료합니다.")