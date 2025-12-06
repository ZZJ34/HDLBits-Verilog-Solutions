#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_141.h"

static inline void tick(Vdut_141 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_141>(ctx.get());

    enum { A = 1, B = 2 };
    uint8_t state = A;

    // Apply async reset
    dut->areset = 1;
    dut->x = 0;
    tick(dut.get(), ctx.get());
    dut->areset = 0;
    state = A;

    auto step = [&](uint8_t x, const char *ctx_str) {
        // Drive input and check output based on current state.
        dut->x = x & 1u;
        dut->eval();

        uint8_t z_exp = 0;
        uint8_t next = state;
        switch (state) {
            case A:
                if (x) {
                    next = B;
                    z_exp = 1;
                } else {
                    next = A;
                    z_exp = 0;
                }
                break;
            case B:
                next = B;
                z_exp = x ? 0 : 1;
                break;
            default:
                next = A;
                z_exp = 0;
                break;
        }

        if (dut->z != z_exp) {
            std::cerr << "[TB] dut_141 failed (" << ctx_str << "): "
                      << "state=" << int(state) << " x=" << int(x)
                      << " expected z=" << int(z_exp)
                      << " got " << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }

        // Advance clock to update internal state.
        tick(dut.get(), ctx.get());
        state = next;
        return EXIT_SUCCESS;
    };

    // Sequence to cover transitions and toggle z in both states.
    if (step(0, "A,x=0") != EXIT_SUCCESS) return EXIT_FAILURE; // stay A, z=0
    if (step(1, "A,x=1_to_B") != EXIT_SUCCESS) return EXIT_FAILURE; // A->B, z=1
    if (step(1, "B,x=1") != EXIT_SUCCESS) return EXIT_FAILURE; // stay B, z=0
    if (step(0, "B,x=0") != EXIT_SUCCESS) return EXIT_FAILURE; // stay B, z=1

    // Pulse reset again to toggle areset and return to A.
    dut->areset = 1;
    dut->eval();
    tick(dut.get(), ctx.get());
    dut->areset = 0;
    state = A;

    // One more short sequence to exercise both paths again.
    if (step(1, "A,x=1_second") != EXIT_SUCCESS) return EXIT_FAILURE;
    if (step(0, "B,x=0_second") != EXIT_SUCCESS) return EXIT_FAILURE;

    std::cout << "[TB] dut_141 passed: simple 2-state FSM with z behavior" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

