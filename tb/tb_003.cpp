#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_003.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_003>(context.get());

    const std::array<uint8_t, 3> stimuli{0U, 1U, 0U};
    for (auto value : stimuli)
    {
        dut->in = value;
        dut->eval();

        if (dut->out != value)
        {
            std::cerr << "[TB] dut_003 failed: expected out="
                      << static_cast<int>(value) << ", got "
                      << static_cast<int>(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_003 passed: in==out for all stimuli" << std::endl;

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
