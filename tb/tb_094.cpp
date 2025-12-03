#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_094.h"

struct Stim094 {
    uint8_t j;
    uint8_t k;
};

static inline void tick(Vdut_094 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_094>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->j = 0;
    dut->k = 0;
    dut->eval();

    const Stim094 pattern[] = {
        {1u, 0u}, // set
        {0u, 0u}, // hold
        {0u, 1u}, // reset
        {1u, 1u}, // toggle 0->1
        {1u, 1u}, // toggle 1->0
    };

    for (const auto &s : pattern) {
        dut->j = s.j;
        dut->k = s.k;
        tick(dut.get(), ctx.get());

        const uint8_t code = static_cast<uint8_t>((s.j << 1) | s.k);
        switch (code) {
            case 0u: /* hold */ break;
            case 1u: q_model = 0u; break;
            case 2u: q_model = 1u; break;
            case 3u: q_model = static_cast<uint8_t>((~q_model) & 1u); break;
        }

        if (dut->Q != q_model) {
            std::cerr << "[TB] dut_094 failed: j=" << int(s.j)
                      << " k=" << int(s.k)
                      << " expected Q=" << int(q_model)
                      << " got " << int(dut->Q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_094 passed: JK flip-flop behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

