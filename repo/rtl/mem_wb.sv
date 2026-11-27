module mem_wb(
    input logic clk,
    input logic rst,
    input logic en,
    input logic clr,
    input logic RegWriteM,
    input logic MemReadM,
    input logic [31:0] ExecResultM,
    input logic [31:0] ReadDataM,
    input logic [4:0] RdM,
    output logic RegWriteW,
    output logic MemReadW,
    output logic [31:0] ExecResultW,
    output logic [31:0] ReadDataW,
    output logic [4:0] RdW
);

always_ff @(posedge clk) begin
    if(rst || clr) begin
        RegWriteW <= 1'b0;
        MemReadW <= 1'b0;
        ExecResultW <= 32'b0;
        ReadDataW <= 32'b0;
        RdW <= 5'b0;
    end
    else if(en) begin
        RegWriteW <= RegWriteM;
        MemReadW <= MemReadM;
        ExecResultW <= ExecResultM;
        ReadDataW <= ReadDataM;
        RdW <= RdM;
    end
end
endmodule
