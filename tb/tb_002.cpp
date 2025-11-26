#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_002.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_002>(context.get());
    dut->eval();

    if (dut->zero != 0)
    {
        std::cerr << "[TB] dut_002 failed: expected zero=0, got "
                  << static_cast<int>(dut->zero) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_002 passed: zero=" << static_cast<int>(dut->zero)
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
