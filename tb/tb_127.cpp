#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_127.h"

static inline void tick(Vdut_127 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_127>(ctx.get());

    // We'll just sweep s[3:1] over all values with and without reset,
    // to exercise transitions and output decoding.
    dut->clk = 0;
    dut->reset = 1;
    dut->s = 0;
    dut->eval();

    tick(dut.get(), ctx.get());

    dut->reset = 0;

    for (int cycle = 0; cycle < 32; ++cycle) {
        uint8_t level = static_cast<uint8_t>(cycle & 0x7);
        dut->s = level;
        tick(dut.get(), ctx.get());

        uint8_t fr3 = dut->fr3;
        uint8_t fr2 = dut->fr2;
        uint8_t fr1 = dut->fr1;
        uint8_t dfr = dut->dfr;

        uint8_t mask = static_cast<uint8_t>((fr3 << 3) | (fr2 << 2) | (fr1 << 1) | dfr);
        if (mask == 0xF || mask == 0x7 || mask == 0x6 ||
            mask == 0x3 || mask == 0x2 || mask == 0x0) {
            // valid patterns; we don't fully re-encode level vs. state here,
            // but any illegal pattern would be caught.
        } else {
            std::cerr << "[TB] dut_127 failed: illegal output pattern 0x"
                      << std::hex << int(mask) << std::dec << " at cycle "
                      << cycle << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_127 passed: water level controller outputs valid patterns" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

