#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_049.h"

int main(int argc, char** argv){
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);
    auto dut = std::make_unique<Vdut_049>(ctx.get());
    for (int i=0;i<4;++i){
        uint8_t a=(i>>0)&1u, b=(i>>1)&1u;
        dut->a=a; dut->b=b; dut->eval();
        uint8_t and_=a&b, or_=a|b, xor_=a^b;
        if (dut->out_and!=and_||dut->out_or!=or_||dut->out_xor!=xor_) return EXIT_FAILURE;
        if (dut->out_nand!=uint8_t(~and_&1u)) return EXIT_FAILURE;
        if (dut->out_nor!=uint8_t(~or_&1u)) return EXIT_FAILURE;
        if (dut->out_xnor!=uint8_t(~xor_&1u)) return EXIT_FAILURE;
        if (dut->out_anotb!=uint8_t(a & (~b & 1u))) return EXIT_FAILURE;
    }
    // Bidirectional toggles: walk inputs through a sequence that makes every output toggle 0->1->0 twice
    auto check=[&](uint8_t a, uint8_t b){
        dut->a=a; dut->b=b; dut->eval();
        uint8_t and_=a&b, or_=a|b, xor_=a^b;
        if (dut->out_and!=and_||dut->out_or!=or_||dut->out_xor!=xor_) return false;
        if (dut->out_nand!=uint8_t(~and_&1u)) return false;
        if (dut->out_nor!=uint8_t(~or_&1u)) return false;
        if (dut->out_xnor!=uint8_t(~xor_&1u)) return false;
        if (dut->out_anotb!=uint8_t(a & (~b & 1u))) return false;
        return true;
    };
    // Sequence: 00 -> 11 -> 00 -> 10 -> 00 -> 01 -> 00, repeated
    for (int rep=0; rep<2; ++rep) {
        if (!check(0,0)) return EXIT_FAILURE;
        if (!check(1,1)) return EXIT_FAILURE;
        if (!check(0,0)) return EXIT_FAILURE;
        if (!check(1,0)) return EXIT_FAILURE;
        if (!check(0,0)) return EXIT_FAILURE;
        if (!check(0,1)) return EXIT_FAILURE;
        if (!check(0,0)) return EXIT_FAILURE;
    }
    std::cout << "[TB] dut_049 passed: logic reductions suite" << std::endl;
#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE"); if(!covPath||!*covPath) covPath="coverage.dat"; VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
