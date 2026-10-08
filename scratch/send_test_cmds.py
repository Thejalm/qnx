import socket
import time

s = socket.socket()
s.connect(('127.0.0.1', 9002))

for seq in range(1, 10):
    payload = f"CMD,2,{seq},0,0,0,0,0,0,SYSTEM_NORMAL"
    c = 0
    for ch in payload:
        c ^= ord(ch)
    pkt = f"${payload}*{c:02X}\r\n".encode()
    print("Sending:", pkt)
    s.sendall(pkt)
    time.sleep(0.1)

time.sleep(0.5)
s.close()
print("Sent 10 commands successfully!")
