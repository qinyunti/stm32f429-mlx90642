#ifndef FIFO_H
#define FIFO_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>

/**
 * \struct fifo_st
 * FIFO缓冲区结构.
 */
typedef struct 
{
  uint32_t in;          /**< 写入索引        */ 
  uint32_t out;         /**< 读出索引        */ 
  uint32_t len;         /**< 有效数据长度    */ 
  uint32_t buffer_len;  /**< 有效长度        */ 
  uint8_t* buffer;      /**< 缓存,用户分配   */

} fifo_st;
  
/**
 * \fn fifo_in
 * 往fifo里写数据
 * \param[in] dev \ref fifo_st
 * \param[in] buffer 待写入的数据
 * \param[in] len 待写入的长度
 * \retval 返回实际写入的数据量
 */
uint32_t fifo_in(fifo_st* dev, uint8_t* buffer, uint32_t len);

/**
 * \fn fifo_out
 * 从fifo读出数据
 * \param[in] dev \ref fifo_st
 * \param[in] buffer 存读出的数据
 * \param[in] len 需要读出的数据长度
 * \retval 返回实际读出的数据量
 */
uint32_t fifo_out(fifo_st* dev, uint8_t* buffer, uint32_t len);


uint32_t fifo_getlen(fifo_st* dev);

void fifo_clean(fifo_st* dev);
uint32_t fifo_getfree(fifo_st* dev);
#ifdef __cplusplus
}
#endif

#endif