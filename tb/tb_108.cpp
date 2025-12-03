#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_108.h"

struct Stim108 {
    uint8_t load;
    uint8_t ena0;
    uint8_t ena1;
};

static inline void tick(Vdut_108 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_108>(ctx.get());

    // Represent 100-bit values as low 64 bits + high 36 bits.
    uint64_t lo = 0;
    uint64_t hi = 0;

    auto set_u100 = [&](uint64_t alo, uint64_t ahi36) {
        dut->data[0] = static_cast<uint32_t>(alo & 0xFFFFFFFFull);
        dut->data[1] = static_cast<uint32_t>((alo >> 32) & 0xFFFFFFFFull);
        dut->data[2] = static_cast<uint32_t>(ahi36 & 0xFFFFFFFFull);
        dut->data[3] = static_cast<uint32_t>((ahi36 >> 32) & 0xFull);
    };

    auto get_bit_q = [&](int bit) -> uint8_t {
        const int wi = bit >> 5;
        const int bi = bit & 31;
        return static_cast<uint8_t>((dut->q[wi] >> bi) & 1u);
    };

    auto rotate_right1 = [&](uint64_t& rlo, uint64_t& rhi) {
        uint8_t bit0 = static_cast<uint8_t>(rlo & 1ull);           // old q[0]
        uint8_t bit64 = static_cast<uint8_t>(rhi & 1ull);          // old q[64]
        uint64_t new_lo = (rlo >> 1) | (static_cast<uint64_t>(bit64) << 63);
        uint64_t new_hi = ((rhi >> 1) & ((1ull << 36) - 1ull)) | (static_cast<uint64_t>(bit0) << 35);
        rlo = new_lo;
        rhi = new_hi;
    };

    auto rotate_left1 = [&](uint64_t& rlo, uint64_t& rhi) {
        uint8_t bit99 = static_cast<uint8_t>((rhi >> 35) & 1ull);  // old q[99]
        uint8_t bit63 = static_cast<uint8_t>((rlo >> 63) & 1ull);  // old q[63]
        uint64_t new_lo = (rlo << 1) | static_cast<uint64_t>(bit99);
        uint64_t new_hi = ((rhi << 1) & ((1ull << 36) - 1ull)) | static_cast<uint64_t>(bit63);
        rlo = new_lo;
        rhi = new_hi;
    };

    dut->clk = 0;
    dut->load = 0;
    dut->ena = 0;
    ctx->timeInc(1);

    // Initial pattern: bit0 and bit99 set.
    lo = 1ull;
    hi = (1ull << 35);
    set_u100(lo, hi);

    const Stim108 pattern[] = {
        {1u, 0u, 0u}, // load initial data
        {0u, 1u, 0u}, // rotate right twice
        {0u, 1u, 0u},
        {0u, 0u, 1u}, // rotate left twice
        {0u, 0u, 1u},
        {0u, 0u, 0u}  // hold
    };

    for (const auto &s : pattern) {
        dut->load = s.load;
        dut->ena = static_cast<uint8_t>((s.ena1 << 1) | s.ena0);

        tick(dut.get(), ctx.get());

        if (s.load) {
            // q should match loaded data on first cycle.
        } else if (s.ena0 && !s.ena1) {
            rotate_right1(lo, hi);
        } else if (!s.ena0 && s.ena1) {
            rotate_left1(lo, hi);
        }

        // Compare DUT q bits with model when not in hold-only case.
        if (!s.load && (s.ena0 || s.ena1)) {
            for (int bit = 0; bit < 100; ++bit) {
                uint8_t exp = (bit < 64)
                                  ? static_cast<uint8_t>((lo >> bit) & 1ull)
                                  : static_cast<uint8_t>((hi >> (bit - 64)) & 1ull);
                uint8_t got = get_bit_q(bit);
                if (exp != got) {
                    std::cerr << "[TB] dut_108 failed: bit " << bit
                              << " expected " << int(exp)
                              << " got " << int(got) << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }
    }

    std::cout << "[TB] dut_108 passed: 100-bit bidirectional rotation" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
