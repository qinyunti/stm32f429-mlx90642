#include "stm32f4_regs.h"
#include "gpio.h"
#include "clock.h"
#include "spi.h"

static uint32_t reg_base[6]={0x40013000,0x40003800,0x40003C00,0x40013400,0x40015000,0x40015400};

void spi_init(int id, spi_cfg_st* cfg){
	
	volatile uint16_t* cr1 = (uint16_t*)(reg_base[id-1] + 0x00);
	//volatile uint16_t* cr2 = (uint16_t*)(reg_base[id-1] + 0x04);
	volatile uint32_t *RCC_APB1ENR = (void *)(RCC_BASE + 0x40);
	volatile uint32_t *RCC_APB2ENR = (void *)(RCC_BASE + 0x44);
	volatile uint32_t *RCC_AHB1ENR = (void *)(RCC_BASE + 0x30);

	switch(id){
		case 1:
			/* 引脚时钟使能 引脚初始化 
			* PA4  SPI1_NSS  GPIO
			* PA5  SPI1_SCK  AF5
			* PA6  SPI1_MISO AF5 
			* PA7  SPI1_MOSI AF5
			*/
			*RCC_AHB1ENR |= (1u<<0); /* GPIOA */
			gpio_set((void*)GPIOA_BASE, 'A', 4, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
			gpio_write((void*)GPIOA_BASE, 'A', 4, 1);	
			gpio_set_alt((void*)GPIOA_BASE, 'A', 5, 0, GPIOx_OSPEEDR_OSPEEDRy_HIGH, 0, 0x5);
			gpio_set_alt((void*)GPIOA_BASE, 'A', 6, 0, GPIOx_OSPEEDR_OSPEEDRy_HIGH, 0, 0x5);
			gpio_set_alt((void*)GPIOA_BASE, 'A', 7, 0, GPIOx_OSPEEDR_OSPEEDRy_HIGH, 0, 0x5);
		break;
		case 5:
			/**
			 * PF7 SPI5_SCK   AF5
			 * PF9 SPI5_MOSI  AF5
			 * PC2 CS  GPIO
			 * PD13 数据/命令 GPIO
			 */
			*RCC_AHB1ENR |= (1u<<5); /* GPIOF */
			gpio_set_alt((void*)GPIOA_BASE, 'F', 7, 0, GPIOx_OSPEEDR_OSPEEDRy_HIGH, 0, 0x5);
			gpio_set_alt((void*)GPIOA_BASE, 'F', 9, 0, GPIOx_OSPEEDR_OSPEEDRy_HIGH, 0, 0x5);

			*RCC_AHB1ENR |= (1u<<2); /* GPIOC */
			gpio_set((void*)GPIOA_BASE, 'C', 2, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
			gpio_write((void*)GPIOA_BASE, 'C', 2, 1);
		break;
	}

	/* SPI时钟初始化 */
	if(id==1){
		*RCC_APB2ENR |= (1u<<12); /* SPI1 */
	}else if(id == 2){
		*RCC_APB1ENR |= (1u<<14); /* SPI2 */
	}else if(id == 3){
		*RCC_APB1ENR |= (1u<<15); /* SPI3 */
	}else if(id == 5){
		*RCC_APB2ENR |= (1u<<20); /* SPI5 */
	}
	/* SPI配置 */
	uint16_t tmp = 0;
	uint32_t div = 0;
	if(cfg->databits == 16){
		tmp |= (1u<<11); /* 0: 8-bit data frame format is selected for transmission/reception 1: 16-bit */
	}
	tmp |= (1u<<9); /* 1: Software slave management enabled */
	tmp |= (1u<<8);
	if(cfg->msb == 0){
		tmp |= (1u<<7);  /* 0: MSB transmitted first 1: LSB transmitted first */
	}
	tmp |= (1u<<6);  /* 1: Peripheral enabled */
	if((id == 1) || (id == 5)){
		div = clock_get_apb2() / cfg->baud;
	}else{
		div = clock_get_apb1() / cfg->baud;	
	}
	/* 5:3 */
	if(div <= 2){
		tmp |= (0u<<3);
	}else if(div <= 4){
		tmp |= (1u<<3);
	}else if(div <= 8){
		tmp |= (2u<<3);
	}else if(div <= 16){
		tmp |= (3u<<3);
	}else if(div <= 32){
		tmp |= (4u<<3);
	}else if(div <= 64){
		tmp |= (5u<<3);
	}else if(div <= 128){
		tmp |= (6u<<3);
	}else{
		tmp |= (7u<<3);	
	}
	tmp |= (1u<<2);  /* 1: Master configuration */
	if(cfg->mode & 0x02){
		tmp |= (1u<<1);  /* 0: CK to 0 when idle 1: CK to 1 when idle */
	}
	if(cfg->mode & 0x01){
		tmp |= (1u<<0);  /* 0: The first clock transition is the first data capture edge 1: The second clock transition is the first data capture edge */
	}	
	*cr1 = tmp;
}

uint32_t spi_transfer(int id, uint8_t* tx, uint8_t* rx, uint32_t len, int flag)
{
	volatile uint16_t* sr = (uint16_t*)(reg_base[id-1] + 0x08);
	volatile uint16_t* dr = (uint16_t*)(reg_base[id-1] + 0x0C);
    uint8_t rx_tmp;
	uint8_t tx_tmp = 0xFF;
	/* CS拉低 */
	switch(id){
		case 1:
		gpio_write((void*)GPIOA_BASE, 'A', 4, 0);
		break;
		case 5:
		gpio_write((void*)GPIOA_BASE, 'C', 2, 0);
		break;
	}	
	for(uint32_t i=0; i<len; i++){
		if(tx != (uint8_t*)0){
            *dr = (uint16_t)tx[i];
		}else{
			*dr = (uint16_t)tx_tmp; /* 无tx也要发送数据 */
		}
		while((*sr & (uint16_t)0x01) == 0); /* wait RXNE */
		rx_tmp = (uint8_t)*dr;
		if(rx != (uint8_t*)0){
			rx[i] = rx_tmp;
		}
	}

	if(flag){
		/* CS拉高 */
		switch(id){
			case 1:
			gpio_write((void*)GPIOA_BASE, 'A', 4, 1);
			break;
			case 5:
			gpio_write((void*)GPIOA_BASE, 'C', 2, 1);
			break;
		}	
	}
    return len;
}


