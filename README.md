# QNX-Based Fully Wired Real-Time Safety & Automation Orchestrator

An end-to-end industrial college engineering prototype demonstrating deterministic real-time safety orchestration, sensor acquisition, fault management, and historical web telemetry synchronization.

---

## 1. System Architecture

```text
SENSORS (BME280, MQ-2 Gas, Flame IR)
   ↓
ESP32-C3 INPUT NODE (10 Hz Telemetry Framing + XOR-8 Checksum)
   ↓
USB / SERIAL
   ↓
NODE COMPUTER (Gateway Bridge)
   ↓
ETHERNET / PRIVATE LAN
   ↓
QNX MASTER (Real-Time SCHED_FIFO Priority 25 Decision Engine)
   ↓
ESP32-C3 OUTPUT NODE (Dual Buzzers, Fan & Pump Relays, OLED & LEDs)
   ↓
LOCAL BACKEND & SYNC LAYER (FastAPI + PostgreSQL / SQLite)
   ↓
REACT MONITORING DASHBOARD (Sub-Second WebSocket Telemetry)
```

---

## 2. Project Directory Structure

```text
QNX-Wired-Safety-Automation-Orchestrator/
│
├── firmware/
│   ├── input_node/
│   │   └── input_node.ino          # ESP32-C3 Input Node C++ Source
│   ├── output_node/
│   │   └── output_node.ino         # ESP32-C3 Output Actuators & OLED C++ Source
│   ├── node.md                     # Hardware wiring & telemetry specs
│   └── output_node.md              # Hardware wiring & command specs
│
├── qnx_momentics/
│   ├── .project                    # Eclipse Momentics Project Descriptor
│   ├── .cproject                   # QNX SDP 8.0 Toolchain Configuration
│   ├── Makefile                    # QNX qcc / POSIX Makefile
│   ├── include/                    # Protocol, Safety Engine, Fault Manager & IPC Headers
│   ├── source/                     # C Real-Time Subsystems & Main Decision Loop
│   ├── configuration/              # safety_config.ini
│   ├── documentation/              # qnx_architecture.md
│   └── node_bridge.py              # Node Computer Serial-to-Ethernet Bridge
│
├── backend/
│   ├── app/
│   │   ├── main.py                 # FastAPI Application & WebSocket Broadcaster
│   │   ├── config.py               # PostgreSQL & Environment Settings
│   │   ├── database/               # SQLAlchemy Models & Session Engine
│   │   ├── schemas/                # Pydantic Schemas
│   │   ├── routers/                # System, Sensors, Events, Devices, Faults
│   │   └── services/               # QNX Log Tailing & Live Sync Service
│   ├── database/schema.sql         # PostgreSQL DDL
│   └── requirements.txt            # Python Dependencies
│
├── frontend/
│   ├── src/
│   │   ├── App.jsx                 # Live React Dashboard Layout
│   │   ├── index.css               # Mission-Control Dark Theme & Design Tokens
│   │   └── components/             # Header, LiveSensors, ActuatorGrid, Metrics, EventsLog, Faults
│   └── package.json                # React + Vite Configuration
│
└── start_system.py                 # Unified Multi-Process System Runner
```

---

## 3. Quick Start Guide

### Step 1: Launch Backend, QNX Core, and Node Simulators
In Terminal 1:
```powershell
python start_system.py
```

### Step 2: Start the React Frontend Dashboard
In Terminal 2:
```powershell
cd frontend
npm run dev
```
Open **`http://localhost:3000`** in your browser.

---

## 4. Key Real-Time Specifications

* **Decision Cycle Latency**: $\approx 15\text{–}35\text{ }\mu\text{s}$ (Evaluated on QNX Neutrino RTOS).
* **Deterministic Sampling**: $100\text{ ms (10 Hz)}$ with monotonic sequence ordering.
* **Fail-Safe Watchdog**: Output Node trips into safe mode within $2.0\text{ s}$ of link severance.
* **Backend Isolation**: Immediate physical safety actions continue locally even if the database or web layer goes offline.
