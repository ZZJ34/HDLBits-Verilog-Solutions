#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_146.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_146>(ctx.get());

    auto apply = [&](uint8_t y, uint8_t w) {
        // y is [6:1], pack into bits [5:0] of the C++ signal.
        dut->y = y & 0x3Fu;
        dut->w = w & 1u;
        dut->eval();

        uint8_t Y2_exp = ((y & 0x01u) && !w) ? 1u : 0u;
        uint8_t Y4_exp = (((y >> 1) & 1u) && w)
                       | (((y >> 2) & 1u) && w)
                       | (((y >> 4) & 1u) && w)
                       | (((y >> 5) & 1u) && w);

        if (dut->Y2 != Y2_exp || dut->Y4 != Y4_exp) {
            std::cerr << "[TB] dut_146 failed: y=0x" << std::hex << int(y)
                      << " w=" << std::dec << int(w)
                      << " expected Y2=" << int(Y2_exp)
                      << " Y4=" << int(Y4_exp)
                      << " got Y2=" << int(dut->Y2)
                      << " Y4=" << int(dut->Y4) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Sweep a subset that toggles all bits: 0x00,0x3F and then single-bit patterns.
    apply(0x00u, 0); apply(0x00u, 1);
    apply(0x3Fu, 0); apply(0x3Fu, 1);
    for (uint8_t bit = 0; bit < 6; ++bit) {
        uint8_t y = static_cast<uint8_t>(1u << bit);
        apply(y, 0);
        apply(y, 1);
    }

    std::cout << "[TB] dut_146 passed: simple output decode" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

