#ifndef SPIFLASH_H
#define SPIFLASH_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>

typedef uint32_t (*flash_spi_io)(uint8_t* tx_buffer, uint8_t* rx_buffer, uint32_t len, int flag);

/**
 * \struct flash_dev_st
 * FLASH结构.
 */
typedef struct 
{
	flash_spi_io spi_trans;  /**< spi传输接口 */
	void* buffer;            /**< sector缓存  */
	uint32_t sector_size;    /**< sector大小  */
	uint32_t sector_bits;    /**< sector大小 位数  */	
	uint32_t page_size;      /**< page大小  */
	uint32_t page_bits;      /**< page大小 位数  */	
} flash_dev_st;

/**
 * \fn flash_read
 * 读数据
 * \param[in] dev \ref flash_dev_st
 * \param[in] addr 读开始地址
 * \param[out] buffer 存储读出的数据
 * \param[in] len 待读出的长度
 * \retval 返回实际读出的数据长度
 */
uint32_t flash_read(flash_dev_st* dev, uint8_t* buffer, uint32_t addr, uint32_t len);

/**
 * \fn flash_write
 * 写数据
 * \param[in] dev \ref flash_dev_st
 * \param[in] addr 写开始地址
 * \param[out] buffer 存储待写的数据
 * \param[in] len 待写入的长度
 * \retval 返回实际写入的数据长度
 */
uint32_t flash_write(flash_dev_st* dev, uint8_t* buffer, uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
