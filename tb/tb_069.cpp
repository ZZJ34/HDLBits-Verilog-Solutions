#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_069.h"

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_069>(ctx.get());
    for(int x=0;x<16;++x){ for(int y=0;y<16;++y){ dut->x=x; dut->y=y; dut->eval(); if (dut->sum != uint8_t((x+y)&0x1Fu)) return EXIT_FAILURE; }}
    for(int rep=0;rep<2;++rep){ dut->x=0; dut->y=0; dut->eval(); dut->x=0xF; dut->eval(); dut->x=0; dut->eval(); dut->y=0xF; dut->eval(); dut->y=0; dut->eval(); }
    std::cout<<"[TB] dut_069 passed: 4-bit adder"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

