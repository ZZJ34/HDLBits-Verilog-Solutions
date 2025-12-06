#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_123.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_123>(ctx.get());

    // Exhaustive test over state (A..D) and in=0/1.
    for (uint8_t state = 0; state < 4; ++state) {
        for (uint8_t in = 0; in < 2; ++in) {
            dut->state = state;
            dut->in = in;
            dut->eval();
            uint8_t next_exp = 0;
            switch (state) {
                case 0: next_exp = (in ? 1 : 0); break; // A
                case 1: next_exp = (in ? 1 : 2); break; // B
                case 2: next_exp = (in ? 3 : 0); break; // C
                case 3: next_exp = (in ? 1 : 2); break; // D
            }
            uint8_t out_exp = (state == 3) ? 1u : 0u;
            if (dut->next_state != next_exp || dut->out != out_exp) {
                std::cerr << "[TB] dut_123 failed: state=" << int(state)
                          << " in=" << int(in)
                          << " expected next=" << int(next_exp)
                          << " out=" << int(out_exp)
                          << " got next=" << int(dut->next_state)
                          << " out=" << int(dut->out) << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    std::cout << "[TB] dut_123 passed: Mealy combinational FSM block" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

