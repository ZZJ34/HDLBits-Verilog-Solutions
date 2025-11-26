#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_004.h"

struct Stimulus
{
    uint8_t a;
    uint8_t b;
    uint8_t c;
};

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_004>(context.get());

    // Order stimuli to force each input to toggle 0->1 and 1->0 for full coverage.
    const std::array<Stimulus, 9> stimuli{{
        {0U, 0U, 0U}, // baseline
        {1U, 0U, 0U}, // a: 0->1
        {1U, 1U, 0U}, // b: 0->1
        {0U, 1U, 0U}, // a: 1->0
        {0U, 1U, 1U}, // c: 0->1
        {0U, 0U, 1U}, // b: 1->0
        {1U, 0U, 0U}, // c: 1->0, a: 0->1
        {1U, 1U, 0U}, // b: 0->1
        {0U, 0U, 0U}, // return to all zero
    }};

    for (const auto &stim : stimuli)
    {
        dut->a = stim.a;
        dut->b = stim.b;
        dut->c = stim.c;
        dut->eval();

        if (dut->w != stim.a || dut->x != stim.b || dut->y != stim.b ||
            dut->z != stim.c)
        {
            std::cerr << "[TB] dut_004 failed: "
                      << "a=" << static_cast<int>(stim.a) << ", "
                      << "b=" << static_cast<int>(stim.b) << ", "
                      << "c=" << static_cast<int>(stim.c) << " "
                      << "=> wxyz="
                      << static_cast<int>(dut->w)
                      << static_cast<int>(dut->x)
                      << static_cast<int>(dut->y)
                      << static_cast<int>(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_004 passed: outputs mirror inputs for all stimuli"
              << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0')
    {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
