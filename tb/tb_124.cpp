#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_124.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto ctx = std::make_unique<VerilatedContext>();
    ctx->traceEverOn(false);

    auto dut = std::make_unique<Vdut_124>(ctx.get());

    // Exhaustive over 4-bit one-hot state and input in.
    for (uint8_t state = 0; state < 4; ++state) {
        uint8_t state_vec = static_cast<uint8_t>(1u << state);
        for (uint8_t in = 0; in < 2; ++in) {
            dut->state = state_vec;
            dut->in = in;
            dut->eval();

            uint8_t next_state_vec = 0;
            switch (state) {
                case 0: // A
                    next_state_vec = in ? (1u << 1) : (1u << 0);
                    break;
                case 1: // B
                    next_state_vec = in ? (1u << 1) : (1u << 2);
                    break;
                case 2: // C
                    next_state_vec = in ? (1u << 3) : (1u << 0);
                    break;
                case 3: // D
                    next_state_vec = in ? (1u << 1) : (1u << 2);
                    break;
            }
            uint8_t out_exp = (state == 3) ? 1u : 0u;

            if (dut->next_state != next_state_vec || dut->out != out_exp) {
                std::cerr << "[TB] dut_124 failed: state=" << int(state)
                          << " in=" << int(in)
                          << " expected next_state_vec=0x" << std::hex << int(next_state_vec)
                          << " out=" << std::dec << int(out_exp)
                          << " got next_state=0x" << std::hex << int(dut->next_state)
                          << " out=" << std::dec << int(dut->out) << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    std::cout << "[TB] dut_124 passed: one-hot encoded FSM combinational block" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

