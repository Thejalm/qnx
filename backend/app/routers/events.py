from typing import List
from fastapi import APIRouter, Depends, Query, status
from sqlalchemy.orm import Session
from app.database.session import get_db
from app.database.models import SafetyEvent
from app.schemas.schemas import SafetyEventResponse, SafetyEventBase

router = APIRouter(prefix="/events", tags=["Safety Events"])

@router.get("", response_model=List[SafetyEventResponse])
def get_safety_events(
    level: str = Query(None, description="Filter by event severity level (INFO, WARN, CRITICAL, FAULT)"),
    limit: int = Query(100, ge=1, le=1000),
    db: Session = Depends(get_db)
):
    query = db.query(SafetyEvent)
    if level:
        query = query.filter(SafetyEvent.level == level.upper())
    return query.order_by(SafetyEvent.id.desc()).limit(limit).all()

@router.post("", response_model=SafetyEventResponse, status_code=status.HTTP_201_CREATED)
def create_safety_event(event_in: SafetyEventBase, db: Session = Depends(get_db)):
    event = SafetyEvent(
        timestamp_ns=event_in.timestamp_ns,
        level=event_in.level,
        source=event_in.source,
        description=event_in.description,
        sensor_val1=event_in.sensor_val1,
        sensor_val2=event_in.sensor_val2,
        synced=True
    )
    db.add(event)
    db.commit()
    db.refresh(event)
    return event
