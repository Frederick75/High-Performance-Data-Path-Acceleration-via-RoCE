#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/oran"

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

echo "Building O-RAN RDMA/RoCE programs..."

$CC $CFLAGS \
    -I"$ROOT/common" \
    "$ROOT/common/rdma_common.c" \
    "$ROOT/oran/oran_rdma_tx.c" \
    -o "$OUT/oran_rdma_tx" \
    $LIBS

$CC $CFLAGS \
    -I"$ROOT/common" \
    "$ROOT/common/rdma_common.c" \
    "$ROOT/oran/oran_rdma_rx.c" \
    -o "$OUT/oran_rdma_rx" \
    $LIBS

echo
echo "Build complete:"
ls -lh "$OUT"
