#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_160.h"

static inline void tick(Vdut_160 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_160>(ctx.get());

    enum { SNT=0, WNT=1, WT=2, ST=3 };
    uint8_t state = WNT; // reset state

    auto apply_reset = [&]() {
        dut->areset = 1;
        dut->train_valid = 0;
        dut->train_taken = 0;
        tick(dut.get(), ctx.get());
        dut->areset = 0;
        state = WNT;
    };

    auto step = [&](uint8_t valid, uint8_t taken, const char *ctx_str) {
        dut->train_valid = valid & 1u;
        dut->train_taken = taken & 1u;

        uint8_t next = state;
        if (valid) {
            switch (state) {
                case SNT: next = taken ? WNT : SNT; break;
                case WNT: next = taken ? WT  : SNT; break;
                case WT:  next = taken ? ST  : WNT; break;
                case ST:  next = taken ? ST  : WT; break;
            }
        }

        tick(dut.get(), ctx.get());
        if (dut->state != next && valid) {
            std::cerr << "[TB] dut_160 failed (" << ctx_str << "): "
                      << "expected state=" << int(next)
                      << " got " << int(dut->state) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (valid) state = next;
    };

    apply_reset();

    // Sequence of outcomes to walk through all states and transitions.
    step(1, 0, "WNT->SNT"); // miss
    step(1, 1, "SNT->WNT"); // hit
    step(1, 1, "WNT->WT");  // hit
    step(1, 1, "WT->ST");   // hit
    step(1, 0, "ST->WT");   // miss
    step(1, 0, "WT->WNT");  // miss
    step(1, 0, "WNT->SNT2");// miss
    step(1, 1, "SNT->WNT2");// hit

    // Some cycles with valid=0 to exercise 'hold' behavior.
    for (int i = 0; i < 4; ++i) {
        step(0, 0, "hold");
    }

    std::cout << "[TB] dut_160 passed: 2-bit saturating branch predictor FSM" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

