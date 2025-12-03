#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_103.h"

struct Stim103 {
    uint8_t reset;
    uint8_t enable;
};

static inline void tick(Vdut_103 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_103>(ctx.get());

    dut->clk = 0;
    dut->reset = 1;
    dut->enable = 0;
    dut->eval();

    const Stim103 pattern[] = {
        {1u, 0u}, // reset -> Q=1
        {0u, 1u}, {0u, 1u}, {0u, 1u}, // count up
        {0u, 0u},                     // hold
        {0u, 1u}, {0u, 1u}, {0u, 1u},
        {0u, 1u}, {0u, 1u}, {0u, 1u},
        {0u, 1u}, {0u, 1u}, {0u, 1u}, // reach/past 12 with enable
        {1u, 1u},                     // reset again
        {0u, 1u}, {0u, 1u}
    };

    for (const auto &s : pattern) {
        // Capture previous Q before the clock edge.
        uint8_t q_prev = dut->Q;
        bool reset = s.reset != 0;
        bool enable = s.enable != 0;
        bool wrap_prev = (q_prev == 12u) && enable;

        uint8_t q_exp;
        if (reset || wrap_prev) {
            q_exp = 1u;
        } else if (enable) {
            q_exp = static_cast<uint8_t>((q_prev + 1u) & 0xFu);
        } else {
            q_exp = q_prev;
        }

        uint8_t c_enable_exp = enable ? 1u : 0u;
        // In this implementation c_load is asserted when the counter output
        // is (or will be) reloaded to 1.
        uint8_t c_load_exp = (reset || ((q_exp == 12u) && enable)) ? 1u : 0u;
        uint8_t c_d_exp = c_load_exp ? 1u : 0u;

        dut->reset = s.reset;
        dut->enable = s.enable;
        tick(dut.get(), ctx.get());

        if (dut->Q != q_exp ||
            dut->c_enable != c_enable_exp ||
            dut->c_load != c_load_exp ||
            dut->c_d != c_d_exp) {
            std::cerr << "[TB] dut_103 failed: reset=" << int(s.reset)
                      << " enable=" << int(s.enable)
                      << " expected Q=" << int(q_exp)
                      << " c_enable=" << int(c_enable_exp)
                      << " c_load=" << int(c_load_exp)
                      << " c_d=" << int(c_d_exp)
                      << " got Q=" << int(dut->Q)
                      << " c_enable=" << int(dut->c_enable)
                      << " c_load=" << int(dut->c_load)
                      << " c_d=" << int(dut->c_d) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_103 passed: counter control and outputs" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
