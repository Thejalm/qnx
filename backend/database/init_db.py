"""
Database initialization and seeding script.
Run with: python -m backend.database.init_db
"""

from app.database.session import engine, Base
from app.database.models import SensorReading, SafetyEvent, ActuatorState, FaultRecord, SystemMetric

def init_db():
    print("[INIT DB] Creating all PostgreSQL / SQLite tables...")
    Base.metadata.create_all(bind=engine)
    print("[INIT DB] Database tables verified successfully.")

if __name__ == "__main__":
    init_db()
