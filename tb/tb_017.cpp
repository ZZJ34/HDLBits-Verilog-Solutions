#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_017.h"

static inline uint8_t bitrev8(uint8_t v) {
    uint8_t r = 0;
    for (int i = 0; i < 8; ++i) {
        r |= ((v >> (7 - i)) & 1u) << i;
    }
    return r;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_017>(context.get());

    const std::array<uint8_t, 4> stimuli{{0x00u, 0xFFu, 0xA5u, 0x00u}};
    for (auto vin : stimuli) {
        dut->in = vin;
        dut->eval();

        const uint8_t expected = bitrev8(vin);
        if (dut->out != expected) {
            std::cerr << "[TB] dut_017 failed: in=0x" << std::hex << int(vin)
                      << std::dec << ", expected out=0x" << std::hex << int(expected)
                      << std::dec << ", got 0x" << std::hex << int(dut->out) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_017 passed: out is bit-reversed in" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

