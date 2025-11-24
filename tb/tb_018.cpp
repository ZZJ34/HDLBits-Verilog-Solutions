#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_018.h"

static inline uint32_t sext8to32(uint8_t v) {
    int8_t s = static_cast<int8_t>(v);
    int32_t e = static_cast<int32_t>(s);
    return static_cast<uint32_t>(e);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_018>(context.get());

    const std::array<uint8_t, 4> stimuli{{0x00u, 0x80u, 0x7Fu, 0xFFu}};
    // Also toggle all bits: 0x00 -> 0xFF -> 0x00
    for (auto vin : stimuli) {
        dut->in = vin;
        dut->eval();

        const uint32_t expected = sext8to32(vin);
        if (dut->out != expected) {
            std::cerr << "[TB] dut_018 failed: in=0x" << std::hex << int(vin)
                      << std::dec << ", expected out=0x" << std::hex << expected
                      << std::dec << ", got 0x" << std::hex << uint32_t(dut->out) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Toggle pass for coverage
    dut->in = 0x00; dut->eval();
    dut->in = 0xFF; dut->eval();
    dut->in = 0x00; dut->eval();

    std::cout << "[TB] dut_018 passed: out is 32-bit sign-extended in" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

