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
    // pretend main_mem
    std::map<uint32_t, uint32_t> mem;
    int latency = 0;
    int wait = -1; // -1 when no request is being handled
    int busy = 0; // cycles the last load or store waited

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
        top->ReadDone = 0;
        top->WriteDone = 0;

        for (int i = 0; i < 4; i++)
            top->ReadLine[i] = 0;

        // memory from the case table
        mem[0x10000] = 0x12345678;
        mem[0x10004] = 0x8000FFFE;
        mem[0x10400] = 0xCAFEF00D;
        mem[0x10800] = 0x0BADF00D;

        // empty the cache
        top->rst = 1;
        runSimulation(1);
        top->rst = 0;
    }

    // pretend main_mem answers: ReadDone with the line or WriteDone, LATENCY cycles after a request
    void respond()
    {
        top->eval();
        if (top->ReadReq || top->WriteReq)
        {
            if (wait < 0)
                wait = latency; // new request, start counting
            if (wait == 0)
            {
                if (top->ReadReq)
                {
                    top->ReadDone = 1;
                    for (int i = 0; i < 4; i++)
                        top->ReadLine[i] = mem[top->ReadAddr + 4 * i];
                }
                else
                {
                    top->WriteDone = 1;
                    memWrite(top->WriteAddr, top->WriteData, top->WriteFunct3);
                }
                wait = -1; // done
            }
            else
                wait--;
        }
        top->eval();
    }

    // one clock edge then Done signals drop again
    void tick()
    {
        runSimulation(1);
        top->ReadDone = 0;
        top->WriteDone = 0;
    }

    // sb, sh or sw into the pretend memory, one byte at a time like data_mem
    void memWrite(uint32_t addr, uint32_t data, int funct3)
    {
        setByte(addr, data & 0xFF);
        if (funct3 == SH || funct3 == SW)
            setByte(addr + 1, (data >> 8) & 0xFF);
        if (funct3 == SW)
        {
            setByte(addr + 2, (data >> 16) & 0xFF);
            setByte(addr + 3, (data >> 24) & 0xFF);
        }
    }

    // change one byte inside its word: clear the old byte then put in the new one
    void setByte(uint32_t addr, uint32_t value)
    {
        uint32_t word = addr - addr % 4;
        int shift = 8 * (addr % 4);
        mem[word] = (mem[word] & ~(0xFFu << shift)) | (value << shift);
    }

    // wait while MemBusy is 1, counting the cycles
    void waitWhileBusy()
    {
        busy = 0;
        respond();
        while (top->MemBusy && busy < 100)
        {
            tick();
            busy++;
            respond();
        }
    }

    // load: return RD once MemBusy drops
    uint32_t load(uint32_t addr, int funct3)
    {
        top->MemRead = 1;
        top->MemWrite = 0;
        top->A = addr;
        top->funct3 = funct3;
        waitWhileBusy();
        uint32_t result = top->RD;
        tick();
        top->MemRead = 0;
        return result;
    }

    // store: finishes once memory says WriteDone
    void store(uint32_t addr, uint32_t data, int funct3)
    {
        top->MemRead = 0;
        top->MemWrite = 1;
        top->A = addr;
        top->WD = data;
        top->funct3 = funct3;
        waitWhileBusy();
        tick();
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
    EXPECT_EQ(top->ReadReq, 1);
    EXPECT_EQ(top->MemBusy, 1);
}

// 2: a miss asks for the start of the line
TEST_F(DataCacheTestbench, MissAddrIsLineStartTest)
{
    top->MemRead = 1;
    top->A = 0x1010C;
    top->funct3 = LW;
    top->eval();
    EXPECT_EQ(top->ReadAddr, 0x10100u);
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

// 10: two lines with the same index both stay, one in each way
TEST_F(DataCacheTestbench, BothWaysTest)
{
    load(0x10000, LW);
    load(0x10400, LW);

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

// 15: a third line replaces the least recently used way
TEST_F(DataCacheTestbench, LruReplaceTest)
{
    load(0x10000, LW);
    load(0x10400, LW);
    load(0x10000, LW); // 0x10400 is now the older one
    EXPECT_EQ(load(0x10800, LW), 0x0BADF00Du);

    EXPECT_EQ(load(0x10000, LW), 0x12345678u);
    EXPECT_EQ(busy, 0); // kept
    load(0x10400, LW);
    EXPECT_GT(busy, 0); // replaced so it misses again
}

// 16: four words in one line, only the first misses
TEST_F(DataCacheTestbench, SequentialWordsTest)
{
    latency = 6;
    load(0x10000, LW);
    EXPECT_GT(busy, 0);
    load(0x10004, LW);
    EXPECT_EQ(busy, 0);
    load(0x10008, LW);
    EXPECT_EQ(busy, 0);
    load(0x1000C, LW);
    EXPECT_EQ(busy, 0);
}

// 17: a store hit also goes through to memory
TEST_F(DataCacheTestbench, StoreWriteThroughTest)
{
    load(0x10000, LW);
    store(0x10000, 0xDEADBEEF, SW);
    EXPECT_EQ(mem[0x10000], 0xDEADBEEFu);
}

// 18: a store miss only writes memory, the line is not brought in
TEST_F(DataCacheTestbench, StoreMissNoAllocateTest)
{
    store(0x10001, 0xAA, SB);
    EXPECT_EQ(mem[0x10000], 0x1234AA78u);
    EXPECT_EQ(load(0x10000, LW), 0x1234AA78u);
    EXPECT_GT(busy, 0); // still a miss
}

// 19: LATENCY 6, a store stalls until WriteDone
TEST_F(DataCacheTestbench, StoreStallTest)
{
    latency = 6;
    store(0x10000, 0x55, SW);
    EXPECT_EQ(busy, 6);
    EXPECT_EQ(mem[0x10000], 0x55u);
}