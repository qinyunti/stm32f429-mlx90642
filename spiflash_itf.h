#ifndef SPIFLASH_ITF_H
#define SPIFLASH_ITF_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>

/**
 * \fn flash_itf_init
 * 初始化
 */
int flash_itf_init(void);

/**
 * \fn flash_itf_deinit
 * 反初始化
 */
int flash_itf_deinit(void);

/**
 * \fn flash_itf_read
 * 读数据
 * \param[in] addr 读开始地址
 * \param[out] buffer 存储读出的数据
 * \param[in] len 待读出的长度
 * \retval 返回实际读出的数据长度
 */
uint32_t flash_itf_read(uint8_t* buffer, uint32_t addr, uint32_t len);

/**
 * \fn flash_itf_write
 * 写数据
 * \param[in] addr 写开始地址
 * \param[out] buffer 存储待写的数据
 * \param[in] len 待写入的长度
 * \retval 返回实际写入的数据长度
 */
uint32_t flash_itf_write(uint8_t* buffer, uint32_t addr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif