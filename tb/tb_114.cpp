#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_114.h"

static inline void tick(Vdut_114 *dut, VerilatedContext *ctx) {
    // KEY[0] is clk
    dut->KEY = static_cast<uint8_t>(dut->KEY & ~0x1u);
    dut->eval();
    ctx->timeInc(1);
    dut->KEY = static_cast<uint8_t>(dut->KEY | 0x1u);
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_114>(ctx.get());

    uint8_t q = 0; // q[3:0] = LEDR

    dut->SW = 0;
    dut->KEY = 0;
    dut->eval();

    auto apply_cycle = [&](uint8_t L, uint8_t E, uint8_t w, uint8_t R) {
        // KEY layout: [3]=w, [2]=L, [1]=E, [0]=clk
        uint8_t key = 0;
        key |= (w & 1u) << 3;
        key |= (L & 1u) << 2;
        key |= (E & 1u) << 1;
        dut->KEY = key;
        dut->SW = R & 0xFu;

        // Compute next q model from previous state (parallel update).
        uint8_t q_prev = q;
        uint8_t new_q = 0;
        for (int i = 3; i >= 0; --i) {
            uint8_t out_prev = (q_prev >> i) & 1u;
            uint8_t w_in = 0;
            if (i == 3) w_in = (key >> 3) & 1u;
            else        w_in = (q_prev >> (i + 1)) & 1u;

            uint8_t temp0 = E ? w_in : out_prev;
            uint8_t temp1 = L ? ((R >> i) & 1u) : temp0;
            if (temp1) new_q |= (1u << i);
        }
        q = new_q;

        tick(dut.get(), ctx.get());

        if ((dut->LEDR & 0xFu) != q) {
            std::cerr << "[TB] dut_114 failed: L=" << int(L)
                      << " E=" << int(E)
                      << " w=" << int(w)
                      << " R=0x" << std::hex << int(R)
                      << " expected q=0x" << int(q)
                      << " got 0x" << int(dut->LEDR & 0xFu) << std::dec
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Parallel load then shift through various patterns.
    apply_cycle(1, 0, 0, 0xAu); // load 1010
    apply_cycle(0, 1, 1, 0x0u); // shift in 1
    apply_cycle(0, 1, 0, 0x0u); // shift in 0
    apply_cycle(1, 0, 0, 0x5u); // load 0101
    apply_cycle(0, 1, 1, 0x0u);
    apply_cycle(0, 1, 1, 0x0u);
    apply_cycle(0, 0, 0, 0x0u); // hold

    std::cout << "[TB] dut_114 passed: chained MUXDFF behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
