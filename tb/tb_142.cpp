#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_142.h"

static inline void tick(Vdut_142 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_142>(ctx.get());

    enum { A = 0, B = 1 };
    uint8_t state = A;
    uint8_t count = 0;
    uint8_t count1 = 0;

    auto step = [&](uint8_t reset, uint8_t s, uint8_t w, const char *ctx_str) {
        dut->reset = reset & 1u;
        dut->s = s & 1u;
        dut->w = w & 1u;

        // Model synchronous behavior.
        if (reset) {
            state = A;
            count = 0;
            count1 = 0;
        } else {
            uint8_t next = state;
            switch (state) {
                case A: next = s ? B : A; break;
                case B: next = B; break;
            }

            // Updates use old state.
            if (state == B) {
                if (count1 == 3) {
                    count = 0;
                    count1 = 0;
                }
                if (w) count = static_cast<uint8_t>(count + 1);
                count1 = static_cast<uint8_t>(count1 + 1);
            }
            state = next;
        }

        tick(dut.get(), ctx.get());

        uint8_t z_exp = ((count == 2) && (count1 == 3)) ? 1u : 0u;
        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_142 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " count=" << int(count)
                      << " count1=" << int(count1)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    };

    // Initial reset.
    if (step(1, 0, 0, "reset") != EXIT_SUCCESS) return EXIT_FAILURE;

    // Stay in A for a few cycles.
    for (int i = 0; i < 3; ++i) {
        if (step(0, 0, 0, "stay_A") != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    // Enter B and run a pattern of w=1/0 to exercise counters.
    for (int i = 0; i < 12; ++i) {
        uint8_t s = 1;
        uint8_t w = (i % 3) ? 1u : 0u;  // 0,1,1,0,1,1,...
        if (step(0, s, w, "run_B") != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    // Another reset pulse and a different pattern.
    if (step(1, 0, 0, "reset2") != EXIT_SUCCESS) return EXIT_FAILURE;
    for (int i = 0; i < 8; ++i) {
        uint8_t s = 1;
        uint8_t w = (i & 1u);
        if (step(0, s, w, "run_B_alt") != EXIT_SUCCESS) return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_142 passed: counter-based FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

