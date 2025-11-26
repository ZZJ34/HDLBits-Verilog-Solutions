#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "verilated.h"
#include "verilated_cov.h"
#include "Vdut_162.h"

namespace
{
    constexpr uint8_t SNT = 0U;
    constexpr uint8_t WNT = 1U;
    constexpr uint8_t WT = 2U;
    constexpr uint8_t ST = 3U;

    struct Inputs
    {
        uint8_t predict_valid;
        uint8_t predict_pc;
        uint8_t train_valid;
        uint8_t train_taken;
        uint8_t train_mispredicted;
        uint8_t train_history;
        uint8_t train_pc;
    };

    struct ModelState
    {
        std::array<uint8_t, 128> pht{};
        uint8_t ghr{};
    };

    uint8_t update_counter(uint8_t current, bool taken)
    {
        switch (current)
        {
            case SNT:
                return taken ? WNT : SNT;
            case WNT:
                return taken ? WT : SNT;
            case WT:
                return taken ? ST : WNT;
            case ST:
            default:
                return taken ? ST : WT;
        }
    }

    void reset_model(ModelState &state)
    {
        state.ghr = 0U;
        state.pht.fill(WNT);
    }
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    auto context = std::make_unique<VerilatedContext>();
    context->traceEverOn(false);

    auto dut = std::make_unique<Vdut_162>(context.get());

    ModelState model{};
    reset_model(model);

    auto apply_reset = [&]() {
        dut->clk = 0U;
        dut->predict_valid = 0U;
        dut->predict_pc = 0U;
        dut->train_valid = 0U;
        dut->train_taken = 0U;
        dut->train_mispredicted = 0U;
        dut->train_history = 0U;
        dut->train_pc = 0U;

        dut->areset = 1U;
        dut->eval();
        dut->areset = 0U;
        dut->eval();

        reset_model(model);
    };

    apply_reset();

    auto run_cycle = [&](const Inputs &in, const std::string &label) -> bool {
        bool expected_taken = false;
        uint8_t expected_history = 0U;
        if (in.predict_valid)
        {
            const uint8_t predict_index =
                static_cast<uint8_t>((in.predict_pc ^ model.ghr) & 0x7FU);
            expected_taken = model.pht[predict_index] >= WT;
            expected_history = model.ghr;
        }

        dut->predict_valid = in.predict_valid;
        dut->predict_pc = in.predict_pc;
        dut->train_valid = in.train_valid;
        dut->train_taken = in.train_taken;
        dut->train_mispredicted = in.train_mispredicted;
        dut->train_history = in.train_history;
        dut->train_pc = in.train_pc;

        dut->clk = 0U;
        dut->eval();

        if (dut->predict_taken != static_cast<uint8_t>(expected_taken) ||
            dut->predict_history != expected_history)
        {
            std::cerr << "[TB] dut_162 predict mismatch (" << label << "): "
                      << "expected taken=" << expected_taken
                      << " history=" << static_cast<int>(expected_history)
                      << ", got taken=" << static_cast<int>(dut->predict_taken)
                      << " history=" << static_cast<int>(dut->predict_history)
                      << std::endl;
            return false;
        }

        dut->clk = 1U;
        dut->eval();

        if (in.train_valid)
        {
            const uint8_t train_index =
                static_cast<uint8_t>((in.train_pc ^ in.train_history) & 0x7FU);
            model.pht[train_index] =
                update_counter(model.pht[train_index], in.train_taken != 0U);
        }

        if (in.train_valid && in.train_mispredicted)
        {
            model.ghr = static_cast<uint8_t>(
                ((in.train_history & 0x3FU) << 1U) | (in.train_taken ? 1U : 0U));
        }
        else if (in.predict_valid)
        {
            model.ghr = static_cast<uint8_t>(
                ((model.ghr & 0x3FU) << 1U) | (expected_taken ? 1U : 0U));
        }

        dut->clk = 0U;
        dut->eval();

        return true;
    };

    // Coverage-focused cycles to toggle wide buses and both misprediction outcomes.
    Inputs in{};
    in = {1U, 0x7FU, 0U, 0U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "predict_pc all ones"))
    {
        return EXIT_FAILURE;
    }

    in = {1U, 0x00U, 0U, 0U, 0U, 0x7FU, 0x7FU};
    if (!run_cycle(in, "predict_pc return to zeros"))
    {
        return EXIT_FAILURE;
    }

    in = {0U, 0x00U, 1U, 1U, 1U, 0x3FU, 0x40U};
    if (!run_cycle(in, "train_index all ones with mispredict"))
    {
        return EXIT_FAILURE;
    }

    in = {0U, 0x00U, 1U, 0U, 1U, 0x00U, 0x00U};
    if (!run_cycle(in, "train_index back to zero with mispredict"))
    {
        return EXIT_FAILURE;
    }

    in = {1U, 0x10U, 1U, 1U, 1U, 0x2AU, 0x55U};
    if (!run_cycle(in, "mispredict with predict_valid high"))
    {
        return EXIT_FAILURE;
    }

    in = {0U, 0x08U, 1U, 0U, 1U, 0x15U, 0x12U};
    if (!run_cycle(in, "mispredict with predict_valid low"))
    {
        return EXIT_FAILURE;
    }

    // Drive PHT[0] through both bits toggling.
    in = {0U, 0x00U, 1U, 1U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "PHT[0] SNT->WNT"))
    {
        return EXIT_FAILURE;
    }

    in = {0U, 0x00U, 1U, 1U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "PHT[0] WNT->WT"))
    {
        return EXIT_FAILURE;
    }

    in = {0U, 0x00U, 1U, 0U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "PHT[0] WT->WNT"))
    {
        return EXIT_FAILURE;
    }

    // Cover train_mispredicted asserted when train_valid is low.
    in = {0U, 0x2AU, 0U, 0U, 1U, 0x15U, 0x3BU};
    if (!run_cycle(in, "mispredict flag without training"))
    {
        return EXIT_FAILURE;
    }

    in = {1U, 0x35U, 0U, 0U, 1U, 0x1EU, 0x21U};
    if (!run_cycle(in, "mispredict flag with predict_valid"))
    {
        return EXIT_FAILURE;
    }

    // Sweep every PHT entry to toggle both counter bits.
    apply_reset();
    for (uint8_t idx = 0; idx < 128U; ++idx)
    {
        in = {0U, 0x00U, 1U, 1U, 0U, 0x00U, idx};
        if (!run_cycle(in, "PHT sweep WNT->WT idx=" + std::to_string(idx)))
        {
            return EXIT_FAILURE;
        }

        in = {0U, 0x00U, 1U, 0U, 0U, 0x00U, idx};
        if (!run_cycle(in, "PHT sweep WT->WNT idx=" + std::to_string(idx)))
        {
            return EXIT_FAILURE;
        }
    }

    // Restore DUT state for functional checks after coverage-oriented stimuli.
    apply_reset();

    // Idle cycle to cover predict_valid low path.
    in = {0U, 0x00U, 0U, 0U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "idle no predict"))
    {
        return EXIT_FAILURE;
    }

    // Basic prediction with cold PHT entry.
    in = {1U, 0x0FU, 0U, 0U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "initial predict without training"))
    {
        return EXIT_FAILURE;
    }

    // Training with misprediction path updates GHR from train_history.
    in = {1U, 0x20U, 1U, 1U, 1U, 0x55U, 0x12U};
    if (!run_cycle(in, "mispredicted training updates PHT and GHR"))
    {
        return EXIT_FAILURE;
    }

    // Prediction after misprediction to observe new GHR usage.
    in = {1U, 0x7FU, 0U, 0U, 0U, 0x00U, 0x00U};
    if (!run_cycle(in, "predict using updated GHR"))
    {
        return EXIT_FAILURE;
    }

    // Begin saturating counter walk on a single PHT entry (index 0x33).
    // WNT -> WT with predict_valid high alongside training.
    in = {1U, 0x01U, 1U, 1U, 0U, 0x0FU, 0x3CU};
    if (!run_cycle(in, "WNT->WT with predict_valid high"))
    {
        return EXIT_FAILURE;
    }

    // WT -> ST.
    in = {0U, 0x10U, 1U, 1U, 0U, 0x1CU, 0x2FU};
    if (!run_cycle(in, "WT->ST training"))
    {
        return EXIT_FAILURE;
    }

    // ST remains ST when trained taken again.
    in = {0U, 0x10U, 1U, 1U, 0U, 0x00U, 0x33U};
    if (!run_cycle(in, "ST hold on taken"))
    {
        return EXIT_FAILURE;
    }

    // Predict at the strong taken counter to validate predict_taken high.
    Inputs predict_strong{};
    predict_strong.predict_valid = 1U;
    predict_strong.predict_pc = static_cast<uint8_t>(model.ghr ^ 0x33U);
    predict_strong.train_valid = 0U;
    predict_strong.train_taken = 0U;
    predict_strong.train_mispredicted = 0U;
    predict_strong.train_history = 0U;
    predict_strong.train_pc = 0U;
    if (!run_cycle(predict_strong, "predict taken when counter >= WT"))
    {
        return EXIT_FAILURE;
    }

    // ST -> WT on not-taken training.
    in = {1U, 0x40U, 1U, 0U, 0U, 0x7FU, 0x4CU};
    if (!run_cycle(in, "ST->WT on not taken"))
    {
        return EXIT_FAILURE;
    }

    // WT -> WNT.
    in = {0U, 0x02U, 1U, 0U, 0U, 0x55U, 0x66U};
    if (!run_cycle(in, "WT->WNT on not taken"))
    {
        return EXIT_FAILURE;
    }

    // WNT -> SNT.
    in = {1U, 0x00U, 1U, 0U, 0U, 0x01U, 0x32U};
    if (!run_cycle(in, "WNT->SNT on not taken"))
    {
        return EXIT_FAILURE;
    }

    // SNT holds on additional not-taken training.
    in = {0U, 0x00U, 1U, 0U, 0U, 0x7EU, 0x4DU};
    if (!run_cycle(in, "SNT hold on not taken"))
    {
        return EXIT_FAILURE;
    }

    // SNT -> WNT when taken.
    in = {1U, 0x15U, 1U, 1U, 0U, 0x08U, 0x3BU};
    if (!run_cycle(in, "SNT->WNT on taken"))
    {
        return EXIT_FAILURE;
    }

    std::cout << "[TB] dut_162 passed all prediction and training scenarios"
              << std::endl;

#if VM_COVERAGE
    const char *covPath = std::getenv("VERILATOR_COV_FILE");
    if (covPath == nullptr || covPath[0] == '\0')
    {
        covPath = "coverage.dat";
    }
    VerilatedCov::write(covPath);
#endif
    return EXIT_SUCCESS;
}
