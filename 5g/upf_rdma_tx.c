#include "../common/rdma_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void)
{
    struct timespec ts;

    clock_gettime(
        CLOCK_MONOTONIC,
        &ts);

    return
        (uint64_t)ts.tv_sec *
        1000000000ULL +
        ts.tv_nsec;
}


int main(
    int argc,
    char **argv)
{
    struct rdma_ctx ctx;

    struct packet_desc *pkt;

    struct ibv_wc wc;

    const char *server;

    size_t payload_len = 1024;

    if (argc < 2) {

        fprintf(stderr,
            "Usage: %s "
            "<rdma_server_ip>\n",
            argv[0]);

        return 1;
    }

    server = argv[1];

    if (rdma_init_client(
            &ctx,
            server,
            RDMA_PORT)) {

        fprintf(stderr,
                "RDMA client init failed\n");

        return 1;
    }

    pkt =
        (struct packet_desc *)
        ctx.buffer;

    memset(pkt, 0,
           sizeof(*pkt));

    pkt->magic =
        MSG_MAGIC_5G;

    pkt->type =
        PKT_5G_N3_UL;

    pkt->length =
        payload_len;

    pkt->teid =
        0x1001;

    pkt->ue_id =
        100;

    pkt->qfi =
        9;

    pkt->timestamp_ns =
        now_ns();

    memset(
        pkt->payload,
        0xAB,
        payload_len);

    size_t total_len =
        sizeof(*pkt) +
        payload_len;

    printf(
        "Posting 5G packet: "
        "TEID=0x%x QFI=%u "
        "bytes=%zu\n",
        pkt->teid,
        pkt->qfi,
        total_len);

    if (rdma_post_send_buffer(
            &ctx,
            1,
            pkt,
            total_len)) {

        perror("rdma_post_send_buffer");
        return 1;
    }

    if (rdma_poll_completion(
            &ctx,
            &wc)) {

        fprintf(stderr,
                "Completion failed\n");

        return 1;
    }

    printf(
        "RDMA send completed\n");

    rdma_cleanup(&ctx);

    return 0;
}
