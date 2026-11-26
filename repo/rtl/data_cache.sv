// Data cache: 2-way set associative, sits between pipeline and main_mem
module data_cache #(
    parameter SETS = 64,
    parameter LINE_BYTES = 16
)(
    // pipeline side
    input logic clk,
    input logic rst,
    input logic MemRead,
    input logic MemWrite,
    input logic [31:0] A,
    input logic [31:0] WD,
    input logic [2:0] funct3,
    output logic [31:0] RD,
    output logic MemBusy, // 1 while an access hasn't finished

    // memory side (to main_mem)
    output logic ReadReq, // ask main_mem for a line (load miss)
    output logic [31:0] ReadAddr, // line address, bottom 4 bits 0
    input logic ReadDone,
    input logic [127:0] ReadLine,
    output logic WriteReq, // write-through store
    output logic [31:0] WriteAddr,
    output logic [31:0] WriteData,
    output logic [2:0] WriteFunct3,
    input logic WriteDone
);

    localparam ADDR_BITS = 17; // memory is 0x00000 to 0x1FFFF
    localparam OFFSET_BITS = $clog2(LINE_BYTES); // 4
    localparam INDEX_BITS  = $clog2(SETS); // 6                    
    localparam TAG_BITS    = ADDR_BITS - INDEX_BITS - OFFSET_BITS; // 7   

    // address split: tag | index | offset
    logic [OFFSET_BITS-1:0] offset; 
    assign offset = A[OFFSET_BITS-1:0]; // A[3:0]

    logic [INDEX_BITS-1:0] index;
    assign index = A[OFFSET_BITS+INDEX_BITS-1:OFFSET_BITS]; // A[9:4]

    logic [TAG_BITS-1:0] tag;
    assign tag = A[ADDR_BITS-1:OFFSET_BITS+INDEX_BITS]; // A[16:10]

    // 2 ways each with a valid bit, tag and line per set
    logic valid0 [SETS-1:0];
    logic valid1 [SETS-1:0];
    logic [TAG_BITS-1:0] tag0 [SETS-1:0];
    logic [TAG_BITS-1:0] tag1 [SETS-1:0];
    logic [LINE_BYTES*8-1:0] data0 [SETS-1:0];
    logic [LINE_BYTES*8-1:0] data1 [SETS-1:0];

    // which way to replace next
    logic lru [SETS-1:0];

    logic hit0;
    logic hit1; 
    logic hit;

    // hit if either way holds this address
    assign hit0 = valid0[index] && (tag0[index] == tag);
    assign hit1 = valid1[index] && (tag1[index] == tag);
    assign hit = hit0 || hit1;

    // read path: line -> word -> byte/half -> RD
    logic [127:0] line;
    assign line = hit0 ? data0[index] : data1[index];

    logic [31:0] word;
    assign word = line[offset[3:2]*32 +: 32];

    logic [7:0] rbyte;
    assign rbyte = word[offset[1:0]*8 +: 8];

    logic [15:0] rhalf;
    assign rhalf = word[offset[1]*16 +: 16];      

    always_comb begin
        case (funct3)
            3'b000: RD = {{24{rbyte[7]}}, rbyte}; // lb
            3'b001: RD = {{16{rhalf[15]}}, rhalf}; // lh
            3'b010: RD = word; // lw 
            3'b100: RD = {24'b0, rbyte}; // lbu
            3'b101: RD = {16'b0, rhalf}; // lhu                              
            default: RD = 32'b0;
        endcase
    end

    // store path: copy line, overwrite stored bytes
    logic [127:0] newline;

    always_comb begin
        newline = line;
        case (funct3)
            3'b000: newline[offset*8 +: 8] = WD[7:0]; // sb
            3'b001: newline[offset[3:1]*16 +: 16] = WD[15:0]; // sh
            3'b010: newline[offset[3:2]*32 +: 32] = WD; // sw
            default: ;
        endcase
    end

    // reset empties the cache, a store hit updates the line
    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < SETS; i++) begin
                valid0[i] <= 1'b0;
                valid1[i] <= 1'b0;
                lru[i] <= 1'b0;
            end
        end else if (MemWrite && hit) begin
            if (hit0)
                data0[index] <= newline;
            else
                data1[index] <= newline;
        end
    end

    // placeholders until later
    assign MemBusy     = 1'b0;
    assign ReadReq     = 1'b0;
    assign ReadAddr    = 32'b0;
    assign WriteReq    = 1'b0;
    assign WriteAddr   = 32'b0;
    assign WriteData   = 32'b0;
    assign WriteFunct3 = 3'b0;

endmodule
