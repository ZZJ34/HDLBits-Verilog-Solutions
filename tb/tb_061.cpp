#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_061.h"

static inline uint8_t ref(uint8_t a,uint8_t b,uint8_t s){ return s?b:a; }

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_061>(ctx.get());
    for(int v=0;v<8;++v){ uint8_t a=v&1u,b=(v>>1)&1u,s=(v>>2)&1u; dut->a=a;dut->b=b;dut->sel=s;dut->eval(); if(dut->out!=ref(a,b,s)) return EXIT_FAILURE; }
    for(int rep=0;rep<2;++rep){ dut->a=0;dut->b=0;dut->sel=0;dut->eval(); if(dut->out!=0) return EXIT_FAILURE; dut->a=1;dut->eval(); if(dut->out!=1) return EXIT_FAILURE; dut->a=0;dut->eval(); if(dut->out!=0) return EXIT_FAILURE; dut->sel=1;dut->eval(); if(dut->out!=0) return EXIT_FAILURE; dut->b=1;dut->eval(); if(dut->out!=1) return EXIT_FAILURE; dut->b=0;dut->eval(); if(dut->out!=0) return EXIT_FAILURE; dut->sel=0; }
    std::cout<<"[TB] dut_061 passed: 2:1 mux"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

