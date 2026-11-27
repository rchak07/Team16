/*
 *  Verifies the data cache against the cases in data_cache_cases.md
 */

#include <map>

#include "testbench.h"

unsigned int ticks = 0;

// funct3 codes, same as data_mem
const int LB = 0b000, LH = 0b001, LW = 0b010, LBU = 0b100, LHU = 0b101;
const int SB = 0b000, SH = 0b001, SW = 0b010;

class DataCacheTestbench : public Testbench
{
protected:
    // pretend refill controller and main_mem
    std::map<uint32_t, uint32_t> mem;
    int latency = 0;
    int wait = -1; // -1 when no miss is being handled
    int lastFillWay = -1;
    int busy = 0; // cycles the last load waited

    void initializeInputs() override
    {
        // every input to 0
        top->clk = 0;
        top->rst = 0;
        top->MemRead = 0;
        top->MemWrite = 0;
        top->A = 0;
        top->WD = 0;
        top->funct3 = 0;
        top->fillEn = 0;
        top->WriteDone = 0;

        for (int i = 0; i < 4; i++)
            top->fillLine[i] = 0;

        // memory from the case table
        mem[0x10000] = 0x12345678;
        mem[0x10004] = 0x8000FFFE;
        mem[0x10400] = 0xCAFEF00D;

        // empty the cache
        top->rst = 1;
        runSimulation(1);
        top->rst = 0;
    }

    // one clock cycle, sends the line LATENCY cycles after the miss
    void cycle()
    {
        top->eval();
        top->fillEn = 0;
        if (top->miss)
        {
            if (wait < 0)
                wait = latency; // new miss, start counting
            if (wait == 0)
            {
                top->fillEn = 1;
                for (int i = 0; i < 4; i++)
                    top->fillLine[i] = mem[top->missAddr + 4 * i];
                lastFillWay = top->fillWay;
                wait = -1; // done
            }
            else
                wait--;
        }
        runSimulation(1);
        top->fillEn = 0;
    }

    // load: wait while MemBusy is 1, then return RD
    uint32_t load(uint32_t addr, int funct3)
    {
        top->MemRead = 1;
        top->MemWrite = 0;
        top->A = addr;
        top->funct3 = funct3;
        top->eval();
        busy = 0;
        while (top->MemBusy && busy < 100)
        {
            cycle();
            busy++;
            top->eval();
        }
        uint32_t result = top->RD;
        cycle();
        top->MemRead = 0;
        return result;
    }

    // store: one cycle with MemWrite = 1
    void store(uint32_t addr, uint32_t data, int funct3)
    {
        top->MemRead = 0;
        top->MemWrite = 1;
        top->A = addr;
        top->WD = data;
        top->funct3 = funct3;
        cycle();
        top->MemWrite = 0;
    }
};

// tests (cases from data_cache_cases.md)

// 1: after reset nothing is valid, so the first load misses
TEST_F(DataCacheTestbench, ResetEmptiesCacheTest)
{
    top->MemRead = 1;
    top->A = 0x10000;
    top->funct3 = LW;
    top->eval();
    EXPECT_EQ(top->miss, 1);
    EXPECT_EQ(top->MemBusy, 1);
}

// 2: a miss asks for the start of the line
TEST_F(DataCacheTestbench, MissAddrIsLineStartTest)
{
    top->MemRead = 1;
    top->A = 0x1010C;
    top->funct3 = LW;
    top->eval();
    EXPECT_EQ(top->missAddr, 0x10100u);
}

// 3: LATENCY 0, busy for 1 cycle then the word
TEST_F(DataCacheTestbench, MissThenFillLatency0Test)
{
    EXPECT_EQ(load(0x10000, LW), 0x12345678u);
    EXPECT_EQ(busy, 1);
}

// 4: LATENCY 6, busy for 7 cycles then the word
TEST_F(DataCacheTestbench, MissThenFillLatency6Test)
{
    latency = 6;
    EXPECT_EQ(load(0x10000, LW), 0x12345678u);
    EXPECT_EQ(busy, 7);
}

// 5: another word in the same line is already there
TEST_F(DataCacheTestbench, SameLineHitsTest)
{
    load(0x10000, LW);
    EXPECT_EQ(load(0x10004, LW), 0x8000FFFEu);
    EXPECT_EQ(busy, 0);
}

// 6, 7: byte 0x80, signed and unsigned
TEST_F(DataCacheTestbench, LoadByteTest)
{
    load(0x10000, LW);
    EXPECT_EQ(load(0x10007, LB), 0xFFFFFF80u);
    EXPECT_EQ(load(0x10007, LBU), 0x00000080u);
}

// 8, 9: upper half 0x8000, signed and unsigned
TEST_F(DataCacheTestbench, LoadUpperHalfTest)
{
    load(0x10000, LW);
    EXPECT_EQ(load(0x10006, LH), 0xFFFF8000u);
    EXPECT_EQ(load(0x10006, LHU), 0x00008000u);
}

// 10: two lines with the same index go into way 0 then way 1 and both stay
TEST_F(DataCacheTestbench, BothWaysTest)
{
    load(0x10000, LW);
    EXPECT_EQ(lastFillWay, 0);
    load(0x10400, LW);
    EXPECT_EQ(lastFillWay, 1);

    EXPECT_EQ(load(0x10000, LW), 0x12345678u);
    EXPECT_EQ(busy, 0);
    EXPECT_EQ(load(0x10400, LW), 0xCAFEF00Du);
    EXPECT_EQ(busy, 0);
}

// 11: sw hit replaces the whole word
TEST_F(DataCacheTestbench, StoreWordHitTest)
{
    load(0x10000, LW);
    store(0x10000, 0xDEADBEEF, SW);
    EXPECT_EQ(load(0x10000, LW), 0xDEADBEEFu);
}

// 12: sb hit only changes byte 1
TEST_F(DataCacheTestbench, StoreByteHitTest)
{
    load(0x10000, LW);
    store(0x10001, 0xAA, SB);
    EXPECT_EQ(load(0x10000, LW), 0x1234AA78u);
}

// 13: sh hit only changes the upper half
TEST_F(DataCacheTestbench, StoreHalfHitTest)
{
    load(0x10000, LW);
    store(0x10002, 0xBBBB, SH);
    EXPECT_EQ(load(0x10000, LW), 0xBBBB5678u);
}

// 14: no load or store, never busy
TEST_F(DataCacheTestbench, IdleNotBusyTest)
{
    top->A = 0x10000;
    top->eval();
    EXPECT_EQ(top->MemBusy, 0);
}