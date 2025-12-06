#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_116.h"

static inline void tick(Vdut_116 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

static inline uint8_t get_bit(const uint32_t *w, int bit) {
    int wi = bit >> 5;
    int bi = bit & 31;
    return static_cast<uint8_t>((w[wi] >> bi) & 1u);
}

static inline void set_word_pattern(uint32_t *w) {
    for (int i = 0; i < 16; ++i) {
        w[i] = static_cast<uint32_t>(0xA5A5A5A5u ^ (0x11111111u * i));
    }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_116>(ctx.get());

    std::array<uint32_t, 16> data_words{};
    set_word_pattern(data_words.data());

    std::array<uint8_t, 512> q_model{};

    // Load initial pattern
    for (int i = 0; i < 16; ++i) {
        dut->data[i] = data_words[i];
        for (int b = 0; b < 32; ++b) {
            int bit = i * 32 + b;
            if (bit < 512) {
                q_model[bit] = static_cast<uint8_t>((data_words[i] >> b) & 1u);
            }
        }
    }

    dut->clk = 0;
    dut->load = 1;
    dut->eval();
    tick(dut.get(), ctx.get());

    // Phase 1: verify load
    for (int bit = 0; bit < 512; ++bit) {
        uint8_t got = get_bit(dut->q, bit);
        if (got != q_model[bit]) {
            std::cerr << "[TB] dut_116 failed after load at bit " << bit << std::endl;
            return EXIT_FAILURE;
        }
    }

    dut->load = 0;

    // Phase 2: run several update steps and verify cellular rule.
    for (int step = 0; step < 16; ++step) {
        std::array<uint8_t, 512> next{};
        for (int i = 0; i < 512; ++i) {
            uint8_t qi = q_model[i];
            uint8_t left = (i == 0) ? 0u : q_model[i - 1];
            uint8_t right = (i == 511) ? 0u : q_model[i + 1];
            next[i] = static_cast<uint8_t>((left ^ right) & 1u);
        }
        q_model = next;

        tick(dut.get(), ctx.get());

        for (int bit = 0; bit < 512; ++bit) {
            uint8_t got = get_bit(dut->q, bit);
            if (got != q_model[bit]) {
                std::cerr << "[TB] dut_116 failed at step " << step
                          << " bit " << bit << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    std::cout << "[TB] dut_116 passed: 512-bit XOR cellular update" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

