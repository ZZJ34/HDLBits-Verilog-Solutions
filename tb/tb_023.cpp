#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_023.h"

static void tick(Vdut_023 *dut, VerilatedContext *context) {
    dut->clk = 0;
    dut->eval();
    context->timeInc(1);
    dut->clk = 1;
    dut->eval();
    context->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_023>(context.get());

    // Drive a pattern that exercises 0->1 and 1->0 propagation through all three DFFs.
    const std::array<uint8_t, 7> stimuli{{1, 0, 1, 1, 0, 0, 1}};
    uint8_t w1 = 0, w2 = 0; // model of internal pipeline

    for (const auto din : stimuli) {
        dut->d = din;
        tick(dut.get(), context.get());

        const uint8_t expected_q = w2;
        if (dut->q != expected_q) {
            std::cerr << "[TB] dut_023 failed: d=" << int(din)
                      << ", expected q=" << int(expected_q)
                      << ", got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }

        // Update model after the sampled edge.
        const uint8_t next_w1 = din;
        const uint8_t next_w2 = w1;
        w1 = next_w1;
        w2 = next_w2;
    }

    std::cout << "[TB] dut_023 passed: q follows a 3-stage DFF pipeline" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
