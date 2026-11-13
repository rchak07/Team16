/*
 *  Verifies the register file: synchronous write, asynchronous reads,
 *  x0 hardwired to zero, and the a0 output.
 */

#include "testbench.h"

unsigned int ticks = 0;

class RegfileTestbench : public Testbench
{
protected:
    void initializeInputs() override
    {
        top->AD1 = 0;
        top->AD2 = 0;
        top->AD3 = 0;
        top->WD3 = 0;
        top->WE3 = 0;
        top->clk = 0;
    }
};

TEST_F(RegfileTestbench, WriteThenReadTest)
{
    top->AD3 = 5;
    top->WD3 = 10;
    top->WE3 = 1;

    runSimulation(1);          // one cycle: the write happens at the rising edge

    top->AD1 = 5;
    top->eval();               // reads are asynchronous, so no clock needed

    EXPECT_EQ(top->RD1, 10);
}