#define _GNU_SOURCE

#include "rdma_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

static int wait_cm_event(
    struct rdma_event_channel *ec,
    enum rdma_cm_event_type expected,
    struct rdma_cm_event **out)
{
    struct rdma_cm_event *event;

    if (rdma_get_cm_event(ec, &event)) {
        perror("rdma_get_cm_event");
        return -1;
    }

    if (event->event != expected) {

        fprintf(stderr,
                "Unexpected RDMA CM event: %s "
                "(expected %s)\n",
                rdma_event_str(event->event),
                rdma_event_str(expected));

        rdma_ack_cm_event(event);

        return -1;
    }

    *out = event;

    return 0;
}


static int create_resources(struct rdma_ctx *ctx)
{
    struct ibv_qp_init_attr qp_attr;

    ctx->pd =
        ibv_alloc_pd(ctx->cm_id->verbs);

    if (!ctx->pd) {
        perror("ibv_alloc_pd");
        return -1;
    }

    ctx->comp_chan =
        ibv_create_comp_channel(
            ctx->cm_id->verbs);

    if (!ctx->comp_chan) {
        perror("ibv_create_comp_channel");
        return -1;
    }

    ctx->cq =
        ibv_create_cq(
            ctx->cm_id->verbs,
            CQ_SIZE,
            NULL,
            ctx->comp_chan,
            0);

    if (!ctx->cq) {
        perror("ibv_create_cq");
        return -1;
    }

    if (ibv_req_notify_cq(
            ctx->cq,
            0)) {

        perror("ibv_req_notify_cq");
        return -1;
    }

    memset(&qp_attr, 0, sizeof(qp_attr));

    qp_attr.qp_type =
        IBV_QPT_RC;

    qp_attr.send_cq =
        ctx->cq;

    qp_attr.recv_cq =
        ctx->cq;

    qp_attr.cap.max_send_wr =
        MAX_WR;

    qp_attr.cap.max_recv_wr =
        MAX_WR;

    qp_attr.cap.max_send_sge =
        1;

    qp_attr.cap.max_recv_sge =
        1;

    if (rdma_create_qp(
            ctx->cm_id,
            ctx->pd,
            &qp_attr)) {

        perror("rdma_create_qp");
        return -1;
    }

    return 0;
}


int rdma_register_buffer(
    struct rdma_ctx *ctx,
    size_t size)
{
    int access =
        IBV_ACCESS_LOCAL_WRITE |
        IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ;

    if (posix_memalign(
            &ctx->buffer,
            4096,
            size)) {

        fprintf(stderr,
                "posix_memalign failed\n");
        return -1;
    }

    memset(ctx->buffer, 0, size);

    ctx->buffer_size = size;

    ctx->mr =
        ibv_reg_mr(
            ctx->pd,
            ctx->buffer,
            size,
            access);

    if (!ctx->mr) {
        perror("ibv_reg_mr");
        return -1;
    }

    return 0;
}


int rdma_init_client(
    struct rdma_ctx *ctx,
    const char *server_ip,
    const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    struct rdma_cm_event *event;

    struct rdma_conn_param conn_param;

    memset(ctx, 0, sizeof(*ctx));

    memset(&hints, 0, sizeof(hints));

    hints.ai_family =
        AF_INET;

    hints.ai_socktype =
        SOCK_STREAM;

    if (getaddrinfo(
            server_ip,
            port,
            &hints,
            &res)) {

        fprintf(stderr,
                "getaddrinfo failed\n");
        return -1;
    }

    ctx->ec =
        rdma_create_event_channel();

    if (!ctx->ec) {
        perror("rdma_create_event_channel");
        return -1;
    }

    if (rdma_create_id(
            ctx->ec,
            &ctx->cm_id,
            ctx,
            RDMA_PS_TCP)) {

        perror("rdma_create_id");
        return -1;
    }

    if (rdma_resolve_addr(
            ctx->cm_id,
            NULL,
            res->ai_addr,
            2000)) {

        perror("rdma_resolve_addr");
        return -1;
    }

    if (wait_cm_event(
            ctx->ec,
            RDMA_CM_EVENT_ADDR_RESOLVED,
            &event))
        return -1;

    rdma_ack_cm_event(event);

    if (rdma_resolve_route(
            ctx->cm_id,
            2000)) {

        perror("rdma_resolve_route");
        return -1;
    }

    if (wait_cm_event(
            ctx->ec,
            RDMA_CM_EVENT_ROUTE_RESOLVED,
            &event))
        return -1;

    rdma_ack_cm_event(event);

    if (create_resources(ctx))
        return -1;

    if (rdma_register_buffer(
            ctx,
            BUFFER_SIZE))
        return -1;

    memset(&conn_param, 0,
           sizeof(conn_param));

    conn_param.initiator_depth = 1;
    conn_param.responder_resources = 1;
    conn_param.retry_count = 7;

    if (rdma_connect(
            ctx->cm_id,
            &conn_param)) {

        perror("rdma_connect");
        return -1;
    }

    if (wait_cm_event(
            ctx->ec,
            RDMA_CM_EVENT_ESTABLISHED,
            &event))
        return -1;

    rdma_ack_cm_event(event);

    freeaddrinfo(res);

    return 0;
}


int rdma_init_server(
    struct rdma_ctx *ctx,
    const char *bind_ip,
    const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    struct rdma_cm_id *listen_id;

    struct rdma_cm_event *event;

    struct rdma_conn_param conn_param;

    memset(ctx, 0, sizeof(*ctx));

    memset(&hints, 0, sizeof(hints));

    hints.ai_family =
        AF_INET;

    hints.ai_socktype =
        SOCK_STREAM;

    hints.ai_flags =
        AI_PASSIVE;

    if (getaddrinfo(
            bind_ip,
            port,
            &hints,
            &res)) {

        fprintf(stderr,
                "getaddrinfo failed\n");
        return -1;
    }

    ctx->ec =
        rdma_create_event_channel();

    if (!ctx->ec) {
        perror("rdma_create_event_channel");
        return -1;
    }

    if (rdma_create_id(
            ctx->ec,
            &listen_id,
            NULL,
            RDMA_PS_TCP)) {

        perror("rdma_create_id");
        return -1;
    }

    if (rdma_bind_addr(
            listen_id,
            res->ai_addr)) {

        perror("rdma_bind_addr");
        return -1;
    }

    if (rdma_listen(
            listen_id,
            8)) {

        perror("rdma_listen");
        return -1;
    }

    printf("Waiting for RDMA connection...\n");

    if (wait_cm_event(
            ctx->ec,
            RDMA_CM_EVENT_CONNECT_REQUEST,
            &event))
        return -1;

    ctx->cm_id = event->id;

    rdma_ack_cm_event(event);

    if (create_resources(ctx))
        return -1;

    if (rdma_register_buffer(
            ctx,
            BUFFER_SIZE))
        return -1;

    memset(&conn_param, 0,
           sizeof(conn_param));

    conn_param.initiator_depth = 1;
    conn_param.responder_resources = 1;
    conn_param.rnr_retry_count = 7;

    if (rdma_accept(
            ctx->cm_id,
            &conn_param)) {

        perror("rdma_accept");
        return -1;
    }

    if (wait_cm_event(
            ctx->ec,
            RDMA_CM_EVENT_ESTABLISHED,
            &event))
        return -1;

    rdma_ack_cm_event(event);

    rdma_destroy_id(listen_id);

    freeaddrinfo(res);

    return 0;
}


int rdma_post_receive(
    struct rdma_ctx *ctx,
    uint64_t wr_id,
    void *addr,
    size_t len)
{
    struct ibv_sge sge;
    struct ibv_recv_wr wr;
    struct ibv_recv_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    memset(&wr, 0, sizeof(wr));

    sge.addr =
        (uintptr_t)addr;

    sge.length =
        len;

    sge.lkey =
        ctx->mr->lkey;

    wr.wr_id =
        wr_id;

    wr.sg_list =
        &sge;

    wr.num_sge =
        1;

    return ibv_post_recv(
        ctx->cm_id->qp,
        &wr,
        &bad_wr);
}


int rdma_post_send_buffer(
    struct rdma_ctx *ctx,
    uint64_t wr_id,
    void *addr,
    size_t len)
{
    struct ibv_sge sge;
    struct ibv_send_wr wr;
    struct ibv_send_wr *bad_wr;

    memset(&sge, 0, sizeof(sge));
    memset(&wr, 0, sizeof(wr));

    sge.addr =
        (uintptr_t)addr;

    sge.length =
        len;

    sge.lkey =
        ctx->mr->lkey;

    wr.wr_id =
        wr_id;

    wr.sg_list =
        &sge;

    wr.num_sge =
        1;

    wr.opcode =
        IBV_WR_SEND;

    wr.send_flags =
        IBV_SEND_SIGNALED;

    return ibv_post_send(
        ctx->cm_id->qp,
        &wr,
        &bad_wr);
}


int rdma_poll_completion(
    struct rdma_ctx *ctx,
    struct ibv_wc *wc)
{
    int n;

    for (;;) {

        n = ibv_poll_cq(
            ctx->cq,
            1,
            wc);

        if (n < 0)
            return -1;

        if (n > 0)
            break;
    }

    if (wc->status !=
        IBV_WC_SUCCESS) {

        fprintf(stderr,
                "Completion error: %s\n",
                ibv_wc_status_str(
                    wc->status));

        return -1;
    }

    return 0;
}


void rdma_cleanup(
    struct rdma_ctx *ctx)
{
    if (ctx->mr)
        ibv_dereg_mr(ctx->mr);

    if (ctx->cm_id &&
        ctx->cm_id->qp)
        rdma_destroy_qp(ctx->cm_id);

    if (ctx->cq)
        ibv_destroy_cq(ctx->cq);

    if (ctx->comp_chan)
        ibv_destroy_comp_channel(
            ctx->comp_chan);

    if (ctx->pd)
        ibv_dealloc_pd(ctx->pd);

    if (ctx->cm_id)
        rdma_destroy_id(ctx->cm_id);

    if (ctx->ec)
        rdma_destroy_event_channel(
            ctx->ec);

    free(ctx->buffer);
}
