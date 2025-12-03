#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_098.h"

static inline void tick_pos(Vdut_098 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

static inline void tick_neg(Vdut_098 *dut, VerilatedContext *ctx) {
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_098>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->d = 0;
    dut->eval();

    // Drive a pattern that changes d before each edge.
    const uint8_t pattern_d_pos[] = {0u, 1u, 0u, 1u};
    const uint8_t pattern_d_neg[] = {1u, 0u, 1u, 0u};

    for (int i = 0; i < 4; ++i) {
        // Rising edge: capture pattern_d_pos[i]
        dut->d = pattern_d_pos[i] & 1u;
        tick_pos(dut.get(), ctx.get());
        q_model = dut->d & 1u;
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_098 failed at posedge i=" << i
                      << " d=" << int(dut->d)
                      << " expected q=" << int(q_model)
                      << " got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }

        // Falling edge: capture pattern_d_neg[i]
        dut->d = pattern_d_neg[i] & 1u;
        tick_neg(dut.get(), ctx.get());
        q_model = dut->d & 1u;
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_098 failed at negedge i=" << i
                      << " d=" << int(dut->d)
                      << " expected q=" << int(q_model)
                      << " got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_098 passed: both-edge triggered behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

