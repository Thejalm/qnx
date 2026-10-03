# ESP32-C3 Output Actuator & OLED Node Documentation

## 1. Overview
The **Output Node** is an actuator controller and local telemetry display running on an **ESP32-C3 (16-Pin DevModule)**. It receives validated control commands from the QNX Master / Node Controller, drives safety actuators (**Active Buzzers**, **Relay-driven Fan & Water Pump**, and **Discrete Warning/Critical LEDs**), renders critical telemetry onto a **0.96" I2C OLED Display**, and implements a **hardware fail-safe watchdog timer**.

---

## 2. Actuator Hardware & Pin Mapping

| Peripheral / Module | Physical Pin / Function | ESP32-C3 GPIO | Hardware Logic / Characteristics |
| :--- | :--- | :--- | :--- |
| **0.96" OLED (SSD1306)** | SDA (I2C Data) | **GPIO 4** | 3.3V I2C Data (Address `0x3C`) |
| **0.96" OLED (SSD1306)** | SCL (I2C Clock) | **GPIO 5** | 3.3V I2C Clock |
| **Primary Alarm Buzzer** | Signal / Positive | **GPIO 0** | Active Buzzer (HIGH = Sound ON, LOW = OFF) |
| **Secondary Warning Buzzer** | Signal / Positive | **GPIO 1** | Active Buzzer (HIGH = Sound ON, LOW = OFF) |
| **Relay 1: Exhaust Fan** | IN1 Control Pin | **GPIO 2** | Active-LOW (LOW = Fan ON, HIGH = Fan OFF) |
| **Relay 2: Water Pump** | IN2 Control Pin | **GPIO 3** | Active-LOW (LOW = Pump ON, HIGH = Pump OFF) |
| **Yellow LED (Warning)** | Anode (+) via 330Ω | **GPIO 6** | HIGH = LED ON, LOW = LED OFF |
| **Red LED (Critical/Fault)** | Anode (+) via 330Ω | **GPIO 7** | HIGH = LED ON, LOW = LED OFF |
| **Onboard Heartbeat LED** | Status Heartbeat | **GPIO 8** | Toggles at 1 Hz (500 ms) |

---

## 3. Communication & Command Protocol

### Command Packet Format (Node Computer / QNX to Output Node):
```text
$CMD,NODE_ID,SEQ,BUZZER1,BUZZER2,RELAY_FAN,RELAY_PUMP,LED_YELLOW,LED_RED,STATUS_TEXT*CHECKSUM\r\n
```

#### Field Specifications:
* `$` : Start of frame.
* `CMD` : Command packet identifier.
* `NODE_ID` : Fixed node identifier (`2` for Output Node).
* `SEQ` : 32-bit sequence number (must match in ACK).
* `BUZZER1` : Primary alarm state (`1` = ON, `0` = OFF).
* `BUZZER2` : Secondary alert state (`1` = ON, `0` = OFF).
* `RELAY_FAN` : Fan power (`1` = Energized/RUN, `0` = OFF).
* `RELAY_PUMP` : Water pump power (`1` = Energized/RUN, `0` = OFF).
* `LED_YELLOW` : Warning indicator (`1` = ON, `0` = OFF).
* `LED_RED` : Critical indicator (`1` = ON, `0` = OFF).
* `STATUS_TEXT` : Alphanumeric status string displayed on OLED (e.g., `NORMAL`, `GAS_WARN`, `FIRE_EVAC`).
* `*` : Checksum delimiter.
* `CHECKSUM` : 2-digit Hex XOR-8 checksum.

### Acknowledgment (ACK) Packet Format (Output Node to Node Computer):
```text
$ACK,NODE_ID,SEQ,BUZ1,BUZ2,FAN,PUMP,LED_Y,LED_R,FAILSAFE*CHECKSUM\r\n
```

---

## 4. Hardware Fail-Safe Watchdog Behavior

If the Output Node does NOT receive a valid, checksum-verified `$CMD` packet for **2000 ms (2 seconds)**:
1. It automatically enters **FAILSAFE STATE**.
2. **Exhaust Fan** is turned **ON** (Relay 1 energized) to purge any potential toxic gas/heat accumulation.
3. **Primary Alarm Buzzer** is activated to alert local personnel of communication loss.
4. **Yellow and Red LEDs** are both illuminated.
5. The OLED updates to show: `SYS: FAILSAFE (LOST LINK)` and `LINK: DOWN`.
6. Once valid serial communication resumes from QNX/Node Computer, normal commanded operation immediately restores.

---

## 5. Required Arduino IDE Libraries

In the Arduino IDE Library Manager (`Ctrl + Shift + I`), install:
1. **Adafruit SSD1306** by Adafruit
2. **Adafruit GFX Library** by Adafruit

---

## 6. Testing & Manual Command Verification

1. Connect the Output Node ESP32-C3 to your computer via USB-C.
2. Select Board: **ESP32C3 Dev Module** (Enable **USB CDC On Boot: Enabled**).
3. Upload `output_node.ino`.
4. Open the Serial Monitor at **115200 baud** (Set line ending to **Newline** / `\n`).

### Manual Serial Test Commands:

#### 1. All Normal / Idle Command:
Send this string in Serial Monitor:
```text
$CMD,2,100,0,0,0,0,0,0,ALL_NORMAL*4E
```
- **Expected Action**: All buzzers and relays OFF, LEDs OFF, OLED displays `SYS: ALL_NORMAL`.
- **Expected Serial Return**:
```text
$ACK,2,100,0,0,0,0,0,0,0*1C
```

#### 2. Gas Warning Test Command (Warning Buzzer 2 ON, Exhaust Fan ON, Yellow LED ON):
```text
$CMD,2,101,0,1,1,0,1,0,GAS_WARNING*4F
```
- **Expected Action**: Buzzer 2 beeps, Relay 1 (Fan) clicks ON, Yellow LED turns ON, OLED shows `SYS: GAS_WARNING`, `FAN: [RUN ]`.

#### 3. Fire Critical Suppression Command (Alarm Buzzer 1 ON, Pump ON, Red LED ON):
```text
$CMD,2,102,1,0,1,1,0,1,FIRE_ALARM*44
```
- **Expected Action**: Alarm Buzzer 1 sounds, Relay 1 (Fan) and Relay 2 (Pump) click ON, Red LED turns ON, OLED shows `SYS: FIRE_ALARM`.

#### 4. Watchdog Fail-Safe Test:
Stop sending commands for more than 2 seconds. Observe:
- Within 2 seconds, Buzzer 1 and Fan activate automatically, and the OLED status displays `FAILSAFE (LOST LINK)`.

---

## 7. Troubleshooting
* **OLED does not turn on**:
  - Verify I2C address: code uses default `0x3C`.
  - Verify SDA is connected to GPIO 4 and SCL to GPIO 5.
* **Relays operate in reverse (turn OFF when commanded ON)**:
  - Standard relay modules are active-LOW. The firmware is pre-configured with active-LOW logic (`digitalWrite(PIN, LOW)` turns relay ON).
* **Commands are ignored**:
  - Verify the checksum at the end of the `$CMD` frame. An invalid checksum causes the packet to be rejected.
