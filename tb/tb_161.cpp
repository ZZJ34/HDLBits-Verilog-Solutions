#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_161.h"

static inline void tick(Vdut_161 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_161>(ctx.get());

    uint32_t hist_model = 0;

    auto apply_reset = [&]() {
        dut->areset = 1;
        dut->predict_valid = 0;
        dut->predict_taken = 0;
        dut->train_mispredicted = 0;
        dut->train_taken = 0;
        dut->train_history = 0;
        tick(dut.get(), ctx.get());
        dut->areset = 0;
        hist_model = 0;
        if (dut->predict_history != hist_model) {
            std::cerr << "[TB] dut_161 failed after reset: expected 0, got 0x"
                      << std::hex << dut->predict_history << std::dec << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    };

    if (apply_reset() != EXIT_SUCCESS) return EXIT_FAILURE;

    auto step = [&](bool train_mispredicted,
                    bool predict_valid,
                    bool predict_taken,
                    uint32_t train_history,
                    bool train_taken,
                    const char *ctx_str) {
        dut->train_mispredicted = train_mispredicted ? 1 : 0;
        dut->predict_valid      = predict_valid ? 1 : 0;
        dut->predict_taken      = predict_taken ? 1 : 0;
        dut->train_history      = train_history;
        dut->train_taken        = train_taken ? 1 : 0;

        // Compute expected next history.
        if (dut->areset) {
            hist_model = 0;
        } else if (train_mispredicted) {
            hist_model = ((train_history << 1) | (train_taken ? 1u : 0u));
        } else if (predict_valid) {
            hist_model = ((hist_model << 1) | (predict_taken ? 1u : 0u));
        } else {
            // No change.
        }

        tick(dut.get(), ctx.get());

        if (dut->predict_history != hist_model) {
            std::cerr << "[TB] dut_161 failed (" << ctx_str << "): "
                      << "expected=0x" << std::hex << hist_model
                      << " got=0x" << dut->predict_history << std::dec << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Phase 1: exercise train_mispredicted path.
    step(true,  false, false, 0x00000000u, true,  "mispred_zero_to1");
    step(true,  true,  true,  0xAAAAAAAAu, false, "mispred_A_to0");
    step(true,  false, true,  0x55555555u, true,  "mispred_5_to1");

    // Phase 2: exercise predict_valid path, shifting in a stream of ones then zeros.
    for (int i = 0; i < 32; ++i) {
        step(false, true, true,  0, false, "predict_stream_ones");
    }
    for (int i = 0; i < 32; ++i) {
        step(false, true, false, 0, false, "predict_stream_zeros");
    }

    // Phase 3: no-update cycles (neither mispred nor valid).
    for (int i = 0; i < 4; ++i) {
        step(false, false, false, 0, false, "idle_no_update");
    }

    std::cout << "[TB] dut_161 passed: global history register" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

