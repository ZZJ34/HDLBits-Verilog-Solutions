#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_135.h"

static inline void tick(Vdut_135 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

// Drive one serial frame: start(0), 8 data bits LSB-first, stop bit (1).
static void send_frame(Vdut_135 *dut, VerilatedContext *ctx, uint8_t data, bool good_stop) {
    // idle high
    dut->in = 1;
    tick(dut, ctx);
    // start bit
    dut->in = 0;
    tick(dut, ctx);
    // 8 data bits
    for (int i = 0; i < 8; ++i) {
        dut->in = (data >> i) & 1u;
        tick(dut, ctx);
    }
    // stop bit
    dut->in = good_stop ? 1u : 0u;
    tick(dut, ctx);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_135>(ctx.get());

    dut->clk = 0;
    dut->reset = 1;
    dut->in = 1;
    dut->eval();

    tick(dut.get(), ctx.get());
    dut->reset = 0;

    // Good frame: expect done pulse after stop bit.
    send_frame(dut.get(), ctx.get(), 0xA5u, true);
    if (!dut->done) {
        std::cerr << "[TB] dut_135 failed: expected done after good frame" << std::endl;
        return EXIT_FAILURE;
    }
    // Idle cycle
    dut->in = 1; tick(dut.get(), ctx.get());

    // Bad frame (bad stop bit): done should remain 0.
    send_frame(dut.get(), ctx.get(), 0x5Au, false);
    if (dut->done) {
        std::cerr << "[TB] dut_135 failed: unexpected done after bad frame" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_135 passed: serial receiver done flag behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

