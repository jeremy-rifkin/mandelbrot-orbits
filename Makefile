TARGET_EXEC := mandelbrot.exe

BUILD_DIR := bin
SRC_DIRS := src

all: $(BUILD_DIR)/$(TARGET_EXEC) README.md

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CC := gcc
CPP := g++

WFLAGS := -Wall -Wextra -Wpedantic -Wvla
DFLAGS := -MMD -MP
CFLAGS :=
CPPFLAGS := -std=c++17

LDFLAGS := -lpthread

# two targets:
#  debug (O0, asan, asserts, etc)
#  release (O3, NDEBUG, etc)
TARGET = release

# target flags
ifeq ($(TARGET), debug)
	CFLAGS = -O0 -g
	ifeq ($(OS),Windows_NT)
	else
		CFLAGS += -fsanitize=address
		LDFLAGS += -fsanitize=address
	endif
else ifeq ($(TARGET), release)
	CFLAGS += -O3 -funroll-loops -ftree-vectorize -ffast-math -flto -march=native -DNDEBUG
	LDFLAGS += -s
endif

CFLAGS += $(WFLAGS) $(DFLAGS)
CPPFLAGS += $(CFLAGS)

MKDIR_P ?= mkdir -p

# exe
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CPP) $(OBJS) -o $@ $(CPPFLAGS) $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CPP) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Warning: there are some issues here if file names have spaces in them
BMP_DIR := photos/bmp
PNG_DIR := photos/png
BMPS := $(shell find $(BMP_DIR) -name "*.bmp")
PNGS := $(BMPS:$(BMP_DIR)/%.bmp=$(PNG_DIR)/%.png)

.PHONY: screenshots

screenshots: $(PNGS)

# The bash nonsense is a windows thing
$(PNG_DIR)/%.png: SHELL:= bash
$(PNG_DIR)/%.png: $(BMP_DIR)/%.bmp
	$(MKDIR_P) $(dir $@)
	bash -c "convert $^ $@"

README.md: README_latex.md
	markdown-math-gh-compiler README_latex.md -o README.md

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)
