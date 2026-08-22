CC      = arm-none-eabi-gcc
CXX     = arm-none-eabi-g++
CFLAGS  = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffreestanding -Og -g3 -Wall -Wextra
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -nostartfiles -T linker_script.ld -specs=nano.specs -Wl,-Map=firmware.map

all: firmware.elf

startup.o: startup.c
	$(CC) -c startup.c -o startup.o $(CFLAGS)

main.o: main.cpp
	$(CXX) -c main.cpp -o main.o $(CXXFLAGS)

.PHONY: all clean

firmware.elf: startup.o main.o
	$(CXX) startup.o main.o -o firmware.elf $(LDFLAGS)


clean:
	rm -f *.o firmware.elf firmware.map