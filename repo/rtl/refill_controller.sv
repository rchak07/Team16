// Connects the cache to the main_mem for the read path. Could consider removing later.
module refill_controller (
    input logic miss,
    input logic [31:0] missAddr,
    output logic fillEn,
    output logic [127: 0] fillLine,
    output logic ReadReq,
    output logic [31:0] ReadAddr,
    input logic ReadDone,
    input logic [127:0] ReadLine  
);

    assign fillEn = ReadDone;
    assign fillLine = ReadLine;
    assign ReadReq = miss;
    assign ReadAddr = missAddr;



endmodule
