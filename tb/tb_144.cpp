#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_144.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_144>(ctx.get());

    auto eval_case = [&](uint8_t y, uint8_t x) {
        dut->y = y & 0x7u;
        dut->x = x & 1u;
        dut->eval();

        uint8_t Y0_exp = 0;
        switch (((y & 0x7u) << 1) | (x & 1u)) {
            case 0b0000: Y0_exp = 0; break; // 000
            case 0b0001: Y0_exp = 1; break; // 001
            case 0b0010: Y0_exp = 1; break; // 001
            case 0b0011: Y0_exp = 0; break; // 100
            case 0b0100: Y0_exp = 0; break; // 010
            case 0b0101: Y0_exp = 1; break; // 001
            case 0b0110: Y0_exp = 1; break; // 001
            case 0b0111: Y0_exp = 0; break; // 010
            case 0b1000: Y0_exp = 1; break; // 011
            case 0b1001: Y0_exp = 0; break; // 100
            default:     Y0_exp = 0; break;
        }

        uint8_t z_exp = (y == 0b011 || y == 0b100) ? 1u : 0u;

        if (dut->z != z_exp || dut->Y0 != Y0_exp) {
            std::cerr << "[TB] dut_144 failed: y=" << int(y)
                      << " x=" << int(x)
                      << " expected Y0=" << int(Y0_exp)
                      << " z=" << int(z_exp)
                      << " got Y0=" << int(dut->Y0)
                      << " z=" << int(dut->z) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Sweep the 10 defined case entries explicitly.
    const uint8_t ys[] = {0,0,1,1,2,2,3,3,4,4};
    const uint8_t xs[] = {0,1,0,1,0,1,0,1,0,1};
    for (size_t i = 0; i < 10; ++i) {
        eval_case(ys[i], xs[i]);
    }

    // Extra patterns to toggle y and x bits beyond the defined cases.
    for (uint8_t y = 0; y < 8; ++y) {
        for (uint8_t x = 0; x < 2; ++x) {
            eval_case(y, x);
        }
    }

    std::cout << "[TB] dut_144 passed: Mealy-style combinational block" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
