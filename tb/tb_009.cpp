#include <array>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_009.h"

struct Stimulus {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
};

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_009>(context.get());

    // Toggle each input 0->1 and 1->0, and exercise w1, w2, and both set.
    const std::array<Stimulus, 9> stimuli{{
        {0U, 0U, 0U, 0U}, // baseline
        {1U, 0U, 0U, 0U}, // a: 0->1
        {1U, 1U, 0U, 0U}, // b: 0->1, w1=1
        {1U, 1U, 1U, 0U}, // c: 0->1
        {1U, 1U, 1U, 1U}, // d: 0->1, w1=1,w2=1
        {0U, 1U, 1U, 1U}, // a: 1->0
        {0U, 0U, 1U, 1U}, // b: 1->0, w2=1
        {0U, 0U, 0U, 1U}, // c: 1->0
        {0U, 0U, 0U, 0U}, // d: 1->0
    }};

    for (const auto &s : stimuli)
    {
        dut->a = s.a;
        dut->b = s.b;
        dut->c = s.c;
        dut->d = s.d;
        dut->eval();

        const uint8_t expected_out = static_cast<uint8_t>(((s.a & s.b) | (s.c & s.d)) & 0x1U);
        const uint8_t expected_out_n = static_cast<uint8_t>((~expected_out) & 0x1U);
        if (dut->out != expected_out || dut->out_n != expected_out_n)
        {
            std::cerr << "[TB] dut_009 failed: a=" << static_cast<int>(s.a)
                      << ", b=" << static_cast<int>(s.b)
                      << ", c=" << static_cast<int>(s.c)
                      << ", d=" << static_cast<int>(s.d)
                      << ", expected out/out_n=" << static_cast<int>(expected_out)
                      << "/" << static_cast<int>(expected_out_n)
                      << ", got " << static_cast<int>(dut->out)
                      << "/" << static_cast<int>(dut->out_n) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_009 passed: out == (a&b)|(c&d) and out_n == ~out for all stimuli" << std::endl;

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

