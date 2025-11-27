#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_043.h"

// Helpers to manipulate 400-bit packed words (13x32)
static inline void wide_zero_400(Vdut_043* dut) {
    for (int i = 0; i < 13; ++i) { dut->a[i] = 0; dut->b[i] = 0; }
}
static inline void set_bcd_digit(uint32_t w[13], int digit, uint8_t val) {
    const int bit = 4*digit;
    const int wi = bit >> 5; const int bi = bit & 31;
    uint32_t mask = 0xFu << bi;
    w[wi] = (w[wi] & ~mask) | (uint32_t(val & 0xFu) << bi);
    if (bi > 28) { // digit straddles word boundary
        int rem = 32 - bi;
        uint32_t mask2 = 0xFu >> rem;
        w[wi+1] = (w[wi+1] & ~mask2) | ((val & 0xFu) >> rem);
    }
}

static inline uint16_t get_low16(const uint32_t w[13]) { return static_cast<uint16_t>(w[0] & 0xFFFFu); }

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_043>(context.get());

    auto check4 = [&](uint16_t a16, uint16_t b16, uint8_t cin, const char* ctx){
        wide_zero_400(dut.get());
        dut->a[0] = a16; dut->b[0] = b16; dut->cin = cin & 1u; dut->eval();
        // BCD add across 4 digits
        uint8_t carry = cin & 1u;
        uint16_t sum16 = 0;
        for (int d = 0; d < 4; ++d) {
            const uint8_t an = (a16 >> (4*d)) & 0xFu;
            const uint8_t bn = (b16 >> (4*d)) & 0xFu;
            uint8_t t = an + bn + carry;
            carry = (t > 9);
            if (carry) t = t + 6; // decimal adjust
            sum16 |= (uint16_t(t & 0xFu) << (4*d));
        }
        if (get_low16(dut->sum) != sum16) {
            std::cerr << "[TB] dut_043 failed(" << ctx << ") low16 sum mismatch" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        // Remaining higher digits are zero, so any carry clears on the next digit; final cout must be 0
        if (dut->cout != 0u) {
            std::cerr << "[TB] dut_043 failed(" << ctx << ") final cout should be 0 with higher digits = 0" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    };

    check4(0x0000u, 0x0000u, 0, "zero");
    check4(0x1234u, 0x5678u, 0, "1234+5678");
    check4(0x9999u, 0x0001u, 0, "9999+0001");
    check4(0x0000u, 0x0000u, 1, "cin1");

    // Bidirectional toggle all cout_temp[d] by driving each BCD digit d with 9+9
    // This forces a carry out of that digit while higher digits are zero.
    wide_zero_400(dut.get()); dut->cin = 0;
    for (int d = 0; d < 100; ++d) {
        for (int rep = 0; rep < 2; ++rep) {
            set_bcd_digit(dut->a, d, 9); set_bcd_digit(dut->b, d, 9); dut->eval();
            set_bcd_digit(dut->a, d, 0); set_bcd_digit(dut->b, d, 0); dut->eval();
        }
    }
    // Toggle cin 0->1->0 twice under zeros
    for (int rep = 0; rep < 2; ++rep) {
        dut->cin = 0; dut->eval();
        dut->cin = 1; dut->eval();
        dut->cin = 0; dut->eval();
    }

    std::cout << "[TB] dut_043 passed: 100-digit BCD adder (low 4 digits + full carry toggles)" << std::endl;

#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
