// Directly joins the cache refill handshake to the team's slow memory.
// The test can issue a read on the backing-memory bus to check write-through.
module cache_memory_pair #(parameter int LATENCY = 6) (
    input  logic clk, rst,
    input  logic MemRead, MemWrite,
    input  logic [31:0] A, WD,
    input  logic [2:0] funct3,
    input  logic ProbeRead,
    input  logic [31:0] ProbeAddr,
    output logic [31:0] RD, missAddr,
    output logic MemBusy, miss, fillWay, ReadDone, WriteReq, WriteDone,
    output logic [127:0] ReadLine
);
    logic [31:0] WriteAddr, WriteData;
    logic [2:0] WriteFunct3;
    data_cache cache (
        .clk, .rst, .MemRead, .MemWrite, .A, .WD, .funct3, .RD, .MemBusy,
        .miss, .missAddr, .fillWay, .fillEn(ReadDone && !ProbeRead),
        .fillLine(ReadLine), .WriteReq, .WriteAddr, .WriteData, .WriteFunct3, .WriteDone
    );
    main_mem #(.LATENCY(LATENCY)) memory (
        .clk, .rst, .ReadReq(ProbeRead || miss),
        .ReadAddr(ProbeRead ? ProbeAddr : missAddr), .ReadDone, .ReadLine,
        .WriteReq, .WriteAddr, .WriteData, .WriteFunct3, .WriteDone
    );
endmodule
