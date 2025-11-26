#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_040.h"

static inline void set_u100(Vdut_040 *dut, uint64_t low64, uint64_t high36) {
    dut->in[0] = static_cast<uint32_t>(low64 & 0xFFFFFFFFull);
    dut->in[1] = static_cast<uint32_t>((low64 >> 32) & 0xFFFFFFFFull);
    dut->in[2] = static_cast<uint32_t>(high36 & 0xFFFFFFFFull);
    dut->in[3] = static_cast<uint32_t>((high36 >> 32) & 0xFull);
}

static inline uint64_t get_out_lo64(const Vdut_040 *dut) {
    return (uint64_t(dut->out[1]) << 32) | uint64_t(dut->out[0]);
}

static inline uint64_t get_out_hi36(const Vdut_040 *dut) {
    return (uint64_t(dut->out[3] & 0xFu) << 32) | uint64_t(dut->out[2]);
}

// Reverse bits in 100-bit vector represented by (low64, high36)
static inline void reverse_u100(uint64_t in_lo, uint64_t in_hi36, uint64_t &out_lo, uint64_t &out_hi36) {
    // Build bit-by-bit
    out_lo = 0; out_hi36 = 0;
    for (int i = 0; i < 100; ++i) {
        int src = i;
        int dst = 99 - i;
        int src_word = (src < 64) ? 0 : 1; // 0=low64, 1=hi36
        uint64_t src_bit = (src_word == 0) ? ((in_lo >> src) & 1ull)
                                           : ((in_hi36 >> (src - 64)) & 1ull);
        if (dst < 64) {
            out_lo |= (src_bit << dst);
        } else {
            out_hi36 |= (src_bit << (dst - 64));
        }
    }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_040>(context.get());

    struct Vec { uint64_t lo; uint64_t hi36; };
    const std::array<Vec, 3> stimuli{{
        {0x0ull, 0x0ull},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFull},
        {0x0123456789ABCDEFull, 0x123456789ull},
    }};

    for (const auto &v : stimuli) {
        set_u100(dut.get(), v.lo, v.hi36);
        dut->eval();

        uint64_t exp_lo = 0, exp_hi = 0;
        reverse_u100(v.lo, v.hi36, exp_lo, exp_hi);
        const uint64_t got_lo = get_out_lo64(dut.get());
        const uint64_t got_hi = get_out_hi36(dut.get()) & 0xFFFFFFFFFull;

        if (((got_lo ^ exp_lo) != 0ull) || ((got_hi ^ exp_hi) & 0xFFFFFFFFFull)) {
            std::cerr << "[TB] dut_040 failed: expected reversed 100-bit value" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Bidirectional toggle each input bit individually: 0->1->0 twice
    uint64_t lo = 0ull, hi = 0ull;
    set_u100(dut.get(), lo, hi);
    dut->eval();
    {
        uint64_t exp_lo = 0, exp_hi = 0;
        reverse_u100(lo, hi, exp_lo, exp_hi);
        if (get_out_lo64(dut.get()) != exp_lo || (get_out_hi36(dut.get()) & 0xFFFFFFFFFull) != exp_hi) {
            std::cerr << "[TB] dut_040 failed: initial zero mismatch" << std::endl;
            return EXIT_FAILURE;
        }
    }
    for (int bit = 0; bit < 100; ++bit) {
        for (int rep = 0; rep < 2; ++rep) {
            // Set bit to 1
            if (bit < 64) lo |= (1ull << bit); else hi |= (1ull << (bit - 64));
            set_u100(dut.get(), lo, hi); dut->eval();
            uint64_t exp_lo = 0, exp_hi = 0; reverse_u100(lo, hi, exp_lo, exp_hi);
            if (get_out_lo64(dut.get()) != exp_lo || (get_out_hi36(dut.get()) & 0xFFFFFFFFFull) != exp_hi) return EXIT_FAILURE;
            // Clear bit back to 0
            if (bit < 64) lo &= ~(1ull << bit); else hi &= ~(1ull << (bit - 64));
            set_u100(dut.get(), lo, hi); dut->eval();
            reverse_u100(lo, hi, exp_lo, exp_hi);
            if (get_out_lo64(dut.get()) != exp_lo || (get_out_hi36(dut.get()) & 0xFFFFFFFFFull) != exp_hi) return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_040 passed: bit-reverse of 100-bit vector" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
