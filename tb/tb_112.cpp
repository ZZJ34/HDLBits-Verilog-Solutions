#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_112.h"

static inline void tick(Vdut_112 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

static inline uint32_t step_lfsr(uint32_t q) {
    uint32_t q0 = q & 1u;
    uint32_t bit22 = (q >> 22) & 1u;
    uint32_t bit2  = (q >> 2) & 1u;
    uint32_t bit1  = (q >> 1) & 1u;

    uint32_t res = 0;
    // new[31] = 0 ^ q[0]
    res |= (q0 << 31);
    // new[30:22] = q[31:23]
    res |= (q & 0xFF800000u) >> 1;
    // new[21] = q[22] ^ q[0]
    res |= ((bit22 ^ q0) << 21);
    // new[20:2] = q[21:3]
    res |= (q & 0x003FFFF8u) >> 1;
    // new[1] = q[2] ^ q[0]
    res |= ((bit2 ^ q0) << 1);
    // new[0] = q[1] ^ q[0]
    res |= (bit1 ^ q0);
    return res;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_112>(ctx.get());

    uint32_t q_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    // First tick with reset to seed q
    tick(dut.get(), ctx.get());
    q_model = 0x1u;
    if (dut->q != q_model) {
        std::cerr << "[TB] dut_112 failed after reset: expected 0x1 got 0x"
                  << std::hex << dut->q << std::dec << std::endl;
        return EXIT_FAILURE;
    }

    // Deassert reset and run for many cycles to exercise taps.
    dut->reset = 0;
    for (int i = 0; i < 128; ++i) {
        q_model = step_lfsr(q_model);
        tick(dut.get(), ctx.get());
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_112 failed at step " << i
                      << " expected q=0x" << std::hex << q_model
                      << " got 0x" << dut->q << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_112 passed: 32-bit LFSR sequence" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
