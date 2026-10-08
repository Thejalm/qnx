import os
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    PROJECT_NAME: str = "QNX Wired Safety & Automation Orchestrator API"
    VERSION: str = "1.0.0"
    API_V1_STR: str = "/api"

    # PostgreSQL connection (default to local postgres or fallback to SQLite for local development)
    POSTGRES_SERVER: str = os.getenv("POSTGRES_SERVER", "localhost")
    POSTGRES_USER: str = os.getenv("POSTGRES_USER", "postgres")
    POSTGRES_PASSWORD: str = os.getenv("POSTGRES_PASSWORD", "postgres")
    POSTGRES_DB: str = os.getenv("POSTGRES_DB", "qnx_safety_db")
    POSTGRES_PORT: str = os.getenv("POSTGRES_PORT", "5432")

    # QNX Master & Input Node connection
    QNX_INPUT_STREAM_IP: str = os.getenv("QNX_INPUT_STREAM_IP", "10.61.30.52")
    QNX_INPUT_STREAM_PORT: int = 9001
    QNX_LOG_FILE: str = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../qnx_momentics/qnx_safety_events.log"))

    @property
    def SQLALCHEMY_DATABASE_URI(self) -> str:
        # Check if DATABASE_URL is explicitly provided
        db_url = os.getenv("DATABASE_URL")
        if db_url:
            return db_url
        return f"postgresql://{self.POSTGRES_USER}:{self.POSTGRES_PASSWORD}@{self.POSTGRES_SERVER}:{self.POSTGRES_PORT}/{self.POSTGRES_DB}"

settings = Settings()
