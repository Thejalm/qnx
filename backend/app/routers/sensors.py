from typing import List
from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session
from app.database.session import get_db
from app.database.models import SensorReading
from app.schemas.schemas import SensorReadingResponse

router = APIRouter(prefix="/sensors", tags=["Sensors"])

@router.get("", response_model=List[SensorReadingResponse])
def get_sensor_readings(
    limit: int = Query(50, ge=1, le=500),
    db: Session = Depends(get_db)
):
    """Returns historical time-series sensor readings."""
    readings = db.query(SensorReading).order_by(SensorReading.id.desc()).limit(limit).all()
    return readings

@router.get("/latest", response_model=SensorReadingResponse)
def get_latest_sensor_reading(db: Session = Depends(get_db)):
    reading = db.query(SensorReading).order_by(SensorReading.id.desc()).first()
    if not reading:
        return SensorReadingResponse(
            id=0, node_id=1, sequence_number=0, timestamp_ms=0,
            created_at=None, temperature=0.0, humidity=0.0, pressure=0.0,
            mq2_raw_adc=0, mq2_digital_alert=False, flame_detected=False,
            flame_raw_adc=4095, fault_flags=0
        )
    return reading
