/*
 *  Verifies the IF/ID stage register against the cases in stage_regs_cases.md
 */

#include "testbench.h"

unsigned int ticks = 0;

class IfIdTestbench : public Testbench
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

    // an instruction and its PC on the inputs
    void setInputs(uint32_t instr, uint32_t pc)
    {
        top->InstrF = instr;
        top->PCF = pc;
        top->PCPlus4F = pc + 4;
        top->PredTakenF = (pc != 0);
        top->PredTargetF = pc + 0x100;
    }

    // outputs hold that instruction and PC
    void checkOutputs(uint32_t instr, uint32_t pc)
    {
        EXPECT_EQ(top->InstrD, instr);
        EXPECT_EQ(top->PCD, pc);
        EXPECT_EQ(top->PCPlus4D, pc + 4);
        EXPECT_EQ(top->PredTakenD, (pc != 0));
        EXPECT_EQ(top->PredTargetD, pc + 0x100);
    }

    // outputs are a nop with everything else 0
    void checkNop()
    {
        EXPECT_EQ(top->InstrD, 0x00000013u);
        EXPECT_EQ(top->PCD, 0u);
        EXPECT_EQ(top->PCPlus4D, 0u);
        EXPECT_EQ(top->PredTakenD, 0);
        EXPECT_EQ(top->PredTargetD, 0u);
    }
};

// tests (cases from stage_regs_cases.md)

// 1: inputs only reach the outputs on the clock edge
TEST_F(IfIdTestbench, PassOnEdgeTest)
{
    setInputs(0x00500093, 0x1000);
    top->eval();
    checkNop();
    runSimulation(1);
    checkOutputs(0x00500093, 0x1000);
}

// 2: en low holds the old values
TEST_F(IfIdTestbench, HoldWhenStalledTest)
{
    setInputs(0x00500093, 0x1000);
    runSimulation(1);
    top->en = 0;
    setInputs(0x00A00113, 0x2000);
    runSimulation(1);
    checkOutputs(0x00500093, 0x1000);
}

// 3: clr turns the instruction into a nop
TEST_F(IfIdTestbench, FlushGivesNopTest)
{
    setInputs(0x00500093, 0x1000);
    runSimulation(1);
    top->clr = 1;
    runSimulation(1);
    checkNop();
}

// 4: clr still clears when en is low
TEST_F(IfIdTestbench, FlushBeatsStallTest)
{
    setInputs(0x00500093, 0x1000);
    runSimulation(1);
    top->en = 0;
    top->clr = 1;
    runSimulation(1);
    checkNop();
}

// 5: rst clears like a flush
TEST_F(IfIdTestbench, ResetTest)
{
    setInputs(0x00500093, 0x1000);
    runSimulation(1);
    top->rst = 1;
    runSimulation(1);
    checkNop();
}