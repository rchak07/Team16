module  main_mem #(
    ADDRESS_WIDTH = 17,  //each position will store 1 byte therefore this 128 kb total.
    LATENCY = 6

)(
    input logic clk,
    input logic rst,
    input logic [31:0] ReadAddr,
    input logic ReadReq,
    output logic ReadDone,
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

    logic [3:0] count;

    logic busy;
    always_ff @(posedge clk, posedge rst) begin
        if (rst) begin
            count <= 4'h1;
            busy <= 1'b0; 
            ReadDone <= 1'b0;
        end
        else begin
            ReadDone <= 1'b0; // ReadDone only high for one clock cycle (unless latency 0 or 1)
            case (busy)
                1'b0: begin
                    count <= 4'h1;
                    if (ReadReq && !ReadDone) begin
                        if (LATENCY <= 1) ReadDone <= 1'b1;
                        else busy <= 1'b1;
                    end
                end
                1'b1: begin
                        if (count < LATENCY - 1) count <= count + 4'h1;
                        else begin
                            ReadDone <= 1'b1;
                            busy <= 1'b0;
                        end
                end
                
            endcase
        end
    end

endmodule
