#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_014.h"

struct Stimulus { uint8_t a; uint8_t b; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_014>(context.get());

    const std::array<Stimulus, 5> stimuli{{
        {0u, 0u},   // baseline
        {7u, 0u},   // a: 0->1
        {7u, 7u},   // b: 0->1
        {0u, 7u},   // a: 1->0
        {0u, 0u},   // b: 1->0
    }};

    for (const auto &s : stimuli) {
        dut->a = s.a & 0x7u;
        dut->b = s.b & 0x7u;
        dut->eval();

        const uint8_t exp_or_bit = static_cast<uint8_t>((s.a | s.b) & 0x7u);
        const uint8_t exp_or_log = static_cast<uint8_t>(((s.a != 0u) || (s.b != 0u)) ? 1u : 0u);
        const uint8_t exp_not_hi = static_cast<uint8_t>((~s.b) & 0x7u);
        const uint8_t exp_not_lo = static_cast<uint8_t>((~s.a) & 0x7u);
        const uint8_t out_or_bit = static_cast<uint8_t>(dut->out_or_bitwise);
        const uint8_t out_not = static_cast<uint8_t>(dut->out_not);

        if (out_or_bit != exp_or_bit || dut->out_or_logical != exp_or_log ||
            ((out_not >> 3) & 0x7u) != exp_not_hi || (out_not & 0x7u) != exp_not_lo) {
            std::cerr << "[TB] dut_014 failed: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", exp or_bit=" << int(exp_or_bit) << ", or_log=" << int(exp_or_log)
                      << ", not_hi/lo=" << int(exp_not_hi) << "/" << int(exp_not_lo)
                      << ", got or_bit=" << int(out_or_bit) << ", or_log=" << int(dut->out_or_logical)
                      << ", out_not=" << int(out_not) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_014 passed: bitwise/logical or and inversion checks" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

