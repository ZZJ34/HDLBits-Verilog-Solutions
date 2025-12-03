#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_101.h"

struct Stim101 {
    uint8_t reset;
};

static inline void tick(Vdut_101 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_101>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    const Stim101 pattern[] = {
        {1u}, // reset -> q=1
        {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, // run through sequence
        {0u}, {0u}, {1u}, {0u}
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        tick(dut.get(), ctx.get());

        if (s.reset || q_model == 10u) {
            q_model = 1u;
        } else {
            q_model = static_cast<uint8_t>((q_model + 1u) & 0xFu);
        }

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_101 failed: reset=" << int(s.reset)
                      << " expected q=" << int(q_model)
                      << " got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_101 passed: 1..10 counter with wrap to 1" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

