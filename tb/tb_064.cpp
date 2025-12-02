#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_064.h"

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_064>(ctx.get());
    // For each sel bit index, toggle the corresponding input bit 0->1->0 twice
    for(int s=0;s<256;++s){
        for(int rep=0; rep<2; ++rep){
            // zero all words
            for(int w=0; w<8; ++w) dut->in[w]=0; dut->sel=s; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
            int wi=s>>5, bi=s&31; dut->in[wi] |= (1u<<bi); dut->eval(); if (dut->out!=1) return EXIT_FAILURE;
            dut->in[wi] &= ~(1u<<bi); dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
        }
    }
    // Ensure each sel bit itself toggles 0->1->0 twice (to cover MSB returning 1->0)
    for(int rep=0; rep<2; ++rep){
        for(int i=0;i<8;++i){
            for(int w=0; w<8; ++w) dut->in[w]=0; dut->sel=0; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
            dut->sel = uint8_t(1u<<i); dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
            dut->sel = 0; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
        }
    }
    std::cout<<"[TB] dut_064 passed: bit-select via sel"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}
