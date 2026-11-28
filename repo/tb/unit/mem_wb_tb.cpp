/*
 *  Verifies the MEM/WB stage register against the cases in stage_regs_cases.md
 */

#include "testbench.h"

unsigned int ticks = 0;

class MemWbTestbench : public Testbench
{
protected:
    void initializeInputs() override
    {
        top->clk = 0;
        top->rst = 0;
        top->en = 1;
        top->clr = 0;
        setInputs(0, 0);

        // start from a reset register
        top->rst = 1;
        runSimulation(1);
        top->rst = 0;
    }

    // a different value on every input (on = 1 or 0 for the control bits)
    void setInputs(uint32_t base, int on)
    {
        top->RegWriteM = on;
        top->MemReadM = on;
        top->ExecResultM = base + 1;
        top->ReadDataM = base + 2;
        top->RdM = on ? 3 : 6;
    }

    // outputs hold the values setInputs gave
    void checkOutputs(uint32_t base, int on)
    {
        EXPECT_EQ(top->RegWriteW, on);
        EXPECT_EQ(top->MemReadW, on);
        EXPECT_EQ(top->ExecResultW, base + 1);
        EXPECT_EQ(top->ReadDataW, base + 2);
        EXPECT_EQ(top->RdW, on ? 3 : 6);
    }

    // a bubble: every output 0, so nothing is written back
    void checkBubble()
    {
        EXPECT_EQ(top->RegWriteW, 0);
        EXPECT_EQ(top->MemReadW, 0);
        EXPECT_EQ(top->ExecResultW, 0u);
        EXPECT_EQ(top->ReadDataW, 0u);
        EXPECT_EQ(top->RdW, 0);
    }
};

// 1: inputs only reach the outputs on clock edge
TEST_F(MemWbTestbench, PassOnEdgeTest)
{
    setInputs(0x100, 1);
    top->eval();
    checkBubble();
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 2: en low holds old values
TEST_F(MemWbTestbench, HoldWhenStalledTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->en = 0;
    setInputs(0x200, 0);
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 3: clr turns instruction into a bubble
TEST_F(MemWbTestbench, FlushGivesBubbleTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->clr = 1;
    runSimulation(1);
    checkBubble();
}

// 4: clr still clears when en is low
TEST_F(MemWbTestbench, FlushBeatsStallTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->en = 0;
    top->clr = 1;
    runSimulation(1);
    checkBubble();
}

// 5: rst clears like a flush
TEST_F(MemWbTestbench, ResetTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->rst = 1;
    runSimulation(1);
    checkBubble();
}