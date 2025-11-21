#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_006.h"

struct Stimulus
{
    uint8_t a;
    uint8_t b;
};

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_006>(context.get());

    // Sequence ensures each input toggles 0->1 and 1->0 for full coverage.
    const std::array<Stimulus, 5> stimuli{{
        {0U, 0U},
        {1U, 0U},
        {1U, 1U},
        {0U, 1U},
        {0U, 0U},
    }};

    for (const auto &stim : stimuli)
    {
        dut->a = stim.a;
        dut->b = stim.b;
        dut->eval();

        const uint8_t expected = static_cast<uint8_t>(stim.a & stim.b);
        if (dut->out != expected)
        {
            std::cerr << "[TB] dut_006 failed: a=" << static_cast<int>(stim.a)
                      << ", b=" << static_cast<int>(stim.b)
                      << ", expected out=" << static_cast<int>(expected)
                      << ", got " << static_cast<int>(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_006 passed: out == (a & b) for all stimuli"
              << std::endl;

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
