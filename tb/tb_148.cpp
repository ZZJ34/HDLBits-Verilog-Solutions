#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_148.h"

static inline void tick(Vdut_148 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_148>(ctx.get());

    enum { A=0, B=1, C=2, D=3, E=4, F=5 };
    uint8_t state = A;

    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->w = 0;
        dut->eval();
        tick(dut.get(), ctx.get());
        dut->reset = 0;
        state = A;
    };

    auto step = [&](uint8_t w, const char *ctx_str) {
        dut->w = w & 1u;

        uint8_t next = state;
        switch ((state << 1) | (w & 1u)) {
            case (A<<1)|0: next = A; break;
            case (A<<1)|1: next = B; break;
            case (B<<1)|0: next = D; break;
            case (B<<1)|1: next = C; break;
            case (C<<1)|0: next = D; break;
            case (C<<1)|1: next = E; break;
            case (D<<1)|0: next = A; break;
            case (D<<1)|1: next = F; break;
            case (E<<1)|0: next = D; break;
            case (E<<1)|1: next = E; break;
            case (F<<1)|0: next = D; break;
            case (F<<1)|1: next = C; break;
            default:       next = A; break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        uint8_t z_exp = (state == E || state == F) ? 1u : 0u;
        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_148 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " w=" << int(w)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    };

    apply_reset();

    // Sequence to cover many transitions and z toggles.
    const uint8_t seq1_vals[] = {
        0, // A->A
        1, // A->B
        1, // B->C
        1, // C->E
        1, // E->E
        0, // E->D
        1, // D->F
        0, // F->D
        0, // D->A
        1, // A->B
        0  // B->D
    };
    for (size_t i = 0; i < sizeof(seq1_vals); ++i) {
        if (step(seq1_vals[i], "seq1") != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    // Another reset and sequences to hit remaining transitions, including
    // {c,0} and default via an illegal state.
    apply_reset();
    // Visit C with w=0 to use {c,0}->D.
    if (step(1, "A->B_for_c0") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(1, "B->C_for_c0") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(0, "C->D_via_c0") != EXIT_SUCCESS) return EXIT_FAILURE;

    std::cout << "[TB] dut_148 passed: 6-state FSM with alternate edges" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
