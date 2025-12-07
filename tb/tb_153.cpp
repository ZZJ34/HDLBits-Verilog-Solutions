#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_153.h"

static inline void tick(Vdut_153 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_153>(ctx.get());

    // Model shift/count register
    uint8_t reg_model = 0;

    auto step = [&](uint8_t shift_ena, uint8_t count_ena, uint8_t data, const char *ctx_str) {
        dut->shift_ena = shift_ena & 1u;
        dut->count_ena = count_ena & 1u;
        dut->data = data & 1u;

        // Compute expected next reg value
        uint8_t next = reg_model;
        if (shift_ena) {
            next = static_cast<uint8_t>(((reg_model << 1) & 0xEu) | (data & 1u));
        } else if (count_ena) {
            next = static_cast<uint8_t>((reg_model - 1u) & 0x0Fu);
        }

        tick(dut.get(), ctx.get());
        reg_model = next;

        if (dut->q != reg_model) {
            std::cerr << "[TB] dut_153 failed (" << ctx_str << "): "
                      << "expected q=0x" << std::hex << int(reg_model)
                      << " got 0x" << int(dut->q) << std::dec << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Phase 1: shift in 4 bits 1,0,1,1
    step(1, 0, 1, "shift1");
    step(1, 0, 0, "shift2");
    step(1, 0, 1, "shift3");
    step(1, 0, 1, "shift4");

    // One cycle with both enables deasserted to exercise the "neither" case.
    step(0, 0, 0, "idle_noop");

    // Phase 2: count down a few steps
    for (int i = 0; i < 5; ++i) {
        step(0, 1, 0, "count_down");
    }

    // Phase 3: interleave shift and count
    step(1, 0, 0, "shift_again");
    step(0, 1, 0, "count_again");
    step(1, 0, 1, "shift_again2");
    step(0, 1, 0, "count_again2");

    std::cout << "[TB] dut_153 passed: shift-or-decrement register" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
