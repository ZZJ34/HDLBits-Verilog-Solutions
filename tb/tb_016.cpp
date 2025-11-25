#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_016.h"

struct Stimulus { uint8_t a,b,c,d,e,f; };

static inline uint32_t pack32(uint8_t a, uint8_t b, uint8_t c,
                              uint8_t d, uint8_t e, uint8_t f) {
    // Right-hand concat: {a,b,c,d,e,f,2'b11}
    uint32_t r = 0;
    r |= (uint32_t(a & 0x1Fu) << 27);
    r |= (uint32_t(b & 0x1Fu) << 22);
    r |= (uint32_t(c & 0x1Fu) << 17);
    r |= (uint32_t(d & 0x1Fu) << 12);
    r |= (uint32_t(e & 0x1Fu) << 7);
    r |= (uint32_t(f & 0x1Fu) << 2);
    r |= 0x3u; // 2'b11
    return r;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_016>(context.get());

    const std::array<Stimulus, 4> stimuli{{
        {0,0,0,0,0,0},
        {31,31,31,31,31,31},
        {5,9,17,3,24,12},
        {0,0,0,0,0,0},
    }};

    for (const auto &s : stimuli) {
        dut->a = s.a; dut->b = s.b; dut->c = s.c; dut->d = s.d; dut->e = s.e; dut->f = s.f;
        dut->eval();

        const uint32_t r = pack32(s.a, s.b, s.c, s.d, s.e, s.f);
        const uint8_t exp_w = static_cast<uint8_t>((r >> 24) & 0xFFu);
        const uint8_t exp_x = static_cast<uint8_t>((r >> 16) & 0xFFu);
        const uint8_t exp_y = static_cast<uint8_t>((r >> 8) & 0xFFu);
        const uint8_t exp_z = static_cast<uint8_t>((r >> 0) & 0xFFu);

        if (dut->w != exp_w || dut->x != exp_x || dut->y != exp_y || dut->z != exp_z) {
            std::cerr << "[TB] dut_016 failed: a..f="
                      << int(s.a) << "," << int(s.b) << "," << int(s.c) << ","
                      << int(s.d) << "," << int(s.e) << "," << int(s.f)
                      << ", expected wxyz=" << int(exp_w) << "," << int(exp_x)
                      << "," << int(exp_y) << "," << int(exp_z)
                      << ", got " << int(dut->w) << "," << int(dut->x)
                      << "," << int(dut->y) << "," << int(dut->z) << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_016 passed: concatenation mapping verified" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

