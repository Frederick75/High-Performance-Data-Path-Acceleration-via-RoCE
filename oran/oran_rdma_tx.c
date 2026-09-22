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

    size_t payload_len = 1400;

    if (argc != 2) {

        fprintf(stderr,
            "Usage: %s "
            "<DU-RDMA-node-IP>\n",
            argv[0]);

        return 1;
    }

    if (rdma_init_client(
            &ctx,
            argv[1],
            RDMA_PORT)) {

        fprintf(stderr,
                "RDMA init failed\n");

        return 1;
    }

    pkt =
        (struct packet_desc *)
        ctx.buffer;

    memset(pkt, 0,
           sizeof(*pkt));

    pkt->magic =
        MSG_MAGIC_ORAN;

    pkt->type =
        PKT_ORAN_F1U;

    pkt->length =
        payload_len;

    pkt->teid =
        0x2201;

    pkt->ue_id =
        2001;

    pkt->drb_id =
        4;

    pkt->qfi =
        7;

    pkt->timestamp_ns =
        now_ns();

    memset(
        pkt->payload,
        0xCD,
        payload_len);

    size_t total =
        sizeof(*pkt) +
        payload_len;

    printf(
        "Sending O-RAN F1-U packet "
        "TEID=0x%x UE=%u DRB=%u\n",
        pkt->teid,
        pkt->ue_id,
        pkt->drb_id);

    if (rdma_post_send_buffer(
            &ctx,
            100,
            pkt,
            total)) {

        perror("send");
        return 1;
    }

    if (rdma_poll_completion(
            &ctx,
            &wc)) {

        return 1;
    }

    printf(
        "O-RAN RDMA transfer complete\n");

    rdma_cleanup(&ctx);

    return 0;
}
