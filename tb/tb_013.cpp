#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_013.h"

static inline uint32_t byteswap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24)
         | ((v & 0x0000FF00u) << 8)
         | ((v & 0x00FF0000u) >> 8)
         | ((v & 0xFF000000u) >> 24);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_013>(context.get());

    const std::array<uint32_t, 4> stimuli{{0x00000000u, 0xFFFFFFFFu, 0x12345678u, 0x00000000u}};
    for (auto vin : stimuli) {
        dut->in = vin;
        dut->eval();

        const uint32_t expected = byteswap32(vin);
        if (dut->out != expected) {
            std::cerr << "[TB] dut_013 failed: in=0x" << std::hex << vin
                      << std::dec << ", expected out=0x" << std::hex << expected
                      << std::dec << ", got 0x" << std::hex << uint32_t(dut->out) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_013 passed: out bytes are reversed from in" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

