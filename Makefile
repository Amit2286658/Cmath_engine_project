APP_NAME := myapp

DEBUG_DIR   := DebugBuild
RELEASE_DIR := ReleaseBuild

# Defaults to clang. Override at the command line, e.g:  make build CC=gcc
CC := clang

# Which variant was requested? "make clean release" / "make build release" /
# "make run release" all pick release; anything without the word "release"
# (including plain "make clean") defaults to debug.
ifneq (,$(filter release,$(MAKECMDGOALS)))
	BUILD_DIR  := $(RELEASE_DIR)
	BUILD_TYPE := Release
else
	BUILD_DIR  := $(DEBUG_DIR)
	BUILD_TYPE := Debug
endif

BIN := $(BUILD_DIR)/$(APP_NAME)

.PHONY: clean build run debug release

# Dummy targets: lets "debug" / "release" appear as a second word on the
# command line (make clean debug, make build release, ...) without Make
# complaining "No rule to make target". They do nothing on their own.
debug release: ;

# Delete only the selected variant's build folder
clean:
	rm -rf $(BUILD_DIR)

# Fresh build of the selected variant, no run. If already built, clean first.
build:
	@if [ -f $(BIN) ]; then \
		echo "Existing $(BUILD_TYPE) build found -- cleaning first"; \
		rm -rf $(BUILD_DIR); \
	fi
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_C_COMPILER=$(CC)
	cmake --build $(BUILD_DIR)

# Run the selected variant. Build it first only if it doesn't exist yet.
run:
	@if [ -f $(BIN) ]; then \
		echo "$(BUILD_TYPE) binary already built -- running"; \
	else \
		$(MAKE) --no-print-directory build $(filter release,$(MAKECMDGOALS)) CC=$(CC); \
	fi
	./$(BIN)