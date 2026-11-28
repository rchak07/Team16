/*
 *  Verifies the EX/MEM stage register against the cases in stage_regs_cases.md
 *  (no clr: this register is never flushed)
 */

#include "testbench.h"

unsigned int ticks = 0;

class ExMemTestbench : public Testbench
{
protected:
    void initializeInputs() override
    {
        top->clk = 0;
        top->rst = 0;
        top->en = 1;
        setInputs(0, 0);

        // start from a reset register
        top->rst = 1;
        runSimulation(1);
        top->rst = 0;
    }

    // a different value on every input (on = 1 or 0 for the control bits)
    void setInputs(uint32_t base, int on)
    {
        top->RegWriteE = on;
        top->MemWriteE = on;
        top->MemReadE = on;
        top->ExecResultE = base + 1;
        top->WriteDataE = base + 2;
        top->RdE = on ? 3 : 6;
        top->funct3E = on ? 5 : 2;
    }

    // outputs hold the values setInputs gave
    void checkOutputs(uint32_t base, int on)
    {
        EXPECT_EQ(top->RegWriteM, on);
        EXPECT_EQ(top->MemWriteM, on);
        EXPECT_EQ(top->MemReadM, on);
        EXPECT_EQ(top->ExecResultM, base + 1);
        EXPECT_EQ(top->WriteDataM, base + 2);
        EXPECT_EQ(top->RdM, on ? 3 : 6);
        EXPECT_EQ(top->funct3M, on ? 5 : 2);
    }

    // every output 0
    void checkCleared()
    {
        EXPECT_EQ(top->RegWriteM, 0);
        EXPECT_EQ(top->MemWriteM, 0);
        EXPECT_EQ(top->MemReadM, 0);
        EXPECT_EQ(top->ExecResultM, 0u);
        EXPECT_EQ(top->WriteDataM, 0u);
        EXPECT_EQ(top->RdM, 0);
        EXPECT_EQ(top->funct3M, 0);
    }
};

// 1: inputs only reach the outputs on clock edge
TEST_F(ExMemTestbench, PassOnEdgeTest)
{
    setInputs(0x100, 1);
    top->eval();
    checkCleared();
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 2: en low holds old values
TEST_F(ExMemTestbench, HoldWhenStalledTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->en = 0;
    setInputs(0x200, 0);
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 5: rst clears everything
TEST_F(ExMemTestbench, ResetTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->rst = 1;
    runSimulation(1);
    checkCleared();
}