#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_149.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_149>(ctx.get());

    auto apply = [&](uint8_t y, uint8_t w) {
        dut->y = y & 0x3Fu;
        dut->w = w & 1u;
        dut->eval();

        uint8_t Y1_exp = ((y & 0x01u) && w) ? 1u : 0u;
        uint8_t Y3_exp = (((y >> 1) & 1u) && !w)
                       | (((y >> 2) & 1u) && !w)
                       | (((y >> 4) & 1u) && !w)
                       | (((y >> 5) & 1u) && !w);

        if (dut->Y1 != Y1_exp || dut->Y3 != Y3_exp) {
            std::cerr << "[TB] dut_149 failed: y=0x" << std::hex << int(y)
                      << " w=" << std::dec << int(w)
                      << " expected Y1=" << int(Y1_exp)
                      << " Y3=" << int(Y3_exp)
                      << " got Y1=" << int(dut->Y1)
                      << " Y3=" << int(dut->Y3) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Toggle patterns to exercise all inputs and outputs.
    apply(0x00u, 0); apply(0x00u, 1);
    apply(0x3Fu, 0); apply(0x3Fu, 1);
    for (uint8_t bit = 0; bit < 6; ++bit) {
        uint8_t y = static_cast<uint8_t>(1u << bit);
        apply(y, 0);
        apply(y, 1);
    }

    std::cout << "[TB] dut_149 passed: simple output decode" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

