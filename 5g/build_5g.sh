#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/5g"

mkdir -p "$OUT"

CC="${CC:-gcc}"

CFLAGS="
-O3
-g
-Wall
-Wextra
-Wpedantic
"

LIBS="
-lrdmacm
-libverbs
-pthread
"

echo "Building 5G RDMA/RoCE programs..."

$CC $CFLAGS \
    -I"$ROOT/common" \
    "$ROOT/common/rdma_common.c" \
    "$ROOT/5g/upf_rdma_tx.c" \
    -o "$OUT/upf_rdma_tx" \
    $LIBS

$CC $CFLAGS \
    -I"$ROOT/common" \
    "$ROOT/common/rdma_common.c" \
    "$ROOT/5g/upf_rdma_rx.c" \
    -o "$OUT/upf_rdma_rx" \
    $LIBS

echo
echo "Build finished:"
ls -lh "$OUT"
