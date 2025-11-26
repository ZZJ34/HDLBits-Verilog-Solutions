#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_033.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_033>(context.get());

    // Goal: 100% coverage. Toggle every input bit 0->1 and 1->0,
    // hit all case items and the default, and exercise output bits both ways.
    auto check = [&](uint8_t exp, const char *ctx) {
        if (dut->out != exp) {
            std::cerr << "[TB] dut_033 failed(" << ctx << ") sel=" << int(dut->sel)
                      << ": expected=" << int(exp) << ", got=" << int(dut->out)
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Init all data inputs low so we have a clean 0 baseline.
    dut->data0 = 0x0; dut->data1 = 0x0; dut->data2 = 0x0; dut->data3 = 0x0;
    dut->data4 = 0x0; dut->data5 = 0x0;

    // Explicitly toggle sel bits both ways: 000 -> 111 -> 000
    dut->sel = 0; dut->eval();
    dut->sel = 7; dut->eval(); // default branch; out must be 0
    check(0x0, "default-7");
    dut->sel = 0; dut->eval(); // return to 000
    check(0x0, "select-0-init");

    // For each data input, while selected:
    //  - observe 0x0
    //  - drive 0xF (forces all bits 0->1)
    //  - drive back to 0x0 (forces 1->0)
    for (int i = 0; i <= 5; ++i) {
        dut->sel = static_cast<uint8_t>(i);
        dut->eval();
        check(0x0, "observe-zero");

        switch (i) {
            case 0: dut->data0 = 0xF; break;
            case 1: dut->data1 = 0xF; break;
            case 2: dut->data2 = 0xF; break;
            case 3: dut->data3 = 0xF; break;
            case 4: dut->data4 = 0xF; break;
            case 5: dut->data5 = 0xF; break;
        }
        dut->eval();
        check(0xF, "drive-ones");

        switch (i) {
            case 0: dut->data0 = 0x0; break;
            case 1: dut->data1 = 0x0; break;
            case 2: dut->data2 = 0x0; break;
            case 3: dut->data3 = 0x0; break;
            case 4: dut->data4 = 0x0; break;
            case 5: dut->data5 = 0x0; break;
        }
        dut->eval();
        check(0x0, "drive-zeros");
    }

    // Re-check the default path explicitly at sel=6 (also not in 0..5)
    dut->sel = 6; dut->eval();
    check(0x0, "default-6");

    std::cout << "[TB] dut_033 passed: 6:1 mux selects correct data; default hit; all inputs toggled" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
