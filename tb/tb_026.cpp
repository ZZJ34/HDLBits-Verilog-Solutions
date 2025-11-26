#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_026.h"

struct Stimulus { uint32_t a; uint32_t b; };

static inline uint32_t add32(uint32_t a, uint32_t b) {
    return static_cast<uint32_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b));
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_026>(context.get());

    std::vector<Stimulus> stimuli;
    stimuli.reserve(8 * 32 + 16);

    // Baseline
    stimuli.push_back({0x00000000u, 0x00000000u});

    // Toggle every bit of a and b high then low to exercise all sum bits.
    for (int i = 0; i < 32; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << i;
        stimuli.push_back({bit, 0x00000000u});
        stimuli.push_back({0x00000000u, 0x00000000u});
        stimuli.push_back({0x00000000u, bit});
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Force carry ripple across the lower 16 bits (cout1 toggles).
    for (int i = 0; i < 16; ++i) {
        const uint32_t lower_ones = (i == 0) ? 0u : ((static_cast<uint32_t>(1u) << i) - 1u);
        stimuli.push_back({lower_ones, 0x00000001u}); // generates carry through i bits
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Force carry ripple inside the upper 16 bits with cin=1 from lower half.
    for (int k = 1; k <= 16; ++k) {
        const uint32_t upper_ones = ((static_cast<uint32_t>(1u) << k) - 1u) << 16;
        const uint32_t a_val = upper_ones | 0x0000FFFFu; // lower all ones to set cin=1
        stimuli.push_back({a_val, 0x00000001u}); // ripple through k upper bits
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Additional mixed patterns to toggle carries on/off in both halves.
    stimuli.push_back({0x12345678u, 0x11111111u});
    stimuli.push_back({0x0000FFFFu, 0x00000001u}); // cout1=1
    stimuli.push_back({0xFFFF0000u, 0x0000FFFFu}); // upper and lower active without cin ripple
    stimuli.push_back({0xFFFFFFFFu, 0x00000001u}); // full carry chain to MSB
    stimuli.push_back({0x00000000u, 0x00000000u}); // return to zero

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint32_t expected = add32(s.a, s.b);
        if (dut->sum != expected) {
            std::cerr << "[TB] dut_026 failed: a=0x" << std::hex << s.a
                      << ", b=0x" << s.b << ", expected sum=0x" << expected
                      << ", got 0x" << dut->sum << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_026 passed: ripple-carry adder matches a+b" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
