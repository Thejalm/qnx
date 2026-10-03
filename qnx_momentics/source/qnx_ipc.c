#include "../include/qnx_ipc.h"
#include "../include/qnx_compat.h"
#include <stdio.h>
#include <string.h>

#if defined(IS_NATIVE_QNX) && IS_NATIVE_QNX == 1

bool qnx_ipc_init(QnxIpcContext* ctx) {
    if (!ctx) return false;
    memset(ctx, 0, sizeof(QnxIpcContext));

    // 1. Create Channels with ChannelCreate()
    ctx->safety_chid = ChannelCreate(0);
    if (ctx->safety_chid < 0) {
        perror("ChannelCreate safety_chid failed");
        return false;
    }

    ctx->output_chid = ChannelCreate(0);
    if (ctx->output_chid < 0) {
        perror("ChannelCreate output_chid failed");
        ChannelDestroy(ctx->safety_chid);
        return false;
    }

    ctx->logger_chid = ChannelCreate(0);
    if (ctx->logger_chid < 0) {
        perror("ChannelCreate logger_chid failed");
        ChannelDestroy(ctx->safety_chid);
        ChannelDestroy(ctx->output_chid);
        return false;
    }

    // 2. Attach Connections with ConnectAttach()
    ctx->safety_coid = ConnectAttach(ND_LOCAL_NODE, 0, ctx->safety_chid, _NTO_SIDE_CHANNEL, 0);
    ctx->output_coid = ConnectAttach(ND_LOCAL_NODE, 0, ctx->output_chid, _NTO_SIDE_CHANNEL, 0);
    ctx->logger_coid = ConnectAttach(ND_LOCAL_NODE, 0, ctx->logger_chid, _NTO_SIDE_CHANNEL, 0);

    if (ctx->safety_coid < 0 || ctx->output_coid < 0 || ctx->logger_coid < 0) {
        perror("ConnectAttach failed");
        qnx_ipc_destroy(ctx);
        return false;
    }

    return true;
}

void qnx_ipc_destroy(QnxIpcContext* ctx) {
    if (!ctx) return;
    if (ctx->safety_coid >= 0) ConnectDetach(ctx->safety_coid);
    if (ctx->output_coid >= 0) ConnectDetach(ctx->output_coid);
    if (ctx->logger_coid >= 0) ConnectDetach(ctx->logger_coid);

    if (ctx->safety_chid >= 0) ChannelDestroy(ctx->safety_chid);
    if (ctx->output_chid >= 0) ChannelDestroy(ctx->output_chid);
    if (ctx->logger_chid >= 0) ChannelDestroy(ctx->logger_chid);
}

#else

// POSIX / Emulation fallback for host environments
bool qnx_ipc_init(QnxIpcContext* ctx) {
    if (!ctx) return false;
    memset(ctx, 0, sizeof(QnxIpcContext));
    ctx->safety_chid = 1;
    ctx->safety_coid = 1;
    ctx->output_chid = 2;
    ctx->output_coid = 2;
    ctx->logger_chid = 3;
    ctx->logger_coid = 3;
    return true;
}

void qnx_ipc_destroy(QnxIpcContext* ctx) {
    if (ctx) memset(ctx, 0, sizeof(QnxIpcContext));
}

#endif
