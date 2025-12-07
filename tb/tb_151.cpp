#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_151.h"

static inline void tick(Vdut_151 *dut, VerilatedContext *ctx) {
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

    auto dut = std::make_unique<Vdut_151>(ctx.get());

    // Enumerate states for a simple model.
    enum {
        A = 0, F1 = 1, TMP0 = 2, TMP1 = 3, TMP2 = 4,
        G1 = 5, G1P = 6, TMP3 = 7, G0P = 8
    };
    int state = A;

    auto apply_reset = [&]() {
        dut->resetn = 0;
        dut->x = 0;
        dut->y = 0;
        tick(dut.get(), ctx.get());
        state = A;
        dut->resetn = 1;
    };

    auto step = [&](uint8_t x, uint8_t y, const char *ctx_str) {
        dut->x = x & 1u;
        dut->y = y & 1u;

        // Compute next state (ignoring async reset here; handled in apply_reset).
        int next = state;
        switch (state) {
            case A:
                next = dut->resetn ? F1 : A;
                break;
            case F1:
                next = TMP0;
                break;
            case TMP0:
                next = x ? TMP1 : TMP0;
                break;
            case TMP1:
                next = x ? TMP1 : TMP2;
                break;
            case TMP2:
                next = x ? G1 : TMP0;
                break;
            case G1:
                next = y ? G1P : TMP3;
                break;
            case TMP3:
                next = y ? G1P : G0P;
                break;
            case G1P:
                next = dut->resetn ? G1P : A;
                break;
            case G0P:
                next = dut->resetn ? G0P : A;
                break;
        }

        tick(dut.get(), ctx.get());
        state = next;

        // Expected outputs depend on next_state (as in RTL).
        uint8_t f_exp = 0;
        uint8_t g_exp = 0;
        switch (state) {
            case F1:  f_exp = 1; g_exp = 0; break;
            case G1:  f_exp = 0; g_exp = 1; break;
            case TMP3:g_exp = 1; break;
            case G1P: g_exp = 1; break;
            case G0P: g_exp = 0; break;
            default:  f_exp = 0; g_exp = 0; break;
        }

        if (dut->f != f_exp || dut->g != g_exp) {
            std::cerr << "[TB] dut_151 failed (" << ctx_str
                      << "): state=" << state
                      << " x=" << int(x) << " y=" << int(y)
                      << " expected f=" << int(f_exp)
                      << " g=" << int(g_exp)
                      << " got f=" << int(dut->f)
                      << " g=" << int(dut->g) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Scenario 1: path to g1p via y=1, exercising g1 and g1p cases.
    apply_reset();
    // A -> f1 -> tmp0
    step(0, 0, "S1.A_to_F1");        // A -> f1
    step(0, 0, "S1.F1_to_TMP0");     // f1 -> tmp0
    // tmp0 -> tmp1 -> tmp2
    step(1, 0, "S1.TMP0_to_TMP1");   // tmp0 -> tmp1
    step(0, 0, "S1.TMP1_to_TMP2");   // tmp1 -> tmp2
    // tmp2 -> g1 (x=1), then g1 -> g1p (y=1)
    step(1, 0, "S1.TMP2_to_G1");     // tmp2 -> g1
    step(0, 1, "S1.G1_to_G1P");      // g1 -> g1p
    // Hold in g1p
    step(0, 1, "S1.G1P_hold");

    // Scenario 2: path through g1 -> tmp3 -> g1p with y=1, covering tmp3->g1p.
    apply_reset();
    step(0, 0, "S2.A_to_F1");
    step(0, 0, "S2.F1_to_TMP0");
    step(1, 0, "S2.TMP0_to_TMP1");
    step(0, 0, "S2.TMP1_to_TMP2");
    step(1, 0, "S2.TMP2_to_G1_y0");  // g1, y=0 -> next tmp3
    step(0, 0, "S2.G1_to_TMP3");     // now in tmp3 (y=0)
    step(0, 1, "S2.TMP3_to_G1P");    // tmp3, y=1 -> g1p
    step(0, 1, "S2.G1P_hold2");

    // Scenario 3: exercise resetn branches in g1p and g0p (if(~resetn)).
    apply_reset();
    // Drive into g1p
    step(0, 0, "S3.A_to_F1");
    step(0, 0, "S3.F1_to_TMP0");
    step(1, 0, "S3.TMP0_to_TMP1");
    step(0, 0, "S3.TMP1_to_TMP2");
    step(1, 0, "S3.TMP2_to_G1");
    step(0, 1, "S3.G1_to_G1P");
    // Now assert resetn low while in g1p
    dut->resetn = 0;
    tick(dut.get(), ctx.get());
    dut->resetn = 1;
    state = A;

    // Drive into g0p and reset there as well.
    apply_reset();
    step(0, 0, "S3b.A_to_F1");
    step(0, 0, "S3b.F1_to_TMP0");
    step(1, 0, "S3b.TMP0_to_TMP1");
    step(0, 0, "S3b.TMP1_to_TMP2");
    step(1, 0, "S3b.TMP2_to_G1");
    step(0, 0, "S3b.G1_to_TMP3");
    step(0, 0, "S3b.TMP3_to_G0P");
    dut->resetn = 0;
    tick(dut.get(), ctx.get());
    dut->resetn = 1;
    state = A;

    std::cout << "[TB] dut_151 passed: FSM with f/g outputs" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
