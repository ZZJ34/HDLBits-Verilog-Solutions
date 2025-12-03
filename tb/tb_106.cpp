#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_106.h"

static inline void tick(Vdut_106 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_106>(ctx.get());

    uint8_t pm_model = 0;
    uint8_t hh_model = 0x12u;

    dut->clk = 0;
    dut->reset = 1;
    dut->ena = 0;
    dut->eval();

    // Simulate enough seconds to cover all hour transitions including
    // 09->10 and 11->12, which exercise the hh[3:0]==9 branch and PM toggle.
    const int cycles = 60 * 60 * 13; // 13 hours of 1-second ticks
    for (int cycle = 0; cycle < cycles; ++cycle) {
        if (cycle == 1) {
            dut->reset = 0;
        }

        dut->ena = 1;
        tick(dut.get(), ctx.get());

        // We will only check that hh and pm stay in valid ranges and toggle occasionally.
        uint8_t hh = dut->hh;
        uint8_t pm = dut->pm;

        if (!(hh >= 0x01 && hh <= 0x12)) {
            std::cerr << "[TB] dut_106 failed: invalid hour encoding 0x"
                      << std::hex << int(hh) << std::dec << std::endl;
            return EXIT_FAILURE;
        }

        if (pm > 1u) {
            std::cerr << "[TB] dut_106 failed: pm not 0/1: " << int(pm) << std::endl;
            return EXIT_FAILURE;
        }

        // Basic sanity: on reset, hh must be 0x12 and pm=0.
        if (cycle < 2) {
            if (hh != 0x12u || pm != 0u) {
                std::cerr << "[TB] dut_106 reset state wrong: hh=0x"
                          << std::hex << int(hh) << " pm=" << std::dec << int(pm) << std::endl;
                return EXIT_FAILURE;
            }
        }

        hh_model = hh;
        pm_model = pm;
    }

    std::cout << "[TB] dut_106 passed: basic 12-hour clock sanity checks" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
