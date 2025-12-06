#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_115.h"

static inline void tick(Vdut_115 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_115>(ctx.get());

    uint8_t q = 0;

    dut->clk = 0;
    dut->enable = 0;
    dut->S = 0;
    dut->A = dut->B = dut->C = 0;
    dut->eval();

    auto check_Z = [&](uint8_t idx) {
        dut->A = (idx >> 2) & 1u;
        dut->B = (idx >> 1) & 1u;
        dut->C = idx & 1u;
        dut->eval();
        uint8_t bit = (q >> idx) & 1u;
        if (dut->Z != bit) {
            std::cerr << "[TB] dut_115 failed: addr=" << int(idx)
                      << " expected Z=" << int(bit)
                      << " got " << int(dut->Z) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Phase 1: enable=1, shift in a known pattern.
    dut->enable = 1;
    const uint8_t seq[] = {1u, 0u, 1u, 1u, 0u, 0u, 1u, 0u};
    for (int i = 0; i < 8; ++i) {
        dut->S = seq[i];
        tick(dut.get(), ctx.get());
        q = static_cast<uint8_t>(((q << 1) & 0xFEu) | (seq[i] & 1u));
        for (uint8_t idx = 0; idx < 8; ++idx) {
            check_Z(idx);
        }
    }

    // Phase 2: disable enable and ensure q holds.
    dut->enable = 0;
    uint8_t q_hold = q;
    for (int i = 0; i < 4; ++i) {
        dut->S = static_cast<uint8_t>(i & 1u);
        tick(dut.get(), ctx.get());
        if (q != q_hold) {
            std::cerr << "[TB] dut_115 failed: q changed when enable=0" << std::endl;
            return EXIT_FAILURE;
        }
        for (uint8_t idx = 0; idx < 8; ++idx) {
            check_Z(idx);
        }
    }

    std::cout << "[TB] dut_115 passed: serial load + muxed readout" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

