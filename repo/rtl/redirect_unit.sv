module redirect_unit (
  input  logic        BranchE, JumpE, JalrE, BranchTakenE,
  input  logic [31:0] ALUResultE, PCTargetE, PCPlus4E,
  input  logic        PredTakenE,
  input  logic [31:0] PredTargetE,
  input  logic        MemBusyM,
  output logic        TakenE,
  output logic [31:0] TargetE,
  output logic        MispredictE,
  output logic [31:0] RedirectPCE,
  output logic        UpdateEn
);
  assign TakenE = JumpE || JalrE || (BranchE && BranchTakenE);
  assign TargetE = JalrE ? (ALUResultE & 32'hffff_fffe) : PCTargetE;
  assign MispredictE = (TakenE != PredTakenE) ||
                       (TakenE && PredTakenE && TargetE != PredTargetE);
  assign RedirectPCE = TakenE ? TargetE : PCPlus4E;

  // A branch held in EX during a cache miss trains only when it advances.
  // JALR is resolved here but is never entered in the predictor.
  assign UpdateEn = (BranchE || JumpE) && !MemBusyM;
endmodule
