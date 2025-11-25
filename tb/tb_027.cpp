#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_027.h"

struct Stimulus { uint32_t a; uint32_t b; };

static inline uint32_t add32(uint32_t a, uint32_t b) {
    return static_cast<uint32_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b));
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_027>(context.get());

    std::vector<Stimulus> stimuli;
    stimuli.reserve(10 * 32);

    // Baseline
    stimuli.push_back({0x00000000u, 0x00000000u});

    // Toggle each lower bit of a and b (cout1 stays 0).
    for (int i = 0; i < 16; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << i;
        stimuli.push_back({bit, 0x00000000u});
        stimuli.push_back({0x00000000u, 0x00000000u});
        stimuli.push_back({0x00000000u, bit});
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Force cout1=1 to select sum3, then back to 0.
    stimuli.push_back({0x0000FFFFu, 0x00000001u}); // cout1=1
    stimuli.push_back({0x00000000u, 0x00000000u}); // cout1=0
    stimuli.push_back({0x0000FFFFu, 0x00000000u}); // cout1=0 with lower ones

    // Toggle upper bits with cout1=0 to exercise sum2 and cout2.
    for (int i = 0; i < 16; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << (16 + i);
        stimuli.push_back({bit, 0x00000000u});
        stimuli.push_back({0x00000000u, bit});
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Toggle upper bits with cout1=1 to exercise sum3 and cout3.
    for (int i = 0; i < 16; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << (16 + i);
        stimuli.push_back({bit | 0x0000FFFFu, 0x00000001u}); // lower carry + upper bit
        stimuli.push_back({0x00000000u, 0x00000000u});
    }

    // Patterns to toggle upper carry outputs explicitly.
    stimuli.push_back({0xFFFF0000u, 0xFFFF0000u}); // cout1=0, cout2=1
    stimuli.push_back({0xFFFFFFFFu, 0xFFFF0001u}); // cout1=1, cout3=1
    stimuli.push_back({0x00000000u, 0x00000000u}); // return to zero

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint32_t expected = add32(s.a, s.b);
        if (dut->sum != expected) {
            std::cerr << "[TB] dut_027 failed: a=0x" << std::hex << s.a
                      << ", b=0x" << s.b << ", expected sum=0x" << expected
                      << ", got 0x" << dut->sum << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_027 passed: carry-select adder returns a+b" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
