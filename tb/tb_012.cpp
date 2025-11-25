#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_012.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_012>(context.get());

    const std::array<uint16_t, 3> stimuli{{0x0000u, 0xFFFFu, 0x0000u}};
    for (auto vin : stimuli) {
        dut->in = vin;
        dut->eval();

        const uint8_t exp_hi = static_cast<uint8_t>((vin >> 8) & 0xFFu);
        const uint8_t exp_lo = static_cast<uint8_t>(vin & 0xFFu);
        if (dut->out_hi != exp_hi || dut->out_lo != exp_lo) {
            std::cerr << "[TB] dut_012 failed: in=0x" << std::hex << int(vin)
                      << std::dec << ", expected hi/lo=" << int(exp_hi) << "/" << int(exp_lo)
                      << ", got " << int(dut->out_hi) << "/" << int(dut->out_lo) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_012 passed: out_hi=in[15:8], out_lo=in[7:0]" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

