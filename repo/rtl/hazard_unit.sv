module hazard_unit (
  input  logic [4:0] Rs1D, Rs2D,
  input  logic [4:0] Rs1E, Rs2E, RdE,
  input  logic       MemReadE,
  input  logic [4:0] RdM,
  input  logic       RegWriteM,
  input  logic [4:0] RdW,
  input  logic       RegWriteW,
  input  logic       MispredictE, MemBusyM,
  output logic [1:0] ForwardAE, ForwardBE,
  output logic       StallF, StallD, StallE, StallM,
  output logic       FlushD, FlushE, FlushW
);
  localparam logic [1:0] FROM_REG = 2'b00, FROM_WB = 2'b01, FROM_MEM = 2'b10;

  // MEM is the younger writer if both stages target the same register.
  function automatic logic [1:0] forward_select(input logic [4:0] source);
    if (RegWriteM && RdM != 5'd0 && RdM == source) return FROM_MEM;
    if (RegWriteW && RdW != 5'd0 && RdW == source) return FROM_WB;
    return FROM_REG;
  endfunction

  logic load_use;
  assign ForwardAE = forward_select(Rs1E);
  assign ForwardBE = forward_select(Rs2E);
  assign load_use = MemReadE && RdE != 5'd0 && (RdE == Rs1D || RdE == Rs2D);

  // A cache wait holds F through M. The unfinished request must not enter WB.
  assign StallM = MemBusyM;
  assign StallE = MemBusyM;
  assign FlushW = MemBusyM;

  // A redirect discards the younger dependency. A lone load-use holds IF/ID
  // and inserts a bubble into ID/EX while allowing the load to reach MEM.
  assign StallF = MemBusyM || (load_use && !MispredictE);
  assign StallD = StallF;
  assign FlushD = !MemBusyM && MispredictE;
  assign FlushE = !MemBusyM && (MispredictE || load_use);
endmodule
