#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_136.h"

static inline void tick(Vdut_136 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

static void send_frame(Vdut_136 *dut, VerilatedContext *ctx, uint8_t data, bool good_stop) {
    // idle high
    dut->in = 1;
    tick(dut, ctx);
    // start bit (0)
    dut->in = 0;
    tick(dut, ctx);
    // 8 data bits LSB-first
    for (int i = 0; i < 8; ++i) {
        dut->in = (data >> i) & 1u;
        tick(dut, ctx);
    }
    // stop bit
    dut->in = good_stop ? 1u : 0u;
    tick(dut, ctx);
}

static void send_good_frame_check(Vdut_136 *dut, VerilatedContext *ctx, uint8_t data) {
    send_frame(dut, ctx, data, true);
    if (!dut->done || dut->out_byte != data) {
        std::cerr << "[TB] dut_136 failed on good frame: expected done=1, out=0x"
                  << std::hex << int(data) << " got done=" << std::dec << int(dut->done)
                  << " out=0x" << std::hex << int(dut->out_byte) << std::dec << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // One idle cycle to allow done to drop and out_byte to return to 0.
    dut->in = 1;
    tick(dut, ctx);
}

static void send_bad_frame_check(Vdut_136 *dut, VerilatedContext *ctx, uint8_t data) {
    send_frame(dut, ctx, data, false);
    if (dut->done) {
        std::cerr << "[TB] dut_136 failed: done should be 0 after bad frame (data=0x"
                  << std::hex << int(data) << ")" << std::dec << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // Idle after bad frame as well.
    dut->in = 1;
    tick(dut, ctx);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_136>(ctx.get());

    dut->clk = 0;
    dut->in = 1;

    // Helper to apply synchronous reset and return to IDLE.
    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->eval();
        tick(dut.get(), ctx.get());
        dut->reset = 0;
    };

    apply_reset();

    // Phase 1: basic good/bad frames to sanity-check behavior.
    send_good_frame_check(dut.get(), ctx.get(), 0x3Cu);
    send_bad_frame_check(dut.get(), ctx.get(), 0xA5u);

    // Phase 2: drive patterns that toggle every output bit (0->1->0).
    // Use a mix of dense patterns and single-bit patterns.
    const uint8_t patterns1[] = {0x00u, 0xFFu, 0xAAu, 0x55u};
    for (uint8_t v : patterns1) {
        send_good_frame_check(dut.get(), ctx.get(), v);
    }
    for (int b = 0; b < 8; ++b) {
        uint8_t v = static_cast<uint8_t>(1u << b);
        send_good_frame_check(dut.get(), ctx.get(), v);
    }

    // Phase 3: exercise WAIT state more thoroughly with several bad frames.
    send_bad_frame_check(dut.get(), ctx.get(), 0x00u); // all zeros
    send_bad_frame_check(dut.get(), ctx.get(), 0xFFu); // all ones

    // Phase 4: toggle reset again during operation to improve reset and state toggles.
    apply_reset();
    // After reset, run a small set of additional frames.
    send_good_frame_check(dut.get(), ctx.get(), 0x0Fu);
    send_good_frame_check(dut.get(), ctx.get(), 0xF0u);

    std::cout << "[TB] dut_136 passed: serial receiver with data latch and full coverage patterns" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
