Install dependencies on Ubuntu:

sudo apt update

sudo apt install \
    build-essential \
    rdma-core \
    ibverbs-utils \
    librdmacm-dev \
    libibverbs-dev \
    infiniband-diags

    Verify RoCE hardware

    Check devices:

    ibv_devices

    Then:

    ibv_devinfo

    Check RDMA links:

    rdma link

    Typical output might include:

    link mlx5_0/1 state ACTIVE physical_state LINK_UP

    Test the 5G implementation

    Receiver:
    
    sudo ./build/5g/upf_rdma_rx 192.168.100.20

    Sender:

    sudo ./build/5g/upf_rdma_tx 192.168.100.20

    Expected receiver output:
    Waiting for RDMA connection...
    Waiting for 5G packet...

    UPF accelerated packet
      TEID : 0x1001
      UE   : 100
      QFI  : 9
      Size : 1024
    
