#
#      Makefile
# 
#           Project:  UART for megaAVR, tinyAVR & AVR DA DD DB EA
#           Author:   Hans-Henrik Fuxelius   
#           Date:     Uppsala, 2023-05-24           
# 

# DEVICE ....... The AVR device you compile for
# CLOCK ........ Target AVR clock rate in Hertz
# OBJECTS ...... The object files created from your source files. This list is
#                usually the same as the list of source files with suffix ".o".
# PROGRAMMER ... Options to avrdude which define the hardware you use for
#                uploading to the AVR and the interface where this hardware
#                is connected.
# FUSES ........ Parameters for avrdude to flash the fuses appropriately.

######################################################################################
TARGET      = at4808_uart
CLOCK       = 2666666
FUSE2 		= 0x01
FUSE5 		= 0xC9
FUSE8 		= 0x00

DEVICE      = atmega4808
PARTNO      = m4808

######################################################################################
# These paths are relocateble to serve multiple projects
# TOOLCHAIN_PATH  = ~/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino5/bin
TOOLCHAIN_PATH  = ~/Library/AVR/avr-toolchain/bin
# AVR_HAXX_PATH   = avr_haxx
AVR_HAXX_PATH   = ~/Library/AVR/avr_haxx

######################################################################################
AVR_GCC     = $(TOOLCHAIN_PATH)/avr-gcc
AVR_OBJCOPY = $(TOOLCHAIN_PATH)/avr-objcopy
AVR_SIZE    = $(TOOLCHAIN_PATH)/avr-size

AVR_DUDE    = avrdude

SERIAL_PORT = $(shell find /dev/cu.usbserial-* | head -n 1)

PROGRAMMER  = -c atmelice_updi -Pusb -b9600 -p $(PARTNO)

SOURCES   := $(shell find * -type f -name "*.c")
TODAY     := $(shell date +%Y%m%d_%H%M%S)
OBJDIR    := .objects
DEPLOYDIR := .deploy
OBJECTS   := $(addprefix $(OBJDIR)/,$(SOURCES:.c=.o))
FUSES      = -U fuse2:w:$(FUSE2):m -U fuse5:w:$(FUSE5):m -U fuse8:w:$(FUSE8):m 
SIZE       = $(AVR_SIZE) --format=avr --mcu=$(DEVICE) $(TARGET).elf

######################################################################################
AVRDUDE = $(AVR_DUDE) $(PROGRAMMER)

COMPILE = $(AVR_GCC) -Wall -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -Og -std=gnu11 \
		  -I"$(AVR_HAXX_PATH)/include" -B"$(AVR_HAXX_PATH)/devices/$(DEVICE)" \
		  -ffunction-sections -MD -MP -fdata-sections -fpack-struct -fshort-enums -g2 

######################################################################################
# symbolic targets:
all: $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(AVR_OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $(TARGET).elf $(TARGET).hex
	$(AVR_OBJCOPY) -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex $(TARGET).elf $(TARGET).eep || exit 0
	$(AVR_OBJCOPY) -h -S $(TARGET).elf > $(TARGET).lss
	$(AVR_OBJCOPY) -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $(TARGET).elf $(TARGET).srec
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

# file targets:
$(TARGET).elf: $(OBJECTS)
	$(COMPILE) $^ -o $@
	$(SIZE)

$(OBJECTS): $(OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(COMPILE) -c $< -o $@

-include $(OBJECTS:.o=.d)

deploy:
	mkdir -p $(DEPLOYDIR)
	cp $(TARGET).hex $(DEPLOYDIR)/$(TARGET)_$(TODAY).hex
	md5sum $(DEPLOYDIR)/$(TARGET)_$(TODAY).hex > $(DEPLOYDIR)/$(TARGET)_$(TODAY).md5

flash:	all			
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

fuse:
	$(AVRDUDE) $(FUSES)

install: flash fuse

serial:
	tio $(SERIAL_PORT) -b 9600 -d 8 -p none -s 1

clean:
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).lss $(TARGET).srec $(TARGET)_cipher.hex $(OBJECTS)