#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_048.h"

static inline uint8_t ref(uint8_t in1, uint8_t in2, uint8_t in3){
    return in3 ^ static_cast<uint8_t>(~(in1 ^ in2) & 1u);
}

int main(int argc, char** argv){
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);
    auto dut = std::make_unique<Vdut_048>(ctx.get());
    for (int i=0;i<8;++i){
        uint8_t in1=(i>>0)&1u, in2=(i>>1)&1u, in3=(i>>2)&1u;
        dut->in1=in1; dut->in2=in2; dut->in3=in3; dut->eval();
        if (dut->out != ref(in1,in2,in3)) return EXIT_FAILURE;
    }
    // Bidirectional toggles: toggle each input bit while others fixed to 0
    for (int rep=0; rep<2; ++rep) {
        dut->in1=0; dut->in2=0; dut->in3=0; dut->eval(); if (dut->out!=ref(0,0,0)) return EXIT_FAILURE;
        dut->in1=1; dut->eval(); if (dut->out!=ref(1,0,0)) return EXIT_FAILURE;
        dut->in1=0; dut->eval(); if (dut->out!=ref(0,0,0)) return EXIT_FAILURE;
        dut->in2=1; dut->eval(); if (dut->out!=ref(0,1,0)) return EXIT_FAILURE;
        dut->in2=0; dut->eval(); if (dut->out!=ref(0,0,0)) return EXIT_FAILURE;
        dut->in3=1; dut->eval(); if (dut->out!=ref(0,0,1)) return EXIT_FAILURE;
        dut->in3=0; dut->eval(); if (dut->out!=ref(0,0,0)) return EXIT_FAILURE;
    }
    std::cout << "[TB] dut_048 passed: in3 ^ ~(in1 ^ in2)" << std::endl;
#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE"); if(!covPath||!*covPath) covPath="coverage.dat"; VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
