#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_156.h"

static inline void tick(Vdut_156 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_156>(ctx.get());

    enum {
        IDLE=0, S1=1, S11=2, S110=3, S1101=4,
        SHIFT1=5, SHIFT2=6, SHIFT3=7, COUNT=8, DONE=9
    };
    uint8_t state = IDLE;

    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->data = 0;
        dut->done_counting = 0;
        dut->ack = 0;
        tick(dut.get(), ctx.get());
        dut->reset = 0;
        state = IDLE;
    };

    auto step = [&](uint8_t data, uint8_t done_cnt, uint8_t ack, const char *ctx_str) {
        dut->data = data & 1u;
        dut->done_counting = done_cnt & 1u;
        dut->ack = ack & 1u;

        uint8_t next = state;
        switch (state) {
            case IDLE:  next = data ? S1    : IDLE; break;
            case S1:    next = data ? S11   : IDLE; break;
            case S11:   next = data ? S11   : S110; break;
            case S110:  next = data ? S1101 : IDLE; break;
            case S1101: next = SHIFT1; break;
            case SHIFT1:next = SHIFT2; break;
            case SHIFT2:next = SHIFT3; break;
            case SHIFT3:next = COUNT; break;
            case COUNT: next = done_cnt ? DONE : COUNT; break;
            case DONE:  next = ack ? IDLE : DONE; break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        uint8_t shift_exp = (state == S1101 || state == SHIFT1 || state == SHIFT2 || state == SHIFT3) ? 1u : 0u;
        uint8_t counting_exp = (state == COUNT) ? 1u : 0u;
        uint8_t done_exp = (state == DONE) ? 1u : 0u;

        if (dut->shift_ena != shift_exp ||
            dut->counting != counting_exp ||
            dut->done != done_exp) {
            std::cerr << "[TB] dut_156 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " data=" << int(data)
                      << " done_cnt=" << int(done_cnt)
                      << " ack=" << int(ack)
                      << " got shift_ena=" << int(dut->shift_ena)
                      << " counting=" << int(dut->counting)
                      << " done=" << int(dut->done) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Scenario 1: drive "1101" then shift and count to DONE, then ack.
    apply_reset();
    step(1, 0, 0, "seq_1");
    step(1, 0, 0, "seq_11");
    step(0, 0, 0, "seq_110");
    step(1, 0, 0, "seq_1101");
    // SHIFT1,2,3, then COUNT
    step(0, 0, 0, "SHIFT1");
    step(0, 0, 0, "SHIFT2");
    step(0, 0, 0, "SHIFT3");
    step(0, 0, 0, "COUNT_start");
    // stay COUNT a few cycles
    for (int i = 0; i < 3; ++i) step(0, 0, 0, "COUNT_hold");
    // finish counting
    step(0, 1, 0, "COUNT_done_to_DONE");
    // stay DONE until ack
    step(0, 0, 0, "DONE_hold");
    step(0, 0, 1, "DONE_ack_to_IDLE");

    // Scenario 2: incomplete pattern that never reaches 1101 (stay in IDLE/early states).
    apply_reset();
    const uint8_t seq2[] = {1,0,1,0,0,1};
    for (size_t i = 0; i < sizeof(seq2); ++i) step(seq2[i], 0, 0, "partial_pattern");

    std::cout << "[TB] dut_156 passed: S1101/shift/count/done FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

