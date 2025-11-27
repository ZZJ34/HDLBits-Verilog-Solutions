#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_044.h"

int main(int argc, char** argv){
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);
    auto dut = std::make_unique<Vdut_044>(ctx.get());
    for (int rep = 0; rep < 2; ++rep) {
        dut->in = 0; dut->eval(); if (dut->out != 0) return EXIT_FAILURE;
        dut->in = 1; dut->eval(); if (dut->out != 1) return EXIT_FAILURE;
    }
    dut->in = 0; dut->eval(); if (dut->out != 0) return EXIT_FAILURE;
    std::cout << "[TB] dut_044 passed: wire-through" << std::endl;
#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE"); if(!covPath||!*covPath) covPath="coverage.dat"; VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

