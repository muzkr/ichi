
PROJECT = ichi

ENABLE_LOGGING ?= 0
VERSION_STRING ?= 1.1.0


TARGET = $(PROJECT)_$(VERSION_STRING)

######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -Og


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES = \
src/main.c \
src/board.c \
src/dfu.c \
src/dfu_write.c \
src/internal_flash.c \
src/py25q16.c \
src/lcd.c \
src/usbd_msc_impl.c \
src/py32f071_it.c \
src/system_py32f071.c \
lib/Middlewares/CherryUSB/core/usbd_core.c \
lib/Middlewares/CherryUSB/port/usb_dc_py32.c \
lib/Middlewares/CherryUSB/class/msc/usbd_msc.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_dma.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_gpio.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_rcc.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_pwr.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_spi.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_usart.c \
lib/Drivers/PY32F071_HAL_Driver/Src/py32f071_ll_utils.c


# ASM sources
ASM_SOURCES = \
startup_py32f071xx.s \


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

PYTHON = python3
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m0plus

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS = \
-DPY32F071xB \
-DUSE_FULL_LL_DRIVER \
-DVERSION_STRING=\"$(VERSION_STRING)\"

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES = \
-Isrc \
-Ilib/Drivers/PY32F071_HAL_Driver/Inc \
-Ilib/Drivers/CMSIS/Device/PY32F071/Include \
-Ilib/Drivers/CMSIS/Include \
-Ilib/Middlewares/CherryUSB/port \
-Ilib/Middlewares/CherryUSB/core \
-Ilib/Middlewares/CherryUSB/common \
-Ilib/Middlewares/CherryUSB/class/msc


ifeq ($(ENABLE_LOGGING),1)
C_SOURCES += \
	src/log.c \
	lib/Utilities/printf/printf.c \
	lib/Utilities/lwrb-3.2.0/lwrb/src/lwrb/lwrb.c
C_INCLUDES += \
	-Ilib/Utilities/printf \
	-Ilib/Utilities/lwrb-3.2.0/lwrb/src/include
C_DEFS += -DENABLE_LOGGING
endif


# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = py32f071xb.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections \
	-Wl,--print-memory-usage

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).uf2


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	$(PYTHON) utils/fw-pack.py $@ "muzkr" $(VERSION_STRING) $(BUILD_DIR)/$(TARGET).packed.bin
	
$(BUILD_DIR)/%.uf2: $(BUILD_DIR)/%.bin | $(BUILD_DIR)
	$(PYTHON) utils/uf2conv.py -c -b 0x08002800 -o $@ $<

$(BUILD_DIR):
	mkdir $@		

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***