#ifndef IO_IIC_H
#define IO_IIC_H

#ifdef __cplusplus
    extern "C"{
#endif

#include <stdint.h>

typedef void    (*io_iic_scl_write_pf)(uint8_t val);   /**< SCL写接口     */
typedef void    (*io_iic_sda_write_pf)(uint8_t val);   /**< SDA写接口     */
typedef void    (*io_iic_sda_2read_pf)(void);          /**< SDA转为读接口 */
typedef uint8_t (*io_iic_sda_read_pf)(void);           /**< SDA读接口     */
typedef void    (*io_iic_delay_us_pf)(uint32_t delay); /**< 延时接口      */
typedef void    (*io_iic_init_pf)(void);               /**< 初始化接口     */
typedef void    (*io_iic_deinit_pf)(void);             /**< 解除初始化接口 */

/**
 * \struct io_iic_dev_st
 * 接口结构体
*/
typedef struct
{
    io_iic_scl_write_pf scl_write;   /**< scl写接口    */
    io_iic_sda_write_pf sda_write;   /**< sda写接口    */
    io_iic_sda_2read_pf sda_2read;   /**< sda转为读接口 */
    io_iic_sda_read_pf  sda_read;    /**< sda读接口     */
    io_iic_delay_us_pf  delay_pf;    /**< 延时接口      */
    io_iic_init_pf      init;        /**< 初始化接口    */
    io_iic_deinit_pf    deinit;      /**< 解除初始化接口 */
    volatile uint32_t            delayus;     /**< 延迟时间      */
} io_iic_dev_st;

/**
 * \fn io_iic_start
 * 发送启动信号
 * \param[in] dev \ref io_iic_dev_st
*/
void io_iic_start(io_iic_dev_st* dev);

/**
 * \fn io_iic_stop
 * 发送停止信号
 * \param[in] dev \ref io_iic_dev_st
*/
void io_iic_stop(io_iic_dev_st* dev);

/**
 * \fn io_iic_write
 * 写一个字节
 * \param[in] dev \ref io_iic_dev_st
 * \param[in] val 待写入的值
 * \retval 0 写成功(收到了ACK)
 * \retval -2 写失败(未收到ACK)
 * \retval -1 参数错误
*/
int io_iic_write(io_iic_dev_st* dev, uint8_t val);

/**
 * \fn io_iic_read
 * 读一个字节
 * \param[in] dev \ref io_iic_dev_st
 * \param[out] val 存储读到的值
 * \param[in] ack 1发送NACK 0发送ACK
 * \retval 0 读成功
 * \retval -1 参数错误
*/
int io_iic_read(io_iic_dev_st* dev, uint8_t* val, uint8_t ack);

/**
 * \fn io_iic_init
 * 初始化
 * \param[in] dev \ref io_iic_dev_st
*/
void io_iic_init(io_iic_dev_st* dev);

/**
 * \fn io_iic_deinit
 * 解除初始化
 * \param[in] dev \ref io_iic_dev_st
*/
void io_iic_deinit(io_iic_dev_st* dev);

#ifdef __cplusplus
    }
#endif

#endif