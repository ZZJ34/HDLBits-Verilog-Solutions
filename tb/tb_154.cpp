#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_154.h"

static inline void tick(Vdut_154 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_154>(ctx.get());

    enum { IDLE=0, S1=1, S11=2, S110=3, S1101=4 };
    uint8_t state = IDLE;

    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->data = 0;
        tick(dut.get(), ctx.get());
        dut->reset = 0;
        state = IDLE;
    };

    auto step = [&](uint8_t data, const char *ctx_str) {
        dut->data = data & 1u;

        uint8_t next = state;
        switch (state) {
            case IDLE:  next = data ? S1    : IDLE; break;
            case S1:    next = data ? S11   : IDLE; break;
            case S11:   next = data ? S11   : S110; break;
            case S110:  next = data ? S1101 : IDLE; break;
            case S1101: next = S1101; break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        uint8_t z_exp = (state == S1101) ? 1u : 0u;
        if (dut->start_shifting != z_exp) {
            std::cerr << "[TB] dut_154 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " data=" << int(data)
                      << " expected start=" << int(z_exp)
                      << " got " << int(dut->start_shifting) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Scenario 1: feed exact "1101" sequence.
    apply_reset();
    step(1, "1");
    step(1, "11");
    step(0, "110");
    step(1, "1101");
    // Hold in S1101
    for (int i = 0; i < 3; ++i) step(1, "hold_1101");

    // Scenario 2: random-ish pattern with multiple partial matches.
    apply_reset();
    const uint8_t seq[] = {0,1,1,0,1,1,1,0,1,0,0,1};
    for (size_t i = 0; i < sizeof(seq); ++i) {
        step(seq[i], "mixed");
    }

    std::cout << "[TB] dut_154 passed: 1101 sequence detector with latch" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

