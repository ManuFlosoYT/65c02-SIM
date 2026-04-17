#!/bin/bash
set -euo pipefail

BIN_PATH=""
ROM_PATH=""
RUN_CYCLES=200000000
WARMUP_CYCLES=20000000
RUNS=5
KEEP_LOGS=false

print_usage() {
    cat <<'EOF'
Usage: ./scripts/benchmark-room.sh [options]

Options:
  --bin <path>            Emulator binary path
  --rom <path>            room.bin path
  --cycles <N>            Cycles per measured run (default: 200000000)
  --warmup-cycles <N>     Warmup cycles before measured runs (default: 20000000)
  --runs <N>              Number of measured runs (default: 5)
  --keep-logs             Keep per-run logs in /tmp
  --help                  Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bin)
            BIN_PATH="$2"
            shift 2
            ;;
        --rom)
            ROM_PATH="$2"
            shift 2
            ;;
        --cycles)
            RUN_CYCLES="$2"
            shift 2
            ;;
        --warmup-cycles)
            WARMUP_CYCLES="$2"
            shift 2
            ;;
        --runs)
            RUNS="$2"
            shift 2
            ;;
        --keep-logs)
            KEEP_LOGS=true
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            print_usage
            exit 1
            ;;
    esac
done

if [[ -z "$BIN_PATH" ]]; then
    for candidate in "build/release/SIM_65C02" "output/SIM_65C02" "build/debug/SIM_65C02"; do
        if [[ -x "$candidate" ]]; then
            BIN_PATH="$candidate"
            break
        fi
    done
fi

if [[ -z "$ROM_PATH" ]]; then
    ROM_PATH="output/rom/room.bin"
fi

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binary not found or not executable: $BIN_PATH" >&2
    echo "Build first: ./scripts/build-linux.sh" >&2
    exit 1
fi

if [[ ! -f "$ROM_PATH" ]]; then
    echo "ROM not found: $ROM_PATH" >&2
    echo "Generate it with: ./scripts/compile-bin.sh room" >&2
    exit 1
fi

if ! [[ "$RUN_CYCLES" =~ ^[0-9]+$ ]] || ! [[ "$WARMUP_CYCLES" =~ ^[0-9]+$ ]] || ! [[ "$RUNS" =~ ^[0-9]+$ ]]; then
    echo "--cycles, --warmup-cycles and --runs must be positive integers" >&2
    exit 1
fi

if [[ "$RUN_CYCLES" -le 0 ]] || [[ "$WARMUP_CYCLES" -lt 0 ]] || [[ "$RUNS" -le 0 ]]; then
    echo "Invalid numeric values for benchmark options" >&2
    exit 1
fi

RESULTS_FILE=$(mktemp)
LOG_PREFIX=$(mktemp -u /tmp/room-bench.XXXXXX)
trap 'rm -f "$RESULTS_FILE"; if [[ "$KEEP_LOGS" == "false" ]]; then rm -f "${LOG_PREFIX}".*.log 2>/dev/null || true; fi' EXIT

run_headless() {
    local cycles=$1
    local log_file=$2
    "$BIN_PATH" --headless --rom "$ROM_PATH" --run-cycles "$cycles" >"$log_file" 2>&1
}

validate_run_log() {
    local log_file=$1
    if grep -q "Emulator stopped with code:" "$log_file"; then
        echo "Detected emulator stop in benchmark run. See log: $log_file" >&2
        exit 1
    fi
}

echo "Benchmark setup:"
echo "  bin:    $BIN_PATH"
echo "  rom:    $ROM_PATH"
echo "  cycles: $RUN_CYCLES"
echo "  runs:   $RUNS"
echo

if [[ "$WARMUP_CYCLES" -gt 0 ]]; then
    warmup_log="${LOG_PREFIX}.warmup.log"
    run_headless "$WARMUP_CYCLES" "$warmup_log"
    validate_run_log "$warmup_log"
fi

printf "%-5s %-12s %-15s %-10s\n" "Run" "Elapsed(s)" "Cycles/s" "Sim(MHz)"
for run in $(seq 1 "$RUNS"); do
    run_log="${LOG_PREFIX}.${run}.log"
    start_ns=$(date +%s%N)
    run_headless "$RUN_CYCLES" "$run_log"
    end_ns=$(date +%s%N)
    validate_run_log "$run_log"

    elapsed_ns=$((end_ns - start_ns))
    elapsed_s=$(awk -v ns="$elapsed_ns" 'BEGIN { printf "%.6f", ns / 1000000000.0 }')
    cycles_per_sec=$(awk -v c="$RUN_CYCLES" -v ns="$elapsed_ns" 'BEGIN { printf "%.2f", c * 1000000000.0 / ns }')
    sim_mhz=$(awk -v cps="$cycles_per_sec" 'BEGIN { printf "%.2f", cps / 1000000.0 }')

    printf "%-5s %-12s %-15s %-10s\n" "$run" "$elapsed_s" "$cycles_per_sec" "$sim_mhz"
    printf "%s\n" "$sim_mhz" >> "$RESULTS_FILE"
done

mean_mhz=$(awk '{sum+=$1} END { if (NR>0) printf "%.2f", sum/NR; else print "0.00" }' "$RESULTS_FILE")
best_mhz=$(sort -n "$RESULTS_FILE" | tail -n 1)
worst_mhz=$(sort -n "$RESULTS_FILE" | head -n 1)
median_mhz=$(sort -n "$RESULTS_FILE" | awk '{a[NR]=$1} END { if (NR==0) {print "0.00"; exit} if (NR%2==1) {printf "%.2f", a[(NR+1)/2]} else {printf "%.2f", (a[NR/2] + a[NR/2+1]) / 2} }')

echo
echo "Summary:"
echo "  mean_mhz:   $mean_mhz"
echo "  median_mhz: $median_mhz"
echo "  best_mhz:   $best_mhz"
echo "  worst_mhz:  $worst_mhz"
