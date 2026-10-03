import datetime
from typing import Optional, List
from pydantic import BaseModel, ConfigDict

class SensorReadingBase(BaseModel):
    node_id: int = 1
    sequence_number: int
    timestamp_ms: int
    temperature: float
    humidity: float
    pressure: float
    mq2_raw_adc: int
    mq2_digital_alert: bool
    flame_detected: bool
    flame_raw_adc: int
    fault_flags: int = 0

class SensorReadingResponse(SensorReadingBase):
    id: int
    created_at: datetime.datetime
    model_config = ConfigDict(from_attributes=True)

class SafetyEventBase(BaseModel):
    timestamp_ns: str
    level: str
    source: str
    description: str
    sensor_val1: float = 0.0
    sensor_val2: float = 0.0

class SafetyEventResponse(SafetyEventBase):
    id: int
    created_at: datetime.datetime
    synced: bool
    model_config = ConfigDict(from_attributes=True)

class ActuatorStateBase(BaseModel):
    node_id: int = 2
    sequence_number: int
    buzzer1_alarm: bool
    buzzer2_warning: bool
    relay_fan: bool
    relay_pump: bool
    led_yellow: bool
    led_red: bool
    status_text: str
    failsafe_active: bool

class ActuatorStateResponse(ActuatorStateBase):
    id: int
    updated_at: datetime.datetime
    model_config = ConfigDict(from_attributes=True)

class FaultRecordResponse(BaseModel):
    id: int
    created_at: datetime.datetime
    fault_type: str
    details: str
    resolved: bool
    model_config = ConfigDict(from_attributes=True)

class SystemStatusResponse(BaseModel):
    qnx_state: str
    latency_us: float
    eval_latency_us: float = 18.5
    loop_latency_ms: float = 100.0
    is_comm_healthy: bool
    total_packets_received: int
    total_dropped_packets: int
    total_corrupt_packets: int
    latest_sensor: Optional[SensorReadingResponse] = None
    latest_actuator: Optional[ActuatorStateResponse] = None
    active_events_count: int = 0
    active_faults_count: int = 0
