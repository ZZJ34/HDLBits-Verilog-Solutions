#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_093.h"

static inline void tick(Vdut_093 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_093>(ctx.get());

    uint8_t q0 = 0, q1 = 0, q2 = 0;

    dut->clk = 0;
    dut->x = 0;
    dut->eval();

    const uint8_t pattern[] = {0u, 1u, 0u, 1u, 1u, 0u, 1u, 0u};

    for (uint8_t x_val : pattern) {
        dut->x = x_val & 1u;
        tick(dut.get(), ctx.get());

        uint8_t prev_q0 = q0;
        uint8_t prev_q1 = q1;
        uint8_t prev_q2 = q2;

        q0 = static_cast<uint8_t>((prev_q0 ^ x_val) & 1u);
        q1 = static_cast<uint8_t>((~prev_q1 & x_val) & 1u);
        q2 = static_cast<uint8_t>((~prev_q2 | x_val) & 1u);

        uint8_t z_model = static_cast<uint8_t>(!(q0 | q1 | q2));

        if (dut->z != z_model) {
            std::cerr << "[TB] dut_093 failed: x=" << int(x_val)
                      << " expected z=" << int(z_model)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_093 passed: 3-bit state machine output" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

