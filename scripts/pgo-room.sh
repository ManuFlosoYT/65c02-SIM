#!/bin/bash
set -euo pipefail

TRAIN_CYCLES=150000000
BENCH_CYCLES=200000000
TRAIN_RUNS=3
BENCH_RUNS=3
NATIVE="OFF"

print_usage() {
    cat <<'EOF'
Usage: ./scripts/pgo-room.sh [options]

Options:
  --train-cycles <N>   Cycles per PGO training run (default: 150000000)
  --bench-cycles <N>   Cycles per benchmark run (default: 200000000)
  --train-runs <N>     Number of training runs (default: 3)
  --bench-runs <N>     Number of benchmark runs (default: 3)
  --native             Enable -march=native/-mtune=native
  --help               Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --train-cycles) TRAIN_CYCLES="$2"; shift 2 ;;
        --bench-cycles) BENCH_CYCLES="$2"; shift 2 ;;
        --train-runs) TRAIN_RUNS="$2"; shift 2 ;;
        --bench-runs) BENCH_RUNS="$2"; shift 2 ;;
        --native) NATIVE="ON"; shift ;;
        --help) print_usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; print_usage; exit 1 ;;
    esac
done

for value in "$TRAIN_CYCLES" "$BENCH_CYCLES" "$TRAIN_RUNS" "$BENCH_RUNS"; do
    if ! [[ "$value" =~ ^[0-9]+$ ]] || [[ "$value" -le 0 ]]; then
        echo "All numeric options must be positive integers." >&2
        exit 1
    fi
done

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

mkdir -p output/rom
./scripts/compile-bin.sh room >/dev/null

PGO_DATA_DIR="$ROOT_DIR/build/release-pgo-generate/pgo-data"
rm -rf "$PGO_DATA_DIR"

echo "[1/3] Building PGO generate binary..."
cmake -S . -B build/release-pgo-generate \
    -DCMAKE_BUILD_TYPE=Release \
    -DPGO_GENERATE=ON \
    -DPGO_USE=OFF \
    -DPERF_TUNE_NATIVE="$NATIVE" \
    -DPGO_DATA_DIR="$PGO_DATA_DIR" >/dev/null
cmake --build build/release-pgo-generate --target SIM_65C02 -j"$(nproc)" >/dev/null

echo "[2/3] Training profile data with room.bin..."
for run in $(seq 1 "$TRAIN_RUNS"); do
    build/release-pgo-generate/SIM_65C02 --headless --rom output/rom/room.bin --run-cycles "$TRAIN_CYCLES" >/dev/null 2>&1
    echo "  trained run $run/$TRAIN_RUNS"
done

echo "[3/3] Building PGO use binary and benchmarking..."
cmake -S . -B build/release-pgo-use \
    -DCMAKE_BUILD_TYPE=Release \
    -DPGO_GENERATE=OFF \
    -DPGO_USE=ON \
    -DPERF_TUNE_NATIVE="$NATIVE" \
    -DPGO_DATA_DIR="$PGO_DATA_DIR" >/dev/null
cmake --build build/release-pgo-use --target SIM_65C02 -j"$(nproc)" >/dev/null

./scripts/benchmark-room.sh \
    --bin build/release-pgo-use/SIM_65C02 \
    --rom output/rom/room.bin \
    --runs "$BENCH_RUNS" \
    --cycles "$BENCH_CYCLES" \
    --warmup-cycles 5000000
