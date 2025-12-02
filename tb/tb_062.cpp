#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_062.h"

static inline void set_u100(Vdut_062* d, uint64_t alo,uint64_t ahi36,uint64_t blo,uint64_t bhi36,uint8_t sel){
    d->a[0]=uint32_t(alo&0xFFFFFFFFull); d->a[1]=uint32_t((alo>>32)&0xFFFFFFFFull); d->a[2]=uint32_t(ahi36&0xFFFFFFFFull); d->a[3]=uint32_t((ahi36>>32)&0xFull);
    d->b[0]=uint32_t(blo&0xFFFFFFFFull); d->b[1]=uint32_t((blo>>32)&0xFFFFFFFFull); d->b[2]=uint32_t(bhi36&0xFFFFFFFFull); d->b[3]=uint32_t((bhi36>>32)&0xFull);
    d->sel=sel&1u;
}

static inline uint8_t get_bit(const uint32_t w[4], int bit){ int wi=bit>>5, bi=bit&31; return (w[wi]>>bi)&1u; }

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_062>(ctx.get());
    // Basic
    set_u100(dut.get(),0,0,~0ull,0xFFFFFFFFFull,0); dut->eval(); for(int i=0;i<100;++i){ if(get_bit(dut->out,i)!=0) return EXIT_FAILURE; }
    set_u100(dut.get(),0,0,~0ull,0xFFFFFFFFFull,1); dut->eval(); for(int i=0;i<100;++i){ if(get_bit(dut->out,i)!=1) return EXIT_FAILURE; }
    // Bit toggles on a when sel=0; on b when sel=1
    uint64_t alo=0,ahi=0,blo=0,bhi=0; set_u100(dut.get(),alo,ahi,blo,bhi,0);
    for(int i=0;i<100;++i){ for(int rep=0;rep<2;++rep){ if(i<64) alo|=(1ull<<i); else ahi|=(1ull<<(i-64)); set_u100(dut.get(),alo,ahi,blo,bhi,0); dut->eval(); if(!get_bit(dut->out,i)) return EXIT_FAILURE; if(i<64) alo&=~(1ull<<i); else ahi&=~(1ull<<(i-64)); set_u100(dut.get(),alo,ahi,blo,bhi,0); dut->eval(); if(get_bit(dut->out,i)) return EXIT_FAILURE; }}
    set_u100(dut.get(),0,0,0,0,1);
    for(int i=0;i<100;++i){ for(int rep=0;rep<2;++rep){ if(i<64) blo|=(1ull<<i); else bhi|=(1ull<<(i-64)); set_u100(dut.get(),0,0,blo,bhi,1); dut->eval(); if(!get_bit(dut->out,i)) return EXIT_FAILURE; if(i<64) blo&=~(1ull<<i); else bhi&=~(1ull<<(i-64)); set_u100(dut.get(),0,0,blo,bhi,1); dut->eval(); if(get_bit(dut->out,i)) return EXIT_FAILURE; }}
    // Toggle sel
    set_u100(dut.get(),~0ull,0xFFFFFFFFFull,0,0,0); dut->eval(); if(!get_bit(dut->out,0)){} // just touch
    for(int rep=0;rep<2;++rep){ set_u100(dut.get(),~0ull,0xFFFFFFFFFull,0,0,1); dut->eval(); set_u100(dut.get(),~0ull,0xFFFFFFFFFull,0,0,0); dut->eval(); }
    std::cout<<"[TB] dut_062 passed: 100-bit 2:1 mux"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

