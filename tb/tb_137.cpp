#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_137.h"

static inline void tick(Vdut_137 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

static void send_frame(Vdut_137 *dut, VerilatedContext *ctx,
                       uint8_t data, uint8_t parity_bit, bool good_stop) {
    // idle
    dut->in = 1;
    tick(dut, ctx);
    // start
    dut->in = 0;
    tick(dut, ctx);
    // 8 data bits
    for (int i = 0; i < 8; ++i) {
        dut->in = (data >> i) & 1u;
        tick(dut, ctx);
    }
    // parity bit
    dut->in = parity_bit & 1u;
    tick(dut, ctx);
    // stop bit
    dut->in = good_stop ? 1u : 0u;
    tick(dut, ctx);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_137>(ctx.get());

    dut->clk = 0;
    dut->reset = 1;
    dut->in = 1;
    dut->eval();

    tick(dut.get(), ctx.get());
    dut->reset = 0;

    auto parity_odd = [](uint8_t v) {
        uint8_t p = 0;
        for (int i = 0; i < 8; ++i) p ^= (v >> i) & 1u;
        return p & 1u;
    };

    // Phase 1: Good frame with correct odd parity over data+parity bit.
    uint8_t d1 = 0x5Au;
    // For odd parity, choose parity bit so that XOR(data bits, parity_bit) == 1.
    uint8_t p1 = parity_odd(d1) ^ 1u;
    send_frame(dut.get(), ctx.get(), d1, p1, true);
    if (!dut->done || dut->out_byte != d1) {
        std::cerr << "[TB] dut_137 failed on good frame" << std::endl;
        return EXIT_FAILURE;
    }

    // Idle
    dut->in = 1; tick(dut.get(), ctx.get());

    // Phase 2: Bad parity frame: parity bit flipped relative to the correct odd parity.
    uint8_t d2 = 0xA5u;
    // Start from the correct odd-parity bit and flip it to create a bad frame.
    uint8_t p2 = (parity_odd(d2) ^ 1u) ^ 1u; // wrong parity
    send_frame(dut.get(), ctx.get(), d2, p2, true);
    if (dut->done) {
        std::cerr << "[TB] dut_137 failed: done should be 0 on bad parity" << std::endl;
        return EXIT_FAILURE;
    }

    // Phase 3: Additional good frames with varied data to toggle every out_byte bit.
    const uint8_t good_patterns[] = {
        0x00u, 0xFFu, 0xAAu, 0x55u, 0x0Fu, 0xF0u
    };
    for (uint8_t v : good_patterns) {
        uint8_t p = parity_odd(v) ^ 1u;  // correct odd-parity bit
        send_frame(dut.get(), ctx.get(), v, p, true);
        if (!dut->done || dut->out_byte != v) {
            std::cerr << "[TB] dut_137 failed on extended good frame: data=0x"
                      << std::hex << int(v) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
        // One idle cycle between frames so done/out_byte return to 0 and bits toggle back.
        dut->in = 1;
        tick(dut.get(), ctx.get());
    }

    // Phase 4: Explicitly exercise WAIT state by forcing CHECK->WAIT via a bad
    // stop bit (stop=0), then allowing WAIT to transition back towards IDLE.
    dut->reset = 1;
    dut->eval();
    tick(dut.get(), ctx.get());
    dut->reset = 0;

    // Frame that will go through CHECK with stop bit=0, causing CHECK->WAIT.
    uint8_t d_wait = 0x3Cu;
    // Use a correct odd-parity bit, but drive stop bit low (good_stop=false).
    uint8_t p_wait = parity_odd(d_wait) ^ 1u;
    send_frame(dut.get(), ctx.get(), d_wait, p_wait, false);
    // After this frame, we should not assert done (we took WAIT, not STOP).
    if (dut->done) {
        std::cerr << "[TB] dut_137 failed: done should be 0 on WAIT path frame" << std::endl;
        return EXIT_FAILURE;
    }

    // A couple of extra idle bits to allow WAIT to transition back to IDLE cleanly.
    dut->in = 0;
    tick(dut.get(), ctx.get());
    dut->in = 1;
    tick(dut.get(), ctx.get());

    // Phase 5: Toggle reset once more late in the sequence to improve reset/parity toggles.
    dut->reset = 1;
    dut->eval();
    tick(dut.get(), ctx.get());
    dut->reset = 0;

    std::cout << "[TB] dut_137 passed: serial receiver with parity check" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
