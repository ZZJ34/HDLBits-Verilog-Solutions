#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_130.h"

static inline void tick(Vdut_130 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_130>(ctx.get());

    enum { WALK_L = 0, WALK_R = 1, FALL_L = 2, FALL_R = 3, DIG_L = 4, DIG_R = 5 };
    uint8_t state = WALK_L;

    dut->clk = 0;
    dut->areset = 1;
    dut->bump_left = dut->bump_right = 0;
    dut->ground = 1;
    dut->dig = 0;
    dut->eval();

    tick(dut.get(), ctx.get());
    state = WALK_L;
    dut->areset = 0;

    struct Stim130 { uint8_t bl, br, ground, dig; };
    const Stim130 seq[] = {
        {0u,0u,1u,0u}, // WALK_L
        {1u,0u,1u,0u}, // bump_left -> WALK_R
        {0u,0u,1u,1u}, // dig -> DIG_R
        {0u,0u,1u,1u}, // stay DIG_R while ground
        {0u,0u,0u,1u}, // ground=0 -> FALL_R
        {0u,0u,0u,0u}, // keep falling
        {0u,0u,1u,0u}, // ground=1 -> WALK_R
        {0u,1u,1u,0u}, // bump_right -> WALK_L
        {0u,0u,1u,1u}, // dig -> DIG_L
        {0u,0u,0u,1u}, // ground=0 -> FALL_L
        {0u,0u,1u,0u}, // ground=1 -> WALK_L
    };

    for (const auto &s : seq) {
        dut->bump_left = s.bl;
        dut->bump_right = s.br;
        dut->ground = s.ground;
        dut->dig = s.dig;
        tick(dut.get(), ctx.get());

        switch (state) {
            case WALK_L:
                if (!s.ground) state = FALL_L;
                else if (s.dig) state = DIG_L;
                else if (s.bl) state = WALK_R;
                else state = WALK_L;
                break;
            case WALK_R:
                if (!s.ground) state = FALL_R;
                else if (s.dig) state = DIG_R;
                else if (s.br) state = WALK_L;
                else state = WALK_R;
                break;
            case FALL_L:
                state = s.ground ? WALK_L : FALL_L;
                break;
            case FALL_R:
                state = s.ground ? WALK_R : FALL_R;
                break;
            case DIG_L:
                state = s.ground ? DIG_L : FALL_L;
                break;
            case DIG_R:
                state = s.ground ? DIG_R : FALL_R;
                break;
        }

        uint8_t wl = dut->walk_left;
        uint8_t wr = dut->walk_right;
        uint8_t aaah = dut->aaah;
        uint8_t dig_out = dut->digging;

        if ((state == WALK_L || state == DIG_L || state == FALL_L) &&
            wl != (state == WALK_L) && dig_out != (state == DIG_L)) {
            // We'll check outputs explicitly by state below.
        }

        // Walk outputs
        if (state == WALK_L && (wl != 1u || wr != 0u)) {
            std::cerr << "[TB] dut_130 failed: expected WALK_L walk signals" << std::endl;
            return EXIT_FAILURE;
        }
        if (state == WALK_R && (wl != 0u || wr != 1u)) {
            std::cerr << "[TB] dut_130 failed: expected WALK_R walk signals" << std::endl;
            return EXIT_FAILURE;
        }
        if ((state == FALL_L || state == FALL_R || state == DIG_L || state == DIG_R) &&
            (wl != 0u || wr != 0u)) {
            std::cerr << "[TB] dut_130 failed: expected no walking while falling/digging" << std::endl;
            return EXIT_FAILURE;
        }

        // Falling
        if ((state == FALL_L || state == FALL_R) && aaah != 1u) {
            std::cerr << "[TB] dut_130 failed: expected aaah=1 while falling" << std::endl;
            return EXIT_FAILURE;
        }
        if ((state == WALK_L || state == WALK_R || state == DIG_L || state == DIG_R) && aaah != 0u) {
            std::cerr << "[TB] dut_130 failed: expected aaah=0 while not falling" << std::endl;
            return EXIT_FAILURE;
        }

        // Digging
        if ((state == DIG_L || state == DIG_R) && dig_out != 1u) {
            std::cerr << "[TB] dut_130 failed: expected digging=1" << std::endl;
            return EXIT_FAILURE;
        }
        if ((state == WALK_L || state == WALK_R || state == FALL_L || state == FALL_R) && dig_out != 0u) {
            std::cerr << "[TB] dut_130 failed: expected digging=0" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_130 passed: extended Lemmings walk/fall/dig FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

