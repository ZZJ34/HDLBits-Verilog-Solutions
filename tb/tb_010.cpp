#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_010.h"

struct Stimulus {
    uint8_t p1a, p1b, p1c, p1d, p1e, p1f;
    uint8_t p2a, p2b, p2c, p2d;
};

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_010>(context.get());

    // Sequence toggles every input 0->1 and 1->0 and exercises each OR term
    // individually and simultaneously to achieve full coverage.
    const std::array<Stimulus, 23> stimuli{{
        // baseline
        {0,0,0, 0,0,0,  0,0,0,0},
        // Drive p1a..p1c to make w3=1, then drop p1a
        {1,0,0, 0,0,0,  0,0,0,0},
        {1,1,0, 0,0,0,  0,0,0,0},
        {1,1,1, 0,0,0,  0,0,0,0}, // w3=1
        {0,1,1, 0,0,0,  0,0,0,0},
        // Drive p1d..p1f to make w4=1, then bring p1a back to get both w3&w4=1
        {0,1,1, 1,0,0,  0,0,0,0},
        {0,1,1, 1,1,0,  0,0,0,0},
        {0,1,1, 1,1,1,  0,0,0,0}, // w4=1
        {1,1,1, 1,1,1,  0,0,0,0}, // both w3 & w4 = 1
        // Exercise p2 OR-terms: w1 then w2
        {1,1,1, 1,1,1,  1,0,0,0},
        {1,1,1, 1,1,1,  1,1,0,0}, // w1=1
        {1,1,1, 1,1,1,  0,1,0,0},
        {1,1,1, 1,1,1,  0,1,1,0},
        {1,1,1, 1,1,1,  0,1,1,1}, // w2=1
        {1,1,1, 1,1,1,  0,1,0,1},
        {1,1,1, 1,1,1,  0,1,0,0},
        {1,1,1, 1,1,1,  0,0,0,0},
        // Return all p2 to 0 and toggle remaining p1 signals low
        {1,1,1, 1,1,1,  0,0,0,0},
        {1,0,1, 1,1,1,  0,0,0,0},
        {1,0,0, 1,1,1,  0,0,0,0},
        {1,0,0, 0,1,1,  0,0,0,0},
        {1,0,0, 0,0,1,  0,0,0,0},
        {0,0,0, 0,0,0,  0,0,0,0},
    }};

    for (const auto &s : stimuli)
    {
        dut->p1a = s.p1a;
        dut->p1b = s.p1b;
        dut->p1c = s.p1c;
        dut->p1d = s.p1d;
        dut->p1e = s.p1e;
        dut->p1f = s.p1f;
        dut->p2a = s.p2a;
        dut->p2b = s.p2b;
        dut->p2c = s.p2c;
        dut->p2d = s.p2d;
        dut->eval();

        const uint8_t w1 = static_cast<uint8_t>((s.p2a & s.p2b) & 0x1U);
        const uint8_t w2 = static_cast<uint8_t>((s.p2c & s.p2d) & 0x1U);
        const uint8_t w3 = static_cast<uint8_t>((s.p1a & s.p1b & s.p1c) & 0x1U);
        const uint8_t w4 = static_cast<uint8_t>((s.p1d & s.p1e & s.p1f) & 0x1U);
        const uint8_t exp_p1y = static_cast<uint8_t>((w3 | w4) & 0x1U);
        const uint8_t exp_p2y = static_cast<uint8_t>((w1 | w2) & 0x1U);

        if (dut->p1y != exp_p1y || dut->p2y != exp_p2y)
        {
            std::cerr << "[TB] dut_010 failed: "
                      << "p1=[" << int(s.p1a) << int(s.p1b) << int(s.p1c)
                      << int(s.p1d) << int(s.p1e) << int(s.p1f) << "] "
                      << "p2=[" << int(s.p2a) << int(s.p2b) << int(s.p2c)
                      << int(s.p2d) << "] "
                      << "expected p1y/p2y=" << int(exp_p1y) << "/" << int(exp_p2y)
                      << ", got " << int(dut->p1y) << "/" << int(dut->p2y)
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_010 passed: p1y == (p1a&p1b&p1c)|(p1d&p1e&p1f) and p2y == (p2a&p2b)|(p2c&p2d)" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

