.PHONY: all burn clean

all:
	arm-none-eabi-gcc \
		-O2 \
		-mcpu=cortex-m4 -mthumb \
		-Wl,--gc-sections \
		-o image.elf \
		-TSTM32F407VGTX_FLASH.ld \
		-Iinclude/CMSIS_5/CMSIS/Core/Include \
		async_svc.c startup_stm32f407vgtx.s
	
	arm-none-eabi-objcopy -O binary image.elf image.bin

burn:
	st-flash write image.bin 0x8000000

clean:
	rm -f image.elf image.bin
