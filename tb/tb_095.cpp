#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_095.h"

static inline void tick(Vdut_095 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_095>(ctx.get());

    uint8_t prev_in = 0;

    dut->clk = 0;
    dut->in = 0;
    dut->eval();

    const uint8_t pattern[] = {
        0x00u, 0xFFu, 0x00u, 0xAAu, 0x55u, 0x00u,
        0x0Fu, 0xF0u, 0x00u
    };

    for (uint8_t in_val : pattern) {
        dut->in = in_val;
        tick(dut.get(), ctx.get());

        uint8_t pedge_model = static_cast<uint8_t>((~prev_in) & in_val);
        if (dut->pedge != pedge_model) {
            std::cerr << "[TB] dut_095 failed: prev_in=0x" << std::hex << int(prev_in)
                      << " in=0x" << int(in_val)
                      << " expected pedge=0x" << int(pedge_model)
                      << " got 0x" << int(dut->pedge) << std::dec << std::endl;
            return EXIT_FAILURE;
        }

        prev_in = in_val;
    }

    std::cout << "[TB] dut_095 passed: per-bit posedge detector" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

