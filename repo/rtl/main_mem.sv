module  main_mem #(
    ADDRESS_WIDTH = 17,  //each position will store 1 byte therefore this 128 kb total.
    LATENCY = 0

)(
    input logic ReadReq,
    input logic [31:0] ReadAddr,
    output logic [127:0] ReadLine // 16 bytes is 128 bits as 1 byte = 8 bits

);

    logic [7:0] mem [2**ADDRESS_WIDTH-1:0]; // each element contains a byte;
    logic [ADDRESS_WIDTH-1:0] a;
    assign a = ReadAddr[ADDRESS_WIDTH - 1:0];

    initial begin
        $display("Loading Main memory.");
        $readmemh("data.hex", mem, 'h10000);
    end

    always_comb begin
        for (int i = 0; i < 16; i++) begin
            ReadLine[i*8 +: 8] = mem[a + i[ADDRESS_WIDTH-1:0]]; //integer is 32 bit, taking first 17 bits to avoid linting error.
        end
    end

endmodule
