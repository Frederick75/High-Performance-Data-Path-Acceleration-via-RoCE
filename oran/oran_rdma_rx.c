#include "../common/rdma_common.h"

#include <stdio.h>


static void du_process_packet(
    struct packet_desc *pkt)
{
    printf("\nO-DU accelerated packet\n");

    printf(
        "  TEID : 0x%x\n",
        pkt->teid);

    printf(
        "  UE   : %u\n",
        pkt->ue_id);

    printf(
        "  DRB  : %u\n",
        pkt->drb_id);

    printf(
        "  QFI  : %u\n",
        pkt->qfi);

    printf(
        "  Size : %u\n",
        pkt->length);

    /*
     * O-DU processing pipeline:
     *
     * F1-U:
     *
     * 1. GTP-U decapsulation
     * 2. UE lookup
     * 3. DRB lookup
     * 4. PDCP processing
     * 5. RLC processing
     * 6. QoS queue mapping
     * 7. scheduler interaction
     * 8. MAC processing
     * 9. High-PHY/fronthaul preparation
     */
}


int main(
    int argc,
    char **argv)
{
    struct rdma_ctx ctx;

    struct packet_desc *pkt;

    struct ibv_wc wc;

    const char *bind_ip =
        NULL;

    if (argc > 1)
        bind_ip = argv[1];

    if (rdma_init_server(
            &ctx,
            bind_ip,
            RDMA_PORT)) {

        return 1;
    }

    pkt =
        (struct packet_desc *)
        ctx.buffer;

    if (rdma_post_receive(
            &ctx,
            10,
            pkt,
            BUFFER_SIZE)) {

        perror("post receive");
        return 1;
    }

    printf(
        "Waiting for O-RAN "
        "RDMA traffic...\n");

    if (rdma_poll_completion(
            &ctx,
            &wc)) {

        return 1;
    }

    if (pkt->magic !=
        MSG_MAGIC_ORAN) {

        fprintf(stderr,
                "Bad O-RAN message\n");

        return 1;
    }

    du_process_packet(pkt);

    rdma_cleanup(&ctx);

    return 0;
}
