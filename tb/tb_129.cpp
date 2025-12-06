#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_129.h"

static inline void tick(Vdut_129 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_129>(ctx.get());

    enum { WALK_L = 0, WALK_R = 1, FALL_L = 2, FALL_R = 3 };
    uint8_t state = WALK_L;

    dut->clk = 0;
    dut->areset = 1;
    dut->bump_left = dut->bump_right = 0;
    dut->ground = 1;
    dut->eval();

    tick(dut.get(), ctx.get());
    state = WALK_L;
    dut->areset = 0;

    struct Stim129 { uint8_t bl, br, ground; };
    const Stim129 seq[] = {
        {0u,0u,1u}, // start WALK_L
        {1u,0u,1u}, // bump_left -> WALK_R
        {0u,0u,0u}, // ground=0 while WALK_R -> FALL_R
        {0u,0u,0u}, // stay FALL_R
        {0u,0u,1u}, // ground=1 -> WALK_R (via FALL_R branch)
        {0u,1u,1u}, // bump_right -> WALK_L
        {0u,0u,0u}, // ground=0 -> FALL_L
        {0u,0u,0u}, // stay FALL_L
        {0u,0u,1u}, // ground=1 -> WALK_L
    };

    for (const auto &s : seq) {
        dut->bump_left = s.bl;
        dut->bump_right = s.br;
        dut->ground = s.ground;
        tick(dut.get(), ctx.get());

        switch (state) {
            case WALK_L:
                state = (s.ground ? (s.bl ? WALK_R : WALK_L) : FALL_L);
                break;
            case WALK_R:
                state = (s.ground ? (s.br ? WALK_L : WALK_R) : FALL_R);
                break;
            case FALL_L:
                state = (s.ground ? WALK_L : FALL_L);
                break;
            case FALL_R:
                state = (s.ground ? WALK_R : FALL_R);
                break;
        }

        uint8_t wl = dut->walk_left;
        uint8_t wr = dut->walk_right;
        uint8_t aaah = dut->aaah;

        if (state == WALK_L && (wl != 1u || wr != 0u || aaah != 0u)) {
            std::cerr << "[TB] dut_129 failed: expected WALK_L" << std::endl;
            return EXIT_FAILURE;
        }
        if (state == WALK_R && (wl != 0u || wr != 1u || aaah != 0u)) {
            std::cerr << "[TB] dut_129 failed: expected WALK_R" << std::endl;
            return EXIT_FAILURE;
        }
        if (state == FALL_L && (aaah != 1u)) {
            std::cerr << "[TB] dut_129 failed: expected FALL_L aaah=1" << std::endl;
            return EXIT_FAILURE;
        }
        if (state == FALL_R && (aaah != 1u)) {
            std::cerr << "[TB] dut_129 failed: expected FALL_R aaah=1" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_129 passed: Lemmings walk/fall FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
