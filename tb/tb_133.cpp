#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_133.h"

static inline void tick(Vdut_133 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_133>(ctx.get());

    enum { BYTE1=0, BYTE2=1, BYTE3=2, DONE=3 };
    uint8_t state = BYTE1;

    dut->clk = 0;
    dut->reset = 1;
    dut->in = 0;
    dut->eval();

    tick(dut.get(), ctx.get());
    state = BYTE1;
    dut->reset = 0;

    uint8_t current_in = 0;

    auto step = [&](uint8_t in_val) {
        // Bit 3 is the start-of-frame indicator used by the FSM.
        uint8_t in3 = (in_val >> 3) & 1u;

        // Model next state
        uint8_t next;
        switch (state) {
            case BYTE1: next = in3 ? BYTE2 : BYTE1; break;
            case BYTE2: next = BYTE3; break;
            case BYTE3: next = DONE; break;
            case DONE:  next = in3 ? BYTE2 : BYTE1; break;
            default:    next = BYTE1; break;
        }
        dut->in = in_val;
        tick(dut.get(), ctx.get());
        state = next;
        current_in = in_val;
        uint8_t done_exp = (state == DONE) ? 1u : 0u;
        if (dut->done != done_exp) {
            std::cerr << "[TB] dut_133 failed: in=0x" << std::hex << int(in_val)
                      << " (in[3]=" << std::dec << int(in3) << ")"
                      << " expected done=" << int(done_exp)
                      << " got " << int(dut->done) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Phase 1: original functional sequence using only in[3] (other bits 0).
    step(0x00u);          // stay BYTE1 (in[3]=0)
    step(0x08u);          // BYTE1->BYTE2 (in[3]=1)
    step(0x00u);          // BYTE2->BYTE3 (in[3]=0)
    step(0x00u);          // BYTE3->DONE (done asserted, in[3]=0)
    step(0x00u);          // DONE->BYTE1 (in[3]=0)
    step(0x08u);          // BYTE1->BYTE2
    step(0x00u);          // BYTE2->BYTE3
    step(0x00u);          // BYTE3->DONE

    // Phase 2: dedicated patterns to toggle every input bit (0->1 and 1->0)
    // while preserving the correct FSM behavior (still only depends on in[3]).
    auto toggle_bit = [&](int bit) {
        // Force bit high
        uint8_t v1 = current_in | static_cast<uint8_t>(1u << bit);
        step(v1);
        // Then low again
        uint8_t v2 = v1 & static_cast<uint8_t>(~(1u << bit));
        step(v2);
    };

    for (int b = 0; b < 8; ++b) {
        toggle_bit(b);
    }

    std::cout << "[TB] dut_133 passed: 3-byte done handshake FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
