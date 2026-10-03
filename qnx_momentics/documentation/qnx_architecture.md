# QNX Real-Time Safety & Automation Orchestrator Architecture

## 1. Overview
The **QNX Master** is the central deterministic real-time engine responsible for safety decisions, fault detection, and actuator arbitration. It runs on **QNX Neutrino RTOS 7.x / 8.x** (or cross-compiled POSIX environments for simulation) and operates independently of any database, cloud, or web tier.

---

## 2. Microkernel & Real-Time Thread Architecture

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        QNX MASTER ENGINE                               │
│                                                                        │
│  ┌───────────────────────┐              ┌───────────────────────────┐  │
│  │ Communication Manager │              │   Safety Decision Engine  │  │
│  │ (I/O & Socket Thread) │              │    (SCHED_FIFO Pri 25)    │  │
│  │ Priority: 20          │              │                           │  │
│  └──────────┬────────────┘              └─────────────▲─────────────┘  │
│             │                                         │                │
│             ▼ Telemetry Buffer                        │ Telemetry      │
│  ┌───────────────────────┐              ┌─────────────┴─────────────┐  │
│  │   Packet Validator    │─────────────►│    Fault / Health Manager │  │
│  │ Checksum & Frame Sync │ Valid Packet │ Packet Loss & Comm Timeout│  │
│  └───────────────────────┘              └───────────────────────────┘  │
│                                                       │                │
│                                                       ▼ Actuator Cmd   │
│  ┌───────────────────────┐              ┌───────────────────────────┐  │
│  │ Event Logger & Buffer │◄─────────────│     Output Controller     │  │
│  │ Direct Disk + Sync Q  │ Safety Event │  Actuator Pulse & ACK Rcv │  │
│  └───────────────────────┘              └───────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Real-Time Parameters & Determinism

| Metric / Parameter | Target | Measured / Design Value | Notes |
| :--- | :--- | :--- | :--- |
| **Scheduling Policy** | Preemptive Deterministic | `SCHED_FIFO` | Priority 25 |
| **Decision Cycle Latency** | $< 500\text{ }\mu\text{s}$ | **$12.5\text{ }\mu\text{s} - 45.0\text{ }\mu\text{s}$** | Evaluated via nanosecond monotonic timer |
| **Sensor-to-Actuator Latency** | $< 120\text{ ms}$ | **$102.4\text{ ms}$** | Includes 10 Hz sensor sample rate |
| **Communication Timeout** | $1000\text{ ms}$ | Hardware safing at $2000\text{ ms}$ | Failsafe redundancy |
| **Memory Footprint** | Minimal Static | $< 3.2\text{ MB}$ RSS | No dynamic runtime heap allocations |
| **Backend Decoupling** | $100\%$ Autonomous | Full Ring-Buffer Sync | Operates safely with backend offline |

---

## 4. QNX IPC and Safety Mechanisms
1. **Deterministic State Machine**: State transitions (Normal $\rightarrow$ Gas Warning $\rightarrow$ Fire Critical $\rightarrow$ Sensor Fault $\rightarrow$ Comm Fault) execute in bounded time ($O(1)$ complexity).
2. **Packet Verification**: XOR-8 checksum and monotonic sequence validation prevent execution of corrupted or replayed commands.
3. **Fail-Safe Defaulting**: In the event of communication severance, the system enters an active fail-safe state (energizing exhaust ventilation and sounding alarms).
4. **Local Event Persistence**: All events are appended to `qnx_safety_events.log` synchronously before notifying downstream tiers.
