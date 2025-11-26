#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_035.h"

static inline uint8_t lsb_index8(uint8_t v) {
    if (v == 0) return 0;
    for (uint8_t i = 0; i < 8; ++i) if (v & (1u << i)) return i;
    return 0;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_035>(context.get());

    // Cover each casez item with one-hot at positions 0..7, plus default with 0
    const std::array<uint8_t, 9> stimuli{{0x00u, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u, 0x80u}};

    for (auto v : stimuli) {
        dut->in = v; dut->eval();
        const uint8_t expected = lsb_index8(v);
        if (dut->pos != expected) {
            std::cerr << "[TB] dut_035 failed: in=0x" << std::hex << int(v) << std::dec
                      << ", expected pos=" << int(expected)
                      << ", got " << int(dut->pos) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Bidirectional toggles for each input bit: 0->1->0 twice
    for (int i = 0; i < 8; ++i) {
        for (int rep = 0; rep < 2; ++rep) {
            dut->in = 0x00u; dut->eval(); if (dut->pos != 0) return EXIT_FAILURE;
            uint8_t v = static_cast<uint8_t>(1u << i);
            dut->in = v; dut->eval(); if (dut->pos != i) return EXIT_FAILURE;
        }
        dut->in = 0x00u; dut->eval(); if (dut->pos != 0) return EXIT_FAILURE;
    }

    // Walk pos up and down to ensure pos bits toggle both ways
    const uint8_t walk[] = {1u,2u,4u,8u,16u,32u,64u,128u,64u,32u,16u,8u,4u,2u,1u,0u};
    for (auto v : walk) {
        dut->in = v; dut->eval();
        const uint8_t expected = lsb_index8(v);
        if (dut->pos != expected) return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_035 passed: casez LSB encoder behavior verified" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
