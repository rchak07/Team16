# Data cache case table

Memory before each test (little-endian, byte 0 is the lowest):

| Address | Word |
|---|---|
| 0x10000 | 0x12345678 |
| 0x10004 | 0x8000FFFE |
| 0x10400 | 0xCAFEF00D (same index as 0x10000, different tag) |
| 0x10800 | 0x0BADF00D (same index again, third tag) |

| # | Case | Do | Expect |
|---|---|---|---|
| 1 | Reset empties the cache | reset, then lw 0x10000 | miss = 1, MemBusy = 1 |
| 2 | Miss asks for the line start | lw 0x1010C | missAddr = 0x10100 |
| 3 | Miss then fill, LATENCY 0 | lw 0x10000 | busy 1 cycle, then RD = 0x12345678 |
| 4 | Miss then fill, LATENCY 6 | lw 0x10000 | busy 7 cycles, then RD = 0x12345678 |
| 5 | Same line hits | after 3, lw 0x10004 | no miss, RD = 0x8000FFFE |
| 6 | lb sign-extends | lb 0x10007 | RD = 0xFFFFFF80 |
| 7 | lbu zero-extends | lbu 0x10007 | RD = 0x00000080 |
| 8 | lh from the upper half | lh 0x10006 | RD = 0xFFFF8000 |
| 9 | lhu from the upper half | lhu 0x10006 | RD = 0x00008000 |
| 10 | Both ways | lw 0x10000 then lw 0x10400 | first fills way 0, second fills way 1, then both hit |
| 11 | sw hit | after lw 0x10000: sw 0xDEADBEEF to 0x10000, lw 0x10000 | RD = 0xDEADBEEF |
| 12 | sb hit | after lw 0x10000: sb 0xAA to 0x10001, lw 0x10000 | RD = 0x1234AA78 |
| 13 | sh hit | after lw 0x10000: sh 0xBBBB to 0x10002, lw 0x10000 | RD = 0xBBBB5678 |
| 14 | Idle | MemRead = 0, MemWrite = 0 | MemBusy = 0 |
| 15 | LRU replaces the older way | lw 0x10000, lw 0x10400, lw 0x10000 again, then lw 0x10800 | 0x10800 replaces 0x10400, so 0x10000 still hits and 0x10400 misses |
| 16 | Sequential words | lw 0x10000, 0x10004, 0x10008, 0x1000C | 1 miss, then 3 hits |