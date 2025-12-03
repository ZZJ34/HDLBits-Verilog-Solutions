#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_092.h"

struct Stim092 {
    uint8_t L;
    uint8_t R;
    uint8_t E;
    uint8_t w;
};

static inline void tick(Vdut_092 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_092>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->L = 0;
    dut->R = 0;
    dut->E = 0;
    dut->w = 0;
    dut->eval();

    const Stim092 pattern[] = {
        {1u, 1u, 0u, 0u}, // load R=1
        {1u, 0u, 1u, 1u}, // load R=0 (E ignored)
        {0u, 0u, 1u, 1u}, // E=1, w=1
        {0u, 0u, 1u, 0u}, // E=1, w=0
        {0u, 0u, 0u, 1u}, // hold (E=0)
        {0u, 0u, 0u, 0u}, // hold again
    };

    for (const auto &s : pattern) {
        dut->L = s.L;
        dut->R = s.R;
        dut->E = s.E;
        dut->w = s.w;
        tick(dut.get(), ctx.get());

        if (s.L) {
            q_model = s.R & 1u;
        } else if (s.E) {
            q_model = s.w & 1u;
        }

        if (dut->Q != q_model) {
            std::cerr << "[TB] dut_092 failed: "
                      << "L=" << int(s.L)
                      << " R=" << int(s.R)
                      << " E=" << int(s.E)
                      << " w=" << int(s.w)
                      << " expected Q=" << int(q_model)
                      << " got " << int(dut->Q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_092 passed: load/enable DFF" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

