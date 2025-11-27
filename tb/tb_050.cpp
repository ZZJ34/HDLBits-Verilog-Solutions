#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_050.h"

int main(int argc, char** argv){
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);
    auto dut = std::make_unique<Vdut_050>(ctx.get());
    // Test a few combinations; p1y/p2y are 4-input NANDs (~&)
    auto check=[&](uint8_t p1, uint8_t p2){
        dut->p1a=(p1>>0)&1u; dut->p1b=(p1>>1)&1u; dut->p1c=(p1>>2)&1u; dut->p1d=(p1>>3)&1u;
        dut->p2a=(p2>>0)&1u; dut->p2b=(p2>>1)&1u; dut->p2c=(p2>>2)&1u; dut->p2d=(p2>>3)&1u;
        dut->eval();
        uint8_t p1y = uint8_t(~(((p1>>0)&1u)&((p1>>1)&1u)&((p1>>2)&1u)&((p1>>3)&1u)) & 1u);
        uint8_t p2y = uint8_t(~(((p2>>0)&1u)&((p2>>1)&1u)&((p2>>2)&1u)&((p2>>3)&1u)) & 1u);
        if (dut->p1y!=p1y || dut->p2y!=p2y) return EXIT_FAILURE;
        return EXIT_SUCCESS;
    };
    if (check(0x0,0x0) || check(0xF,0xF) || check(0x7,0xE) || check(0x1,0x8)) {
        std::cerr << "[TB] dut_050 failed" << std::endl; return EXIT_FAILURE;
    }
    // Bidirectional toggles: individually toggle each input of both NANDs 0->1->0 twice while others 0
    auto set_pins=[&](uint8_t p1, uint8_t p2){
        dut->p1a=(p1>>0)&1u; dut->p1b=(p1>>1)&1u; dut->p1c=(p1>>2)&1u; dut->p1d=(p1>>3)&1u;
        dut->p2a=(p2>>0)&1u; dut->p2b=(p2>>1)&1u; dut->p2c=(p2>>2)&1u; dut->p2d=(p2>>3)&1u;
        dut->eval();
        uint8_t p1y = uint8_t(~(((p1>>0)&1u)&((p1>>1)&1u)&((p1>>2)&1u)&((p1>>3)&1u)) & 1u);
        uint8_t p2y = uint8_t(~(((p2>>0)&1u)&((p2>>1)&1u)&((p2>>2)&1u)&((p2>>3)&1u)) & 1u);
        if (dut->p1y!=p1y || dut->p2y!=p2y) return false; return true;
    };
    // p1 toggles
    for (int bit=0; bit<4; ++bit) {
        for (int rep=0; rep<2; ++rep) {
            if (!set_pins(0x0,0x0)) return EXIT_FAILURE;
            if (!set_pins(uint8_t(1u<<bit),0x0)) return EXIT_FAILURE;
            if (!set_pins(0x0,0x0)) return EXIT_FAILURE;
        }
    }
    // p2 toggles
    for (int bit=0; bit<4; ++bit) {
        for (int rep=0; rep<2; ++rep) {
            if (!set_pins(0x0,0x0)) return EXIT_FAILURE;
            if (!set_pins(0x0,uint8_t(1u<<bit))) return EXIT_FAILURE;
            if (!set_pins(0x0,0x0)) return EXIT_FAILURE;
        }
    }
    std::cout << "[TB] dut_050 passed: two 4-input NANDs" << std::endl;
#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE"); if(!covPath||!*covPath) covPath="coverage.dat"; VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
