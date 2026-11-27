#!/usr/bin/env bash
# Run from anywhere. Optional arguments select INDEX_BITS (default: 2 and 6).
# GoogleTest overrides: GTEST_CFLAGS="-I..." GTEST_LIBS="-L... -lgtest_main -lgtest -lpthread".
set -eu
here=$(cd "$(dirname "$0")" && pwd)
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
if command -v pkg-config >/dev/null && pkg-config --exists gtest_main; then
    cflags=${GTEST_CFLAGS:-$(pkg-config --cflags gtest_main)}
    libs=${GTEST_LIBS:-$(pkg-config --libs gtest_main)}
else
    cflags=${GTEST_CFLAGS:-}
    libs=${GTEST_LIBS:--lgtest_main -lgtest -lpthread}
fi
if [ "$#" -eq 0 ]; then set -- 2 6; fi
status=0
for bits in "$@"; do
    case "$bits" in 1|2|3|4|5|6|7|8|9|10) ;; *)
        echo "INDEX_BITS must be in 1..10 for these test addresses" >&2
        exit 2
    esac
    echo "Testing INDEX_BITS=$bits (memory latencies 0, 6, 12)"
    verilator --cc --exe --build -Wall --prefix Vdut --top-module data_cache \
        --Mdir "$build/idx$bits" "-GINDEX_BITS=$bits" \
        -CFLAGS "$cflags -DTEST_CACHE_INDEX_BITS=$bits" -LDFLAGS "$libs" \
        "$here/../rtl/data_cache.sv" "$here/unit/data_cache-reference_tb.cpp"
    "$build/idx$bits/Vdut" || status=1
done
exit "$status"
