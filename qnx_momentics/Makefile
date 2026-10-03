# ==============================================================================
# Makefile for QNX Real-Time Safety & Automation Orchestrator
# Target: QNX Neutrino RTOS 8.0 (qcc)
# ==============================================================================

# Ensure QNX Environment variables are explicitly exported for qcc
export QNX_HOST ?= C:/Users/Thejal/qnx800/host/win64/x86_64
export QNX_TARGET ?= C:/Users/Thejal/qnx800/target/qnx
export QNX_CONFIGURATION ?= C:/Users/Thejal/.qnx
export PATH := $(QNX_HOST)/usr/bin:$(PATH)

CC = qcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -D__QNXNTO__
SRC = source/main.c \
      source/protocol.c \
      source/safety_engine.c \
      source/fault_manager.c \
      source/event_logger.c \
      source/comm_manager.c \
      source/qnx_ipc.c \
      source/thread_manager.c

TARGET = qnx_orchestrator

# Libraries for QNX Neutrino RTOS (libsocket for TCP/IP network stack)
LIBS = -lsocket

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	@echo [BUILD SUCCESS] Compiled $(TARGET) for QNX Neutrino RTOS

clean:
	rm -f $(TARGET) *.o
