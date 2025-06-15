#include <stdlib.h>
#include <stdint.h>

#include "stm32f4_regs.h"
#include "clock.h"
#include "uart.h"
#include "gpio.h"
#include "sdram.h"
#include "xprintf.h"
#include "spiflash_itf.h"
#include "shell.h"
#include "shell_func.h"
#include "lcd_test.h"
#include "MLX90642_disp.h"
#include "lcd_itf.h"

#if defined(USE_IS42S16320F)
	#define SDRAM_SIZE (64ul*1024ul*1024ul)
#else
	#define SDRAM_SIZE (8ul*1024ul*1024ul)
#endif

static void *gpio_base = (void *)GPIOA_BASE;

static void xprintf_out(int ch){
	uint8_t c=(uint8_t)ch;
	uart_send(1, &c, 1);
}

static uint32_t shell_read(uint8_t *buff, uint32_t len)
{
	return uart_read(1,buff,len);
}

static void shell_write(uint8_t *buff, uint32_t len)
{
	uart_send(1,buff,len);
}

static int user_main(void)
{
	volatile uint32_t *FLASH_KEYR = (void *)(FLASH_BASE + 0x04);
	volatile uint32_t *FLASH_CR = (void *)(FLASH_BASE + 0x10);

	if (*FLASH_CR & FLASH_CR_LOCK) {
		*FLASH_KEYR = 0x45670123;
		*FLASH_KEYR = 0xCDEF89AB;
	}
	*FLASH_CR &= ~(FLASH_CR_ERRIE | FLASH_CR_EOPIE | FLASH_CR_PSIZE_MASK);
	*FLASH_CR |= FLASH_CR_PSIZE_X32;
	*FLASH_CR |= FLASH_CR_LOCK;

	xdev_out(xprintf_out);
	clock_setup();

	gpio_set_usart(gpio_base, 'A', 9, 7);
	gpio_set_usart(gpio_base, 'A', 10, 7);
	uart_init(1, 1000000);

	systick_init();
	asm volatile ("cpsie i");
	asm volatile ("cpsie f");

	/* 地址线 */
	/* PF0~PF5 A0~A5 */
	gpio_set_fmc(gpio_base, 'F', 0);
	gpio_set_fmc(gpio_base, 'F', 1);
	gpio_set_fmc(gpio_base, 'F', 2);
	gpio_set_fmc(gpio_base, 'F', 3);
	gpio_set_fmc(gpio_base, 'F', 4);
	gpio_set_fmc(gpio_base, 'F', 5);
	/* PF12~15 A6~A9 */
	gpio_set_fmc(gpio_base, 'F', 12);
	gpio_set_fmc(gpio_base, 'F', 13);
	gpio_set_fmc(gpio_base, 'F', 14);
	gpio_set_fmc(gpio_base, 'F', 15);
	/* PG0~PG1 A10~A11 PG2~A12 */
	gpio_set_fmc(gpio_base, 'G', 0);
	gpio_set_fmc(gpio_base, 'G', 1);
	gpio_set_fmc(gpio_base, 'G', 2);
	/* 数据线 */
	/* PD14~15 PD0~1 AF2 */
	gpio_set_fmc(gpio_base, 'D', 14);
	gpio_set_fmc(gpio_base, 'D', 15);
	/* PD0~PD1 D2~D3 AF12 */
	gpio_set_fmc(gpio_base, 'D', 0);
	gpio_set_fmc(gpio_base, 'D', 1);
	/* PE7~15 D4~12 */
	gpio_set_fmc(gpio_base, 'E', 7);
	gpio_set_fmc(gpio_base, 'E', 8);
	gpio_set_fmc(gpio_base, 'E', 9);
	gpio_set_fmc(gpio_base, 'E', 10);
	gpio_set_fmc(gpio_base, 'E', 11);
	gpio_set_fmc(gpio_base, 'E', 12);
	gpio_set_fmc(gpio_base, 'E', 13);
	gpio_set_fmc(gpio_base, 'E', 14);
	gpio_set_fmc(gpio_base, 'E', 15);
	/* PD8~10 D13~15 AF12 */
	gpio_set_fmc(gpio_base, 'D', 8);
	gpio_set_fmc(gpio_base, 'D', 9);
	gpio_set_fmc(gpio_base, 'D', 10);

	/* 控制线 */
	/* PE0~1 NBL0~1 AF2 */
	gpio_set_fmc(gpio_base, 'E', 0);
	gpio_set_fmc(gpio_base, 'E', 1);
	/* PB5 SDCKE AF12 */
	gpio_set_fmc(gpio_base, 'B', 5);
	/* PB6 SDNE AF12 */
	gpio_set_fmc(gpio_base, 'B', 6);
	/* PC0 SDNWE AF12 */
	gpio_set_fmc(gpio_base, 'C', 0);
	/* PF11 SDNRAS */
	gpio_set_fmc(gpio_base, 'F', 11);
	/* PG4~PG5 BA0~BA1 */
	gpio_set_fmc(gpio_base, 'G', 4);
	gpio_set_fmc(gpio_base, 'G', 5);
	/* PG8 SDCLK */
	gpio_set_fmc(gpio_base, 'G', 8);
	/* PG15 SDNCAS */
	gpio_set_fmc(gpio_base, 'G', 15);

	xprintf("sdram init ...\r\n");
	sdram_init();

	flash_itf_init();
	lcd_itf_init();
	//lcd_test();

	shell_set_itf(shell_read, shell_write, (shell_cmd_cfg*)g_shell_cmd_list_ast, 1);
	mlx90642_disp_init();
	while(1){
		shell_exec();
		mlx90642_disp();
	}
	return 0;
}

static void noop(void)
{
	uint8_t c = 'E';
	uart_send(1, &c, 1);
	while (1) {
	}
}

extern unsigned int _end_text;
extern unsigned int _start_data;
extern unsigned int _end_data;
extern unsigned int _start_bss;
extern unsigned int _end_bss;
extern unsigned int _data_load_addr;
extern unsigned int _data_start;
extern unsigned int _data_end;

void reset(void)
{
	unsigned int *src, *dst;

	asm volatile ("cpsid i");

	src = &_data_load_addr;
	dst = &_data_start;
	while (dst < &_data_end) {
		*dst++ = *src++;
	}

	dst = &_start_bss;
	while (dst < &_end_bss) {
		*dst++ = 0;
	}

	user_main();
}


extern unsigned long _stack_top;

__attribute__((section(".vector_table")))
__attribute__((used)) static void (*vector_table[16 + 91])(void) = {
	(void (*))&_stack_top,
	reset,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	tick,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	uart1_irqhandler,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
	noop,
};
