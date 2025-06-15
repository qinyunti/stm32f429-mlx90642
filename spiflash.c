#include "string.h"
#include <stdlib.h>
#include <stdio.h>
#include "spiflash.h"

#define FLASH_CMD_WEL 0x06
#define FLASH_CMD_PAGEPROGRAM 0x02
#define FLASH_CMD_READ 0x03
#define FLASH_CMD_ERASESECTOR 0x20
#define FLASH_CMD_READSR1 0x05
#define FLASH_CMD_READSR2 0x35
#define FLASH_CMD_READSR3 0x15

#define FLASH_CMD_WRITESR1 0x01
#define FLASH_CMD_WRITESR2 0x31
#define FLASH_CMD_WRITESR3 0x11

#define FLASH_SR1_BUSY  (1 << 0)

int flash_write_enable(flash_dev_st* dev)
{
	uint8_t cmd = FLASH_CMD_WEL;
	dev->spi_trans(&cmd, 0, 1, 1);
	return 0;
}

int flash_read_sr1(flash_dev_st* dev, uint8_t* sr)
{
	uint8_t cmd[2] = {FLASH_CMD_READSR1,0xFF};
	uint8_t rx[2];
	dev->spi_trans(cmd, rx, 2, 1);
	*sr = rx[1];
	return 0;
}

int flash_read_sr2(flash_dev_st* dev, uint8_t* sr)
{
	uint8_t cmd[2] = {FLASH_CMD_READSR2,0xFF};
	uint8_t rx[2];
	dev->spi_trans(cmd, rx, 2, 1);
	*sr = rx[1];
	return 0;
}

int flash_read_sr3(flash_dev_st* dev, uint8_t* sr)
{
	uint8_t cmd[2] = {FLASH_CMD_READSR3,0xFF};
	uint8_t rx[2];
	dev->spi_trans(cmd, rx, 2, 1);
	*sr = rx[1];
	return 0;
}


int flash_write_sr1(flash_dev_st* dev, uint8_t val)
{
	uint8_t cmd[2] = {FLASH_CMD_WRITESR1,0};
	cmd[1] = val;
	dev->spi_trans(cmd, 0, 2, 1);
	return 0;
}

int flash_write_sr2(flash_dev_st* dev, uint8_t val)
{
	uint8_t cmd[2] = {FLASH_CMD_WRITESR2,0};
	cmd[1] = val;
	dev->spi_trans(cmd, 0, 2, 1);
	return 0;
}


int flash_write_sr3(flash_dev_st* dev, uint8_t val)
{
	uint8_t cmd[2] = {FLASH_CMD_WRITESR3,0};
	cmd[1] = val;
	dev->spi_trans(cmd, 0, 2, 1);
	return 0;
}


int flash_wait_busy(flash_dev_st* dev)
{
	uint8_t sr = 0;
	do{
		flash_read_sr1(dev, &sr);
	} while(FLASH_SR1_BUSY & sr);
	return 0;
}


int flash_erase_sector(flash_dev_st* dev, uint32_t addr)
{
	uint8_t cmd[4];
	flash_write_enable(dev);
    cmd[0] = FLASH_CMD_ERASESECTOR;
	cmd[1] = (uint8_t)(addr >> 16 & 0xFF);
	cmd[2] = (uint8_t)(addr >> 8  & 0xFF);
	cmd[3] = (uint8_t)(addr >> 0  & 0xFF);
	dev->spi_trans(cmd, 0, 4, 1);
	flash_wait_busy(dev);
	return 0;
}


int flash_pageprogram(flash_dev_st* dev, uint8_t* buffer, uint32_t addr, uint32_t len)
{
	uint8_t cmd[4];
    cmd[0] = FLASH_CMD_PAGEPROGRAM;
	cmd[1] = (uint8_t)(addr >> 16 & 0xFF);
	cmd[2] = (uint8_t)(addr >> 8  & 0xFF);
	cmd[3] = (uint8_t)(addr >> 0  & 0xFF);
	flash_write_enable(dev);
	dev->spi_trans(cmd, 0, 4, 0);
	dev->spi_trans(buffer, 0, len, 1);
	flash_wait_busy(dev);
	return 0;
}
	

uint32_t flash_read(flash_dev_st* dev, uint8_t* buffer, uint32_t addr, uint32_t len)
{
    uint8_t cmd[4];
    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (uint8_t)(addr >> 16 & 0xFF);
    cmd[2] = (uint8_t)(addr >> 8  & 0xFF);
    cmd[3] = (uint8_t)(addr >> 0  & 0xFF);
	dev->spi_trans(cmd, 0, 4, 0);
	dev->spi_trans(0, buffer, len, 1);
	return len;
}


uint32_t flash_write(flash_dev_st* dev, uint8_t* buffer, uint32_t addr, uint32_t len)
{
   /** 
	*   |                     |                    |                  |                        |
	*             |                                                                   |
	*             <                           len                                     >      
    *             addr
	*   <sec_head >                                                    <  sec_tail  >
	*   start_addr
    */
	uint32_t sec_head;
	uint32_t sec_tail = 0;
	uint32_t sec_mid_num;
	uint32_t start_addr;
	uint32_t end_addr;
	
	uint32_t fill;
	
	start_addr = addr & (~(dev->sector_size-1));
	end_addr = (addr+len) & (~(dev->sector_size-1));
	sec_head = addr & (dev->sector_size-1);   
	if((end_addr != start_addr) || (sec_head == 0)){
		sec_tail = (addr + len) & (dev->sector_size-1); 
	}
	sec_mid_num = (end_addr - start_addr) >> dev->sector_bits;
	if((sec_head != 0) || (sec_tail != 0)){
		if(sec_mid_num > 1){
			sec_mid_num--;
		}else{
			sec_mid_num = 0;
		}
	}
	/* head */
	if(sec_head > 0)
    {
		//flash_read(dev,dev->buffer, start_addr, dev->sector_size); 
		flash_read(dev,dev->buffer+sec_head, start_addr+sec_head, dev->sector_size-sec_head); 
		fill =( len > (dev->sector_size-sec_head)) ? (dev->sector_size-sec_head) : len;
		memcpy(dev->buffer+sec_head,buffer,fill);
		flash_erase_sector(dev,start_addr);
        for (uint32_t j=0; j<16; j++) {
			flash_pageprogram(dev,dev->buffer + (j<<dev->page_bits),start_addr + (j<<dev->page_bits), dev->page_size);  
    	}
		buffer += fill;
		start_addr += dev->sector_size;
	}
	/* mid */
	for(uint32_t i=0; i<sec_mid_num; i++)
    {
	    flash_erase_sector(dev,start_addr);
        for (uint32_t j=0; j<16; j++) {
			flash_pageprogram(dev, buffer + (j<<dev->page_bits), start_addr + (j<<dev->page_bits), dev->page_size);  
        }
		buffer += dev->sector_size;
		start_addr += dev->sector_size;
	}
	/* tail */
	if(sec_tail > 0)
    {
		flash_read(dev,dev->buffer+sec_tail, start_addr+sec_tail, dev->sector_size - sec_tail); 
		memcpy(dev->buffer,buffer,sec_tail);
		flash_erase_sector(dev,start_addr);
        for (uint32_t j=0; j<16; j++) {
			flash_pageprogram(dev, dev->buffer + (j<<dev->page_bits),start_addr + (j<<dev->page_bits), dev->page_size);  
        }
	}
	return len;
}

