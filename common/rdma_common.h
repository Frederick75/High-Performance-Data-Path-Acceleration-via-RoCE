#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#include <stdint.h>
#include <stddef.h>

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

#define RDMA_PORT           "7471"

#define BUFFER_SIZE         (2 * 1024 * 1024)
#define MAX_WR              512
#define CQ_SIZE             1024

#define MSG_MAGIC_5G        0x35475550
#define MSG_MAGIC_ORAN      0x4f52414e

enum packet_type {
    PKT_5G_N3_UL = 1,
    PKT_5G_N6_DL,
    PKT_ORAN_F1U,
    PKT_ORAN_FRONTHAUL
};

struct packet_desc {
    uint32_t magic;
    uint32_t type;

    uint32_t length;
    uint32_t flags;

    uint32_t teid;
    uint32_t ue_id;

    uint16_t drb_id;
    uint16_t qfi;

    uint64_t timestamp_ns;

    uint8_t payload[];
};

struct rdma_ctx {
    struct rdma_event_channel *ec;
    struct rdma_cm_id *cm_id;

    struct ibv_pd *pd;
    struct ibv_cq *cq;

    struct ibv_comp_channel *comp_chan;

    struct ibv_mr *mr;

    void *buffer;
    size_t buffer_size;
};

int rdma_init_client(
    struct rdma_ctx *ctx,
    const char *server_ip,
    const char *port);

int rdma_init_server(
    struct rdma_ctx *ctx,
    const char *bind_ip,
    const char *port);

int rdma_register_buffer(
    struct rdma_ctx *ctx,
    size_t size);

int rdma_post_receive(
    struct rdma_ctx *ctx,
    uint64_t wr_id,
    void *addr,
    size_t len);

int rdma_post_send_buffer(
    struct rdma_ctx *ctx,
    uint64_t wr_id,
    void *addr,
    size_t len);

int rdma_poll_completion(
    struct rdma_ctx *ctx,
    struct ibv_wc *wc);

void rdma_cleanup(
    struct rdma_ctx *ctx);

#endif
