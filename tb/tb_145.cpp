#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_145.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_145>(ctx.get());

    auto eval_case = [&](uint8_t y, uint8_t w) {
        // HDL has [3:1], but Verilator packs into a 3-bit bus; treat bits [2:0]
        dut->y = y & 0x7u;
        dut->w = w & 1u;
        dut->eval();

        // Recreate only the cases that exist in RTL; others never used there.
        uint8_t Y2_exp = 0;
        switch (((y & 0x7u) << 1) | (w & 1u)) {
            case 0b0000: Y2_exp = 0; break; // 001
            case 0b0001: Y2_exp = 0; break; // 000
            case 0b0010: Y2_exp = 1; break; // 010
            case 0b0011: Y2_exp = 1; break; // 011
            case 0b0100: Y2_exp = 0; break; // 100
            case 0b0101: Y2_exp = 1; break; // 011
            case 0b0110: Y2_exp = 0; break; // 101
            case 0b0111: Y2_exp = 0; break; // 000
            case 0b1000: Y2_exp = 0; break; // 100
            case 0b1001: Y2_exp = 1; break; // 011
            case 0b1010: Y2_exp = 1; break; // 010
            case 0b1011: Y2_exp = 1; break; // 011
            default:     Y2_exp = 0; break;
        }

        if (dut->Y2 != Y2_exp) {
            std::cerr << "[TB] dut_145 failed: y=" << int(y)
                      << " w=" << int(w)
                      << " expected Y2=" << int(Y2_exp)
                      << " got Y2=" << int(dut->Y2) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Sweep all y (0..7) and w (0..1).
    for (uint8_t y = 0; y < 8; ++y) {
        for (uint8_t w = 0; w < 2; ++w) {
            eval_case(y, w);
        }
    }

    // Extra patterns to strongly toggle each input bit and the Y2 output.
    // Drive Y2 through 0 -> 1 -> 0 explicitly.
    eval_case(0b000, 0); // Y = 001, Y2=0
    eval_case(0b001, 0); // Y = 010, Y2=1
    eval_case(0b100, 0); // Y = 100, Y2=0

    // Toggle individual y bits: 0 -> 1 -> 0 while keeping w constant.
    for (uint8_t bit = 0; bit < 3; ++bit) {
        uint8_t mask = static_cast<uint8_t>(1u << bit);
        eval_case(0, 0);
        eval_case(mask, 0);
        eval_case(0, 0);
    }

    // Toggle w explicitly with a fixed y.
    eval_case(0b010, 0);
    eval_case(0b010, 1);
    eval_case(0b010, 0);

    std::cout << "[TB] dut_145 passed: combinational next-state decoder" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
