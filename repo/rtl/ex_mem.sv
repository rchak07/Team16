module ex_mem(
    input logic clk,
    input logic rst,
    input logic en,
    input logic RegWriteE,
    input logic MemWriteE,
    input logic MemReadE,
    input logic [31:0] ExecResultE,
    input logic [31:0] WriteDataE,
    input logic [4:0] RdE,
    input logic [2:0] funct3E,
    output logic RegWriteM,
    output logic MemWriteM,
    output logic MemReadM,
    output logic [31:0] ExecResultM,
    output logic [31:0] WriteDataM,
    output logic [4:0] RdM,
    output logic [2:0] funct3M
);

always_ff @(posedge clk) begin
    if(rst) begin
        RegWriteM <= 1'b0;
        MemWriteM <= 1'b0;
        MemReadM <= 1'b0;
        ExecResultM <= 32'b0;
        WriteDataM <= 32'b0;
        RdM <= 5'b0;
        funct3M <= 3'b0;
    end 
    else if(en) begin
        RegWriteM <= RegWriteE;
        MemWriteM <= MemWriteE;
        MemReadM <= MemReadE;
        ExecResultM <= ExecResultE;
        WriteDataM <= WriteDataE;
        RdM <= RdE;
        funct3M <= funct3E;
    end
end
endmodule
