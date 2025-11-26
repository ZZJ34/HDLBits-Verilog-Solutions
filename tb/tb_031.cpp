#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_031.h"

struct Stimulus { uint8_t a, b, s1, s2; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_031>(context.get());

    // Toggle all inputs and hit both mux paths
    const std::array<Stimulus, 9> stimuli{{
        {0,0,0,0}, {1,0,0,0}, {1,1,0,0}, {1,1,1,0},
        {1,1,1,1}, {0,1,1,1}, {0,1,0,1}, {0,1,0,0}, {0,0,0,0},
    }};

    for (const auto &s : stimuli) {
        dut->a = s.a; dut->b = s.b; dut->sel_b1 = s.s1; dut->sel_b2 = s.s2;
        dut->eval();
        const uint8_t exp = (s.s1 && s.s2) ? s.b : s.a;
        if (dut->out_assign != exp || dut->out_always != exp) {
            std::cerr << "[TB] dut_031 failed: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", s1/s2=" << int(s.s1) << "/" << int(s.s2)
                      << ", expected=" << int(exp)
                      << ", got assign/always=" << int(dut->out_assign)
                      << "/" << int(dut->out_always) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_031 passed: mux behavior matches in both styles" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

