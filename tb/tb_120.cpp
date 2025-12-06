#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_120.h"

struct Stim120 {
    uint8_t reset;
    uint8_t in;
};

static inline void tick(Vdut_120 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_120>(ctx.get());

    enum { A = 0, B = 1 };
    uint8_t state = B;

    dut->clk = 0;
    dut->reset = 1;
    dut->in = 0;
    dut->eval();

    const Stim120 pattern[] = {
        {1u, 0u}, // sync reset to B
        {0u, 0u},
        {0u, 1u},
        {0u, 1u},
        {0u, 0u},
        {1u, 1u},
        {0u, 1u},
        {0u, 0u},
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        dut->in = s.in;
        tick(dut.get(), ctx.get());

        if (s.reset) {
            state = B;
        } else {
            switch (state) {
                case B: state = (s.in ? B : A); break;
                case A: state = (s.in ? A : B); break;
            }
        }

        uint8_t out_exp = (state == B) ? 1u : 0u;
        if (dut->out != out_exp) {
            std::cerr << "[TB] dut_120 failed: reset=" << int(s.reset)
                      << " in=" << int(s.in)
                      << " expected out=" << int(out_exp)
                      << " got " << int(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_120 passed: 2-state FSM with sync reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

