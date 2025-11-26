#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_036.h"

struct Stimulus { uint16_t sc; uint8_t l,d,r,u; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_036>(context.get());

    const std::array<Stimulus, 6> stimuli{{
        {0x0000u, 0,0,0,0},
        {0xE06Bu, 1,0,0,0},
        {0xE072u, 0,1,0,0},
        {0xE074u, 0,0,1,0},
        {0xE075u, 0,0,0,1},
        {0x1234u, 0,0,0,0},
    }};

    auto check = [&](uint16_t sc, uint8_t l, uint8_t d, uint8_t r, uint8_t u, const char* ctx) {
        dut->scancode = sc; dut->eval();
        if (dut->left != l || dut->down != d || dut->right != r || dut->up != u) {
            std::cerr << "[TB] dut_036 failed(" << ctx << ") sc=0x" << std::hex << sc << std::dec
                      << ", expected l/d/r/u=" << int(l) << "/" << int(d)
                      << "/" << int(r) << "/" << int(u)
                      << ", got " << int(dut->left) << "/" << int(dut->down)
                      << "/" << int(dut->right) << "/" << int(dut->up) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    for (const auto &s : stimuli) {
        check(s.sc, s.l, s.d, s.r, s.u, "sanity");
    }

    // Bidirectional toggle all scancode bits 0->1->0 twice; outputs should remain 0
    for (int i = 0; i < 16; ++i) {
        for (int rep = 0; rep < 2; ++rep) {
            check(0x0000u, 0,0,0,0, "toggle_zero");
            check(static_cast<uint16_t>(1u << i), 0,0,0,0, "toggle_bit_on");
        }
        check(0x0000u, 0,0,0,0, "toggle_bit_off");
    }

    // Bidirectional toggle each output by its specific scancode (0->1->0 twice)
    for (int rep = 0; rep < 2; ++rep) {
        check(0x0000u, 0,0,0,0, "out_all_zero");
        check(0xE06Bu, 1,0,0,0, "left_on");
        check(0x0000u, 0,0,0,0, "left_off");

        check(0xE072u, 0,1,0,0, "down_on");
        check(0x0000u, 0,0,0,0, "down_off");

        check(0xE074u, 0,0,1,0, "right_on");
        check(0x0000u, 0,0,0,0, "right_off");

        check(0xE075u, 0,0,0,1, "up_on");
        check(0x0000u, 0,0,0,0, "up_off");
    }

    std::cout << "[TB] dut_036 passed: scancode mapping ok" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
