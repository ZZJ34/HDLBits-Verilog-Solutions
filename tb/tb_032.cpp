#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_032.h"

struct Stimulus { uint8_t cpu_overheated, arrived, gas_empty; };

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);
    auto dut = std::make_unique<Vdut_032>(context.get());

    const std::array<Stimulus, 8> stimuli{{
        {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}, {0,1,0}, {0,1,1}, {1,1,1}, {0,0,0}
    }};

    for (const auto &s : stimuli) {
        dut->cpu_overheated = s.cpu_overheated;
        dut->arrived = s.arrived;
        dut->gas_tank_empty = s.gas_empty;
        dut->eval();

        const uint8_t exp_shut = s.cpu_overheated ? 1u : 0u;
        const uint8_t exp_drive = (!s.arrived) ? static_cast<uint8_t>((~s.gas_empty) & 0x1u) : 0u;
        if (dut->shut_off_computer != exp_shut || dut->keep_driving != exp_drive) {
            std::cerr << "[TB] dut_032 failed: overheated=" << int(s.cpu_overheated)
                      << ", arrived=" << int(s.arrived) << ", gas_empty=" << int(s.gas_empty)
                      << ", expected shut/drive=" << int(exp_shut) << "/" << int(exp_drive)
                      << ", got " << int(dut->shut_off_computer) << "/" << int(dut->keep_driving)
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "[TB] dut_032 passed: control outputs match spec" << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0') { covPath = "coverage.dat"; }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}

