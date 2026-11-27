module held_operands (
  input  logic        clk, rst, StallE,
  input  logic [31:0] ForwardedAE, ForwardedBE,
  output logic [31:0] OperandAE, OperandBE
);
  logic        held;
  logic [31:0] savedA, savedB;

  // The first wait edge also bubbles WB. Save the forwarded operands before
  // WB disappears, including values needed by branches and stores.
  always_ff @(posedge clk) begin
    if (rst) begin
      held <= 1'b0;
    end else begin
      if (StallE && !held) begin
        savedA <= ForwardedAE;
        savedB <= ForwardedBE;
      end
      held <= StallE;
    end
  end

  // Keep the operands through the rising edge that finally accepts EX.
  assign OperandAE = held ? savedA : ForwardedAE;
  assign OperandBE = held ? savedB : ForwardedBE;
endmodule
