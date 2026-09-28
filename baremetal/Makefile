TARGET = firmware
BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CPU = -mcpu=cortex-m0plus -mthumb

CFLAGS = $(CPU) -Wall -Wextra -O0 -g \
-I./CMSIS/Include \
-I./CMSIS/Device/ST/STM32C0xx/Include \
-I./include \
-DSTM32C031xx

LDSCRIPT = STM32C031C6Tx_FLASH.ld

SOURCES = \
src/main.c \
src/uart.c \
src/adc.c \
src/pwm.c \
src/system_stm32c0xx.c \
src/runtime_stubs.c \
startup/startup_stm32c031xx.s

OBJECTS = \
$(BUILD_DIR)/main.o \
$(BUILD_DIR)/uart.o \
$(BUILD_DIR)/adc.o \
$(BUILD_DIR)/pwm.o \
$(BUILD_DIR)/system_stm32c0xx.o \
$(BUILD_DIR)/runtime_stubs.o \
$(BUILD_DIR)/startup_stm32c031xx.o

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/runtime_stubs.o: src/runtime_stubs.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/uart.o: src/uart.c include/uart.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/adc.o: src/adc.c include/adc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pwm.o: src/pwm.c include/pwm.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/system_stm32c0xx.o: src/system_stm32c0xx.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup_stm32c031xx.o: startup/startup_stm32c031xx.s | $(BUILD_DIR)
	$(CC) $(CPU) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(CPU) -T$(LDSCRIPT) -nostartfiles $(OBJECTS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)
