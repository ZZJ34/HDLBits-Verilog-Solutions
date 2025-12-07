#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_159.h"

static inline void tick(Vdut_159 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_159>(ctx.get());

    uint16_t counter = 0;

    auto load_and_check = [&](uint16_t val, const char *ctx_str) {
        dut->load = 1;
        dut->data = val & 0x3FFu;
        counter = val & 0x3FFu;
        tick(dut.get(), ctx.get());
        // tc is 1 only when data == 0
        uint8_t tc_exp = (counter == 0) ? 1u : 0u;
        if (dut->tc != tc_exp) {
            std::cerr << "[TB] dut_159 failed on load (" << ctx_str << "): "
                      << "data=" << counter
                      << " expected tc=" << int(tc_exp)
                      << " got " << int(dut->tc) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        dut->load = 0;
    };

    auto run_down = [&](int cycles, const char *ctx_str) {
        for (int i = 0; i < cycles; ++i) {
            // Next value
            if (counter > 1) {
                counter -= 1;
            } else {
                counter = 0;
            }
            tick(dut.get(), ctx.get());
            uint8_t tc_exp = (counter == 0) ? 1u : 0u;
            if (dut->tc != tc_exp) {
                std::cerr << "[TB] dut_159 failed during run (" << ctx_str << "): "
                          << "counter=" << counter
                          << " expected tc=" << int(tc_exp)
                          << " got " << int(dut->tc) << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    };

    // Scenario 1: load several distinctive patterns and count down to zero.
    const uint16_t patterns[] = {
        0x000u,  // all zeros
        0x001u,
        0x3FFu,  // all ones (10 bits)
        0x155u,  // 0101...
        0x2AAu   // 1010...
    };
    for (uint16_t p : patterns) {
        load_and_check(p, "pattern");
        // Run enough cycles to reach zero (bounded).
        int cycles = (p > 0) ? (p + 2) : 5;
        run_down(cycles, "down_from_pattern");
    }

    std::cout << "[TB] dut_159 passed: down-counter with terminal count tc" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
