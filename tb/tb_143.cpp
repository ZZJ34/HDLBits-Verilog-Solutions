#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_143.h"

static inline void tick(Vdut_143 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_143>(ctx.get());

    enum { A = 0, B = 1, C = 2, D = 3, E = 4 };
    uint8_t state = A;

    auto step = [&](uint8_t reset, uint8_t x, const char *ctx_str) {
        dut->reset = reset & 1u;
        dut->x = x & 1u;

        if (reset) {
            state = A;
        } else {
            uint8_t next = state;
            switch (state) {
                case A: next = x ? B : A; break;
                case B: next = x ? E : B; break;
                case C: next = x ? B : C; break;
                case D: next = x ? C : B; break;
                case E: next = x ? E : D; break;
            }
            state = next;
        }

        tick(dut.get(), ctx.get());

        uint8_t z_exp = (state == D || state == E) ? 1u : 0u;
        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_143 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " x=" << int(x)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    };

    // Initial reset.
    if (step(1, 0, "reset") != EXIT_SUCCESS) return EXIT_FAILURE;

    // Drive a sequence that visits all states and toggles z.
    struct Vec { uint8_t x; const char *ctx; };
    const Vec seq1[] = {
        {0, "A->A"},
        {1, "A->B"},
        {0, "B->B"},
        {1, "B->E"},
        {0, "E->D"},
        {0, "D->B"},
        {1, "B->E_again"},
        {1, "E->E_self"},
        {0, "E->D_again"},
        {1, "D->C"},
        {0, "C->C"},
        {1, "C->B_from_C"}
    };
    for (const auto &v : seq1) {
        if (step(0, v.x, v.ctx) != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    // Another reset and a shorter path to D/E.
    if (step(1, 0, "reset2") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(0, 1, "A->B_short") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(0, 1, "B->E_short") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(0, 0, "E->D_short") != EXIT_SUCCESS) return EXIT_FAILURE;

    std::cout << "[TB] dut_143 passed: 5-state FSM with z on D/E" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

