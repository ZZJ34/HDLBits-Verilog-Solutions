#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_107.h"

struct Stim107 {
    uint8_t areset;
    uint8_t load;
    uint8_t ena;
    uint8_t data;
};

static inline void tick(Vdut_107 *dut, VerilatedContext *ctx) {
    dut->clk = 0;
    dut->eval();
    ctx->timeInc(1);
    dut->clk = 1;
    dut->eval();
    ctx->timeInc(1);
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_107>(ctx.get());

    uint8_t q_model = 0;

    dut->clk = 0;
    dut->areset = 1;
    dut->load = 0;
    dut->ena = 0;
    dut->data = 0;
    dut->eval();

    const Stim107 pattern[] = {
        {1u, 0u, 0u, 0u}, // async reset -> q=0
        {0u, 1u, 0u, 0xAu}, // load 0xA
        {0u, 0u, 1u, 0x0u}, // shift
        {0u, 0u, 1u, 0x0u},
        {0u, 0u, 0u, 0x0u}, // hold
        {1u, 0u, 0u, 0x0u}, // async reset again
        {0u, 1u, 0u, 0x5u}, // load 0x5
        {0u, 0u, 1u, 0x0u}, // shift
    };

    for (const auto &s : pattern) {
        dut->areset = s.areset;
        dut->load = s.load;
        dut->ena = s.ena;
        dut->data = s.data;
        tick(dut.get(), ctx.get());

        if (s.areset) {
            q_model = 0;
        } else if (s.load) {
            q_model = s.data & 0xFu;
        } else if (s.ena) {
            q_model = static_cast<uint8_t>((q_model >> 1) & 0x7u);
        }

        if (dut->q != q_model) {
            std::cerr << "[TB] dut_107 failed: areset=" << int(s.areset)
                      << " load=" << int(s.load)
                      << " ena=" << int(s.ena)
                      << " data=0x" << std::hex << int(s.data)
                      << " expected q=0x" << int(q_model)
                      << " got 0x" << int(dut->q) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_107 passed: loadable right shift register with async reset" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

