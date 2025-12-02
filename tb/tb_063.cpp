#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_063.h"

static inline void set_in(Vdut_063* d, uint16_t v){ d->a=v; d->b=v; d->c=v; d->d=v; d->e=v; d->f=v; d->g=v; d->h=v; d->i=v; }

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_063>(ctx.get());
    // Basic: select each input and verify
    const uint16_t vals[9]={0x0000,0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,0x8888};
    dut->a=vals[0]; dut->b=vals[1]; dut->c=vals[2]; dut->d=vals[3]; dut->e=vals[4]; dut->f=vals[5]; dut->g=vals[6]; dut->h=vals[7]; dut->i=vals[8];
    for(int s=0;s<9;++s){ dut->sel=s; dut->eval(); if (dut->out != vals[s]) return EXIT_FAILURE; }
    // Default branch
    dut->sel=15; dut->eval(); if (dut->out != 0xFFFFu) return EXIT_FAILURE;
    // Bidirectional: for each source, toggle 0->FFFF->0 while selected
    for(int s=0;s<9;++s){ set_in(dut.get(),0x0000); dut->sel=s; dut->eval(); if(dut->out!=0x0000) return EXIT_FAILURE; switch(s){case 0: dut->a=0xFFFF; break; case 1: dut->b=0xFFFF; break; case 2: dut->c=0xFFFF; break; case 3: dut->d=0xFFFF; break; case 4: dut->e=0xFFFF; break; case 5: dut->f=0xFFFF; break; case 6: dut->g=0xFFFF; break; case 7: dut->h=0xFFFF; break; case 8: dut->i=0xFFFF; break;} dut->eval(); if(dut->out!=0xFFFF) return EXIT_FAILURE; set_in(dut.get(),0x0000); dut->eval(); if(dut->out!=0x0000) return EXIT_FAILURE; }
    // Toggle sel to/from default and a valid case
    set_in(dut.get(),0x0000); dut->a=0xFFFF; for(int rep=0;rep<2;++rep){ dut->sel=0; dut->eval(); if(dut->out!=0xFFFF) return EXIT_FAILURE; dut->sel=15; dut->eval(); if(dut->out!=0xFFFF) return EXIT_FAILURE; dut->sel=0; dut->eval(); if(dut->out!=0xFFFF) return EXIT_FAILURE; }
    std::cout<<"[TB] dut_063 passed: 9:1 mux with default"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

