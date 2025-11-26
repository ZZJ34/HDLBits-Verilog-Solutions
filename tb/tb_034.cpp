#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_034.h"

static inline uint8_t lsb_index(uint8_t v) {
    if (v == 0) return 0;
    for (uint8_t i = 0; i < 4; ++i) {
        if (v & (1u << i)) return i;
    }
    return 0;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_034>(context.get());

    // Iterate all 16 input patterns to cover case items
    for (uint8_t v = 0; v < 16; ++v) {
        dut->in = v & 0xFu; dut->eval();
        const uint8_t expected = lsb_index(v);
        if (dut->pos != expected) {
            std::cerr << "[TB] dut_034 failed: in=" << int(v)
                      << ", expected pos=" << int(expected)
                      << ", got " << int(dut->pos) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Bidirectional toggles: individually toggle each input bit 0->1->0
    for (int i = 0; i < 4; ++i) {
        uint8_t v = 0u;
        dut->in = v; dut->eval();
        if (dut->pos != 0) { std::cerr << "[TB] dut_034 failed: pos should be 0 at in=0" << std::endl; return EXIT_FAILURE; }
        v = static_cast<uint8_t>(1u << i);
        dut->in = v; dut->eval();
        if (dut->pos != i) { std::cerr << "[TB] dut_034 failed: pos mismatch when in=1<<i" << std::endl; return EXIT_FAILURE; }
        v = 0u;
        dut->in = v; dut->eval();
        if (dut->pos != 0) { std::cerr << "[TB] dut_034 failed: pos should be 0 after clearing bit" << std::endl; return EXIT_FAILURE; }
    }

    // Create a pos walk to exercise pos bits both ways: 0,1,2,3,2,1,0
    const uint8_t pos_walk[] = {1u,2u,4u,8u,4u,2u,1u,0u};
    for (uint8_t v : pos_walk) {
        dut->in = v; dut->eval();
        const uint8_t expected = lsb_index(v);
        if (dut->pos != expected) {
            std::cerr << "[TB] dut_034 failed on pos walk: in=" << int(v)
                      << ", expected pos=" << int(expected)
                      << ", got " << int(dut->pos) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_034 passed: pos equals index of LSB set (or 0)" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
