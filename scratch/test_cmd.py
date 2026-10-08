import socket
import time

s = socket.socket()
s.connect(('127.0.0.1', 9002))
payload = 'CMD,2,100,0,0,1,0,1,0,TEST_FAN_ON'
c = 0
for ch in payload:
    c ^= ord(ch)
pkt = f"${payload}*{c:02X}\r\n".encode()
print("Sending packet:", pkt)
s.sendall(pkt)
s.settimeout(2.0)
try:
    ack = s.recv(128)
    print("Received ACK:", ack.decode())
except Exception as e:
    print("No ACK received:", e)
s.close()
