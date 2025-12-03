#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_105.h"

static inline void tick(Vdut_105 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_105>(ctx.get());

    uint16_t q_model = 0;
    uint8_t ena_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->eval();

    // Run enough cycles to wrap all BCD digits at least once.
    const int cycles = 12000;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        if (cycle == 1) {
            dut->reset = 0;
        }

        tick(dut.get(), ctx.get());

        if (dut->reset) {
            q_model = 0;
            ena_model = 0;
        } else {
            uint8_t d0 = static_cast<uint8_t>(q_model & 0xFu);
            uint8_t d1 = static_cast<uint8_t>((q_model >> 4) & 0xFu);
            uint8_t d2 = static_cast<uint8_t>((q_model >> 8) & 0xFu);
            uint8_t d3 = static_cast<uint8_t>((q_model >> 12) & 0xFu);

            d0++;
            if (d0 == 10u) {
                d0 = 0;
                d1++;
            }
            if (d1 == 10u) {
                d1 = 0;
                d2++;
            }
            if (d2 == 10u) {
                d2 = 0;
                d3++;
            }
            if (d3 == 10u) {
                d3 = 0;
                d2 = 0;
                d1 = 0;
                d0 = 0;
            }

            q_model = static_cast<uint16_t>(d0 | (d1 << 4) | (d2 << 8) | (d3 << 12));

            uint8_t e1 = (d0 == 9u) ? 1u : 0u;
            uint8_t e2 = (d0 == 9u && d1 == 9u) ? 1u : 0u;
            uint8_t e3 = (d0 == 9u && d1 == 9u && d2 == 9u) ? 1u : 0u;
            ena_model = static_cast<uint8_t>((e3 << 2) | (e2 << 1) | e1);
        }

        uint16_t dut_q = static_cast<uint16_t>(dut->q);
        uint8_t dut_ena = static_cast<uint8_t>(dut->ena);

        if (dut_q != q_model || dut_ena != ena_model) {
            std::cerr << "[TB] dut_105 failed at cycle " << cycle
                      << " expected q=0x" << std::hex << q_model
                      << " ena=0x" << int(ena_model)
                      << " got q=0x" << int(dut_q)
                      << " ena=0x" << int(dut_ena) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_105 passed: 4-digit BCD counter with enables" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

