#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_025.h"

struct Stimulus { uint32_t a; uint32_t b; };

static inline uint32_t add32(uint32_t a, uint32_t b) {
    return static_cast<uint32_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b));
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_025>(context.get());

    std::vector<Stimulus> stimuli;
    stimuli.reserve(4 * 32 + 8);

    // Baseline zero
    stimuli.push_back({0x00000000u, 0x00000000u});

    // Toggle every bit of a high then low, and every bit of b high then low.
    for (int i = 0; i < 32; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << i;
        stimuli.push_back({bit, 0x00000000u});
        stimuli.push_back({0x00000000u, 0x00000000u});
        stimuli.push_back({0x00000000u, bit});
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Exercise carry from lower to upper half (cout1 toggles)
    stimuli.push_back({0x0000FFFFu, 0x00000001u}); // cout1=1
    stimuli.push_back({0x00000000u, 0x00000000u}); // cout1 back to 0

    // Exercise upper carry (cout upper toggles)
    stimuli.push_back({0xFFFFFFFFu, 0x00000001u}); // upper cout=1
    stimuli.push_back({0x00000000u, 0x00000000u}); // upper cout back to 0

    // Additional mixed pattern for upper bits activity without carry
    stimuli.push_back({0x70000000u, 0x0F000000u});
    stimuli.push_back({0x00000000u, 0x00000000u});

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint32_t expected = add32(s.a, s.b);
        if (dut->sum != expected) {
            std::cerr << "[TB] dut_025 failed: a=0x" << std::hex << s.a
                      << ", b=0x" << s.b << ", expected sum=0x" << expected
                      << ", got 0x" << dut->sum << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_025 passed: sum == a+b across the 16-bit blocks" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
