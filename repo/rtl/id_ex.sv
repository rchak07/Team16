module id_ex(
    input logic clk,
    input logic rst,
    input logic en,
    input logic clr,
    input logic RegWriteD,
    input logic [1:0] ResultSrcD,
    input logic MemWriteD,
    input logic MemReadD,
    input logic [3:0] ALUControlD,
    input logic ALUSrcD,
    input logic ALUSrcAD,
    input logic BranchD,
    input logic JumpD,
    input logic JalrD,
    input logic [31:0] RD1D,
    input logic [31:0] RD2D,
    input logic [31:0] ImmExtD,
    input logic [31:0] PCD,
    input logic [31:0] PCPlus4D,
    input logic [4:0] Rs1D,
    input logic [4:0] Rs2D,
    input logic [4:0] RdD,
    input logic [2:0] funct3D,
    input logic PredTakenD,
    input logic [31:0] PredTargetD,
    output logic RegWriteE,
    output logic [1:0] ResultSrcE,
    output logic MemWriteE,
    output logic MemReadE,
    output logic [3:0] ALUControlE,
    output logic ALUSrcE,
    output logic ALUSrcAE,
    output logic BranchE,
    output logic JumpE,
    output logic JalrE,
    output logic [31:0] RD1E,
    output logic [31:0] RD2E,
    output logic [31:0] ImmExtE,
    output logic [31:0] PCE,
    output logic [31:0] PCPlus4E,
    output logic [4:0] Rs1E,
    output logic [4:0] Rs2E,
    output logic [4:0] RdE,
    output logic [2:0] funct3E,
    output logic PredTakenE,
    output logic [31:0] PredTargetE
);

always_ff @(posedge clk) begin
    if(rst || clr) begin
        RegWriteE <= 1'b0;
        ResultSrcE <= 2'b0;
        MemWriteE <= 1'b0;
        MemReadE <= 1'b0;
        ALUControlE <= 4'b0;
        ALUSrcE <= 1'b0;
        ALUSrcAE <= 1'b0;
        BranchE <= 1'b0;
        JumpE <= 1'b0;
        JalrE <= 1'b0;
        RD1E <= 32'b0;
        RD2E <= 32'b0;
        ImmExtE <= 32'b0;
        PCE <= 32'b0;
        PCPlus4E <= 32'b0;
        Rs1E <= 5'b0;
        Rs2E <= 5'b0;
        RdE <= 5'b0;
        funct3E <= 3'b0;
        PredTakenE <= 1'b0;
        PredTargetE <= 32'b0;
    end
    else if(en) begin
        RegWriteE <= RegWriteD;
        ResultSrcE <= ResultSrcD;
        MemWriteE <= MemWriteD;
        MemReadE <= MemReadD;
        ALUControlE <= ALUControlD;
        ALUSrcE <= ALUSrcD;
        ALUSrcAE <= ALUSrcAD;
        BranchE <= BranchD;
        JumpE <= JumpD;
        JalrE <= JalrD;
        RD1E <= RD1D;
        RD2E <= RD2D;
        ImmExtE <= ImmExtD;
        PCE <= PCD;
        PCPlus4E <= PCPlus4D;
        Rs1E <= Rs1D;
        Rs2E <= Rs2D;
        RdE <= RdD;
        funct3E <= funct3D;
        PredTakenE <= PredTakenD;
        PredTargetE <= PredTargetD;
    end
end
endmodule
