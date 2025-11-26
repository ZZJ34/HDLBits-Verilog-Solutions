#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_037.h"

struct Stimulus { uint8_t a,b,c,d; };

static inline uint8_t umin(uint8_t x, uint8_t y) { return x < y ? x : y; }

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_037>(context.get());

    auto drive_and_check = [&](uint8_t aa, uint8_t bb, uint8_t cc, uint8_t dd, const char* ctx) {
        dut->a = aa; dut->b = bb; dut->c = cc; dut->d = dd;
        dut->eval();
        const uint8_t expected = umin(umin(aa, bb), umin(cc, dd));
        if (dut->min != expected) {
            std::cerr << "[TB] dut_037 failed(" << ctx << ") a/b/c/d="
                      << int(aa) << "/" << int(bb) << "/" << int(cc) << "/" << int(dd)
                      << ", expected min=" << int(expected)
                      << ", got " << int(dut->min) << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    // Basic sanity cases
    drive_and_check(1,2,3,4,   "sanity1");
    drive_and_check(5,2,1,7,   "sanity2");
    drive_and_check(9,8,10,7,  "sanity3");
    drive_and_check(0,255,128,64, "sanity4");

    // Bidirectional toggle goals: Every bit of a,b,c,d,w1,w2,w3,min should toggle
    // 0->1 and 1->0 at least once. We create four phases so w1/w2/w3/min follow
    // each respective input while it toggles 00->FF->00->FF->00.

    // Phase A: propagate 'a' to min (b=c=d=0xFF so min==a)
    for (int rep = 0; rep < 2; ++rep) {
        drive_and_check(0x00, 0xFF, 0xFF, 0xFF, "A a=00");
        drive_and_check(0xFF, 0xFF, 0xFF, 0xFF, "A a=FF");
    }
    drive_and_check(0x00, 0xFF, 0xFF, 0xFF, "A a=00 end");

    // Phase B: propagate 'b' to min (a=c=d=0xFF so min==b)
    for (int rep = 0; rep < 2; ++rep) {
        drive_and_check(0xFF, 0x00, 0xFF, 0xFF, "B b=00");
        drive_and_check(0xFF, 0xFF, 0xFF, 0xFF, "B b=FF");
    }
    drive_and_check(0xFF, 0x00, 0xFF, 0xFF, "B b=00 end");

    // Phase C: propagate 'c' to min (a=b=d=0xFF so min==c)
    for (int rep = 0; rep < 2; ++rep) {
        drive_and_check(0xFF, 0xFF, 0x00, 0xFF, "C c=00");
        drive_and_check(0xFF, 0xFF, 0xFF, 0xFF, "C c=FF");
    }
    drive_and_check(0xFF, 0xFF, 0x00, 0xFF, "C c=00 end");

    // Phase D: propagate 'd' to min (a=b=c=0xFF so min==d)
    for (int rep = 0; rep < 2; ++rep) {
        drive_and_check(0xFF, 0xFF, 0xFF, 0x00, "D d=00");
        drive_and_check(0xFF, 0xFF, 0xFF, 0xFF, "D d=FF");
    }
    drive_and_check(0xFF, 0xFF, 0xFF, 0x00, "D d=00 end");

    std::cout << "[TB] dut_037 passed: min(a,b,c,d) computed via chained mins" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
