# QNX Safety Orchestrator Backend (FastAPI + PostgreSQL)

## 1. Overview
The **Backend Layer** acts as the data synchronization, historical storage, and REST/WebSocket API layer for the QNX Safety Orchestrator. It stores time-series sensor data, records safety trip events, maintains actuator states, and streams live telemetry to the React frontend.

---

## 2. API Endpoints Reference

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/` | API status and root documentation link |
| `GET` | `/docs` | Interactive Swagger UI API Explorer |
| `GET` | `/api/system/status` | Current system health, QNX state, metrics & latest states |
| `GET` | `/api/system/metrics` | QNX real-time loop latency & packet loss history |
| `GET` | `/api/sensors` | Historical time-series sensor readings (`?limit=50`) |
| `GET` | `/api/sensors/latest` | Most recent physical sensor sample |
| `GET` | `/api/events` | Log of safety events, trips, and alarms (`?level=CRITICAL`) |
| `POST` | `/api/events` | Ingest local QNX safety event record |
| `GET` | `/api/devices` | Current states of Fan, Pump, Buzzers, and LEDs |
| `GET` | `/api/faults` | Hardware and communication fault diagnostic log |
| `WS` | `/ws/telemetry` | Sub-second real-time WebSocket telemetry stream |

---

## 3. How to Run the Backend

### Step 1: Install Python Dependencies
```powershell
pip install -r backend/requirements.txt
```

### Step 2: Start the FastAPI Server
```powershell
cd backend
python -m uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

---

## 4. Swagger Interactive UI
Open your browser and navigate to:
```text
http://localhost:8000/docs
```
You will be able to test all REST endpoints and view schemas interactively.
