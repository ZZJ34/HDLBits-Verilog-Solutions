#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_065.h"

static inline void zero_1024(Vdut_065* d){ for(int i=0;i<32;++i) d->in[i]=0; }
static inline void set_nibble(Vdut_065* d, int idx, uint8_t val){ int bit=4*idx; int wi=bit>>5, bi=bit&31; uint32_t mask=0xFu<<bi; d->in[wi]=(d->in[wi]&~mask)|((uint32_t(val&0xFu))<<bi); if(bi>28){ int rem=32-bi; uint32_t mask2=0xFu>>rem; d->in[wi+1]=(d->in[wi+1]&~mask2)|((val&0xFu)>>rem);} }

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_065>(ctx.get());
    // For each nibble index select and toggle 0->F->0 twice
    for(int idx=0; idx<256; ++idx){
        for(int rep=0; rep<2; ++rep){
            zero_1024(dut.get()); dut->sel=idx; dut->eval(); if (dut->out!=0) return EXIT_FAILURE;
            set_nibble(dut.get(), idx, 0xFu); dut->eval(); if (dut->out!=0xFu) return EXIT_FAILURE;
            set_nibble(dut.get(), idx, 0x0u); dut->eval(); if (dut->out!=0x0u) return EXIT_FAILURE;
        }
    }
    std::cout<<"[TB] dut_065 passed: 4-bit slice select"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

