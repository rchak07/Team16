module if_id (
    input logic clk,
    input logic rst,
    input logic en,
    input logic clr,
    input logic [31:0] InstrF,
    input logic [31:0] PCF,
    input logic [31:0] PCPlus4F,
    input logic PredTakenF,
    input logic [31:0] PredTargetF,
    output logic [31:0] InstrD,
    output logic [31:0] PCD,
    output logic [31:0] PCPlus4D,
    output logic PredTakenD,
    output logic [31:0] PredTargetD
);

always_ff @(posedge clk) begin
    if(rst || clr) begin
        InstrD <= 32'h00000013;  // load a nop
        PCD <= 32'b0;   // initialise register values to 0 in case of a reset or flush
        PCPlus4D <= 32'b0;
        PredTakenD <= 1'b0;
        PredTargetD <= 32'b0;
    end
    else if(en) begin
        InstrD <= InstrF;
        PCD <= PCF;
        PCPlus4D <= PCPlus4F;
        PredTakenD <= PredTakenF;
        PredTargetD <= PredTargetF;
    end
end


endmodule
