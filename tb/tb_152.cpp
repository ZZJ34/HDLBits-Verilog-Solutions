#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_152.h"

static inline void tick(Vdut_152 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_152>(ctx.get());

    // Reset
    dut->reset = 1;
    tick(dut.get(), ctx.get());
    dut->reset = 0;

    // Simple model of q
    uint32_t q_model = 0;

    // Run for a bit over 1000 cycles to exercise wrap-around from 999 -> 0.
    for (int i = 0; i < 1105; ++i) {
        // Check current output
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_152 failed: cycle " << i
                      << " expected q=" << q_model
                      << " got " << dut->q << std::endl;
            return EXIT_FAILURE;
        }

        // Compute next model value and tick
        if (q_model < 999) q_model += 1;
        else q_model = 0;
        tick(dut.get(), ctx.get());
    }

    std::cout << "[TB] dut_152 passed: 0..999 counter with wrap-around" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

