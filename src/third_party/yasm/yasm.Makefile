# This file is generated by gyp; do not edit.

export builddir_name ?= src/third_party/yasm/out
.PHONY: all
all:
	$(MAKE) -C ../.. config_sources genmacro genperf_libs genmodule genstring genversion re2c genperf generate_files yasm
