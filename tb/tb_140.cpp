#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_140.h"

static inline void tick(Vdut_140 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_140>(ctx.get());

    enum { A=0, B=1, C=2 };
    uint8_t state = A;

    dut->clk = 0;
    dut->areset = 1;
    dut->x = 0;
    dut->eval();

    tick(dut.get(), ctx.get());
    state = A;
    dut->areset = 0;

    auto step = [&](uint8_t x) {
        uint8_t next = state;
        switch (state) {
            case A: next = x ? B : A; break;
            case B: next = x ? C : B; break;
            case C: next = x ? C : B; break;
        }
        dut->x = x;
        tick(dut.get(), ctx.get());
        state = next;
        uint8_t z_exp = (state == B) ? 1u : 0u;
        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_140 failed: x=" << int(x)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    const uint8_t seq[] = {0,1,1,0,1,0,0,1,1,1};
    for (uint8_t b : seq) step(b);

    std::cout << "[TB] dut_140 passed: 3-state FSM with z on state B" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

