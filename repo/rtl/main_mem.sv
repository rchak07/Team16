module  main_mem #(
    ADDRESS_WIDTH = 17,  //each position will store 1 byte therefore this 128 kb total.
    LATENCY = 6

)(
    input logic clk,
    input logic rst,
    /*verilator lint_off UNUSED*/
    input logic [31:0] ReadAddr,
    /*verilator lint_off UNUSED*/
    input logic ReadReq,
    output logic ReadDone,
    output logic [127:0] ReadLine, // 16 bytes is 128 bits as 1 byte = 8 bits
    input logic WriteReq,
    /*verilator lint_off UNUSED*/
    input logic [31:0] WriteAddr,
    /*verilator lint_off UNUSED*/
    input logic [31:0] WriteData,
    input logic [2:0] WriteFunct3,
    output logic WriteDone
);

    logic [7:0] mem [2**ADDRESS_WIDTH-1:0]; // each element contains a byte;
    logic [ADDRESS_WIDTH-1:0] a;
    assign a = ReadAddr[ADDRESS_WIDTH - 1:0];
    logic [ADDRESS_WIDTH-1:0] b;
    assign b = WriteAddr[ADDRESS_WIDTH - 1:0];


    initial begin
        $display("Loading Main memory.");
        $readmemh("data.hex", mem, 'h10000);
    end

    always_comb begin
        for (int i = 0; i < 16; i++) begin
            ReadLine[i*8 +: 8] = mem[a + i[ADDRESS_WIDTH-1:0]]; //integer is 32 bit, taking first 17 bits to avoid linting error.
        end
    end

    
    // always_ff  @(posedge clk && is_write) begin
    //     for (int i = 0; i < 4; i++) begin
    //         // ReadLine[i*8 +: 8] = mem[a + i[ADDRESS_WIDTH-1:0]]; //integer is 32 bit, taking first 17 bits to avoid linting error.
    //         mem[a + i[ADDRESS_WIDTH-1:0]] <= WriteData[i*8 +: 8];
    //     end
    // end

    always_ff @(posedge clk) begin //entirely copied from data_mem
        if (WriteDone) begin
            case (WriteFunct3)
                3'b000: mem[b] <= WriteData[7:0];
                3'b001: begin
                    mem[b+1] <= WriteData[15:8];
                    mem[b] <= WriteData[7:0];
                end
                3'b010: begin
                    mem[b+3] <= WriteData[31:24];
                    mem[b+2] <= WriteData[23:16];
                    mem[b+1] <= WriteData[15:8];
                    mem[b] <= WriteData[7:0];          
                end
                default: ;
            endcase
        end
    end



    logic [3:0] count;

    logic busy, is_write;
    always_ff @(posedge clk, posedge rst) begin
        if (rst) begin
            count <= 4'h1;
            busy <= 1'b0; 
            ReadDone <= 1'b0;
            WriteDone <= 1'b0;
            is_write <= 1'b0;
        end
        else begin
            ReadDone <= 1'b0; // ReadDone only high for one clock cycle 
            WriteDone <= 1'b0;
            case (busy)
                1'b0: begin
                    count <= 4'h1;
                    if (ReadReq && !ReadDone) begin
                        is_write <= 1'b0;
                        if (LATENCY <= 1) ReadDone <= 1'b1;
                        else busy <= 1'b1;
                    end
                    else if (WriteReq && !WriteDone) begin
                        is_write <= 1'b1;
                        if (LATENCY <= 1) WriteDone <= 1'b1;
                        else begin
                            busy <= 1'b1;

                        end
                    end
                end
                1'b1: begin
                        if (count < LATENCY - 1) count <= count + 4'h1;
                        else begin
                            if (is_write) begin 
                                WriteDone <= 1'b1;
                                is_write <= 1'b0;
                            end
                            else ReadDone <= 1'b1;
                            busy <= 1'b0;
                        end
                end
                
            endcase
        end
    end

endmodule
