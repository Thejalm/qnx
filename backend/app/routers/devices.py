from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from app.database.session import get_db
from app.database.models import ActuatorState
from app.schemas.schemas import ActuatorStateResponse

router = APIRouter(prefix="/devices", tags=["Actuators & Devices"])

@router.get("", response_model=ActuatorStateResponse)
def get_actuator_status(db: Session = Depends(get_db)):
    state = db.query(ActuatorState).order_by(ActuatorState.id.desc()).first()
    if not state:
        return ActuatorStateResponse(
            id=0, node_id=2, sequence_number=0, updated_at=None,
            buzzer1_alarm=False, buzzer2_warning=False,
            relay_fan=False, relay_pump=False,
            led_yellow=False, led_red=False,
            status_text="NORMAL", failsafe_active=False
        )
    return state
