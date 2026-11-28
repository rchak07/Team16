/*
 *  Verifies the ID/EX stage register against the cases in stage_regs_cases.md
 */

#include "testbench.h"

unsigned int ticks = 0;

class IdExTestbench : public Testbench
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
        top->RegWriteD = on;
        top->ResultSrcD = on ? 2 : 1;
        top->MemWriteD = on;
        top->MemReadD = on;
        top->ALUControlD = on ? 0b1001 : 0b0110;
        top->ALUSrcD = on;
        top->ALUSrcAD = on;
        top->BranchD = on;
        top->JumpD = on;
        top->JalrD = on;
        top->RD1D = base + 1;
        top->RD2D = base + 2;
        top->ImmExtD = base + 3;
        top->PCD = base + 4;
        top->PCPlus4D = base + 5;
        top->Rs1D = on ? 1 : 4;
        top->Rs2D = on ? 2 : 5;
        top->RdD = on ? 3 : 6;
        top->funct3D = on ? 5 : 2;
        top->PredTakenD = on;
        top->PredTargetD = base + 6;
    }

    // outputs hold the values setInputs gave
    void checkOutputs(uint32_t base, int on)
    {
        EXPECT_EQ(top->RegWriteE, on);
        EXPECT_EQ(top->ResultSrcE, on ? 2 : 1);
        EXPECT_EQ(top->MemWriteE, on);
        EXPECT_EQ(top->MemReadE, on);
        EXPECT_EQ(top->ALUControlE, on ? 0b1001 : 0b0110);
        EXPECT_EQ(top->ALUSrcE, on);
        EXPECT_EQ(top->ALUSrcAE, on);
        EXPECT_EQ(top->BranchE, on);
        EXPECT_EQ(top->JumpE, on);
        EXPECT_EQ(top->JalrE, on);
        EXPECT_EQ(top->RD1E, base + 1);
        EXPECT_EQ(top->RD2E, base + 2);
        EXPECT_EQ(top->ImmExtE, base + 3);
        EXPECT_EQ(top->PCE, base + 4);
        EXPECT_EQ(top->PCPlus4E, base + 5);
        EXPECT_EQ(top->Rs1E, on ? 1 : 4);
        EXPECT_EQ(top->Rs2E, on ? 2 : 5);
        EXPECT_EQ(top->RdE, on ? 3 : 6);
        EXPECT_EQ(top->funct3E, on ? 5 : 2);
        EXPECT_EQ(top->PredTakenE, on);
        EXPECT_EQ(top->PredTargetE, base + 6);
    }

    // a bubble: nothing that changes the CPU's state
    void checkBubble()
    {
        EXPECT_EQ(top->RegWriteE, 0);
        EXPECT_EQ(top->MemWriteE, 0);
        EXPECT_EQ(top->MemReadE, 0);
        EXPECT_EQ(top->BranchE, 0);
        EXPECT_EQ(top->JumpE, 0);
        EXPECT_EQ(top->JalrE, 0);
        EXPECT_EQ(top->RdE, 0);
    }
};

// 1: inputs only reach the outputs on clock edge
TEST_F(IdExTestbench, PassOnEdgeTest)
{
    setInputs(0x100, 1);
    top->eval();
    checkBubble();
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 2: en low holds old values
TEST_F(IdExTestbench, HoldWhenStalledTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->en = 0;
    setInputs(0x200, 0);
    runSimulation(1);
    checkOutputs(0x100, 1);
}

// 3: clr turns instruction into a bubble
TEST_F(IdExTestbench, FlushGivesBubbleTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->clr = 1;
    runSimulation(1);
    checkBubble();
}

// 4: clr still clears when en is low
TEST_F(IdExTestbench, FlushBeatsStallTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->en = 0;
    top->clr = 1;
    runSimulation(1);
    checkBubble();
}

// 5: rst clears like a flush
TEST_F(IdExTestbench, ResetTest)
{
    setInputs(0x100, 1);
    runSimulation(1);
    top->rst = 1;
    runSimulation(1);
    checkBubble();
}