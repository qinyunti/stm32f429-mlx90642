CROSS_COMPILE ?= arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

CFLAGS := -mthumb -mcpu=cortex-m4 -fno-builtin --specs=nano.specs
#CFLAGS += -save-temps
#--specs=nosys.specs
#--specs=nano.specs
CFLAGS += -ffunction-sections -fdata-sections  -nostdlib
CFLAGS += -Os -std=gnu99 -Wall -nostartfiles -g  -Imlx90642-library/inc -ICMSIS/ -I./ 
LINKERFLAGS :=  --gc-sections
obj-y += io_iic.o mlx90642-library/src/MLX90642.o mlx90642-library/src/MLX90642_depends.o MLX90642_test.o ili9341v.o lcd_itf.o string.o stm32f429-mlx90642.o xmodem.o shell.o shell_func.o uart.o fifo.o clock.o spi.o gpio.o sdram.o xprintf.o spiflash.o spiflash_itf.o

all: stm32f429-mlx90642

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

stm32f429-mlx90642: stm32f429-mlx90642.o $(obj-y)
	$(LD) -T stm32f429.lds $(LINKERFLAGS) -o stm32f429-mlx90642.elf $(obj-y)
	$(OBJCOPY) -Obinary stm32f429-mlx90642.elf stm32f429-mlx90642.bin
	$(SIZE) stm32f429-mlx90642.elf

clean:
	@rm -f *.o *.elf *.bin *.lst *.i *.s

