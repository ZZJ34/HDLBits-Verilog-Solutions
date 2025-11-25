#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_019.h"

struct Stimulus { uint8_t a,b,c,d,e; };

static inline uint32_t build_w1(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    // {{5{a}}, {5{b}}, {5{c}}, {5{d}}, {5{e}}}
    uint32_t v = 0;
    for (int i = 0; i < 5; ++i) { v = (v << 1) | (a & 1u); }
    for (int i = 0; i < 5; ++i) { v = (v << 1) | (b & 1u); }
    for (int i = 0; i < 5; ++i) { v = (v << 1) | (c & 1u); }
    for (int i = 0; i < 5; ++i) { v = (v << 1) | (d & 1u); }
    for (int i = 0; i < 5; ++i) { v = (v << 1) | (e & 1u); }
    return v & 0x1FFFFFFu; // 25 bits
}

static inline uint32_t build_w2(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    // {5{a,b,c,d,e}}
    uint32_t v = 0;
    for (int rep = 0; rep < 5; ++rep) {
        v = (v << 1) | (a & 1u);
        v = (v << 1) | (b & 1u);
        v = (v << 1) | (c & 1u);
        v = (v << 1) | (d & 1u);
        v = (v << 1) | (e & 1u);
    }
    return v & 0x1FFFFFFu; // 25 bits
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_019>(context.get());

    const std::array<Stimulus, 7> stimuli{{
        {0,0,0,0,0},
        {1,0,0,0,0},
        {1,1,0,0,0},
        {1,1,1,0,0},
        {1,1,1,1,0},
        {1,1,1,1,1},
        {0,0,0,0,0},
    }};

    for (const auto &s : stimuli) {
        dut->a = s.a; dut->b = s.b; dut->c = s.c; dut->d = s.d; dut->e = s.e;
        dut->eval();

        const uint32_t w1 = build_w1(s.a, s.b, s.c, s.d, s.e);
        const uint32_t w2 = build_w2(s.a, s.b, s.c, s.d, s.e);
        const uint32_t expected = (~(w1 ^ w2)) & 0x1FFFFFFu; // 25 bits
        const uint32_t out = static_cast<uint32_t>(dut->out);
        if ((out & 0x1FFFFFFu) != expected) {
            std::cerr << "[TB] dut_019 failed: a=" << int(s.a) << ",b=" << int(s.b)
                      << ",c=" << int(s.c) << ",d=" << int(s.d) << ",e=" << int(s.e)
                      << ", expected out=0x" << std::hex << expected << std::dec
                      << ", got 0x" << std::hex << (out & 0x1FFFFFFu) << std::dec << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_019 passed: replicated structure and bitwise ops verified" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

