#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_068.h"

static inline void ref(uint8_t a,uint8_t b,uint8_t cin,uint8_t &cout_vec,uint8_t &sum_vec){
    uint8_t c=cin&1u; sum_vec=0; cout_vec=0; for(int i=0;i<3;++i){ uint8_t s=((a>>i)&1u)^((b>>i)&1u)^c; uint8_t co=(((a>>i)&1u)&((b>>i)&1u))|(((a>>i)&1u)&c)|(((b>>i)&1u)&c); if(s) sum_vec|=(1u<<i); if(co) cout_vec|=(1u<<i); c=co; }
}

int main(int argc,char**argv){
    Verilated::commandArgs(argc,argv);
    auto ctx=std::make_unique<VerilatedContext>(); ctx->traceEverOn(false);
    auto dut=std::make_unique<Vdut_068>(ctx.get());
    for(int a=0;a<8;++a){ for(int b=0;b<8;++b){ for(int c=0;c<2;++c){ dut->a=a; dut->b=b; dut->cin=c; dut->eval(); uint8_t co,su; ref(a,b,c,co,su); if(dut->sum!=su||dut->cout!=co) return EXIT_FAILURE; }}}
    // Toggle each input bit twice
    for(int i=0;i<3;++i){ for(int rep=0;rep<2;++rep){ dut->a=0; dut->b=0; dut->cin=0; dut->eval(); dut->a=(1u<<i); dut->eval(); dut->a=0; dut->eval(); dut->b=(1u<<i); dut->eval(); dut->b=0; dut->eval(); }}
    for(int rep=0;rep<2;++rep){ dut->cin=0; dut->eval(); dut->cin=1; dut->eval(); dut->cin=0; dut->eval(); }
    std::cout<<"[TB] dut_068 passed: 3-bit ripple"<<std::endl;
#if VM_COVERAGE
    const char* p=getenv("VERILATOR_COV_FILE"); if(!p||!*p) p="coverage.dat"; VerilatedCov::write(p);
#endif
    return EXIT_SUCCESS;
}

