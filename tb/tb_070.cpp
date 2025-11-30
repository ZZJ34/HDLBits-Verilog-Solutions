#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_070.h"

static inline uint8_t oflow(uint8_t a,uint8_t b,uint8_t s){ uint8_t sa=(a>>7)&1u, sb=(b>>7)&1u, ss=(s>>7)&1u; return (uint8_t)((sa==sb) && (ss!=sa)); }

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_070>(ctx.get());
    auto check=[&](uint8_t a,uint8_t b){ dut->a=a; dut->b=b; dut->eval(); uint8_t s=uint8_t(a+b); if (dut->s!=s) return false; if (dut->overflow!=oflow(a,b,s)) return false; return true; };
    // Targeted cases
    if (!check(0x00,0x00)) return EXIT_FAILURE;
    if (!check(0x7F,0x01)) return EXIT_FAILURE; // +127 + 1 => overflow
    if (!check(0x80,0x80)) return EXIT_FAILURE; // -128 + -128 => overflow
    if (!check(0x7F,0xFF)) return EXIT_FAILURE; // +127 + -1 => no overflow
    if (!check(0x80,0x01)) return EXIT_FAILURE; // -128 + 1 => no overflow
    // Bit toggles for a and b
    dut->a=0; dut->b=0; dut->eval();
    for(int i=0;i<8;++i){ for(int rep=0;rep<2;++rep){ dut->a = uint8_t(1u<<i); dut->eval(); if (!check(dut->a, dut->b)) return EXIT_FAILURE; dut->a=0; dut->eval(); if (!check(dut->a, dut->b)) return EXIT_FAILURE; }}
    for(int i=0;i<8;++i){ for(int rep=0;rep<2;++rep){ dut->b = uint8_t(1u<<i); dut->eval(); if (!check(dut->a, dut->b)) return EXIT_FAILURE; dut->b=0; dut->eval(); if (!check(dut->a, dut->b)) return EXIT_FAILURE; }}
    std::cout<<"[TB] dut_070 passed: 8-bit add with overflow"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

