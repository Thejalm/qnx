import serial
import time

port = "COM4"
baud = 115200

print("Opening COM4 with DTR=True, RTS=False, write_timeout=0...")
ser = serial.Serial(port, baud, timeout=0.2, write_timeout=0)
ser.dtr = True
ser.rts = False

print("Waiting 2.5 seconds to let ESP32 finish booting and stabilize...")
time.sleep(2.5)

# Read any boot messages
boot_data = ser.read(ser.in_waiting or 1000)
print(f"Boot output ({len(boot_data)} bytes): {repr(boot_data)}")

# Now send a test command packet
payload = "CMD,2,10,0,0,1,0,1,0,TESTING"
chk = 0
for c in payload:
    chk ^= ord(c)
pkt = f"${payload}*{chk:02X}\r\n".encode("ascii")

print(f"Sending: {pkt}")
n = ser.write(pkt)
print(f"Wrote {n} bytes.")

print("Listening for 4 seconds...")
t0 = time.time()
while time.time() - t0 < 4.0:
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print("--> RX:", repr(data))
    time.sleep(0.05)

ser.close()
print("Test completed.")
