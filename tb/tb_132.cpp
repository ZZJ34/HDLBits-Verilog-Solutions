#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_132.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_132>(ctx.get());

    // Phase 0: explicitly toggle each state bit 0->1->0 with in=0 to
    // exercise state input toggles.
    for (int s = 0; s < 10; ++s) {
        uint16_t bit = static_cast<uint16_t>(1u << s);
        dut->state = 0;
        dut->in = 0;
        dut->eval();
        dut->state = bit;
        dut->eval();
        dut->state = 0;
        dut->eval();
    }

    // Phase 1: for each one-hot state, toggle 'in' 0->1->0 twice to exercise
    // both branches of each product term and the outputs.
    for (int s = 0; s < 10; ++s) {
        uint16_t state = static_cast<uint16_t>(1u << s);
        for (int rep = 0; rep < 2; ++rep) {
            for (uint8_t in_val : {uint8_t(0), uint8_t(1), uint8_t(0)}) {
                dut->state = state;
                dut->in = in_val;
                dut->eval();
                uint8_t out1_exp = ((state & ((1u<<8)|(1u<<9))) ? 1u : 0u);
                uint8_t out2_exp = ((state & ((1u<<7)|(1u<<9))) ? 1u : 0u);
                if (dut->out1 != out1_exp || dut->out2 != out2_exp) {
                    std::cerr << "[TB] dut_132 failed: state bit " << s
                              << " in=" << int(in_val)
                              << " expected out1=" << int(out1_exp)
                              << " out2=" << int(out2_exp)
                              << " got out1=" << int(dut->out1)
                              << " out2=" << int(dut->out2) << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }
    }

    std::cout << "[TB] dut_132 passed: next-state and outputs combinational block" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
