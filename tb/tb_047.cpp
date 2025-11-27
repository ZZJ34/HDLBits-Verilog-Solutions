#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_047.h"

int main(int argc, char** argv){
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);
    auto dut = std::make_unique<Vdut_047>(ctx.get());
    const std::array<std::array<uint8_t,3>,4> tbl{{
        std::array<uint8_t,3>{0,0,0},
        std::array<uint8_t,3>{0,1,0},
        std::array<uint8_t,3>{1,0,1},
        std::array<uint8_t,3>{1,1,0},
    }};
    for (auto t: tbl){ dut->in1=t[0]; dut->in2=t[1]; dut->eval(); if (dut->out!=t[2]) return EXIT_FAILURE; }
    // Bidirectional toggles: drive in1, then in2, back to zero; repeat
    for (int rep = 0; rep < 2; ++rep) {
        dut->in1=0; dut->in2=0; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
        dut->in1=1; dut->eval(); if (dut->out!=1) return EXIT_FAILURE;
        dut->in1=0; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
        dut->in2=1; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
        dut->in2=0; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
    }
    std::cout << "[TB] dut_047 passed: a & ~b" << std::endl;
#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE"); if(!covPath||!*covPath) covPath="coverage.dat"; VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
