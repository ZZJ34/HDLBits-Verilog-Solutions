#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_128.h"

static inline void tick(Vdut_128 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_128>(ctx.get());

    enum { LEFT = 0, RIGHT = 1 };
    uint8_t state = LEFT;

    dut->clk = 0;
    dut->areset = 1;
    dut->bump_left = 0;
    dut->bump_right = 0;
    dut->eval();

    // After reset, should walk left.
    tick(dut.get(), ctx.get());
    state = LEFT;

    struct Stim128 { uint8_t bl, br; };
    const Stim128 seq[] = {
        {0u,0u}, // stay LEFT
        {1u,0u}, // bump_left -> RIGHT
        {0u,0u}, // stay RIGHT
        {0u,1u}, // bump_right -> LEFT
        {1u,0u}, // LEFT->RIGHT again
        {0u,0u}
    };

    dut->areset = 0;

    for (const auto &s : seq) {
        dut->bump_left = s.bl;
        dut->bump_right = s.br;
        tick(dut.get(), ctx.get());

        switch (state) {
            case LEFT:  state = s.bl ? RIGHT : LEFT; break;
            case RIGHT: state = s.br ? LEFT : RIGHT; break;
        }

        uint8_t wl = dut->walk_left;
        uint8_t wr = dut->walk_right;
        if (state == LEFT && (wl != 1u || wr != 0u)) {
            std::cerr << "[TB] dut_128 failed: expected LEFT, got wl=" << int(wl)
                      << " wr=" << int(wr) << std::endl;
            return EXIT_FAILURE;
        }
        if (state == RIGHT && (wl != 0u || wr != 1u)) {
            std::cerr << "[TB] dut_128 failed: expected RIGHT, got wl=" << int(wl)
                      << " wr=" << int(wr) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_128 passed: basic Lemmings left/right FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

