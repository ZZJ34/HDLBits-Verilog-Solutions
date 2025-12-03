#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_091.h"

struct Stim091 {
    uint8_t L;
    uint8_t r_in;
    uint8_t q_in;
};

static inline void tick(Vdut_091 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_091>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->L = 0;
    dut->r_in = 0;
    dut->q_in = 0;
    dut->eval();

    const Stim091 pattern[] = {
        {1u, 1u, 0u}, // load r_in=1
        {0u, 0u, 0u}, // follow q_in=0
        {0u, 0u, 1u}, // follow q_in=1
        {1u, 0u, 1u}, // load r_in=0
        {0u, 1u, 1u}, // follow q_in=1
        {0u, 1u, 0u}, // follow q_in=0
    };

    for (const auto &s : pattern) {
        dut->L = s.L;
        dut->r_in = s.r_in;
        dut->q_in = s.q_in;
        tick(dut.get(), ctx.get());

        if (s.L) {
            q_model = s.r_in & 1u;
        } else {
            q_model = s.q_in & 1u;
        }

        if (dut->Q != q_model) {
            std::cerr << "[TB] dut_091 failed: "
                      << "L=" << int(s.L)
                      << " r_in=" << int(s.r_in)
                      << " q_in=" << int(s.q_in)
                      << " expected Q=" << int(q_model)
                      << " got " << int(dut->Q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_091 passed: muxed DFF with load" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

