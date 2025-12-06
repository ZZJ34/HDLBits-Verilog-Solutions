#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_134.h"

static inline void tick(Vdut_134 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_134>(ctx.get());

    enum { BYTE1=0, BYTE2=1, BYTE3=2, DONE=3 };
    uint8_t state = BYTE1;
    uint32_t data_model = 0;
    dut->clk = 0;

    // Helper: apply synchronous reset and align the local model.
    auto apply_reset = [&]() {
        dut->reset = 1;
        dut->in = 0;
        dut->eval();
        tick(dut.get(), ctx.get());
        state = BYTE1;
        data_model = 0;
        dut->reset = 0;
    };

    apply_reset();

    auto step = [&](uint8_t in_val) {
        // Model state
        uint8_t next;
        switch (state) {
            case BYTE1: next = (in_val & 0x8u) ? BYTE2 : BYTE1; break;
            case BYTE2: next = BYTE3; break;
            case BYTE3: next = DONE; break;
            case DONE:  next = (in_val & 0x8u) ? BYTE2 : BYTE1; break;
            default:    next = BYTE1; break;
        }

        dut->in = in_val;
        tick(dut.get(), ctx.get());

        // Datapath: data <= {data[15:8], data[7:0], in};
        data_model = ((data_model << 8) & 0xFFFF00u) | in_val;
        state = next;

        uint8_t done_exp = (state == DONE) ? 1u : 0u;
        uint32_t out_exp = done_exp ? data_model : 0u;

        if (dut->done != done_exp || dut->out_bytes != out_exp) {
            std::cerr << "[TB] dut_134 failed: in=0x" << std::hex << int(in_val)
                      << " expected done=" << std::dec << int(done_exp)
                      << " out=0x" << std::hex << out_exp
                      << " got done=" << std::dec << int(dut->done)
                      << " out=0x" << std::hex << dut->out_bytes << std::dec << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Phase 1: sweep input space 0..255 to exercise FSM transitions and datapath.
    for (int v = 0; v < 256; ++v) {
        step(static_cast<uint8_t>(v));
    }

    // Phase 2: apply another reset (to fully toggle reset) and sweep again with
    // reverse patterns to further toggle all internal and output bits.
    apply_reset();
    for (int v = 255; v >= 0; --v) {
        step(static_cast<uint8_t>(v));
    }

    std::cout << "[TB] dut_134 passed: exhaustive byte sweeps with coverage-focused patterns" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
