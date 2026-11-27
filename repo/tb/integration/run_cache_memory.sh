#!/usr/bin/env bash
# Example: ./run_cache_memory.sh /path/to/main-mem/repo/rtl/main_mem.sv
set -eu

here=$(cd "$(dirname "$0")" && pwd)
rtl=$(cd "$here/../../rtl" && pwd)
memory_source=${1:-"$rtl/main_mem.sv"}
if [[ $# -gt 1 || ! -f "$memory_source" ]]; then
    echo "Usage: $0 [path/to/main_mem.sv]" >&2
    exit 2
fi
memory_source="$(cd "$(dirname "$memory_source")" && pwd)/$(basename "$memory_source")"
build=$(mktemp -d "${TMPDIR:-/tmp}/team16-cache-memory.XXXXXXXX")
trap 'rm -rf "$build"' EXIT
cd "$build"

# Mixed reset styles meet at the wrapper. The published main_mem counter is
# four bits wide and is compared with the integer LATENCY parameter.
cat > checks.vlt <<'RULES'
`verilator_config
lint_off -rule SYNCASYNCNET -file "*cache_memory_pair.sv"
lint_off -rule WIDTHEXPAND -file "*main_mem.sv"
RULES

status=0
for latency in 0 6 12; do
    if ! verilator -Wall --cc --exe --build --top-module cache_memory_pair \
        "-GLATENCY=$latency" --Mdir "obj_$latency" checks.vlt \
        "$here/cache_memory_pair.sv" "$rtl/data_cache.sv" "$memory_source" \
        "$here/cache_memory_tb.cpp" >"build_$latency.log" 2>&1; then
        cat "build_$latency.log" >&2
        status=1
        continue
    fi
    "obj_$latency/Vcache_memory_pair" "$latency" || status=1
done
exit "$status"
