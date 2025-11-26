VERILATOR ?= verilator
# -Wno-fatal: do not treat warnings as errors (needed for vector logical ops etc.)
VERILATOR_FLAGS ?= -Wall -Wno-DECLFILENAME --coverage -Wno-fatal
export CCACHE_DISABLE ?= 1
DUT ?= 001
TOP ?= top_module
PREFIX ?= Vdut_$(DUT)
MODEL := $(PREFIX)

BUILD_DIR := build
COVERAGE_ROOT := coverage
DUT_SRC := dut/dut_$(DUT).v
TB_SRC := tb/tb_$(DUT).cpp
BUILD_SUBDIR := $(BUILD_DIR)/tb_$(DUT)
BIN := $(BUILD_SUBDIR)/V$(TOP)
COV_DIR := $(COVERAGE_ROOT)/dut_$(DUT)
COV_DAT := $(COV_DIR)/coverage.dat
COV_INFO := $(COV_DIR)/coverage.info
COV_ANNOTATE_DIR := $(COV_DIR)/annotate

.PHONY: all run_tb clean coverage_report

all: run_tb

$(BUILD_SUBDIR):
	@mkdir -p $@

$(BIN): $(DUT_SRC) $(TB_SRC) | $(BUILD_SUBDIR)
	$(VERILATOR) $(VERILATOR_FLAGS) --cc $(DUT_SRC) --exe ../../$(TB_SRC) \
		--top-module $(TOP) --prefix $(PREFIX) -o V$(TOP) -Mdir $(BUILD_SUBDIR)
	$(MAKE) -C $(BUILD_SUBDIR) -f $(MODEL).mk V$(TOP)

run_tb: $(BIN)
	@mkdir -p $(COV_DIR)
	@echo "[RUN] DUT=$(DUT)"
	VERILATOR_COV_FILE=$(COV_DAT) ./$(BIN)
	@test -f $(COV_DAT) || (echo "[ERROR] Coverage data missing for DUT $(DUT)" && exit 1)
	$(MAKE) coverage_report \
		COV_DAT=$(COV_DAT) \
		COV_INFO=$(COV_INFO) \
		COV_ANNOTATE_DIR=$(COV_ANNOTATE_DIR)

coverage_report:
	@if [ -z "$(COV_DAT)" ] || [ -z "$(COV_INFO)" ] || [ -z "$(COV_ANNOTATE_DIR)" ]; then \
		echo "[ERROR] coverage_report requires COV_DAT, COV_INFO, and COV_ANNOTATE_DIR"; \
		exit 1; \
	fi
	@mkdir -p $(COV_ANNOTATE_DIR)
	# Line coverage (LCOV format) and annotated sources
	verilator_coverage --write-info $(COV_INFO) $(COV_DAT)
	verilator_coverage --annotate-min 1 --annotate $(COV_ANNOTATE_DIR) $(COV_DAT)
	# Toggle coverage summary (per-bit points, bidirectional) and combined summary
	@awk -v dut=dut/dut_$(DUT).v 'BEGIN{ltot=0;lcov=0;tt=0;tc=0} FNR==NR&&/^DA:/{ltot++;split($$0,a,",");if(a[2]+0>0)lcov++;next} /^C /&&$$0~dut&&$$0~/v_toggle/{tt++;if($$NF+0>=2)tc++;next} END{combt=ltot+tt;combc=lcov+tc; printf("Toggle coverage (bidirectional) (%d/%d) %0.2f%%\n",tc,tt,(tt?100.0*tc/tt:0)); printf("Combined coverage (lines+toggles) (%d/%d) %0.2f%%\n",combc,combt,(combt?100.0*combc/combt:0))}' $(COV_INFO) $(COV_DAT)

clean:
	rm -rf $(BUILD_DIR) $(COVERAGE_ROOT) coverage.dat coverage.info coverage_annotate
