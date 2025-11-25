#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_011.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_011>(context.get());

    // Stimuli sequence toggles each input bit 0->1 and 1->0 at least once.
    const std::array<uint8_t, 7> stimuli{{
        0b000, // baseline
        0b001, // bit0: 0->1
        0b011, // bit1: 0->1
        0b010, // bit0: 1->0
        0b110, // bit2: 0->1
        0b100, // bit1: 1->0
        0b000, // bit2: 1->0
    }};

    for (auto v : stimuli)
    {
        dut->vec = v & 0x7u;
        dut->eval();

        const uint8_t exp_o0 = static_cast<uint8_t>((v >> 0) & 0x1u);
        const uint8_t exp_o1 = static_cast<uint8_t>((v >> 1) & 0x1u);
        const uint8_t exp_o2 = static_cast<uint8_t>((v >> 2) & 0x1u);
        const uint8_t outv = static_cast<uint8_t>(dut->outv);

        if (outv != (v & 0x7u) || dut->o0 != exp_o0 || dut->o1 != exp_o1 || dut->o2 != exp_o2)
        {
            std::cerr << "[TB] dut_011 failed: vec=" << int(v)
                      << ", expected outv=" << int(v & 0x7u)
                      << ", o0/o1/o2=" << int(exp_o0) << "/" << int(exp_o1) << "/" << int(exp_o2)
                      << ", got outv=" << int(outv)
                      << ", o0/o1/o2=" << int(dut->o0) << "/" << int(dut->o1) << "/" << int(dut->o2)
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_011 passed: outv mirrors vec and o[2:0]==vec[2:0] for all stimuli" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0')
    {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

