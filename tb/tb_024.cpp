#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_024.h"

struct Stimulus { uint8_t d; uint8_t sel; };

static void tick(Vdut_024 *dut, VerilatedContext *context) {
    dut->clk = 0;
    dut->eval();
    context->timeInc(1);
    dut->clk = 1;
    dut->eval();
    context->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_024>(context.get());

    // Walk all sel cases multiple times while shifting varied data to toggle every bit
    // through w1/w2/w3 and exercise each mux arm.
    const std::array<Stimulus, 12> stimuli{{
        {0x00u, 0u},
        {0xFFu, 0u},
        {0x00u, 0u},
        {0xFFu, 0u},
        {0x00u, 0u},
        {0xAAu, 1u},
        {0x55u, 2u},
        {0x0Fu, 3u},
        {0xF0u, 1u},
        {0x00u, 2u},
        {0xFFu, 3u},
        {0x00u, 3u},
    }};

    uint8_t w1 = 0u, w2 = 0u;

    for (const auto &s : stimuli) {
        dut->d = s.d;
        dut->sel = s.sel;
        tick(dut.get(), context.get());

        const uint8_t next_w1 = s.d;
        const uint8_t next_w2 = w1;
        const uint8_t next_w3 = w2;

        uint8_t expected_q = 0u;
        switch (s.sel & 0x3u) {
            case 0u: expected_q = s.d; break;
            case 1u: expected_q = next_w1; break;
            case 2u: expected_q = next_w2; break;
            case 3u: expected_q = next_w3; break;
        }

        if (dut->q != expected_q) {
            std::cerr << "[TB] dut_024 failed: d=0x" << std::hex << int(s.d)
                      << std::dec << ", sel=" << int(s.sel)
                      << ", expected q=0x" << std::hex << int(expected_q)
                      << std::dec << ", got 0x" << std::hex << int(dut->q) << std::dec
                      << std::endl;
            return EXIT_FAILURE;
        }

        w1 = next_w1;
        w2 = next_w2;
    }

    std::cout << "[TB] dut_024 passed: q taps d/w1/w2/w3 per sel" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
