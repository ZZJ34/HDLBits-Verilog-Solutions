#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_067.h"

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_067>(ctx.get());
    for(int v=0;v<8;++v){ uint8_t a=v&1u,b=(v>>1)&1u,c=(v>>2)&1u; dut->a=a;dut->b=b;dut->cin=c;dut->eval(); uint8_t sum=(a^b)^c; uint8_t cout=(a&b)|(a&c)|(b&c); if(dut->sum!=sum||dut->cout!=cout) return EXIT_FAILURE; }
    for(int rep=0;rep<2;++rep){ dut->a=0;dut->b=0;dut->cin=0;dut->eval(); dut->a=1;dut->eval(); dut->a=0;dut->eval(); dut->b=1;dut->eval(); dut->b=0;dut->eval(); dut->cin=1;dut->eval(); dut->cin=0;dut->eval(); }
    std::cout<<"[TB] dut_067 passed: full adder"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

