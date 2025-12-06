#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_111.h"

struct Stim111 {
    uint8_t L;
    uint8_t R; // 3-bit load
};

static inline void tick(Vdut_111 *dut, VerilatedContext *ctx) {
    dut->KEY = static_cast<uint8_t>(dut->KEY & ~0x1u); // clk=0
    dut->eval();
    ctx->timeInc(1);
    dut->KEY = static_cast<uint8_t>(dut->KEY | 0x1u); // clk=1
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_111>(ctx.get());

    uint8_t Q = 0;

    dut->SW = 0;
    dut->KEY = 0; // KEY[0]=clk, KEY[1]=L
    dut->eval();

    const Stim111 pattern[] = {
        {1u, 0x5u}, // load 101
        {0u, 0x0u},
        {0u, 0x0u},
        {1u, 0x3u}, // load 011
        {0u, 0x0u},
        {0u, 0x0u},
        {1u, 0x7u}, // load 111
        {0u, 0x0u},
        {0u, 0x0u},
    };

    for (const auto &s : pattern) {
        // Drive load and data
        dut->SW = s.R & 0x7u;
        uint8_t key = dut->KEY;
        key = static_cast<uint8_t>((key & ~0x2u) | ((s.L & 0x1u) << 1)); // KEY[1]=L
        dut->KEY = key;

        // Compute expected next Q
        if (s.L) {
            Q = s.R & 0x7u;
        } else {
            uint8_t q2 = (Q >> 2) & 1u;
            uint8_t q1 = (Q >> 1) & 1u;
            uint8_t q0 = Q & 1u;
            uint8_t new2 = static_cast<uint8_t>(q2 ^ q1);
            uint8_t new1 = q0;
            uint8_t new0 = q2;
            Q = static_cast<uint8_t>((new2 << 2) | (new1 << 1) | new0);
        }

        tick(dut.get(), ctx.get());

        if ((dut->LEDR & 0x7u) != Q) {
            std::cerr << "[TB] dut_111 failed: L=" << int(s.L)
                      << " R=" << int(s.R)
                      << " expected LEDR=" << int(Q)
                      << " got " << int(dut->LEDR & 0x7u) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_111 passed: 3-bit LFSR with load" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

