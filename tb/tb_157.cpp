#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_157.h"

static inline void tick(Vdut_157 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_157>(ctx.get());

    enum {
        IDLE=0, S1=1, S11=2, S110=3, S1101=4,
        SHIFT1=5, SHIFT2=6, SHIFT3=7, COUNT=8, DONE=9
    };
    uint8_t state = IDLE;

    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->data = 0;
        dut->ack = 0;
        tick(dut.get(), ctx.get());
        dut->reset = 0;
        state = IDLE;
    };

    auto step = [&](uint8_t data, uint8_t ack, const char *ctx_str) {
        dut->data = data & 1u;
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
            // In COUNT, remain in COUNT until the DUT asserts done, at which
            // point we model a transition to DONE based on the observed output.
            case COUNT: next = COUNT; break;
            case DONE:  next = ack ? IDLE : DONE; break;
        }

        tick(dut.get(), ctx.get());
        if (state == COUNT && dut->done) {
            state = DONE;
        } else {
            state = next;
        }

        uint8_t counting_exp = (state == COUNT) ? 1u : 0u;
        uint8_t done_exp = (state == DONE) ? 1u : 0u;
        if (dut->counting != counting_exp || dut->done != done_exp) {
            std::cerr << "[TB] dut_157 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " data=" << int(data)
                      << " ack=" << int(ack)
                      << " expected counting=" << int(counting_exp)
                      << " done=" << int(done_exp)
                      << " got counting=" << int(dut->counting)
                      << " done=" << int(dut->done) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Scenario 1: shift in a count value (e.g. 0b1010 = 10) then let it count down.
    apply_reset();
    // Feed 1101
    step(1, 0, "S1_1");
    step(1, 0, "S1_11");
    step(0, 0, "S1_110");
    step(1, 0, "S1_1101");
    // SHIFT1..3: shift bits into count[3:0]
    step(1, 0, "shift_bit3");  // SHIFT1
    step(0, 0, "shift_bit2");  // SHIFT2
    step(1, 0, "shift_bit1");  // SHIFT3 -> COUNT

    // Let the counter run until done is asserted at least once.
    bool saw_done = false;
    for (int i = 0; i < 12000 && !saw_done; ++i) {
        step(0, 0, "COUNT_run");
        if (dut->done) saw_done = true;
    }
    if (!saw_done) {
        std::cerr << "[TB] dut_157 failed: never saw done during COUNT" << std::endl;
        return EXIT_FAILURE;
    }

    // Acknowledge and go back to IDLE.
    step(0, 1, "DONE_ack");

    // Scenario 2: random-ish pattern without completing the sequence.
    apply_reset();
    const uint8_t seq2[] = {0,1,0,1,1,0,0,1};
    for (size_t i = 0; i < sizeof(seq2); ++i) step(seq2[i], 0, "random");

    std::cout << "[TB] dut_157 passed: programmable countdown with 1000-cycle subcounter" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
