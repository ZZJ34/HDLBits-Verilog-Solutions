#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_147.h"

static inline void tick(Vdut_147 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_147>(ctx.get());

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

    apply_reset();

    auto step = [&](uint8_t w, const char *ctx_str) {
        dut->w = w & 1u;

        // Compute next_state based on current state and w
        uint8_t next = state;
        switch ((state << 1) | (w & 1u)) {
            case (A<<1)|0: next = B; break;
            case (A<<1)|1: next = A; break;
            case (B<<1)|0: next = C; break;
            case (B<<1)|1: next = D; break;
            case (C<<1)|0: next = E; break;
            case (C<<1)|1: next = D; break;
            case (D<<1)|0: next = F; break;
            case (D<<1)|1: next = A; break;
            case (E<<1)|0: next = E; break;
            case (E<<1)|1: next = D; break;
            case (F<<1)|0: next = C; break;
            case (F<<1)|1: next = D; break;
            default:       next = A; break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        uint8_t z_exp = (state == E || state == F) ? 1u : 0u;
        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_147 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " w=" << int(w)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    };

    // Drive a sequence that walks through all states and toggles z.
    const uint8_t seq1_vals[] = {
        0, // A->B
        0, // B->C
        0, // C->E
        0, // E->E
        1, // E->D
        0, // D->F
        0, // F->C
        1, // C->D
        1, // D->A
        1  // A->A
    };
    for (size_t i = 0; i < sizeof(seq1_vals); ++i) {
        if (step(seq1_vals[i], "seq1") != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    // Second reset and sequences to hit remaining transitions, including
    // {b,1} and {f,1}, and default path via an illegal state pattern.
    apply_reset();
    // Drive into B then take w=1 to use {b,1}->D.
    if (step(0, "A->B_again") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(1, "B->D_via_b1") != EXIT_SUCCESS) return EXIT_FAILURE;
    // From D with w=1 goes back to A (already used earlier), but also re-use.
    if (step(1, "D->A_again") != EXIT_SUCCESS) return EXIT_FAILURE;
    // Drive into F then take w=1 to use {f,1}->D.
    if (step(0, "A->B_for_f") != EXIT_SUCCESS) return EXIT_FAILURE; // A->B
    if (step(0, "B->C_for_f") != EXIT_SUCCESS) return EXIT_FAILURE; // B->C
    if (step(0, "C->E_for_f") != EXIT_SUCCESS) return EXIT_FAILURE; // C->E
    if (step(1, "E->D_for_f") != EXIT_SUCCESS) return EXIT_FAILURE; // E->D
    if (step(0, "D->F_for_f") != EXIT_SUCCESS) return EXIT_FAILURE; // D->F
    if (step(1, "F->D_via_f1") != EXIT_SUCCESS) return EXIT_FAILURE; // F,w=1

    std::cout << "[TB] dut_147 passed: 6-state FSM with z on E/F" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
