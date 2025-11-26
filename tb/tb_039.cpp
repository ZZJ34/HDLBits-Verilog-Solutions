#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_039.h"

// Helper to set 100-bit input as (low 64 bits, high 36 bits)
static inline void set_u100(Vdut_039 *dut, uint64_t low64, uint64_t high36) {
    // Verilated wide signals are little-endian 32-bit words
    dut->in[0] = static_cast<uint32_t>(low64 & 0xFFFFFFFFull);
    dut->in[1] = static_cast<uint32_t>((low64 >> 32) & 0xFFFFFFFFull);
    dut->in[2] = static_cast<uint32_t>(high36 & 0xFFFFFFFFull);
    dut->in[3] = static_cast<uint32_t>((high36 >> 32) & 0xFull); // only 4 bits used
}

static inline unsigned popcnt64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ull);
    x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
    return (unsigned)((((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full) * 0x0101010101010101ull) >> 56);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_039>(context.get());

    struct Vec { uint64_t lo; uint64_t hi36; };
    const std::array<Vec, 5> stimuli{{
        {0x0000000000000000ull, 0x0ull},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFull},
        {0xAAAAAAAAAAAAAAAAull, 0xAAAAAAAAAull},
        {0x5555555555555555ull, 0x555555555ull},
        {0x0000000000000001ull, 0x000000000ull}, // odd parity
    }};

    for (const auto &v : stimuli) {
        set_u100(dut.get(), v.lo, v.hi36);
        dut->eval();

        // Compute reductions
        const bool all_ones = (v.lo == 0xFFFFFFFFFFFFFFFFull) && ((v.hi36 & 0xFFFFFFFFFull) == 0xFFFFFFFFFull);
        const bool any_one = (v.lo != 0ull) || ((v.hi36 & 0xFFFFFFFFFull) != 0ull);
        unsigned hi_pop = popcnt64(v.hi36 & 0xFFFFFFFFFull);
        unsigned lo_pop = popcnt64(v.lo);
        const bool parity = ((hi_pop + lo_pop) & 1u) != 0u;

        if (dut->out_and != (unsigned)all_ones || dut->out_or != (unsigned)any_one || dut->out_xor != (unsigned)parity) {
            std::cerr << "[TB] dut_039 failed: expected and/or/xor=" << all_ones << "/" << any_one
                      << "/" << parity << ", got " << (int)dut->out_and << "/" << (int)dut->out_or
                      << "/" << (int)dut->out_xor << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Bidirectional toggles
    // 1) Ensure out_and and out_or both toggle 0->1->0
    set_u100(dut.get(), 0ull, 0ull); dut->eval();
    if (dut->out_and != 0 || dut->out_or != 0 || dut->out_xor != 0) return EXIT_FAILURE;
    set_u100(dut.get(), 0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFull); dut->eval();
    if (dut->out_and != 1 || dut->out_or != 1) return EXIT_FAILURE;
    set_u100(dut.get(), 0ull, 0ull); dut->eval();
    if (dut->out_and != 0 || dut->out_or != 0) return EXIT_FAILURE;

    // 2) For each input bit: toggle 0->1->0 twice from a zero baseline
    uint64_t lo = 0ull, hi = 0ull;
    set_u100(dut.get(), lo, hi); dut->eval();
    for (int bit = 0; bit < 100; ++bit) {
        for (int rep = 0; rep < 2; ++rep) {
            // Set bit
            if (bit < 64) lo |= (1ull << bit); else hi |= (1ull << (bit - 64));
            set_u100(dut.get(), lo, hi); dut->eval();
            // out_or must be 1; out_xor must be 1 when one bit set from zero baseline
            if (dut->out_or != 1 || dut->out_xor != 1 || dut->out_and != 0) return EXIT_FAILURE;
            // Clear bit back to zero
            if (bit < 64) lo &= ~(1ull << bit); else hi &= ~(1ull << (bit - 64));
            set_u100(dut.get(), lo, hi); dut->eval();
            if (dut->out_or != 0 || dut->out_xor != 0 || dut->out_and != 0) return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_039 passed: reductions on 100-bit vector verified" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
