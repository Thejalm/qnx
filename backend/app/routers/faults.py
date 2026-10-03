from typing import List
from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session
from app.database.session import get_db
from app.database.models import FaultRecord
from app.schemas.schemas import FaultRecordResponse

router = APIRouter(prefix="/faults", tags=["Fault Diagnostics"])

@router.get("", response_model=List[FaultRecordResponse])
def get_fault_history(db: Session = Depends(get_db)):
    return db.query(FaultRecord).order_by(FaultRecord.id.desc()).limit(100).all()
