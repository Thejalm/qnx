# ESP32-C3 Input Node Documentation & Wiring Guide

## 1. Overview
The **Input Node** is an embedded data acquisition unit running on an **ESP32-C3 (16-Pin DevModule)**. Its primary role is to sample physical safety and environmental sensors deterministically at **10 Hz (100 ms interval)**, validate hardware health, package the readings into a structured checksum-protected frame, and stream the telemetry over wired USB-Serial to the Node Computer.

---

## 2. Sensor Hardware & Pin Mapping

| Sensor Module | Physical Pin / Function | ESP32-C3 GPIO | Function / Notes |
| :--- | :--- | :--- | :--- |
| **BME280** | SDA (I2C Data) | **GPIO 4** | 3.3V I2C Bus |
| **BME280** | SCL (I2C Clock) | **GPIO 5** | 3.3V I2C Bus |
| **BME280** | VCC / GND | 3V3 / GND | Power supply |
| **MQ-2 Gas** | A0 (Analog Output) | **GPIO 0** | ADC1_CH0 (0 - 4095) |
| **MQ-2 Gas** | D0 (Digital Threshold) | **GPIO 1** | Active LOW on threshold trip |
| **MQ-2 Gas** | VCC / GND | 5V (or 3V3) / GND | Note: Heater requires 5V from USB/Ext |
| **Flame Sensor** | D0 (Digital Trigger) | **GPIO 2** | Active LOW on flame detection |
| **Flame Sensor** | A0 (Analog Level) | **GPIO 3** | ADC1_CH3 (IR intensity) |
| **Flame Sensor** | VCC / GND | 3V3 / GND | Power supply |
| **Onboard LED** | Status Heartbeat | **GPIO 8** | Blinks at 1 Hz (500 ms toggle) |

---

## 3. Telemetry Packet Protocol Specification

Telemetry packets are emitted over Hardware Serial at **115200 baud**.

### Packet Framing:
```text
$IN,NODE_ID,SEQ,TIMESTAMP_MS,TEMP_C,HUM_PCT,PRESS_HPA,MQ2_ADC,MQ2_ALERT,FLAME_DET,FLAME_ADC,FAULT_FLAGS*CHECKSUM\r\n
```

### Field Definitions:
1. `$` : Start-of-frame delimiter.
2. `IN` : Telemetry message type (Input Node).
3. `NODE_ID` : Fixed node identifier (`1` for Input Node).
4. `SEQ` : Unsigned 32-bit monotonically incrementing sequence counter (detects dropped/out-of-order packets).
5. `TIMESTAMP_MS` : Local millisecond timestamp (`millis()`).
6. `TEMP_C` : Temperature in degrees Celsius (e.g., `28.45`). `-999.0` on sensor fault.
7. `HUM_PCT` : Relative humidity percentage (e.g., `55.20`). `-999.0` on sensor fault.
8. `PRESS_HPA` : Barometric pressure in hectopascals (e.g., `1013.25`).
9. `MQ2_ADC` : 12-bit raw analog gas reading (`0` - `4095`).
10. `MQ2_ALERT` : Digital threshold alert (`1` = Gas Alert, `0` = Normal).
11. `FLAME_DET` : Digital flame detection status (`1` = Flame Detected, `0` = No Flame).
12. `FLAME_ADC` : 12-bit analog flame IR intensity (`0` - `4095`).
13. `FAULT_FLAGS` : Bitmask of hardware faults:
    - Bit 0 (`0x01`): BME280 disconnected or out-of-range.
    - Bit 1 (`0x02`): MQ-2 sensor open/short circuit fault.
    - Bit 2 (`0x04`): Flame sensor fault.
14. `*` : Checksum delimiter.
15. `CHECKSUM` : 2-character hexadecimal XOR-8 checksum over all bytes between `$` and `*`.
16. `\r\n` : End-of-frame line terminator.

---

## 4. Required Arduino IDE Libraries

In the Arduino IDE Library Manager (`Ctrl + Shift + I`), install:
1. **Adafruit BME280 Library** by Adafruit (and its dependency *Adafruit Unified Sensor*).

---

## 5. Testing & Verification Procedure

1. Connect the ESP32-C3 board to the computer via USB-C.
2. Open Arduino IDE:
   - Select Board: **ESP32C3 Dev Module** (or **Generic ESP32-C3**).
   - Enable: **USB CDC On Boot: Enabled** (if using direct native USB on ESP32-C3).
   - Select your COM Port.
3. Click **Upload** to compile and flash `input_node.ino`.
4. Open the **Serial Monitor** at **115200 baud**.

### Expected Normal Serial Output:
```text
$IN,1,0,105,27.34,54.12,1012.80,340,0,0,3850,0*4E
$IN,1,1,205,27.35,54.10,1012.78,342,0,0,3852,0*4B
$IN,1,2,305,27.36,54.08,1012.82,339,0,0,3848,0*42
```

### Expected Flame / Gas Trigger Test Output:
- When a lighter/flame is brought near the flame sensor:
```text
$IN,1,142,14305,28.10,53.80,1012.75,350,0,1,410,0*5A
```
*(Notice `FLAME_DET` switches from `0` to `1` and `FLAME_ADC` drops significantly)*.

---

## 6. Troubleshooting
* **BME280 shows `-999.0` and FAULT_FLAGS = 1**:
  - Verify I2C connections: SDA to GPIO 4, SCL to GPIO 5.
  - Check I2C address: code automatically tries both `0x76` and `0x77`.
* **MQ-2 readings always 0 or 4095**:
  - Ensure the MQ-2 module receives 5V (VCC) because the internal heater requires 5V to warm up the sensing element.
* **No Serial Output**:
  - Ensure "USB CDC On Boot" is set to "Enabled" in Arduino IDE Tools menu for ESP32-C3 boards.
