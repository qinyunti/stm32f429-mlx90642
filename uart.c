#include <stdint.h>
#include "stm32f429xx.h"

#include "clock.h"

//#define USART_SR_TXE	(1 << 7)

//#define USART_CR1_RE	(1 << 2)
//#define USART_CR1_TE	(1 << 3)
//#define USART_CR1_RXNEIE	(1 << 5)
//#define USART_CR1_UE	(1 << 13)

static uint32_t reg_base[1]={0x40011000};

#include "uart.h"
#include "fifo.h"

#define CriticalAlloc()
#define EnterCritical()	asm volatile ("cpsid i")
#define ExitCritical()  asm volatile ("cpsie i")

static uint8_t s_uart_rx_buffer[1029];

static fifo_st s_uart_fifo_dev[2]=
{
	{
	 .in = 0,
	 .len = 0,
	 .out = 0,
	 .buffer = s_uart_rx_buffer,
	 .buffer_len = sizeof(s_uart_rx_buffer),
	},
};

void uart_rx_cb(int id, uint8_t* buff, uint32_t len)
{
	fifo_in(&(s_uart_fifo_dev[id-1]), buff, len);
}

void uart1_irqhandler(void)
{
	volatile uint32_t *USART_SR  = (uint32_t*)(reg_base[1-1] + 0x00);
	volatile uint32_t *USART_DR  = (uint32_t*)(reg_base[1-1] + 0x04);
	uint8_t ch;
	/* 读字节 */
	if(*USART_SR & USART_SR_RXNE){
		ch = (uint8_t)(*USART_DR);
		/* 清中断 读DR自动清除标志 */
		uart_rx_cb(1, &ch, 1);
	}
}

void uart_init(int id, uint32_t baud)
{
	if(id == 1){
		volatile uint32_t *USART_BRR = (uint32_t*)(reg_base[id-1] + 0x08);
		volatile uint32_t *USART_CR1 = (uint32_t*)(reg_base[id-1] + 0x0C);
		volatile uint32_t *USART_CR2 = (uint32_t*)(reg_base[id-1] + 0x10);
		volatile uint32_t *USART_CR3 = (uint32_t*)(reg_base[id-1] + 0x14);
		uint32_t int_div, frac_div, val;

		*USART_CR1 &= ~(USART_CR1_TE | USART_CR1_RE);

		*USART_CR2 = 0;
		*USART_CR3 = 0;

		int_div = (25 * (clock_get_ahb()/2)) / (4 * baud);
		val = (int_div / 100) << 4;
		frac_div = int_div - 100 * (val >> 4);
		val |= ((frac_div * 16 + 50) / 100) & 0xf;
		*USART_BRR = val;

		*USART_CR1 |= USART_CR1_UE;

		*USART_CR1 |= USART_CR1_RXNEIE;

		//NVIC_SetPriority(USART1_IRQn,2,0);
		NVIC_EnableIRQ(USART1_IRQn);

		*USART_CR1 |= (USART_CR1_TE | USART_CR1_RE);
	}
}

uint32_t uart_send(int id, uint8_t* buffer, uint32_t len)
{
	if(id == 1){
		for(uint32_t i=0;i<len;i++)
		{
			volatile uint32_t *USART_SR  = (uint32_t*)(reg_base[id-1] + 0x00);
			volatile uint32_t *USART_DR  = (uint32_t*)(reg_base[id-1] + 0x04);

			while (!(*USART_SR & USART_SR_TXE));
			*USART_DR = buffer[i];
		}
	}
    return len;
}

uint32_t uart_read(int id, uint8_t* buffer, uint32_t len)
{
    uint32_t rlen;
    CriticalAlloc();
    EnterCritical();
    rlen = fifo_out(&(s_uart_fifo_dev[id-1]), buffer, len);
    ExitCritical();
    return rlen;
}

uint32_t uart_getrxlen(int id)
{
	return fifo_getlen(&(s_uart_fifo_dev[id-1]));
}
