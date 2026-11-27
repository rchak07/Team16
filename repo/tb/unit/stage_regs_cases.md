# Stage register case table

Raunak's 4 pipeline registers on B-phase2. EX/MEM has no clr (it is never flushed).

| # | Registers | Case | Do | Expect |
|---|---|---|---|---|
| 1 | all 4 | Passes on the edge | en = 1, new inputs, check before and after one clock | outputs unchanged before the edge, equal to the inputs after |
| 2 | all 4 | Holds when stalled | load values, then en = 0, change the inputs, clock | outputs keep the old values |
| 3 | IF/ID | Flush gives a nop | clr = 1, clock | InstrD = 0x00000013 (addi x0, x0, 0), every other output 0 |
| 3 | ID/EX, MEM/WB | Flush gives a bubble | clr = 1, clock | RegWrite, MemWrite, MemRead, Branch, Jump, Jalr all 0, Rd = 0 |
| 4 | IF/ID, ID/EX, MEM/WB | Flush beats stall | clr = 1 and en = 0, clock | still flushed |
| 5 | all 4 | Reset | rst = 1, clock | same as a flush (EX/MEM: every output 0) |

Every input gets a different value so two swapped wires would be caught.