#include "../common/rdma_common.h"

#include <stdio.h>
#include <stdlib.h>


static void process_upf_packet(
    struct packet_desc *pkt)
{
    printf(
        "UPF accelerated packet\n");

    printf(
        "  TEID : 0x%x\n",
        pkt->teid);

    printf(
        "  UE   : %u\n",
        pkt->ue_id);

    printf(
        "  QFI  : %u\n",
        pkt->qfi);

    printf(
        "  Size : %u\n",
        pkt->length);

    /*
     * Production pipeline:
     *
     * 1. TEID lookup
     * 2. PDR match
     * 3. FAR action
     * 4. QER enforcement
     * 5. URR accounting
     * 6. DPI classification
     * 7. GTP-U decapsulation
     * 8. NAT/routing
     * 9. Forward to N6
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

        fprintf(stderr,
                "RDMA server init failed\n");

        return 1;
    }

    pkt =
        (struct packet_desc *)
        ctx.buffer;

    if (rdma_post_receive(
            &ctx,
            1,
            pkt,
            BUFFER_SIZE)) {

        perror("rdma_post_receive");
        return 1;
    }

    printf(
        "Waiting for 5G packet...\n");

    if (rdma_poll_completion(
            &ctx,
            &wc)) {

        return 1;
    }

    if (wc.opcode !=
        IBV_WC_RECV) {

        fprintf(stderr,
                "Unexpected completion\n");

        return 1;
    }

    if (pkt->magic !=
        MSG_MAGIC_5G) {

        fprintf(stderr,
                "Invalid 5G message\n");

        return 1;
    }

    process_upf_packet(pkt);

    rdma_cleanup(&ctx);

    return 0;
}
