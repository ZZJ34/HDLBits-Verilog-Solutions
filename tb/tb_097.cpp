#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_097.h"

struct Stim097 {
    uint8_t reset;
    uint32_t in;
};

static inline void tick(Vdut_097 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_097>(ctx.get());

    uint32_t prev_in = 0;
    uint32_t out_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->in = 0;
    dut->eval();

    const Stim097 pattern[] = {
        {1u, 0x00000000u}, // reset: out = 0
        {0u, 0xFFFFFFFFu}, // 0->1, no falling edges
        {0u, 0x00000000u}, // 1->0 on all bits
        {1u, 0x00000000u}, // reset again
        {0u, 0xAAAAAAAAu}, // pattern A
        {0u, 0x55555555u}, // 1->0 and 0->1 mixed
        {0u, 0x00000000u}, // 1->0 on bits that were 1
        {1u, 0x00000000u}, // clear latched bits
        {0u, 0x0F0F0F0Fu}, // more patterns
        {0u, 0x00000000u}, // final fall edges
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        dut->in = s.in;
        tick(dut.get(), ctx.get());

        if (s.reset) {
            out_model = 0;
        } else {
            out_model |= (prev_in & ~s.in);
        }

        if (dut->out != out_model) {
            std::cerr << "[TB] dut_097 failed: reset=" << int(s.reset)
                      << " prev_in=0x" << std::hex << prev_in
                      << " in=0x" << s.in
                      << " expected out=0x" << out_model
                      << " got 0x" << dut->out << std::dec << std::endl;
            return EXIT_FAILURE;
        }

        prev_in = s.in;
    }

    std::cout << "[TB] dut_097 passed: 1->0 event latch with reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

