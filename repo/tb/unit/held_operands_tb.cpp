#include <cstdint>

#include "base_testbench.h"

unsigned int ticks = 0;

class HeldOperandsTest : public BaseTestbench
{
protected:
    void clockEdge()
    {
        top->clk = 1;
        top->eval();
        top->clk = 0;
        top->eval();
    }

    void initializeInputs() override
    {
        top->clk = 0;
        top->rst = 1;
        top->StallE = 0;
        top->ForwardedAE = 0;
        top->ForwardedBE = 0;
        top->eval();
        clockEdge();
        top->rst = 0;
    }

    void drive(std::uint32_t a, std::uint32_t b, bool stall)
    {
        top->ForwardedAE = a;
        top->ForwardedBE = b;
        top->StallE = stall;
        top->eval();
    }

    void expectOperands(std::uint32_t a, std::uint32_t b)
    {
        EXPECT_EQ(top->OperandAE, a);
        EXPECT_EQ(top->OperandBE, b);
    }
};

TEST_F(HeldOperandsTest, ForwardedValuesSurviveTheWbBubbleUntilExecuteAdvances)
{
    drive(7, 11, true);
    expectOperands(7, 11);
    clockEdge();

    drive(0, 99, true);
    clockEdge();
    expectOperands(7, 11);

    drive(0, 99, false);
    expectOperands(7, 11);
    clockEdge();
    expectOperands(0, 99);

    // A second stalled instruction must capture its own operands.
    drive(13, 17, true);
    clockEdge();
    drive(15, 19, false);
    expectOperands(13, 17);
}

TEST_F(HeldOperandsTest, RunningOperandsPassThroughAndResetDiscardsHeldValues)
{
    drive(1, 2, false);
    expectOperands(1, 2);
    drive(3, 4, true);
    clockEdge();

    drive(5, 6, false);
    top->rst = 1;
    clockEdge();
    expectOperands(5, 6);
}
