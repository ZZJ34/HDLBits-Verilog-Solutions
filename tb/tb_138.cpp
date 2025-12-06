#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_138.h"

static inline void tick(Vdut_138 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_138>(ctx.get());

    dut->clk = 0;
    dut->in = 0;

    // Mirror DUT state encoding for a simple reference model.
    enum {
        ST_NONE = 0,
        ST_ONE  = 1,
        ST_TWO  = 2,
        ST_THREE= 3,
        ST_FOUR = 4,
        ST_FIVE = 5,
        ST_SIX  = 6,
        ST_DISC = 7,
        ST_FLAG = 8,
        ST_ERR  = 9
    };

    uint8_t state = ST_NONE;

    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->in = 0;
        dut->eval();
        tick(dut.get(), ctx.get());
        dut->reset = 0;
        state = ST_NONE;
    };

    auto check_outputs = [&](const char *ctx_str) {
        uint8_t disc_exp = (state == ST_DISC) ? 1u : 0u;
        uint8_t flag_exp = (state == ST_FLAG) ? 1u : 0u;
        uint8_t err_exp  = (state == ST_ERR)  ? 1u : 0u;

        if (dut->disc != disc_exp || dut->flag != flag_exp || dut->err != err_exp) {
            std::cerr << "[TB] dut_138 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " expected disc=" << int(disc_exp)
                      << " flag=" << int(flag_exp)
                      << " err=" << int(err_exp)
                      << " got disc=" << int(dut->disc)
                      << " flag=" << int(dut->flag)
                      << " err=" << int(dut->err) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    auto step = [&](uint8_t bit, const char *ctx_str) {
        bit &= 1u;
        dut->in = bit;

        // Compute next state based on current model state and input bit.
        uint8_t next = state;
        switch (state) {
            case ST_NONE:  next = bit ? ST_ONE   : ST_NONE; break;
            case ST_ONE:   next = bit ? ST_TWO   : ST_NONE; break;
            case ST_TWO:   next = bit ? ST_THREE : ST_NONE; break;
            case ST_THREE: next = bit ? ST_FOUR  : ST_NONE; break;
            case ST_FOUR:  next = bit ? ST_FIVE  : ST_NONE; break;
            case ST_FIVE:  next = bit ? ST_SIX   : ST_DISC; break;
            case ST_SIX:   next = bit ? ST_ERR   : ST_FLAG; break;
            case ST_DISC:  next = bit ? ST_ONE   : ST_NONE; break;
            case ST_FLAG:  next = bit ? ST_ONE   : ST_NONE; break;
            case ST_ERR:   next = bit ? ST_ERR   : ST_NONE; break;
            default:       next = ST_NONE; break;
        }

        tick(dut.get(), ctx.get());
        state = next;
        check_outputs(ctx_str);
    };

    // Phase 1: reset and sanity check (stay in NONE on zeros, no outputs asserted).
    apply_reset();
    step(0, "phase1.none_0");
    step(0, "phase1.none_0_again");

    // Phase 2: exercise DISC and both outgoing transitions from DISC.
    // Pattern: 0 1 1 1 1 1 0 -> DISC, then 0 (DISC->NONE), then another DISC with 1 (DISC->ONE).
    apply_reset();
    step(0, "disc.seq.none0");
    step(1, "disc.seq.one");
    step(1, "disc.seq.two");
    step(1, "disc.seq.three");
    step(1, "disc.seq.four");
    step(1, "disc.seq.five");
    step(0, "disc.seq.enter_disc");  // reach DISC, disc should be 1
    step(0, "disc.seq.disc_to_none"); // DISC with in=0

    // Second time: go to DISC again and take in=1 branch.
    apply_reset();
    step(0, "disc2.seq.none0");
    step(1, "disc2.seq.one");
    step(1, "disc2.seq.two");
    step(1, "disc2.seq.three");
    step(1, "disc2.seq.four");
    step(1, "disc2.seq.five");
    step(0, "disc2.seq.enter_disc");  // DISC
    step(1, "disc2.seq.disc_to_one"); // DISC with in=1

    // Phase 3: exercise FLAG and both outgoing transitions from FLAG.
    // Pattern for FLAG: 0 1 1 1 1 1 1 0 (0 followed by six 1s then 0).
    apply_reset();
    step(0, "flag.seq.none0");
    step(1, "flag.seq.one");
    step(1, "flag.seq.two");
    step(1, "flag.seq.three");
    step(1, "flag.seq.four");
    step(1, "flag.seq.five");
    step(1, "flag.seq.six");
    step(0, "flag.seq.enter_flag"); // FLAG
    step(1, "flag.seq.flag_to_one"); // FLAG with in=1

    // Second time to take FLAG with in=0 branch.
    apply_reset();
    step(0, "flag2.seq.none0");
    step(1, "flag2.seq.one");
    step(1, "flag2.seq.two");
    step(1, "flag2.seq.three");
    step(1, "flag2.seq.four");
    step(1, "flag2.seq.five");
    step(1, "flag2.seq.six");
    step(0, "flag2.seq.enter_flag"); // FLAG
    step(0, "flag2.seq.flag_to_none"); // FLAG with in=0

    // Phase 4: exercise ERR and both outgoing transitions from ERR.
    // Pattern for ERR: seven 1s (from NONE).
    apply_reset();
    step(1, "err.seq.one");
    step(1, "err.seq.two");
    step(1, "err.seq.three");
    step(1, "err.seq.four");
    step(1, "err.seq.five");
    step(1, "err.seq.six");
    step(1, "err.seq.enter_err");  // reach ERR
    step(1, "err.seq.err_stay_err"); // ERR with in=1
    step(0, "err.seq.err_to_none");   // ERR with in=0

    std::cout << "[TB] dut_138 passed: HDLC flag/disc/err pattern FSM with full coverage" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
