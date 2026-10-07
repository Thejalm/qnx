from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from app.database.session import get_db
from app.database.models import SensorReading, ActuatorState, SafetyEvent, FaultRecord, SystemMetric
from app.schemas.schemas import SystemStatusResponse, SensorReadingResponse, ActuatorStateResponse

router = APIRouter(prefix="/system", tags=["System Status & Metrics"])

@router.get("/status", response_model=SystemStatusResponse)
def get_system_status(db: Session = Depends(get_db)):
    latest_sensor = db.query(SensorReading).order_by(SensorReading.id.desc()).first()
    latest_actuator = db.query(ActuatorState).order_by(ActuatorState.id.desc()).first()
    latest_metric = db.query(SystemMetric).order_by(SystemMetric.id.desc()).first()
    
    events_count = db.query(SafetyEvent).count()
    faults_count = db.query(FaultRecord).filter(FaultRecord.resolved == False).count()
    received = latest_sensor.sequence_number if latest_sensor else 0
    dropped = 0

    if latest_sensor:
        if latest_sensor.flame_detected or latest_sensor.temperature >= 50.0:
            qnx_state = "FIRE_CRITICAL"
        elif latest_sensor.mq2_digital_alert or latest_sensor.mq2_raw_adc >= 1800:
            qnx_state = "GAS_WARNING"
        elif latest_sensor.temperature >= 35.0:
            qnx_state = "ELEVATED_TEMP"
        else:
            qnx_state = "NORMAL"
    else:
        qnx_state = "NORMAL"

    eval_latency = latest_metric.eval_latency_us if latest_metric else 18.5
    loop_latency = latest_metric.loop_latency_ms if latest_metric else 100.0

    return SystemStatusResponse(
        qnx_state=qnx_state,
        latency_us=eval_latency,
        eval_latency_us=eval_latency,
        loop_latency_ms=loop_latency,
        is_comm_healthy=True,
        total_packets_received=received,
        total_dropped_packets=dropped,
        total_corrupt_packets=0,
        latest_sensor=SensorReadingResponse.model_validate(latest_sensor) if latest_sensor else None,
        latest_actuator=ActuatorStateResponse.model_validate(latest_actuator) if latest_actuator else None,
        active_events_count=events_count,
        active_faults_count=faults_count
    )

@router.get("/metrics")
def get_system_metrics(limit: int = 50, db: Session = Depends(get_db)):
    metrics = db.query(SystemMetric).order_by(SystemMetric.id.desc()).limit(limit).all()
    return metrics
