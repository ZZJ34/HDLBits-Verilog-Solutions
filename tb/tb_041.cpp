#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_041.h"

// dut_041: popcount of 255-bit input (in[254:0]) -> 8-bit out
static inline void wide_zero(Vdut_041* dut) {
    // 255 bits => 8x32-bit words
    for (int i = 0; i < 8; ++i) dut->in[i] = 0u;
}
static inline void wide_set_bit(Vdut_041* dut, int bit, bool val) {
    const int w = bit >> 5;
    const int b = bit & 31;
    const uint32_t mask = 1u << b;
    if (val) dut->in[w] |= mask; else dut->in[w] &= ~mask;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_041>(context.get());

    // Start from zero; out must be 0
    wide_zero(dut.get());
    dut->eval();
    if (dut->out != 0) { std::cerr << "[TB] dut_041 failed: out!=0 at zero" << std::endl; return EXIT_FAILURE; }

    // Toggle each input bit 0->1->0 and track expected popcount
    uint16_t expected = 0;
    for (int i = 0; i < 255; ++i) {
        wide_set_bit(dut.get(), i, true); dut->eval();
        if (++expected != dut->out) { std::cerr << "[TB] dut_041 failed: after set bit " << i << std::endl; return EXIT_FAILURE; }
        wide_set_bit(dut.get(), i, false); dut->eval();
        if (--expected != dut->out) { std::cerr << "[TB] dut_041 failed: after clear bit " << i << std::endl; return EXIT_FAILURE; }
    }

    // All ones -> 255
    for (int i = 0; i < 255; ++i) wide_set_bit(dut.get(), i, true);
    dut->eval();
    if (dut->out != 255u) { std::cerr << "[TB] dut_041 failed: all ones expected 255" << std::endl; return EXIT_FAILURE; }
    // Back to zero
    wide_zero(dut.get()); dut->eval(); if (dut->out != 0) return EXIT_FAILURE;

    std::cout << "[TB] dut_041 passed: popcount over 255 bits" << std::endl;

#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

