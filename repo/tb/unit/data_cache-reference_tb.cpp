#include <array>
#include <cstdint>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "Vdut.h"

namespace {

#ifndef TEST_CACHE_INDEX_BITS
#define TEST_CACHE_INDEX_BITS 6
#endif

constexpr unsigned lineBytes = 16;
constexpr unsigned sameSetStride = lineBytes << TEST_CACHE_INDEX_BITS;
constexpr unsigned base = 0x10000;
constexpr unsigned byte = 0, half = 1, word = 2, unsignedByte = 4, unsignedHalf = 5;
using Memory = std::vector<std::uint8_t>;

unsigned width(unsigned funct3) {
    return 1u << (funct3 & 3u);
}

std::uint32_t readBytes(const Memory& bytes, unsigned address, unsigned funct3) {
    const unsigned size = width(funct3);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < size; ++i)
        value |= std::uint32_t(bytes.at(address + i)) << (8 * i);
    if ((funct3 == byte || funct3 == half) && (bytes.at(address + size - 1) & 0x80))
        value |= ~std::uint32_t(0) << (8 * size);
    return value;
}

void writeBytes(Memory& bytes, unsigned address, unsigned funct3, std::uint32_t value) {
    for (unsigned i = 0; i < width(funct3); ++i)
        bytes.at(address + i) = (value >> (8 * i)) & 0xff;
}

class DataCacheTest : public ::testing::TestWithParam<unsigned> {
protected:
    Vdut dut;
    Memory backing, expected;

    void tick() {
        dut.clk = 1;
        dut.eval();
        dut.clk = 0;
        dut.eval();
    }

    void idle() {
        dut.MemRead = 0;
        dut.MemWrite = 0;
        dut.fillEn = 0;
        dut.WriteDone = 0;
        dut.eval();
    }

    void reset() {
        idle();
        dut.rst = 1;
        dut.eval();
        tick();
        dut.rst = 0;
        dut.eval();
    }

    void SetUp() override {
        backing.resize(0x20000);
        std::mt19937 initialBytes(16);
        for (auto& b : backing) b = initialBytes() & 0xff;
        writeBytes(backing, base, word, 0x12345678);
        writeBytes(backing, base + 4, word, 0x8000fffe);
        writeBytes(backing, base + sameSetStride, word, 0xcafef00d);
        expected = backing;
        dut.clk = 0;
        dut.A = 0;
        dut.WD = 0;
        dut.funct3 = word;
        for (unsigned i = 0; i < lineBytes / 4; ++i) dut.fillLine[i] = 0;
        reset();
    }

    void TearDown() override { dut.final(); }

    // This responder waits GetParam() cycles, then presents a line for one fill edge.
    // Multiple fills are allowed for accesses crossing a line boundary.
    void load(unsigned address, unsigned funct3, int expectedFills = -1) {
        SCOPED_TRACE(::testing::Message() << "load at " << address << ", funct3 " << funct3);
        dut.MemRead = 1;
        dut.A = address;
        dut.funct3 = funct3;
        dut.eval();
        unsigned line = 0, remaining = 0, fills = 0;
        bool pending = false;
        for (unsigned cycles = 0; cycles < 2 * (GetParam() + 3) + 4; ++cycles) {
            ASSERT_FALSE(dut.WriteReq);
            if (!dut.MemBusy) {
                ASSERT_FALSE(pending);
                ASSERT_FALSE(dut.miss);
                EXPECT_EQ(dut.RD, readBytes(expected, address, funct3));
                if (expectedFills >= 0) {
                    EXPECT_EQ(fills, unsigned(expectedFills));
                    EXPECT_EQ(cycles, unsigned(expectedFills) * (GetParam() + 1));
                }
                tick();
                idle();
                return;
            }
            if (!pending && dut.miss) {
                line = dut.missAddr;
                const unsigned firstLine = address & ~(lineBytes - 1);
                const unsigned lastLine = (address + width(funct3) - 1) & ~(lineBytes - 1);
                ASSERT_TRUE(line == firstLine || line == lastLine) << "missAddr " << line;
                remaining = GetParam();
                pending = true;
            }
            if (pending) {
                if (remaining == 0) {
                    for (unsigned i = 0; i < lineBytes / 4; ++i)
                        dut.fillLine[i] = readBytes(backing, line + 4 * i, word);
                    dut.fillEn = 1;
                    ++fills;
                    pending = false;
                } else {
                    --remaining;
                }
            }
            dut.eval();
            tick();
            dut.fillEn = 0;
            dut.eval();
        }
        FAIL() << "load never completed";
    }

    void checkWritePayload(unsigned address, unsigned funct3, std::uint32_t value) {
        ASSERT_EQ(dut.WriteAddr, address);
        ASSERT_EQ(dut.WriteFunct3, funct3);
        const unsigned shift = (4 - width(funct3)) * 8;
        ASSERT_EQ(std::uint32_t(dut.WriteData) << shift, value << shift);
    }

    // Backing memory changes only on acknowledgement. The independent expected
    // array is updated using the CPU operation, never the cache's output payload.
    void store(unsigned address, unsigned funct3, std::uint32_t value) {
        SCOPED_TRACE(::testing::Message() << "store at " << address << ", funct3 " << funct3);
        dut.MemWrite = 1;
        dut.A = address;
        dut.WD = value;
        dut.funct3 = funct3;
        dut.eval();
        bool requested = false;
        unsigned remaining = GetParam();
        for (unsigned cycles = 0; cycles < GetParam() + 8; ++cycles) {
            ASSERT_FALSE(dut.miss) << "no-write-allocate store requested a refill";
            if (dut.WriteReq) {
                ASSERT_NO_FATAL_FAILURE(checkWritePayload(address, funct3, value));
                requested = true;
            }
            if (requested && remaining == 0) {
                dut.WriteDone = 1;
                dut.eval();
                ASSERT_FALSE(dut.MemBusy) << "acknowledged store kept the pipeline stalled";
                ASSERT_NO_FATAL_FAILURE(checkWritePayload(address, funct3, value));
                writeBytes(backing, dut.WriteAddr, dut.WriteFunct3, dut.WriteData);
                writeBytes(expected, address, funct3, value);
                tick();
                idle();
                return;
            }
            ASSERT_TRUE(dut.MemBusy) << "store completed before backing memory accepted it";
            if (requested) --remaining;
            tick();
        }
        FAIL() << "store never reached backing memory";
    }
};

TEST_P(DataCacheTest, IdleAndResetInvalidateEvenAPreviouslyCachedLine) {
    EXPECT_FALSE(dut.MemBusy);
    EXPECT_FALSE(dut.miss);
    EXPECT_FALSE(dut.WriteReq);
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
    reset();
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
}

TEST_P(DataCacheTest, FourSequentialWordsHaveOneMissAndThreeHits) {
    for (unsigned i = 0; i < 4; ++i)
        ASSERT_NO_FATAL_FAILURE(load(base + 4 * i, word, i == 0 ? 1 : 0));
    ASSERT_NO_FATAL_FAILURE(load(base + 0x10c, word, 1));
}

TEST_P(DataCacheTest, SignedAndUnsignedLoadsAtEachByteLane) {
    ASSERT_NO_FATAL_FAILURE(load(base + 4, word, 1));
    for (unsigned offset = 0; offset < lineBytes; ++offset) {
        for (unsigned code : {byte, unsignedByte})
            ASSERT_NO_FATAL_FAILURE(load(base + offset, code, 0));
        if (offset % 2 == 0)
            for (unsigned code : {half, unsignedHalf})
                ASSERT_NO_FATAL_FAILURE(load(base + offset, code, 0));
    }
}

TEST_P(DataCacheTest, ReusedLineSurvivesAThirdTagInTheSameSet) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + sameSetStride, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
    ASSERT_NO_FATAL_FAILURE(load(base + 2 * sameSetStride, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
    ASSERT_NO_FATAL_FAILURE(load(base + sameSetStride, word, 1));
}

TEST_P(DataCacheTest, OtherSetsKeepTheirLinesDuringReplacement) {
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes, word, 1));
    for (unsigned i = 0; i < 3; ++i)
        ASSERT_NO_FATAL_FAILURE(load(base + i * sameSetStride, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes, word, 0));
}

TEST_P(DataCacheTest, ByteStoreHitWritesThroughAndPreservesNeighbours) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(store(base + 1, byte, 0xdeadbeaa));
    EXPECT_EQ(readBytes(backing, base, word), 0x1234aa78u);
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
}

TEST_P(DataCacheTest, HalfStoreHitWritesThroughAndPreservesNeighbours) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(store(base + 2, half, 0xdeadbbbb));
    EXPECT_EQ(readBytes(backing, base, word), 0xbbbb5678u);
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
}

TEST_P(DataCacheTest, WordStoreHitRemainsInMemoryAfterInvalidation) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(store(base, word, 0xdeadbeef));
    ASSERT_NO_FATAL_FAILURE(load(base, word, 0));
    reset();
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
}

TEST_P(DataCacheTest, StoreMissWritesMemoryWithoutAllocatingALine) {
    ASSERT_NO_FATAL_FAILURE(store(base + 4, word, 0x89abcdef));
    ASSERT_NO_FATAL_FAILURE(load(base + 4, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + 4, word, 0));
}

TEST_P(DataCacheTest, MisalignedLoadsUseTheSameBytesAsDataMemory) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + 1, unsignedHalf, 0));
    ASSERT_NO_FATAL_FAILURE(load(base + 1, word, 0));
}

TEST_P(DataCacheTest, LoadAcrossLineBoundaryCombinesBothLines) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes - 1, word));
}

TEST_P(DataCacheTest, MisalignedStoreUpdatesAllAffectedBytes) {
    ASSERT_NO_FATAL_FAILURE(load(base, word, 1));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes, word, 1));
    ASSERT_NO_FATAL_FAILURE(store(base + lineBytes - 1, half, 0xaabb));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes - 4, word));
    ASSERT_NO_FATAL_FAILURE(load(base + lineBytes, word));
}

TEST_P(DataCacheTest, SeededLoadsMatchByteArrayAcrossMultipleSets) {
    std::mt19937 random(1601);
    const std::array<unsigned, 5> codes = {{byte, half, word, unsignedByte, unsignedHalf}};
    for (unsigned i = 0; i < 160; ++i) {
        const unsigned code = codes[i % codes.size()];
        const unsigned address = base + ((random() % 4096) & ~(width(code) - 1));
        ASSERT_NO_FATAL_FAILURE(load(address, code));
        ASSERT_FALSE(HasFailure()) << "read sequence index " << i;
    }
}

TEST_P(DataCacheTest, SeededMixedAccessesMatchIndependentByteArray) {
    std::mt19937 random(1602);
    const std::array<unsigned, 5> codes = {{byte, half, word, unsignedByte, unsignedHalf}};
    for (unsigned i = 0; i < 160; ++i) {
        const unsigned storeCode = (i / 2) % 3;
        const unsigned loadCode = codes[(i / 2) % codes.size()];
        const unsigned code = (i % 2 == 0) ? storeCode : loadCode;
        const unsigned address = base + ((random() % 256) & ~(width(code) - 1));
        if (i % 2 == 0) {
            ASSERT_NO_FATAL_FAILURE(store(address, code, random()));
            ASSERT_NO_FATAL_FAILURE(load(address, code));
        } else {
            ASSERT_NO_FATAL_FAILURE(load(address, code));
        }
        ASSERT_FALSE(HasFailure()) << "mixed sequence index " << i;
    }
    EXPECT_EQ(backing, expected);
}

INSTANTIATE_TEST_SUITE_P(MemoryLatency, DataCacheTest, ::testing::Values(0u, 6u, 12u));

}  // namespace
