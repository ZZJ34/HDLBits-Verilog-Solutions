#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_066.h"

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_066>(ctx.get());
    for(int v=0;v<4;++v){ uint8_t a=v&1u,b=(v>>1)&1u; dut->a=a;dut->b=b;dut->eval(); if(dut->sum!=uint8_t(a^b) || dut->cout!=uint8_t(a&b)) return EXIT_FAILURE; }
    for(int rep=0;rep<2;++rep){ dut->a=0;dut->b=0;dut->eval(); if(dut->sum||dut->cout) return EXIT_FAILURE; dut->a=1;dut->eval(); if(dut->sum!=1||dut->cout!=0) return EXIT_FAILURE; dut->a=0;dut->eval(); if(dut->sum||dut->cout) return EXIT_FAILURE; dut->b=1;dut->eval(); if(dut->sum!=1||dut->cout!=0) return EXIT_FAILURE; dut->b=0;dut->eval(); if(dut->sum||dut->cout) return EXIT_FAILURE; }
    std::cout<<"[TB] dut_066 passed: half adder"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

