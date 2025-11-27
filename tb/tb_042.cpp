#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_042.h"

// Helper to set/clear 100-bit vectors in 4x32 words (little-endian words)
static inline void set_u100(Vdut_042* dut, uint64_t a_lo, uint64_t a_hi36,
                            uint64_t b_lo, uint64_t b_hi36,
                            uint8_t cin) {
    dut->a[0] = static_cast<uint32_t>(a_lo & 0xFFFFFFFFull);
    dut->a[1] = static_cast<uint32_t>((a_lo >> 32) & 0xFFFFFFFFull);
    dut->a[2] = static_cast<uint32_t>(a_hi36 & 0xFFFFFFFFull);
    dut->a[3] = static_cast<uint32_t>((a_hi36 >> 32) & 0xFull);
    dut->b[0] = static_cast<uint32_t>(b_lo & 0xFFFFFFFFull);
    dut->b[1] = static_cast<uint32_t>((b_lo >> 32) & 0xFFFFFFFFull);
    dut->b[2] = static_cast<uint32_t>(b_hi36 & 0xFFFFFFFFull);
    dut->b[3] = static_cast<uint32_t>((b_hi36 >> 32) & 0xFull);
    dut->cin = cin & 1u;
}

static inline uint32_t get_bit_u100(const uint32_t w[4], int bit) {
    const int widx = bit >> 5; const int b = bit & 31; return (w[widx] >> b) & 1u;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_042>(context.get());

    auto check = [&](uint64_t alo, uint64_t ahi, uint64_t blo, uint64_t bhi, uint8_t cin, const char* ctx){
        set_u100(dut.get(), alo, ahi, blo, bhi, cin); dut->eval();
        // Reference per-bit ripple
        uint8_t carry = cin & 1u;
        for (int i = 0; i < 100; ++i) {
            const uint8_t ai = (i<64)? ((alo>>i)&1u) : ((ahi>>(i-64))&1u);
            const uint8_t bi = (i<64)? ((blo>>i)&1u) : ((bhi>>(i-64))&1u);
            const uint8_t sum = (ai ^ bi) ^ carry;
            const uint8_t cout = (ai & bi) | (ai & carry) | (bi & carry);
            const uint8_t dut_sum = get_bit_u100(dut->sum, i);
            const uint8_t dut_cout = get_bit_u100(dut->cout, i);
            if (dut_sum != sum || dut_cout != cout) {
                std::cerr << "[TB] dut_042 failed(" << ctx << ") at bit " << i << std::endl;
                std::exit(EXIT_FAILURE);
            }
            carry = cout;
        }
    };

    // Basic sanity
    check(0,0,0,0,0, "zero");
    check(~0ull,0xFFFFFFFFFull, 0,0,0, "allones_plus_zero");
    check(~0ull,0xFFFFFFFFFull, 0,0,1, "allones_plus_zero_cin1");
    // Exercise near carry boundaries
    check(0xFFFFFFFFFFFFFFFEull, 0ull, 1ull, 0ull, 0, "low_carry");
    check(0ull, 1ull, 0ull, 0ull, 1, "high36_carry");

    // Bidirectional toggle coverage
    // 1) Toggle cin 0->1->0 twice under zero inputs
    for (int rep = 0; rep < 2; ++rep) {
        check(0,0,0,0,0, "cin0");
        check(0,0,0,0,1, "cin1");
    }
    check(0,0,0,0,0, "cin0_end");

    // 2) For each bit i, toggle a[i] 0->1->0 twice with b=0, cin=0
    for (int i = 0; i < 100; ++i) {
        uint64_t alo=0, ahi=0, blo=0, bhi=0; uint8_t cin=0;
        auto setbit = [](uint64_t &lo, uint64_t &hi, int bit, bool val){
            if (bit < 64) { if (val) lo |= (1ull<<bit); else lo &= ~(1ull<<bit); }
            else { int k=bit-64; if (val) hi |= (1ull<<k); else hi &= ~(1ull<<k); }
        };
        for (int rep = 0; rep < 2; ++rep) {
            check(alo,ahi,blo,bhi,cin, "a_bit_off");
            setbit(alo,ahi,i,true);  check(alo,ahi,blo,bhi,cin, "a_bit_on");
            setbit(alo,ahi,i,false); check(alo,ahi,blo,bhi,cin, "a_bit_off2");
        }
    }

    // 3) For each bit i, toggle cout[i] via a[i]=b[i]=1  (0->1->0 twice)
    for (int i = 0; i < 100; ++i) {
        uint64_t alo=0, ahi=0, blo=0, bhi=0; uint8_t cin=0;
        auto setbit = [](uint64_t &lo, uint64_t &hi, int bit, bool val){
            if (bit < 64) { if (val) lo |= (1ull<<bit); else lo &= ~(1ull<<bit); }
            else { int k=bit-64; if (val) hi |= (1ull<<k); else hi &= ~(1ull<<k); }
        };
        for (int rep = 0; rep < 2; ++rep) {
            check(alo,ahi,blo,bhi,cin, "ab_off");
            setbit(alo,ahi,i,true); setbit(blo,bhi,i,true);  check(alo,ahi,blo,bhi,cin, "ab_on");
            setbit(blo,bhi,i,false);                          check(alo,ahi,blo,bhi,cin, "ab_b_off");
            setbit(alo,ahi,i,false);                          check(alo,ahi,blo,bhi,cin, "ab_a_off");
        }
    }

    std::cout << "[TB] dut_042 passed: 100-bit ripple-carry adder" << std::endl;

#if VM_COVERAGE
    const char* covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
