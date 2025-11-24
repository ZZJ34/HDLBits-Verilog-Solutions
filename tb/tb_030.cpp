#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_030.h"

struct Stimulus { uint8_t a; uint8_t b; };

static inline void tick(Vdut_030 *dut) {
    // rising edge
    dut->clk = 0; dut->eval();
    dut->clk = 1; dut->eval();
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_030>(context.get());

    dut->clk = 0; dut->a = 0; dut->b = 0; dut->eval();

    // Exercise all input combinations across multiple rising edges
    const std::array<Stimulus, 5> stimuli{{ {0,0}, {1,0}, {1,1}, {0,1}, {0,0} }};

    for (const auto &s : stimuli) {
        // Set inputs, check comb outputs immediately
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint8_t comb = static_cast<uint8_t>((s.a ^ s.b) & 0x1u);
        if (dut->out_assign != comb || dut->out_always_comb != comb) {
            std::cerr << "[TB] dut_030 comb mismatch: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", expected comb=" << int(comb)
                      << ", got out_assign/comb=" << int(dut->out_assign)
                      << "/" << int(dut->out_always_comb) << std::endl;
            return EXIT_FAILURE;
        }

        // Clock the design; FF should capture comb value at rising edge
        tick(dut.get());

        if (dut->out_always_ff != comb) {
            std::cerr << "[TB] dut_030 ff mismatch after tick: a=" << int(s.a)
                      << ", b=" << int(s.b) << ", expected ff=" << int(comb)
                      << ", got " << int(dut->out_always_ff) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_030 passed: comb and ff XOR outputs correct across edges" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

