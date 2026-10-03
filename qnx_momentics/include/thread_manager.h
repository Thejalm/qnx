#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "qnx_ipc.h"
#include "safety_engine.h"
#include "fault_manager.h"
#include "comm_manager.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    QnxIpcContext       ipc;
    SafetyEngineContext safety;
    FaultManagerContext fault;
    CommConfig          comm_cfg;
    volatile bool       is_running;
} QnxOrchestratorRuntime;

bool thread_manager_start(QnxOrchestratorRuntime* rt);
void thread_manager_stop(QnxOrchestratorRuntime* rt);

#ifdef __cplusplus
}
#endif

#endif // THREAD_MANAGER_H
