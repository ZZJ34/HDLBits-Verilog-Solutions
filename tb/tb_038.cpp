#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_038.h"

static inline uint8_t parity8(uint8_t v) {
    v ^= v >> 4; v ^= v >> 2; v ^= v >> 1; return v & 1u;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_038>(context.get());

    // Basic cases
    const std::array<uint8_t, 6> stimuli{{0x00u, 0x01u, 0x03u, 0x07u, 0xFEu, 0xFFu}};

    for (auto v : stimuli) {
        dut->in = v; dut->eval();
        const uint8_t expected = parity8(v);
        if (dut->parity != expected) {
            std::cerr << "[TB] dut_038 failed: in=0x" << std::hex << int(v) << std::dec
                      << ", expected parity=" << int(expected)
                      << ", got " << int(dut->parity) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Bidirectional toggle each input bit 0->1->0 twice
    for (int i = 0; i < 8; ++i) {
        for (int rep = 0; rep < 2; ++rep) {
            uint8_t v = 0x00u; dut->in = v; dut->eval(); if (dut->parity != parity8(v)) return EXIT_FAILURE;
            v = static_cast<uint8_t>(1u << i); dut->in = v; dut->eval(); if (dut->parity != parity8(v)) return EXIT_FAILURE;
        }
        uint8_t v = 0x00u; dut->in = v; dut->eval(); if (dut->parity != parity8(v)) return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_038 passed: parity reduction verified" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
