#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_118.h"

static inline void tick(Vdut_118 *dut, VerilatedContext *ctx) {
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

static inline void set_data_pattern(uint32_t *w) {
    for (int i = 0; i < 8; ++i) {
        w[i] = static_cast<uint32_t>(0x0F0F0F0Fu ^ (0x01010101u * i));
    }
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_118>(ctx.get());

    std::array<uint32_t, 8> data_words{};
    set_data_pattern(data_words.data());

    std::array<uint8_t, 256> q_model{};

    // Phase 1: load initial data for functional Game-of-Life checks
    for (int i = 0; i < 8; ++i) {
        dut->data[i] = data_words[i];
        for (int b = 0; b < 32; ++b) {
            int bit = i * 32 + b;
            if (bit < 256) {
                q_model[bit] = static_cast<uint8_t>((data_words[i] >> b) & 1u);
            }
        }
    }

    dut->clk = 0;
    dut->load = 1;
    dut->eval();
    tick(dut.get(), ctx.get());

    for (int bit = 0; bit < 256; ++bit) {
        uint8_t got = get_bit(dut->q, bit);
        if (got != q_model[bit]) {
            std::cerr << "[TB] dut_118 failed after load at bit " << bit << std::endl;
            return EXIT_FAILURE;
        }
    }

    dut->load = 0;

    auto step_life = [&](const std::array<uint8_t, 256> &cur,
                         std::array<uint8_t, 256> &nxt) {
        auto idx = [](int x, int y) { return x + y * 16; };
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int xm1 = (x == 0) ? 15 : x - 1;
                int xp1 = (x == 15) ? 0 : x + 1;
                int ym1 = (y == 0) ? 15 : y - 1;
                int yp1 = (y == 15) ? 0 : y + 1;
                uint8_t neigh[8] = {
                    cur[idx(xm1, ym1)],
                    cur[idx(xm1, y)],
                    cur[idx(xm1, yp1)],
                    cur[idx(x, ym1)],
                    cur[idx(x, yp1)],
                    cur[idx(xp1, ym1)],
                    cur[idx(xp1, y)],
                    cur[idx(xp1, yp1)]
                };
                uint8_t current = cur[idx(x, y)];
                uint8_t pop = 0;
                for (int k = 0; k < 8; ++k) pop += neigh[k];
                uint8_t next =
                    (pop == 2) ? current :
                    (pop == 3) ? 1u : 0u;
                nxt[idx(x, y)] = next;
            }
        }
    };

    // Simulate a few generations for functional coverage
    for (int step = 0; step < 8; ++step) {
        std::array<uint8_t, 256> next{};
        step_life(q_model, next);
        q_model = next;

        tick(dut.get(), ctx.get());

        for (int bit = 0; bit < 256; ++bit) {
            uint8_t got = get_bit(dut->q, bit);
            if (got != q_model[bit]) {
                std::cerr << "[TB] dut_118 failed at step " << step
                          << " bit " << bit << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // Phase 2: dedicated toggle coverage. For each bit, load a one-hot pattern
    // and then zeros, so every q bit sees 0->1->0 at least once.
    for (int bit = 0; bit < 256; ++bit) {
        // Build one-hot mask
        std::array<uint32_t, 8> mask_words{};
        int wi = bit >> 5;
        int bi = bit & 31;
        mask_words[wi] = static_cast<uint32_t>(1u << bi);

        // Load one-hot
        dut->load = 1;
        for (int i = 0; i < 8; ++i) {
            dut->data[i] = mask_words[i];
        }
        tick(dut.get(), ctx.get());

        for (int b = 0; b < 256; ++b) {
            uint8_t exp = (b == bit) ? 1u : 0u;
            uint8_t got = get_bit(dut->q, b);
            if (got != exp) {
                std::cerr << "[TB] dut_118 failed in toggle phase (set) at bit "
                          << b << std::endl;
                return EXIT_FAILURE;
            }
        }

        // Load zeros
        dut->load = 1;
        for (int i = 0; i < 8; ++i) {
            dut->data[i] = 0u;
        }
        tick(dut.get(), ctx.get());

        for (int b = 0; b < 256; ++b) {
            uint8_t got = get_bit(dut->q, b);
            if (got != 0u) {
                std::cerr << "[TB] dut_118 failed in toggle phase (clear) at bit "
                          << b << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    // Phase 3: pseudo-random patterns to further exercise neighbor/population
    // combinational logic. We deterministically generate a stream of patterns
    // using a simple LFSR to keep the test repeatable.
    auto lfsr_step = [](uint32_t &s) {
        // 32-bit Galois LFSR with taps at 32,22,2,1
        uint32_t bit = ((s >> 0) ^ (s >> 1) ^ (s >> 21) ^ (s >> 31)) & 1u;
        s = (s >> 1) | (bit << 31);
    };

    uint32_t lfsr = 0x1u;
    constexpr int patterns = 64;

    for (int p = 0; p < patterns; ++p) {
        // Build a random-looking board from LFSR
        std::array<uint32_t, 8> rand_words{};
        std::array<uint8_t, 256> cur{};
        for (int wi = 0; wi < 8; ++wi) {
            uint32_t word = 0;
            for (int b = 0; b < 32; ++b) {
                uint8_t bit = static_cast<uint8_t>(lfsr & 1u);
                word |= static_cast<uint32_t>(bit) << b;
                lfsr_step(lfsr);
            }
            rand_words[wi] = word;
            for (int b = 0; b < 32; ++b) {
                int idx = wi * 32 + b;
                if (idx < 256) {
                    cur[idx] = static_cast<uint8_t>((word >> b) & 1u);
                }
            }
        }

        std::array<uint8_t, 256> next{};
        step_life(cur, next);

        // Load cur into DUT
        dut->load = 1;
        for (int i = 0; i < 8; ++i) {
            dut->data[i] = rand_words[i];
        }
        tick(dut.get(), ctx.get());

        for (int bit = 0; bit < 256; ++bit) {
            uint8_t got = get_bit(dut->q, bit);
            if (got != cur[bit]) {
                std::cerr << "[TB] dut_118 random phase load mismatch at pattern "
                          << p << " bit " << bit << std::endl;
                return EXIT_FAILURE;
            }
        }

        // Advance one Life step and check against 'next'
        dut->load = 0;
        tick(dut.get(), ctx.get());
        for (int bit = 0; bit < 256; ++bit) {
            uint8_t got = get_bit(dut->q, bit);
            if (got != next[bit]) {
                std::cerr << "[TB] dut_118 random phase step mismatch at pattern "
                          << p << " bit " << bit << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    std::cout << "[TB] dut_118 passed: 16x16 Game of Life update" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
