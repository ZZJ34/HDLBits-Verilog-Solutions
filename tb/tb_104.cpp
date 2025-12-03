#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_104.h"

static inline void tick(Vdut_104 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_104>(ctx.get());

    // Reference model: three chained BCD digits.
    uint8_t d0 = 0, d1 = 0, d2 = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    // Simulate enough cycles to hit OneHertz at least once (need 1000 cycles).
    const int cycles = 1100;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        // Deassert reset after first cycle
        if (cycle == 1) {
            dut->reset = 0;
        }

        tick(dut.get(), ctx.get());

        // Update reference digits
        if (dut->reset) {
            d0 = d1 = d2 = 0;
        } else {
            d0 = static_cast<uint8_t>((d0 + 1u) % 10u);
            if (d0 == 0) {
                d1 = static_cast<uint8_t>((d1 + 1u) % 10u);
                if (d1 == 0) {
                    d2 = static_cast<uint8_t>((d2 + 1u) % 10u);
                }
            }
        }

        uint8_t c0_exp = 1u;
        uint8_t c1_exp = (d0 == 9u) ? 1u : 0u;
        uint8_t c2_exp = (d0 == 9u && d1 == 9u) ? 1u : 0u;
        uint8_t one_exp = (d0 == 9u && d1 == 9u && d2 == 9u) ? 1u : 0u;

        uint8_t c_enable = dut->c_enable;
        uint8_t c0 = c_enable & 1u;
        uint8_t c1 = (c_enable >> 1) & 1u;
        uint8_t c2 = (c_enable >> 2) & 1u;

        if (c0 != c0_exp || c1 != c1_exp || c2 != c2_exp || dut->OneHertz != one_exp) {
            std::cerr << "[TB] dut_104 failed at cycle " << cycle
                      << " expected c_enable=" << int((c2_exp << 2) | (c1_exp << 1) | c0_exp)
                      << " OneHertz=" << int(one_exp)
                      << " got c_enable=" << int(dut->c_enable)
                      << " OneHertz=" << int(dut->OneHertz) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_104 passed: cascaded BCD enables and OneHertz pulse" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

