CC      = arm-none-eabi-gcc
CXX     = arm-none-eabi-g++
CFLAGS  = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffreestanding -Og -g3 -Wall -Wextra
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++20
BUILD_DIR := build
LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -nostartfiles -T linker_script.ld -specs=nano.specs -Wl,-Map=$(BUILD_DIR)/firmware.map

# 2. Source and Object File Tracking
SRCS_C   := $(wildcard *.c)
SRCS_CPP := $(wildcard *.cpp)

# Generate obj/*.o paths for BOTH types using the shorthand substitution
OBJS     := $(SRCS_C:%.c=$(BUILD_DIR)/%.o) $(SRCS_CPP:%.cpp=$(BUILD_DIR)/%.o)

# Main target
all: $(BUILD_DIR)/firmware.elf

# linking rule
$(BUILD_DIR)/firmware.elf: $(OBJS)
	$(CXX) $^ -o $(BUILD_DIR)/firmware.elf $(LDFLAGS)

# Compilation Rule for C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Compilation Rule for C++ files
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Rule to create the directory if it does not exist
$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean