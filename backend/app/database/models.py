import datetime
from sqlalchemy import Column, Integer, String, Float, Boolean, DateTime, Index
from app.database.session import Base

class SensorReading(Base):
    __tablename__ = "sensor_readings"

    id = Column(Integer, primary_key=True, index=True)
    node_id = Column(Integer, nullable=False, default=1)
    sequence_number = Column(Integer, nullable=False, index=True)
    timestamp_ms = Column(Integer, nullable=False)
    created_at = Column(DateTime, default=datetime.datetime.utcnow, index=True)

    temperature = Column(Float, nullable=False)
    humidity = Column(Float, nullable=False)
    pressure = Column(Float, nullable=False)
    mq2_raw_adc = Column(Integer, nullable=False)
    mq2_digital_alert = Column(Boolean, default=False)
    flame_detected = Column(Boolean, default=False)
    flame_raw_adc = Column(Integer, nullable=False)
    fault_flags = Column(Integer, default=0)

    __table_args__ = (
        Index("idx_sensor_created_at", "created_at"),
    )

class SafetyEvent(Base):
    __tablename__ = "safety_events"

    id = Column(Integer, primary_key=True, index=True)
    timestamp_ns = Column(String, nullable=False)
    created_at = Column(DateTime, default=datetime.datetime.utcnow, index=True)
    level = Column(String(32), nullable=False) # INFO, WARN, CRITICAL, FAULT
    source = Column(String(64), nullable=False)
    description = Column(String(255), nullable=False)
    sensor_val1 = Column(Float, default=0.0)
    sensor_val2 = Column(Float, default=0.0)
    synced = Column(Boolean, default=True)

class ActuatorState(Base):
    __tablename__ = "actuator_states"

    id = Column(Integer, primary_key=True, index=True)
    node_id = Column(Integer, nullable=False, default=2)
    sequence_number = Column(Integer, nullable=False)
    updated_at = Column(DateTime, default=datetime.datetime.utcnow, onupdate=datetime.datetime.utcnow)

    buzzer1_alarm = Column(Boolean, default=False)
    buzzer2_warning = Column(Boolean, default=False)
    relay_fan = Column(Boolean, default=False)
    relay_pump = Column(Boolean, default=False)
    led_yellow = Column(Boolean, default=False)
    led_red = Column(Boolean, default=False)
    status_text = Column(String(64), default="NORMAL")
    failsafe_active = Column(Boolean, default=False)

class FaultRecord(Base):
    __tablename__ = "fault_records"

    id = Column(Integer, primary_key=True, index=True)
    created_at = Column(DateTime, default=datetime.datetime.utcnow, index=True)
    fault_type = Column(String(64), nullable=False) # COMM_TIMEOUT, PACKET_LOSS, SENSOR_FAULT, CRC_CORRUPTION
    details = Column(String(255), nullable=False)
    resolved = Column(Boolean, default=False)

class SystemMetric(Base):
    __tablename__ = "system_metrics"

    id = Column(Integer, primary_key=True, index=True)
    recorded_at = Column(DateTime, default=datetime.datetime.utcnow, index=True)
    qnx_state = Column(String(32), default="NORMAL")
    loop_latency_us = Column(Float, default=0.0)
    packets_received = Column(Integer, default=0)
    packets_dropped = Column(Integer, default=0)
    packets_corrupt = Column(Integer, default=0)
    cpu_utilization = Column(Float, default=0.0)
