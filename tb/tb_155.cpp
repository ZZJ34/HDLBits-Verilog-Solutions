#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_155.h"

static inline void tick(Vdut_155 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_155>(ctx.get());

    enum { S0=0, S1=1, S2=2, S3=3, S4=4 };
    uint8_t state = S0;

    auto step = [&](uint8_t reset, const char *ctx_str) {
        dut->reset = reset & 1u;

        uint8_t next = state;
        switch (state) {
            case S0: next = reset ? S1 : S0; break;
            case S1: next = S2; break;
            case S2: next = S3; break;
            case S3: next = S4; break;
            case S4: next = S0; break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        uint8_t ena_exp = (state == S1 || state == S2 || state == S3 || state == S4) ? 1u : 0u;
        if (dut->shift_ena != ena_exp) {
            std::cerr << "[TB] dut_155 failed (" << ctx_str << "): "
                      << "state=" << int(state)
                      << " reset=" << int(reset)
                      << " expected shift_ena=" << int(ena_exp)
                      << " got " << int(dut->shift_ena) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Run a few cycles with and without reset to walk through all states.
    step(1, "S0->S1");
    step(0, "S1->S2");
    step(0, "S2->S3");
    step(0, "S3->S4");
    step(0, "S4->S0");

    // Another pass with reset inactive to cycle again.
    for (int i = 0; i < 8; ++i) {
        step(0, "cycle");
    }

    // Pulse reset mid-cycle.
    step(1, "pulse_reset");
    step(0, "after_reset");

    std::cout << "[TB] dut_155 passed: 5-state enable sequencer" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

