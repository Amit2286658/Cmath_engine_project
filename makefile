APP_NAME := myapp

DEBUG_DIR   := DebugBuild
RELEASE_DIR := ReleaseBuild

# Defaults to clang. Override at the command line, e.g:  make build CC=gcc
CC := cc

WARN_FLAGS := -Wall -Wextra -Wconversion -Wcast-qual -Wshadow -Wformat=2 -Werror -fvisibility=hidden

ifneq (,$(filter release,$(MAKECMDGOALS)))
	BUILD_DIR  := $(RELEASE_DIR)
	BUILD_TYPE := Release
	CFLAGS_OPT := -O2
	SAN_FLAGS  :=
else
	BUILD_DIR  := $(DEBUG_DIR)
	BUILD_TYPE := Debug
	CFLAGS_OPT := -g -Og
	SAN_FLAGS  := -fsanitize=address,undefined
endif

BIN   := $(BUILD_DIR)/$(APP_NAME)
MATHC := $(BUILD_DIR)/mathc

STDLIB_SOURCES := \
	Library/algebra.math \
	Library/arith.math \
	Library/biology.math \
	Library/bootstrap.math \
	Library/calculus.math \
	Library/chemistry.math \
	Library/complex.math \
	Library/conversions.math \
	Library/geom.math \
	Library/physics.math \
	Library/trig.math \
	Library/units.math

CORE_SRCS := \
	$(wildcard Core/Code/*.c) \
	$(wildcard Core/compile/*.c) \
	$(wildcard Core/session/*.c) \
	Core/lib/delegate/delegate.c \
	Core/lib/hardcode/hardcode.c

INCLUDES := \
	-ICore/Headers \
	-ICore/compile \
	-ICore/session \
	-ICore/lib/delegate \
	-ICore/lib/hardcode

.PHONY: clean build run test stdlib debug release

debug release: ;

clean:
	rm -rf $(BUILD_DIR)

build:
	@if [ -f $(BIN) ]; then \
		echo "Existing $(BUILD_TYPE) build found -- cleaning first"; \
		rm -rf $(BUILD_DIR); \
	fi
	@if command -v cmake >/dev/null 2>&1; then \
		cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_C_COMPILER=$(CC); \
		cmake --build $(BUILD_DIR); \
	else \
		mkdir -p $(BUILD_DIR); \
		$(CC) -std=c11 $(WARN_FLAGS) $(CFLAGS_OPT) $(SAN_FLAGS) \
			$(INCLUDES) $(CORE_SRCS) Test/main.c -lm -o $(BIN); \
	fi

run:
	@if [ -f $(BIN) ]; then \
		echo "$(BUILD_TYPE) binary already built -- running"; \
	else \
		$(MAKE) --no-print-directory build $(filter release,$(MAKECMDGOALS)) CC=$(CC); \
	fi
	./$(BIN)

test: build
	@if command -v ctest >/dev/null 2>&1 && [ -f $(BUILD_DIR)/CTestTestfile.cmake ]; then \
		ctest --test-dir $(BUILD_DIR) --output-on-failure; \
	else \
		./$(BIN); \
	fi

stdlib:
	@if command -v cmake >/dev/null 2>&1; then \
		if [ ! -d $(BUILD_DIR) ]; then \
			cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_C_COMPILER=$(CC); \
		fi; \
		cmake --build $(BUILD_DIR) --target stdlib; \
	else \
		mkdir -p $(BUILD_DIR); \
		$(CC) -std=c11 $(WARN_FLAGS) $(CFLAGS_OPT) $(SAN_FLAGS) \
			$(INCLUDES) $(CORE_SRCS) Library/mathc.c -lm -o $(MATHC); \
		./$(MATHC) Library/stdlib.mathc Core/Headers/stdlib_precompiled.h k_stdlib_mathc $(STDLIB_SOURCES); \
	fi
