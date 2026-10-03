import os
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base
from app.config import settings

# Attempt PostgreSQL connection, fallback gracefully to SQLite for local development
DATABASE_URL = settings.SQLALCHEMY_DATABASE_URI

try:
    if "postgresql" in DATABASE_URL:
        # Test engine with short timeout
        engine = create_engine(DATABASE_URL, pool_pre_ping=True, connect_args={"connect_timeout": 2})
        # Try connecting
        with engine.connect() as conn:
            pass
        print(f"[DATABASE] Connected successfully to PostgreSQL at {settings.POSTGRES_SERVER}:{settings.POSTGRES_PORT}/{settings.POSTGRES_DB}")
    else:
        engine = create_engine(DATABASE_URL)
except Exception as e:
    print(f"[DATABASE NOTICE] PostgreSQL connection not available ({e}). Using local SQLite database 'qnx_safety.db'.")
    DATABASE_URL = "sqlite:///./qnx_safety.db"
    engine = create_engine(DATABASE_URL, connect_args={"check_same_thread": False})

SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base = declarative_base()

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
