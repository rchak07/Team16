#include "Vcache_memory_pair.h"
#include "verilated.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
constexpr std::uint32_t base = 0x10000;
constexpr unsigned timeout = 64;

unsigned pattern(unsigned offset)
{
    return (37 * offset + 143) & 0xff;
}

void writeFixture()
{
    std::ofstream file("data.hex");
    file << std::hex << std::setfill('0');
    for (unsigned i = 0; i < 1024; ++i)
        file << std::setw(2) << pattern(i) << '\n';
    file.close();
    if (!file)
        throw std::runtime_error("could not write data.hex");
}

std::uint32_t expectedWord(unsigned offset)
{
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result |= static_cast<std::uint32_t>(pattern(offset + i)) << (8 * i);
    return result;
}

void tick(Vcache_memory_pair &dut)
{
    dut.clk = 1;
    dut.eval();
    dut.clk = 0;
    dut.eval();
}

unsigned load(Vcache_memory_pair &dut, unsigned offset, bool cold, unsigned latency)
{
    dut.MemRead = 1;
    dut.MemWrite = 0;
    dut.funct3 = 2;
    dut.A = base + offset;
    dut.eval();
    const bool initialMiss = dut.miss;
    const bool fillWay = dut.fillWay;
    unsigned wait = 0;
    unsigned acknowledgements = 0;
    while (dut.MemBusy && wait < timeout) {
        if (!dut.miss || dut.missAddr != (dut.A & ~15u) || dut.fillWay != fillWay)
            throw std::runtime_error("line address, selected way or miss changed during a refill");
        acknowledgements += dut.ReadDone;
        ++wait;
        tick(dut);
    }
    if (dut.MemBusy || initialMiss != cold || dut.RD != expectedWord(offset) ||
        (cold && (acknowledgements != 1 || wait < latency || wait == 0)) ||
        (!cold && wait != 0)) {
        std::fprintf(stderr, "load at 0x%x expected 0x%08x, got 0x%08x; busy cycles %u\n",
                     dut.A, expectedWord(offset), dut.RD, wait);
        throw std::runtime_error("load value, hit timing or refill handshake mismatch");
    }
    tick(dut);
    return wait;
}

std::uint32_t readBackingWord(Vcache_memory_pair &dut, unsigned offset)
{
    // Only sample ReadLine during an acknowledged request.
    dut.ProbeAddr = (base + offset) & ~15u;
    dut.ProbeRead = 1;
    dut.eval();
    for (unsigned wait = 0; !dut.ReadDone && wait < timeout; ++wait)
        tick(dut);
    if (!dut.ReadDone)
        throw std::runtime_error("backing-memory read timed out");
    const auto result = dut.ReadLine[(offset & 15u) / 4];
    dut.ProbeRead = 0;
    dut.eval();
    tick(dut);
    return result;
}

bool store(Vcache_memory_pair &dut, unsigned offset, std::uint32_t value)
{
    dut.MemRead = 0;
    dut.MemWrite = 1;
    dut.A = base + offset;
    dut.WD = value;
    dut.funct3 = 2;
    dut.eval();
    bool requested = dut.WriteReq;
    unsigned wait = 0;
    while (dut.MemBusy && wait < timeout) {
        ++wait;
        tick(dut);
        requested |= dut.WriteReq;
    }
    if (dut.MemBusy)
        throw std::runtime_error("cache store timed out");
    const bool acknowledged = dut.WriteDone;
    tick(dut);
    dut.MemWrite = 0;
    dut.eval();
    const std::uint32_t observed = readBackingWord(dut, offset);
    bool passed = requested && acknowledged && wait > 0 && value == observed;
    std::printf("  store at 0x%x %s: request=%d ack=%d busy_cycles=%u backing=0x%08x expected=0x%08x\n",
                base + offset, passed ? "PASS" : "FAIL", requested, acknowledged, wait, observed, value);
    return passed;
}
}  // namespace

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;
    const unsigned latency = std::strtoul(argv[1], nullptr, 10);
    Verilated::commandArgs(argc, argv);
    try {
        writeFixture();
        Vcache_memory_pair dut;
        dut.clk = 0;
        dut.rst = 1;
        dut.MemRead = 0;
        dut.MemWrite = 0;
        dut.ProbeRead = 0;
        dut.ProbeAddr = 0;
        dut.A = 0;
        dut.WD = 0;
        dut.funct3 = 0;
        dut.eval();
        tick(dut);
        dut.rst = 0;
        dut.eval();

        unsigned fills = 0, hits = 0, maxWait = 0;
        for (unsigned offset = 0; offset < 512; offset += 4) {
            const bool cold = (offset % 16) == 0;
            const unsigned wait = load(dut, offset, cold, latency);
            if (cold) {
                ++fills;
                if (wait > maxWait)
                    maxWait = wait;
            } else {
                ++hits;
            }
        }
        for (int offset = 508; offset >= 0; offset -= 4) {
            load(dut, offset, false, latency);
            ++hits;
        }
        std::printf("LATENCY=%u: PASS %u reads (%u fills, %u hits), maximum busy cycles %u\n",
                    latency, fills + hits, fills, hits, maxWait);
        const bool hitPass = store(dut, 0, 0xaabbccddu);
        const bool missPass = store(dut, 512, 0x1337f00du);
        dut.final();
        return hitPass && missPass ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "LATENCY=" << latency << ": FAIL " << e.what() << '\n';
        return 1;
    }
}
