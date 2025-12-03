#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_109.h"

struct Stim109 {
    uint8_t load;
    uint8_t ena;
    uint8_t amount;
};

static inline void tick(Vdut_109 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_109>(ctx.get());

    uint64_t q_model = 0;

    auto step = [&](uint8_t load, uint8_t ena, uint8_t amount, uint64_t data) {
        if (load) {
            q_model = data;
        } else if (ena) {
            switch (amount & 0x3u) {
                case 0u: // logical left 1
                    q_model <<= 1;
                    break;
                case 1u: // logical left 8
                    q_model <<= 8;
                    break;
                case 2u: { // arithmetic right 1
                    uint8_t msb = static_cast<uint8_t>((q_model >> 63) & 1u);
                    q_model >>= 1;
                    if (msb) {
                        q_model |= (1ull << 63);
                    }
                    break;
                }
                case 3u: { // arithmetic right 8
                    uint8_t msb = static_cast<uint8_t>((q_model >> 63) & 1u);
                    q_model >>= 8;
                    if (msb) {
                        q_model |= 0xFF00000000000000ull;
                    }
                    break;
                }
            }
        }
    };

    dut->clk = 0;
    dut->load = 1;
    dut->ena = 0;
    dut->amount = 0;
    dut->data = 0x8000000000000001ull; // sign bit and LSB set
    dut->eval();

    const Stim109 pattern[] = {
        // Phase 1: exercise all four shift amounts starting from msb=1
        {1u, 0u, 0u}, // load initial data (msb=1)
        {0u, 1u, 0u}, // logical left 1
        {0u, 1u, 1u}, // logical left 8
        {0u, 1u, 2u}, // arithmetic right 1 with msb still 1
        {0u, 1u, 3u}, // arithmetic right 8 with msb still 1
        // Phase 2: reload with msb=0 to exercise q[63]==0 branches of arithmetic right
        {1u, 0u, 0u},
        {0u, 1u, 2u}, // arithmetic right 1 with msb=0
        {0u, 1u, 3u}, // arithmetic right 8 with msb=0
        {0u, 0u, 0u}  // hold
    };

    uint64_t data_init = 0x8000000000000001ull;

    for (size_t i = 0; i < sizeof(pattern)/sizeof(pattern[0]); ++i) {
        const auto &s = pattern[i];
        dut->load = s.load;
        dut->ena = s.ena;
        dut->amount = s.amount;
        if (i == 5) {
            // second load uses a different pattern with msb=0
            dut->data = 0x000000000000FF00ull;
        } else {
            dut->data = data_init;
        }

        tick(dut.get(), ctx.get());

        uint64_t data_for_step = (i == 5) ? 0x000000000000FF00ull : data_init;
        step(s.load, s.ena, s.amount, data_for_step);

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_109 failed at step " << i
                      << " load=" << int(s.load)
                      << " ena=" << int(s.ena)
                      << " amount=" << int(s.amount)
                      << " expected q=0x" << std::hex << q_model
                      << " got 0x" << dut->q << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    // Additional phase: toggle every bit of q via load operations to improve
    // per-bit toggle coverage (0->1->0) independently of shift behavior.
    for (int bit = 0; bit < 64; ++bit) {
        uint64_t mask = 1ull << bit;
        // Load pattern with a single bit set
        dut->load = 1;
        dut->ena = 0;
        dut->amount = 0;
        dut->data = mask;
        tick(dut.get(), ctx.get());
        q_model = mask;
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_109 failed in load-toggle phase (set bit "
                      << bit << ")" << std::endl;
            return EXIT_FAILURE;
        }
        // Load zeros to clear that bit
        dut->load = 1;
        dut->data = 0ull;
        tick(dut.get(), ctx.get());
        q_model = 0ull;
        if (dut->q != q_model) {
            std::cerr << "[TB] dut_109 failed in load-toggle phase (clear bit "
                      << bit << ")" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_109 passed: 64-bit barrel/arithmetic shifter" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
