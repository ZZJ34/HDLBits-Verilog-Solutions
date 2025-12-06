#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_113.h"

static inline void tick(Vdut_113 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_113>(ctx.get());

    uint8_t q0 = 0, q1 = 0, q2 = 0, out_model = 0;

    dut->clk = 0;
    dut->resetn = 0;
    dut->in = 0;
    dut->eval();

    // Initial synchronous reset (active when resetn=0).
    tick(dut.get(), ctx.get());
    q0 = q1 = q2 = 0;
    out_model = 0;

    // Deassert reset and drive a rich pattern on 'in' to toggle all stages.
    dut->resetn = 1;
    const uint8_t in_seq[] = {
        0u, 1u, 0u, 1u, 1u, 0u, 0u, 1u,
        1u, 0u, 0u, 1u, 0u, 1u, 0u, 0u
    };

    for (uint8_t v : in_seq) {
        dut->in = v;
        tick(dut.get(), ctx.get());

        uint8_t prev_q0 = q0, prev_q1 = q1, prev_q2 = q2;
        q0 = v & 1u;
        q1 = prev_q0;
        q2 = prev_q1;
        out_model = prev_q2;

        if (dut->out != out_model) {
            std::cerr << "[TB] dut_113 failed (shift phase): in=" << int(v)
                      << " expected out=" << int(out_model)
                      << " got " << int(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Assert reset again to exercise the reset branch and force q/out back to 0.
    dut->resetn = 0;
    dut->in = 1;
    tick(dut.get(), ctx.get());
    q0 = q1 = q2 = 0;
    out_model = 0;
    if (dut->out != out_model) {
        std::cerr << "[TB] dut_113 failed (reset phase): expected out=0 got "
                  << int(dut->out) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_113 passed: 3-stage shift with reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
