#ifndef XMODEM_H
#define XMODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef uint32_t (*xmodem_io_read_pf)(uint8_t* buffer, uint32_t len); /**< 通讯接收接口 */
typedef uint32_t (*xmodem_io_getrxlen_pf)(void); /**< 通讯接口获取已经接收数据长度 */
typedef uint32_t (*xmodem_io_write_pf)(uint8_t* buffer, uint32_t len);/**< 通讯发送接口 */
typedef void (*xmodem_read_flush_pf)(void);   /**< 通讯接收flush接口 */
typedef void (*xmodem_write_flush_pf)(void);  /**< 通讯发送flush接口 */
typedef uint32_t (*xmodem_mem_read_pf)(uint32_t addr, uint8_t* buffer, uint32_t len);  /**< 读存储接口 */
typedef uint32_t (*xmodem_mem_write_pf)(uint32_t addr, uint8_t* buffer, uint32_t len); /**< 写存储接口 */
typedef uint32_t (*xmodem_getms_pf)(void);                              /**< 获取mS时间戳 */

/**
 * \struct xmodem_cfg_st
 * 接口,参数配置结构体
*/
typedef struct
{ 
    xmodem_io_read_pf io_read;             /**< 通讯接收接口 */
    xmodem_io_getrxlen_pf io_getrxlen;     /**< 通讯接收接口，获取已经接收数据长度  */
    xmodem_io_read_pf io_write;            /**< 通讯发送接口 */
    xmodem_read_flush_pf io_read_flush;    /**< 通讯接收flush接口 */
    xmodem_write_flush_pf io_write_flush;  /**< 通讯发送flush接口 */
    xmodem_mem_read_pf mem_read;           /**< 读存储接口 */
    xmodem_mem_write_pf mem_write;         /**< 写存储接口 */
    xmodem_getms_pf getms;                 /**< 获取mS时间戳接口 */
    uint32_t start_timeout;                /**< 等待启动超时次数,单位每次超时为ack_timeout  */
    uint32_t packet_timeout;               /**< 等待数据包超时时间mS */
    uint32_t ack_timeout;                  /**< 等待响应(头)超时时间mS*/
    uint8_t  crccheck;                     /**< 对于接收时有效1使用CRC校验 0使用累加和校验 */
    uint16_t  plen;                        /**< 对于发送时有效指定包长时128还是1024       */
    uint8_t* buffer;                       /**< 包缓冲区,用户提供,1024字节包需要1024+5    */
    uint32_t addr;                         /**< 存储地址                                */
    uint32_t totallen;                     /**< 传输长度                                */
    uint32_t xferlen;                      /**< 已经传输长度                                */
} xmodem_cfg_st;

/**
 * \fn xmodem_rx
 * 接收处理, 在此之前必须要先调用xmodem_init_rx初始化
 * \retval 0 正在处理,需要循环继续调用xmodem_rx处理
 * \retval 1 接收正常完成
 * \retval -1 参数错误,未初始化对应的接口
 * \retval -2 未初始化,处于空闲状态
 * \retval -3 发送方提前结束
 * \retval -4 启动后超时未收到包头
 * \retval -5 传输过程中,等待包头超时
 * \retval -6 传输过程中,包ID错误,取消传输
 * \retval -7 传输过程中,等待数据超时
 * \retval -8 传输过程中,发送方取消
 * \retval -9 传输过程中,写数据错误
*/
int xmodem_rx(void);

/**
 * \fn xmodem_init_rx
 * 接收初始化
 * 以下成员必须初始化
 * \param[in] cfg \ref xmodem_cfg_st 配置信息
*/
void xmodem_init_rx(xmodem_cfg_st* cfg);

/**
 * \fn xmodem_tx
 * 发送处理, 在此之前必须要先调用xmodem_init_tx初始化
 * \retval 0 正在处理,需要循环继续调用xmodem_tx处理
 * \retval 1 发送正常完成
 * \retval -1 参数错误,未初始化对应的接口
 * \retval -2 未初始化,处于空闲状态
 * \retval -3 接收方提前取消
 * \retval -4 启动阶段超时未收到响应
 * \retval -5 数据阶段超时未收到响应
*/
int xmodem_tx(void);

/**
 * \fn xmodem_init_tx
 * 发送初始化
 * 以下成员必须初始化
 * \param[in] cfg \ref xmodem_cfg_st 配置信息
*/
void xmodem_init_tx(xmodem_cfg_st* cfg);


#ifdef __cplusplus
}
#endif

#endif 