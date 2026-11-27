// Package A developer check. Run from the repository root with Verilator 5.
// The original single-cycle decoder is the reference for the retained controls.
//
// build=$(mktemp -d)
// git show 09aeba2112b8bf58f3bf9caf915ebaaf47da5713:repo/rtl/control_unit.sv | sed 's/module control_unit (/module phase1_control (/' > "$build/phase1_control.sv"
// (verilator --binary --timing --top-module control_path_check --Mdir "$build/obj" -y repo/rtl -y "$build" repo/tb/selfcheck/control_path_check.sv && "$build/obj/Vcontrol_path_check")

module control_path_check;
  logic [6:0] op;
  logic [2:0] funct3, ImmSrc, oldImmSrc;
  logic funct7b5, RegWrite, ALUSrc, ALUSrcA, MemWrite, MemRead, Branch, Jump, Jalr;
  logic oldRegWrite, oldALUSrc, oldALUSrcA, oldMemWrite;
  logic [1:0] ResultSrc, oldResultSrc, oldPCSrc;
  logic [3:0] ALUControl, oldALUControl;

  control_unit decoder (.*);
  phase1_control old_decoder (
    .BranchTaken(1'b1), .RegWrite(oldRegWrite), .ALUSrc(oldALUSrc),
    .ALUSrcA(oldALUSrcA), .MemWrite(oldMemWrite), .ImmSrc(oldImmSrc),
    .ResultSrc(oldResultSrc), .PCSrc(oldPCSrc), .ALUControl(oldALUControl), .*
  );

  logic [4:0] Rs1D, Rs2D, Rs1E, Rs2E, RdE, RdM, RdW;
  logic MemReadE, RegWriteM, RegWriteW, MemBusyM, injectedMispredict;
  logic [1:0] ForwardAE, ForwardBE;
  logic StallF, StallD, StallE, StallM, FlushD, FlushE, FlushW;

  hazard_unit hazard (.MispredictE(injectedMispredict), .*);

  logic BranchE, JumpE, JalrE, BranchTakenE, PredTakenE;
  logic [31:0] ALUResultE, PCTargetE, PCPlus4E, PredTargetE;
  logic TakenE, MispredictE, UpdateEn;
  logic [31:0] TargetE, RedirectPCE;

  redirect_unit redirect (.*);

  // The last match wins, so the younger writer in MEM overrides WB.
  function automatic logic [1:0] expected_forward(input logic [4:0] source);
    logic [1:0] selected = 2'b00;
    if (source != 0 && RegWriteW && RdW == source) selected = 2'b01;
    if (source != 0 && RegWriteM && RdM == source) selected = 2'b10;
    return selected;
  endfunction

  task automatic check_decoder();
    for (int i = 0; i < (1 << 11); i++) begin
      {op, funct3, funct7b5} = 11'(i);
      #1;
      if ({RegWrite, ALUSrc, ALUSrcA, ImmSrc, MemWrite, ResultSrc, ALUControl} !=
          {oldRegWrite, oldALUSrc, oldALUSrcA, oldImmSrc, oldMemWrite, oldResultSrc, oldALUControl})
        $fatal(1, "Retained decode mismatch: op=%h funct3=%h bit30=%b", op, funct3, funct7b5);

      // The old decoder selects a target for each legal branch when BranchTaken is high.
      if (MemRead != (oldRegWrite && oldResultSrc == 2'b01) ||
          Branch != (op == 7'h63 && oldPCSrc == 2'b01) ||
          Jump != (op == 7'h6f) || Jalr != (oldPCSrc == 2'b10))
        $fatal(1, "Instruction class mismatch: op=%h funct3=%h bit30=%b", op, funct3, funct7b5);
    end
    $display("PASS decoder: 2048 inputs");
  endtask

  task automatic check_forwarding();
    // Enumerate both independently chosen source and destination registers and both write enables.
    for (int i = 0; i < (1 << 22); i++) begin
      {Rs1E, Rs2E, RdM, RdW, RegWriteM, RegWriteW} = 22'(i);
      #1;
      if (ForwardAE != expected_forward(Rs1E) || ForwardBE != expected_forward(Rs2E))
        $fatal(1, "Forwarding mismatch: rs1=%d rs2=%d rdM=%d rdW=%d weM=%b weW=%b",
               Rs1E, Rs2E, RdM, RdW, RegWriteM, RegWriteW);
    end
    $display("PASS forwarding: 4194304 inputs");
  endtask

  task automatic check_pipeline_controls();
    logic [6:0] actual, expected;
    // Bit order is Stall{F,D,E,M}, Flush{D,E,W}.
    localparam logic [6:0] RUN = 7'b0000_000, LOAD_USE = 7'b1100_010;
    localparam logic [6:0] REDIRECT = 7'b0000_110, MEMORY_WAIT = 7'b1111_001;
    for (int i = 0; i < (1 << 18); i++) begin
      {Rs1D, Rs2D, RdE, MemReadE, injectedMispredict, MemBusyM} = 18'(i);
      #1;
      if (MemBusyM)
        expected = MEMORY_WAIT;
      else if (injectedMispredict)
        expected = REDIRECT;
      else if (MemReadE && RdE != 0 && (Rs1D == RdE || Rs2D == RdE))
        expected = LOAD_USE;
      else
        expected = RUN;
      actual = {StallF, StallD, StallE, StallM, FlushD, FlushE, FlushW};
      if (actual != expected)
        $fatal(1, "Stall/flush mismatch: inputs=%h expected=%b actual=%b", i, expected, actual);
    end
    $display("PASS stalls and flushes: 262144 inputs");
  endtask

  task automatic check_redirect();
    logic take;
    logic [31:0] target;
    for (int mode = 0; mode < 64; mode++) begin
      {BranchE, JumpE, JalrE, BranchTakenE, PredTakenE, MemBusyM} = 6'(mode);
      for (int offset = 0; offset < 8; offset++) begin
        ALUResultE = 32'h7fff_fffd + offset;  // both odd and even JALR targets
        PCTargetE = 32'hf000_0000 + 2 * offset;
        PCPlus4E = 32'h0080_000c;
        for (int prediction = 0; prediction < 4; prediction++) begin
          case (prediction)
            0: PredTargetE = ALUResultE & ~32'd1;
            1: PredTargetE = ALUResultE;
            2: PredTargetE = PCTargetE;
            3: PredTargetE = PCPlus4E;
          endcase
          #1;
          take = JalrE || JumpE || (BranchE && BranchTakenE);
          target = JalrE ? {ALUResultE[31:1], 1'b0} : PCTargetE;
          if (TakenE != take || TargetE != target || RedirectPCE != (take ? target : PCPlus4E) ||
              MispredictE != (PredTakenE ? (!take || PredTargetE != target) : take) ||
              UpdateEn != ((BranchE || JumpE) && !MemBusyM))
            $fatal(1, "Redirect mismatch: controls=%b ALU=%h predTarget=%h", 6'(mode), ALUResultE, PredTargetE);
        end
      end
    end
    $display("PASS redirects: 2048 inputs");
  endtask

  initial begin
    check_decoder();
    check_forwarding();
    check_pipeline_controls();
    check_redirect();
    $finish;
  end
endmodule
