#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_131.h"

static inline void tick(Vdut_131 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

// Helper to assert basic outputs for different abstract states.
static void check_walk_left(Vdut_131 *dut, const char *ctx) {
    if (!(dut->walk_left == 1 && dut->walk_right == 0 && dut->aaah == 0 && dut->digging == 0)) {
        std::cerr << "[TB] dut_131 failed (" << ctx << "): expected WALK_L outputs\n";
        std::exit(EXIT_FAILURE);
    }
}

static void check_walk_right(Vdut_131 *dut, const char *ctx) {
    if (!(dut->walk_left == 0 && dut->walk_right == 1 && dut->aaah == 0 && dut->digging == 0)) {
        std::cerr << "[TB] dut_131 failed (" << ctx << "): expected WALK_R outputs\n";
        std::exit(EXIT_FAILURE);
    }
}

static void check_fall(Vdut_131 *dut, const char *ctx) {
    if (!(dut->aaah == 1 && dut->digging == 0)) {
        std::cerr << "[TB] dut_131 failed (" << ctx << "): expected FALL outputs\n";
        std::exit(EXIT_FAILURE);
    }
}

static void check_dig(Vdut_131 *dut, const char *ctx) {
    if (!(dut->digging == 1 && dut->aaah == 0)) {
        std::cerr << "[TB] dut_131 failed (" << ctx << "): expected DIG outputs\n";
        std::exit(EXIT_FAILURE);
    }
}

static void check_splatter(Vdut_131 *dut, const char *ctx) {
    if (!(dut->walk_left == 0 && dut->walk_right == 0 && dut->aaah == 0 && dut->digging == 0)) {
        std::cerr << "[TB] dut_131 failed (" << ctx << "): expected SPLATTER-like outputs\n";
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_131>(ctx.get());

    // Common initial reset to WALK_L.
    auto reset_to_walk_left = [&](const char *ctxstr) {
        dut->areset = 1;
        dut->bump_left = 0;
        dut->bump_right = 0;
        dut->ground = 1;
        dut->dig = 0;
        dut->eval();
        tick(dut.get(), ctx.get());
        dut->areset = 0;
        check_walk_left(dut.get(), ctxstr);
    };

    // Phase 1: exercise WALK_L, WALK_R, DIG_L, FALL_L (short) and bump_right.
    reset_to_walk_left("phase1.reset");

    // WALK_L -> WALK_R via bump_left
    dut->bump_left = 1;
    dut->ground = 1;
    dut->dig = 0;
    tick(dut.get(), ctx.get());
    dut->bump_left = 0;
    check_walk_right(dut.get(), "phase1.walk_r_after_bump_left");

    // WALK_R -> WALK_L via bump_right (exercises bump_right logic)
    dut->bump_right = 1;
    tick(dut.get(), ctx.get());
    dut->bump_right = 0;
    check_walk_left(dut.get(), "phase1.walk_l_after_bump_right");

    // WALK_L -> DIG_L via dig while on ground
    dut->dig = 1;
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_dig(dut.get(), "phase1.dig_l");

    // DIG_L -> FALL_L when ground disappears
    dut->ground = 0;
    dut->dig = 1;
    tick(dut.get(), ctx.get());
    check_fall(dut.get(), "phase1.fall_l_start");

    // Stay in FALL_L with ground=0 for a few cycles (covers else next=FALL_L)
    dut->dig = 0;
    for (int i = 0; i < 3; ++i) {
        tick(dut.get(), ctx.get());
        check_fall(dut.get(), "phase1.fall_l_loop");
    }

    // Now bring ground high with small fall count (count<=19), expect go back to WALK_L
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_walk_left(dut.get(), "phase1.fall_l_to_walk_l");

    // Phase 2: long FALL_L to SPLATTER (count>19 path)
    reset_to_walk_left("phase2.reset");
    // Enter FALL_L directly from WALK_L by dropping ground
    dut->ground = 0;
    dut->dig = 0;
    tick(dut.get(), ctx.get()); // first step: into FALL_L
    check_fall(dut.get(), "phase2.fall_l_start");

    // Stay falling long enough so internal count > 19
    for (int i = 0; i < 30; ++i) {
        tick(dut.get(), ctx.get());
        check_fall(dut.get(), "phase2.fall_l_long");
    }

    // Now raise ground to trigger SPLATTER decision with large count
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_splatter(dut.get(), "phase2.splatter_from_fall_l");

    // Phase 3: FALL_R short path back to WALK_R (count<=19 in FALL_R).
    reset_to_walk_left("phase3.reset");
    // WALK_L -> WALK_R
    dut->ground = 1;
    dut->bump_left = 1;
    tick(dut.get(), ctx.get());
    dut->bump_left = 0;
    check_walk_right(dut.get(), "phase3.walk_r_start");

    // Enter FALL_R with ground=0
    dut->ground = 0;
    tick(dut.get(), ctx.get());
    check_fall(dut.get(), "phase3.fall_r_start");

    // Short fall, then ground high -> WALK_R path
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_walk_right(dut.get(), "phase3.fall_r_to_walk_r");

    // Phase 4: long FALL_R to SPLATTER (count>19 path in FALL_R).
    reset_to_walk_left("phase4.reset");
    // WALK_L -> WALK_R
    dut->ground = 1;
    dut->bump_left = 1;
    tick(dut.get(), ctx.get());
    dut->bump_left = 0;
    check_walk_right(dut.get(), "phase4.walk_r_start");

    // Enter FALL_R with ground=0
    dut->ground = 0;
    tick(dut.get(), ctx.get());
    check_fall(dut.get(), "phase4.fall_r_start");

    // Long fall to ensure count>19
    for (int i = 0; i < 30; ++i) {
        tick(dut.get(), ctx.get());
        check_fall(dut.get(), "phase4.fall_r_long");
    }

    // Raise ground to force SPLATTER via FALL_R branch
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_splatter(dut.get(), "phase4.splatter_from_fall_r");

    // Phase 5: exercise DIG_R and drive count high bits to toggle via a long FALL_R.
    reset_to_walk_left("phase5.reset");
    // WALK_L -> WALK_R
    dut->ground = 1;
    dut->bump_left = 1;
    dut->dig = 0;
    tick(dut.get(), ctx.get());
    dut->bump_left = 0;
    check_walk_right(dut.get(), "phase5.walk_r_start");

    // WALK_R -> DIG_R (dig while on ground)
    dut->dig = 1;
    dut->ground = 1;
    tick(dut.get(), ctx.get());
    check_dig(dut.get(), "phase5.dig_r");

    // Stay in DIG_R for a few cycles (ground still high keeps us digging)
    for (int i = 0; i < 3; ++i) {
        tick(dut.get(), ctx.get());
        check_dig(dut.get(), "phase5.dig_r_hold");
    }

    // Drop ground to transition DIG_R -> FALL_R, then fall for many cycles to
    // ensure all bits of the internal count register toggle.
    dut->ground = 0;
    dut->dig = 1;
    tick(dut.get(), ctx.get());
    check_fall(dut.get(), "phase5.fall_r_from_dig_r_start");

    for (int i = 0; i < 150; ++i) {
        tick(dut.get(), ctx.get());
        check_fall(dut.get(), "phase5.fall_r_long_for_count_toggles");
    }

    // Finally, raise ground to exit the long fall path (will splatter).
    dut->ground = 1;
    dut->dig = 0;
    tick(dut.get(), ctx.get());
    check_splatter(dut.get(), "phase5.splatter_after_long_fall_r");

    std::cout << "[TB] dut_131 passed: extended Lemmings with full coverage paths\n";

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
