#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_028.h"

struct Stimulus { uint32_t a; uint32_t b; uint8_t sub; };

static inline uint32_t addsub(uint32_t a, uint32_t b, uint8_t sub) {
    const uint32_t b_xor = sub ? ~b : b;
    return static_cast<uint32_t>(static_cast<uint64_t>(a) + static_cast<uint64_t>(b_xor) + static_cast<uint64_t>(sub & 0x1u));
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_028>(context.get());

    std::vector<Stimulus> stimuli;
    stimuli.reserve(12 * 32);

    // Baseline add/sub toggles.
    stimuli.push_back({0x00000000u, 0x00000000u, 0u});
    stimuli.push_back({0x00000000u, 0x00000000u, 1u});

    // Toggle every bit of a and b in add mode, then in sub mode.
    for (int i = 0; i < 32; ++i) {
        const uint32_t bit = static_cast<uint32_t>(1u) << i;
        stimuli.push_back({bit, 0x00000000u, 0u});
        stimuli.push_back({0x00000000u, bit, 0u});
        stimuli.push_back({bit, 0x00000000u, 1u});
        stimuli.push_back({0x00000000u, bit, 1u});
        stimuli.push_back({0x00000000u, 0x00000000u, 0u});
    }

    // Add: generate carry into upper half and into MSB.
    stimuli.push_back({0x0000FFFFu, 0x00000001u, 0u}); // cout1=1
    stimuli.push_back({0x7FFFFFFFu, 0x00000001u, 0u}); // carry into MSB
    stimuli.push_back({0xFFFFFFFFu, 0x00000001u, 0u}); // full carry chain

    // Sub: borrow across lower/upper halves and from MSB.
    stimuli.push_back({0x00010000u, 0x00000001u, 1u}); // borrow into upper half
    stimuli.push_back({0x00000000u, 0x00000001u, 1u}); // borrow through entire word
    stimuli.push_back({0x80000000u, 0x00000001u, 1u}); // MSB borrow toggles

    // Mixed patterns for additional toggle coverage.
    stimuli.push_back({0x12345678u, 0x11111111u, 0u});
    stimuli.push_back({0x89ABCDEFu, 0x01020304u, 1u});
    stimuli.push_back({0x00000000u, 0x00000000u, 0u});

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->sub = s.sub;
        dut->eval();

        const uint32_t expected = addsub(s.a, s.b, s.sub);
        if (dut->sum != expected) {
            std::cerr << "[TB] dut_028 failed: a=0x" << std::hex << s.a
                      << ", b=0x" << s.b << ", sub=" << std::dec << int(s.sub)
                      << ", expected sum=0x" << std::hex << expected
                      << ", got 0x" << dut->sum << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_028 passed: sum matches add/sub behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
