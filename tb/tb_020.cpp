#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_020.h"

struct Stimulus { uint8_t a; uint8_t b; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_020>(context.get());

    // Walk all input combinations and toggle each bit 0->1->0.
    const std::array<Stimulus, 5> stimuli{{ {0,0}, {1,0}, {1,1}, {0,1}, {0,0} }};

    for (const auto &s : stimuli) {
        dut->a = s.a;
        dut->b = s.b;
        dut->eval();

        const uint8_t expected = static_cast<uint8_t>((s.a ^ s.b) & 0x1u);
        if (dut->out != expected) {
            std::cerr << "[TB] dut_020 failed: a=" << int(s.a) << ", b=" << int(s.b)
                      << ", expected out=" << int(expected) << ", got " << int(dut->out) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_020 passed: out == a^b through mod_a" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
