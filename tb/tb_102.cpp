#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_102.h"

struct Stim102 {
    uint8_t reset;
    uint8_t slowena;
};

static inline void tick(Vdut_102 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_102>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->reset = 1;
    dut->slowena = 0;
    dut->eval();

    const Stim102 pattern[] = {
        {1u, 0u}, // reset
        // count with slowena=1, wrap at 9
        {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u},
        {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u},
        // hold with slowena=0
        {0u, 0u}, {0u, 0u},
        // count again and trigger another wrap
        {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u},
        {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u}, {0u, 1u},
        // final reset
        {1u, 0u}, {0u, 1u}
    };

    for (const auto &s : pattern) {
        dut->reset = s.reset;
        dut->slowena = s.slowena;
        tick(dut.get(), ctx.get());

        if (s.reset || (s.slowena && q_model == 9u)) {
            q_model = 0u;
        } else if (s.slowena) {
            q_model = static_cast<uint8_t>((q_model + 1u) & 0xFu);
        }

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_102 failed: reset=" << int(s.reset)
                      << " slowena=" << int(s.slowena)
                      << " expected q=" << int(q_model)
                      << " got " << int(dut->q) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_102 passed: gated BCD counter" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

