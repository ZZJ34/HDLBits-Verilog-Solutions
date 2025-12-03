#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_110.h"

struct Stim110 {
    uint8_t reset;
};

static inline void tick(Vdut_110 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_110>(ctx.get());

    uint8_t q_model = 0x1u;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    const Stim110 pattern[] = {
        {1u},
        {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u},
        {0u}, {0u}, {0u}, {0u}, {0u}, {0u}
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        tick(dut.get(), ctx.get());

        if (s.reset) {
            q_model = 0x1u;
        } else {
            uint8_t q0 = q_model & 1u;
            uint8_t q4 = (q_model >> 4) & 1u;
            uint8_t q3 = (q_model >> 3) & 1u;
            uint8_t q2 = (q_model >> 2) & 1u;
            uint8_t q1 = (q_model >> 1) & 1u;
            uint8_t new_q4 = q0;
            uint8_t new_q3 = q4;
            uint8_t new_q2 = q3 ^ q0;
            uint8_t new_q1 = q2;
            uint8_t new_q0 = q1;
            q_model = static_cast<uint8_t>((new_q4 << 4) | (new_q3 << 3) |
                                           (new_q2 << 2) | (new_q1 << 1) |
                                           new_q0);
        }

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_110 failed: reset=" << int(s.reset)
                      << " expected q=0x" << std::hex << int(q_model)
                      << " got 0x" << int(dut->q) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_110 passed: 5-bit LFSR sequence with reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

