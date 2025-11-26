#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_005.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_005>(context.get());

    // Toggle input both directions to ensure full coverage.
    const std::array<uint8_t, 3> stimuli{0U, 1U, 0U};
    for (auto value : stimuli)
    {
        dut->in = value;
        dut->eval();

        const uint8_t expected = value ? 0U : 1U;
        if (dut->out != expected)
        {
            std::cerr << "[TB] dut_005 failed: in=" << static_cast<int>(value)
                      << ", expected out=" << static_cast<int>(expected)
                      << ", got " << static_cast<int>(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_005 passed: out is bitwise inversion of in"
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
