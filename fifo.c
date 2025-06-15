#include <string.h>
#include "fifo.h"

#define FIFO_PARAM_CHECK 0

/**
 * in为写入索引 0~(buffer_len-1)。
 * out为读出索引 0~(buffer_len-1)。
 * in == out时可能是满,也可能是空,可以通过len有效数据长度来确认。
 * 写数据in增加,直到追赶到out则满。
 * 读数据则out增加,直到追赶到in则空。
 * in大于out时则[out,in)区间是有效数据。
 * in小于out时则[out,buffer_len)和[0,in)区间是有效数据。
 ***********************************************************
 *     0                                 buffer_len-1 buffer_len
 *     （1）开始 in和out都是0
 *     |                                             |
 *     in(0)
 *     out(0)
 *     len = 0
 *     （2）写入n字节数据 in变为n和out还是0 对应in大于out的情况
 *     |                                             |
 *     out(0)————————————>in(n)                      |   
 *     len = n
 *     （3）读出m字节数据(m<n) in还是n和out变为m 对应in大于out的情况
 *     |                                             |
 *             out(m)————>in(n)
 *     len = n-m
 *     （4）继续写入数据,绕回到开头,对应in小于out的情况
 *     |                                             |
 *             out(m)————————————————————————————————>
 *     ——>in(k)
 *     len = k + buffer_len-m
 */
uint32_t fifo_in(fifo_st* dev, uint8_t* buffer, uint32_t len)
{
  uint32_t space = 0;  /* 用于记录空闲空间大小 */
  /* 参数检查 */
  #if FIFO_PARAM_CHECK
  if((dev == 0) || (buffer == 0) || (len == 0))
  {
    return 0;
  }
  if(dev->buffer == 0)
  {
    return 0;
  }
  #endif

  /* 限制len的最大长度为buffer大小 */
  if(len > dev->buffer_len)
  {
    len = dev->buffer_len;
  }

  /* 计算空闲空间大小 
   * 正常dev->len不应该大于dev->buffer_len
   */
  if(dev->buffer_len >= dev->len)
  {
    space = dev->buffer_len - dev->len; 
  }
  else
  {
    /* 这里不应该出现, 出现则是异常 */
    dev->len = 0;
    space = dev->buffer_len; 
  }

  /* 计算待写入大小, 如果len大于剩余空间则只写入剩余空间大小 */
  len = (len >= space) ? space : len;  
  if(len == 0)
  {
    return 0; /* 这里有可能无剩余空间,直接返回 */
  }

  /* 计算len的长度是否需要有绕回,需要分次写入 */
  space = dev->buffer_len - dev->in; /* 当前写入位置in到缓存末尾剩余可写入空间 */
  if(space >= len)
  {
    /* 当前写入位置in到缓存末尾足够一次写入 */
    memcpy(dev->buffer+dev->in,buffer,len);
  }
  else
  {
    /* 当前写入位置in到缓存末尾不够,还需要绕回到前面写 */
    memcpy(dev->buffer+dev->in,buffer,space);    /* 先写入tail部分  */
    memcpy(dev->buffer,buffer+space,len-space);  /* 再写入绕回头部分 */
  } 
  /* 更新写入索引和有效数据长度 */
  dev->in += len;
  if(dev->in >= dev->buffer_len)
  {
    dev->in -= dev->buffer_len;  /* 判断加减法 替代 dev->in %= dev->buffer->len */
  }
  dev->len += len;  /* dev->len最大dev->buffer->len,无需%= dev->buffer->len */
  return len;
}

uint32_t fifo_out(fifo_st* dev, uint8_t* buffer, uint32_t len)
{
  uint32_t space = 0; 
  /* 参数检查 */
  #if FIFO_PARAM_CHECK
  if((dev == 0) || (buffer == 0) || (len == 0))
  {
    return 0;
  }
  if(dev->buffer == 0)
  {
    return 0;
  }
  #endif
  
  /* 判断是否有数据 */
  if(dev->len == 0)
  {
    return 0;
  }

  /* 可读出数据量取需要的和有的之间的小值 */
  len = (dev->len) > len ? len : dev->len;

  /* 计算len的长度是否需要有绕回,需要分次读出 */
  space = dev->buffer_len - dev->out; /* 当前读出位置out到缓存末尾剩余可读出空间 */
  if(space >= len)
  {
    /* 当前读出位置out到缓存末尾足够一次读出 */
    memcpy(buffer,dev->buffer+dev->out,len);
  }
  else
  {
    /* 当前读出位置out到缓存末尾不够,还需要绕回到前面读 */
    memcpy(buffer,dev->buffer+dev->out,space);    /* 先读出tail部分  */
    memcpy(buffer+space,dev->buffer,len-space);   /* 再读出绕回头部分 */
  } 
  /* 更新读出索引和有效数据长度 */
  dev->out += len;
  if(dev->out >= dev->buffer_len)
  {
    dev->out -= dev->buffer_len;  /* 判断加减法 替代 dev->out %= dev->buffer->len */
  }
  dev->len -= len;   /* 这里dev->len 不可能小于len,不会溢出 */
  return len;
}

uint32_t fifo_getlen(fifo_st* dev)
{
  #if FIFO_PARAM_CHECK
  if(dev == 0)
  {
    return 0;
  }
  #endif
  return dev->len;
}

void fifo_clean(fifo_st* dev)
{
  #if FIFO_PARAM_CHECK
  if(dev == 0)
  {
    return 0;
  }
  #endif
  dev->len = 0;
  dev->in = 0;
  dev->out = 0;
}

uint32_t fifo_getfree(fifo_st* dev)
{
  if(dev == 0)
  {
    return 0;
  }
  return dev->buffer_len - dev->len;
}