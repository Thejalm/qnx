-- =============================================================================
-- PostgreSQL Schema for QNX Real-Time Safety & Automation Orchestrator
-- =============================================================================

CREATE TABLE IF NOT EXISTS sensor_readings (
    id SERIAL PRIMARY KEY,
    node_id INTEGER NOT NULL DEFAULT 1,
    sequence_number INTEGER NOT NULL,
    timestamp_ms BIGINT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    temperature REAL NOT NULL,
    humidity REAL NOT NULL,
    pressure REAL NOT NULL,
    mq2_raw_adc INTEGER NOT NULL,
    mq2_digital_alert BOOLEAN DEFAULT FALSE,
    flame_detected BOOLEAN DEFAULT FALSE,
    flame_raw_adc INTEGER NOT NULL,
    fault_flags INTEGER DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_sensor_readings_created_at ON sensor_readings(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_sensor_readings_seq ON sensor_readings(sequence_number);

CREATE TABLE IF NOT EXISTS safety_events (
    id SERIAL PRIMARY KEY,
    timestamp_ns VARCHAR(64) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    level VARCHAR(32) NOT NULL,
    source VARCHAR(64) NOT NULL,
    description VARCHAR(255) NOT NULL,
    sensor_val1 REAL DEFAULT 0.0,
    sensor_val2 REAL DEFAULT 0.0,
    synced BOOLEAN DEFAULT TRUE
);

CREATE INDEX IF NOT EXISTS idx_safety_events_created_at ON safety_events(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_safety_events_level ON safety_events(level);

CREATE TABLE IF NOT EXISTS actuator_states (
    id SERIAL PRIMARY KEY,
    node_id INTEGER NOT NULL DEFAULT 2,
    sequence_number INTEGER NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    buzzer1_alarm BOOLEAN DEFAULT FALSE,
    buzzer2_warning BOOLEAN DEFAULT FALSE,
    relay_fan BOOLEAN DEFAULT FALSE,
    relay_pump BOOLEAN DEFAULT FALSE,
    led_yellow BOOLEAN DEFAULT FALSE,
    led_red BOOLEAN DEFAULT FALSE,
    status_text VARCHAR(64) DEFAULT 'NORMAL',
    failsafe_active BOOLEAN DEFAULT FALSE
);

CREATE TABLE IF NOT EXISTS fault_records (
    id SERIAL PRIMARY KEY,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    fault_type VARCHAR(64) NOT NULL,
    details VARCHAR(255) NOT NULL,
    resolved BOOLEAN DEFAULT FALSE
);

CREATE TABLE IF NOT EXISTS system_metrics (
    id SERIAL PRIMARY KEY,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    qnx_state VARCHAR(32) DEFAULT 'NORMAL',
    loop_latency_us REAL DEFAULT 0.0,
    packets_received INTEGER DEFAULT 0,
    packets_dropped INTEGER DEFAULT 0,
    packets_corrupt INTEGER DEFAULT 0,
    cpu_utilization REAL DEFAULT 0.0
);
