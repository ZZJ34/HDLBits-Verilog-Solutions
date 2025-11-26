#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_022.h"

struct Stimulus { uint8_t a, b, c, d; };

static inline uint8_t compute_out1(const Stimulus &s) {
    return static_cast<uint8_t>(((s.a & s.b) | (s.c & s.d)) & 0x1u);
}

static inline uint8_t compute_out2(const Stimulus &s) {
    return static_cast<uint8_t>(((s.a | s.b) & (s.c | s.d)) & 0x1u);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_022>(context.get());

    // Same behavior as dut_021; exercise all logical combinations.
    const std::array<Stimulus, 11> stimuli{{
        {0,0,0,0},
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,0,1},
        {1,1,0,0},
        {0,0,1,1},
        {0,1,1,0},
        {0,1,0,1},
        {1,0,1,1},
        {0,0,0,0}
    }};

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->c = s.c;
        dut->d = s.d;
        dut->eval();

        const uint8_t exp1 = compute_out1(s);
        const uint8_t exp2 = compute_out2(s);
        if (dut->out1 != exp1 || dut->out2 != exp2) {
            std::cerr << "[TB] dut_022 failed: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", c=" << int(s.c) << ", d=" << int(s.d)
                      << ", expected out1/out2=" << int(exp1) << "/" << int(exp2)
                      << ", got " << int(dut->out1) << "/" << int(dut->out2) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_022 passed: out1=(a&b)|(c&d), out2=(a|b)&(c|d)" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
