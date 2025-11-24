#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_015.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_015>(context.get());

    const std::array<uint8_t, 4> stimuli{{0x0u, 0xFu, 0x1u, 0x0u}};
    for (auto vin : stimuli) {
        dut->in = vin & 0xFu;
        dut->eval();

        const uint8_t exp_and = ((vin & 0xFu) == 0xFu) ? 1u : 0u;
        const uint8_t exp_or  = ((vin & 0xFu) != 0x00u) ? 1u : 0u;
        uint8_t x = vin & 0xFu;
        const uint8_t exp_xor = static_cast<uint8_t>(((x >> 3) ^ ((x >> 2) & 1u) ^ ((x >> 1) & 1u) ^ (x & 1u)) & 1u);

        if (dut->out_and != exp_and || dut->out_or != exp_or || dut->out_xor != exp_xor) {
            std::cerr << "[TB] dut_015 failed: in=" << int(vin)
                      << ", exp and/or/xor=" << int(exp_and) << "/" << int(exp_or) << "/" << int(exp_xor)
                      << ", got " << int(dut->out_and) << "/" << int(dut->out_or) << "/" << int(dut->out_xor)
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_015 passed: reductions AND/OR/XOR match" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
