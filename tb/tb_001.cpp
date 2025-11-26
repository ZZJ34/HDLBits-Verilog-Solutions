#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_001.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_001>(context.get());
    dut->eval();

    if (dut->one != 1)
    {
        std::cerr << "[TB] dut_001 failed: expected one=1, got "
                  << static_cast<int>(dut->one) << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_001 passed: one=" << static_cast<int>(dut->one)
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
