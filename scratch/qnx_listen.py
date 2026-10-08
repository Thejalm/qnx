import socket
import time

s = socket.socket()
s.connect(('192.168.160.129', 8000))
s.recv(1024)
s.sendall(b'service launcher\r\n')
s.recv(1024)
# Read any stream from launcher console
s.settimeout(2.0)
try:
    while True:
        data = s.recv(1024)
        if not data: break
        print("Launcher stream:", repr(data))
except Exception as e:
    print("Launcher stream timeout/done:", e)
s.close()
