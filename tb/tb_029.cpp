#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_029.h"

struct Stimulus { uint8_t a; uint8_t b; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_029>(context.get());

    // Toggle pattern to exercise all input combinations and transitions
    const std::array<Stimulus, 5> stimuli{{ {0,0}, {1,0}, {1,1}, {0,1}, {0,0} }};

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint8_t exp = static_cast<uint8_t>((s.a & s.b) & 0x1u);
        if (dut->out_assign != exp || dut->out_alwaysblock != exp) {
            std::cerr << "[TB] dut_029 failed: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", expected both outputs=" << int(exp)
                      << ", got out_assign=" << int(dut->out_assign)
                      << ", out_alwaysblock=" << int(dut->out_alwaysblock) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_029 passed: out_assign==out_alwaysblock==a&b" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

