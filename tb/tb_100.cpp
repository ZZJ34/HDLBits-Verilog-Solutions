#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_100.h"

struct Stim100 {
    uint8_t reset;
};

static inline void tick(Vdut_100 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_100>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    const Stim100 pattern[] = {
        {1u}, // reset to 0
        {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, {0u}, // count through 0..9->0
        {0u}, {0u}, {0u}, // a few more cycles
        {1u}, // reset again
        {0u}, {0u}, {0u}
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        tick(dut.get(), ctx.get());

        if (s.reset || q_model == 9u) {
            q_model = 0u;
        } else {
            q_model = static_cast<uint8_t>((q_model + 1u) & 0xFu);
        }

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_100 failed: reset=" << int(s.reset)
                      << " expected q=" << int(q_model)
                      << " got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_100 passed: decade counter with reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

