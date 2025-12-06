#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_121.h"

struct Stim121 {
    uint8_t areset;
    uint8_t j;
    uint8_t k;
};

static inline void tick(Vdut_121 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_121>(ctx.get());

    enum { OFF = 0, ON = 1 };
    uint8_t state = OFF;

    dut->clk = 0;
    dut->areset = 1;
    dut->j = 0;
    dut->k = 0;
    dut->eval();

    const Stim121 pattern[] = {
        {1u, 0u, 0u}, // async reset to OFF
        {0u, 1u, 0u}, // j=1, OFF->ON
        {0u, 0u, 0u}, // hold ON
        {0u, 0u, 1u}, // k=1, ON->OFF
        {0u, 1u, 1u}, // j=1, OFF->ON (k ignored)
        {1u, 0u, 0u}, // reset again
        {0u, 0u, 1u}, // k=1 while OFF: stay OFF
        {0u, 1u, 0u}, // OFF->ON
    };

    for (const auto &s : pattern) {
        dut->areset = s.areset;
        dut->j = s.j;
        dut->k = s.k;

        tick(dut.get(), ctx.get());

        if (s.areset) {
            state = OFF;
        } else {
            switch (state) {
                case OFF: state = (s.j ? ON : OFF); break;
                case ON:  state = (s.k ? OFF : ON); break;
            }
        }

        uint8_t out_exp = (state == ON) ? 1u : 0u;
        if (dut->out != out_exp) {
            std::cerr << "[TB] dut_121 failed: areset=" << int(s.areset)
                      << " j=" << int(s.j) << " k=" << int(s.k)
                      << " expected out=" << int(out_exp)
                      << " got " << int(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_121 passed: JK latch FSM with async reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

