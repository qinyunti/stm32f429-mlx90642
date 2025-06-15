#include <string.h>
#include "xmodem.h"

/* 符号定义 */
#define SOH   0x01
#define STX   0x02
#define EOT   0x04
#define ACK   0x06
#define NAK   0x15
#define CAN   0x18
#define CTRLZ 0x1A

/* 状态 */
#define XMODEM_STATE_IDLE 0

#define XMODEM_STATE_RX_START_WAIT 1
#define XMODEM_STATE_RX_START_WAIT_HEAD 2
#define XMODEM_STATE_RX_DATA_WAIT 3
#define XMODEM_STATE_RX_DATA_WAIT_HEAD 4

#define XMODEM_STATE_TX_START_WAIT 1
#define XMODEM_STATE_TX_DATA 2
#define XMODEM_STATE_TX_ACK_WAIT 3

/**
 * \struct xmodem_state_st
 * 状态机结构体
*/
typedef struct
{  
    uint8_t state;   /**< 状态机的状态 */  
    uint32_t getlen;  /**< 接收到的数据个数 */
    uint8_t pnum;    /**< 包ID            */  
    uint32_t ms;     /**< 上一个状态mS时间戳 */   

} xmodem_state_st;

static xmodem_cfg_st* s_cfg_pst = 0; /* 接口指针,用户初始化 */
static xmodem_state_st s_state_st; 

/**
 * crc计算参考https://www.iar.com/knowledge/support/technical-notes/general/checksum-generation/
*/
static const uint16_t t[] = 
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

static uint16_t crc_nibble_rom(uint16_t sum, uint8_t* p, uint32_t len)
{
    while (len--) 
    {
        // hi nibble
        sum = t[(sum>>12)^(*p >> 4)]^(sum<<4);
        // lo nibble
        sum = t[(sum>>12)^(*p++ & 0xF)]^(sum<<4);
    }
    return sum;
}

static int xmodem_check(uint16_t crc, uint8_t *buf, uint32_t sz)
{
    if (crc != 0) 
    {
        uint16_t crc = crc_nibble_rom(0, buf, sz);
        uint16_t tcrc = ((uint16_t)(buf[sz]) << 8) + (uint16_t)(buf[sz + 1]);
        if (crc == tcrc) 
        {
            return 0;
        }
    } 
    else 
    {
        uint8_t cks = 0;
        for (uint32_t i = 0; i < sz; i++) 
        {
            cks += buf[i];
        }
        if (cks == buf[sz]) 
        {
            return 0;
        }
    }
    return -1;
}

static uint16_t xmodem_check_cal(uint16_t crc, uint8_t *buf, uint32_t sz)
{    
    if (crc != 0) 
    {
        uint16_t check;
        check = crc_nibble_rom(0, buf, sz);
        buf[sz+1] = check & 0xFF;
        buf[sz] = (check>>8) & 0xFF;
        return check;
    } 
    else 
    {
        uint8_t cks = 0;
        uint32_t i;
        for (i = 0; i < sz; i++) 
        {
            cks += buf[i];
        }
        buf[i] = cks;
        return cks;
    }
}

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
int xmodem_rx(void)
{
    int res = 0;
    uint8_t tmp = NAK;
    uint32_t t = s_cfg_pst->getms();
    uint8_t* buf;
    uint16_t len;
    uint32_t getlen;
    if(s_cfg_pst == 0)
    {
        return -1;
    }
    if((s_cfg_pst->io_read == 0) || \
        (s_cfg_pst->io_write == 0) || \
        (s_cfg_pst->mem_write == 0) || \
        (s_cfg_pst->buffer == 0) || \
        (s_cfg_pst->getms == 0))
    {
        return -1;
    }

    switch(s_state_st.state)
    {
        case XMODEM_STATE_IDLE:
            res = -2;
        break;
        case XMODEM_STATE_RX_START_WAIT:
            /* 根据使用校验方式发送不同的启动字符,
             * 然后等待对方发送第一个包的包头
            */
            if(s_cfg_pst->crccheck)
            {
                tmp = 'C';
            }
            else
            {
                tmp = NAK;
            }
            s_cfg_pst->io_write(&tmp,1);
            s_state_st.ms = t;
            s_state_st.state = XMODEM_STATE_RX_START_WAIT_HEAD;
            s_cfg_pst->xferlen = 0;
        break;
        case XMODEM_STATE_RX_START_WAIT_HEAD:
        case XMODEM_STATE_RX_DATA_WAIT_HEAD:
            buf = s_cfg_pst->buffer;
            buf[0] = 0;
            if(0 != s_cfg_pst->io_read(buf,1))
            {
                if((buf[0] == SOH) || (buf[0] == STX))
                {
                    s_cfg_pst->plen = (buf[0] == SOH) ? 128 : 1024;
                    s_state_st.ms = t;
                    s_state_st.state = XMODEM_STATE_RX_DATA_WAIT;
                    s_state_st.getlen = 1;
                }

                /* 发送方结束 */
                if(buf[0] == EOT)
                {
                    tmp = ACK;
                    s_cfg_pst->io_write(&tmp,1);
                    s_state_st.state = XMODEM_STATE_IDLE;

                    if(s_cfg_pst->xferlen >= s_cfg_pst->totallen)
                    {
                        res = 1;
                    }
                    else
                    {
                        res = -3;
                    }
                }

                /* 发送方取消 */
                if(buf[0] == CAN)
                {
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -8;
                }
            }

            if((buf[0] != SOH) && (buf[0] != STX))
            {
                /* 非法值或者没有收到数据,继续等待直到超时 */
                if((t - s_state_st.ms) >= s_cfg_pst->ack_timeout)
                {
                    /* 超时未收到响应,继续重复发送启动字符 
                     * 如果超过了设置的超时时间则退出
                    */
                    if(s_cfg_pst->start_timeout <= 1)
                    {
                        /* 超时退出 */
                        s_state_st.state = XMODEM_STATE_IDLE;
                        res = -4;
                    }
                    else
                    {
                        /* 重新发送启动 */
                        if(s_state_st.state == XMODEM_STATE_RX_START_WAIT_HEAD)
                        {
                            s_cfg_pst->start_timeout--;
                            s_state_st.state = XMODEM_STATE_RX_START_WAIT;
                        }
                        else
                        {
                            /* 数据阶段等待头超时, 结束传输 */
                            s_state_st.state = XMODEM_STATE_IDLE;
                            res = -5;
                        }
                    }
                }
                else
                {
                    /* 未超时继续等待*/
                }
            }
        break;
        case XMODEM_STATE_RX_DATA_WAIT:
            buf = s_cfg_pst->buffer;
            len = s_cfg_pst->plen + 3 + ((s_cfg_pst->crccheck == 0) ? 1 : 2);
            if(s_cfg_pst->io_getrxlen != 0){
                if(s_cfg_pst->io_getrxlen() >= (len - s_state_st.getlen)){
                    /* 这里改为先查询再获取,避免io_read频繁去操作fifo导致关中断时间过长 */
                    getlen = s_cfg_pst->io_read(buf+s_state_st.getlen, len - s_state_st.getlen);
                    s_state_st.getlen += getlen;
                }
            }else{
                /* 没有提供函数io_getrxlen则直接读  */
                getlen = s_cfg_pst->io_read(buf+s_state_st.getlen, len - s_state_st.getlen);
                s_state_st.getlen += getlen; 
            }
            if(s_state_st.getlen >= len)
            {
                /* 接收完,准备判断合法性 */
                if(((uint8_t)(s_state_st.pnum + 1) != buf[1]) || ((buf[1] + buf[2]) != (uint8_t)255))
                {
                    /* 包ID错误,取消传输 */
                    tmp = CAN;
                    s_cfg_pst->io_write(&tmp,1);
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -6;
                }
                else
                {
                    /* 校验正确 */
                    if(0 == xmodem_check(s_cfg_pst->crccheck, buf+3, s_cfg_pst->plen))
                    {
                        s_state_st.pnum = buf[1];
                        s_state_st.state = XMODEM_STATE_RX_DATA_WAIT_HEAD;
                        s_state_st.ms = t;
                        /* 调用写接口存储数据 */
                        if((s_cfg_pst->xferlen + s_cfg_pst->plen) >= s_cfg_pst->totallen)
                        {
                            if(s_cfg_pst->xferlen >= s_cfg_pst->totallen)
                            {
                                /* 已经收到了指定的数据量,但是发送方还在发送,则主动取消 */
                                tmp = CAN;
                                s_cfg_pst->io_write(&tmp,1);
                                s_state_st.state = XMODEM_STATE_IDLE;
                                res = 1;
                            }
                            else
                            {
                                tmp = ACK;
                                s_cfg_pst->io_write(&tmp,1);
                                s_cfg_pst->mem_write(s_cfg_pst->addr+s_cfg_pst->xferlen, buf+3 ,s_cfg_pst->totallen-s_cfg_pst->xferlen);
                                s_cfg_pst->xferlen = s_cfg_pst->totallen;
                            }
                        }
                        else
                        {
                            tmp = ACK;
                            s_cfg_pst->io_write(&tmp,1);
                            if(0 == s_cfg_pst->mem_write(s_cfg_pst->addr+s_cfg_pst->xferlen, buf+3 ,s_cfg_pst->plen))
                            {
                                /* 写错误,取消传输 */
                                tmp = CAN;
                                s_cfg_pst->io_write(&tmp,1);
                                s_state_st.state = XMODEM_STATE_IDLE;
                                res = -9;
                            }
                            else
                            {
                            s_cfg_pst->xferlen += s_cfg_pst->plen;
                        }
                    }
                    }
                    else
                    {
                        tmp = NAK;
                        s_cfg_pst->io_write(&tmp,1);
                        s_state_st.state = XMODEM_STATE_RX_DATA_WAIT_HEAD;
                    }
                }
            }
            else
            {
                /* 未接收完判断超时 */
                if((t - s_state_st.ms) >= s_cfg_pst->packet_timeout)
                {
                    /* 超时退出 */
                    tmp = NAK;
                    s_cfg_pst->io_write(&tmp,1);
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -7;
                }
                else
                {
                    /* 未超时继续等待*/
                }
            }
        break;
    }
    return res;
}

/**
 * \fn xmodem_init_rx
 * 接收初始化
 * 以下成员必须初始化
 * \param[in] cfg \ref xmodem_cfg_st 配置信息
*/
void xmodem_init_rx(xmodem_cfg_st* cfg)
{
    s_cfg_pst = cfg;
    s_state_st.state = XMODEM_STATE_RX_START_WAIT;
    s_state_st.getlen = 0;
    s_state_st.pnum = 0;
    if(s_cfg_pst != 0)
    {
        if(s_cfg_pst->io_read_flush != 0)
        {
            s_cfg_pst->io_read_flush();
        }
    }
}

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
int xmodem_tx(void)
{
    int res = 0;
    uint8_t tmp = 0;
    uint32_t t = s_cfg_pst->getms();
    uint8_t* buf;
    uint16_t len;
    if(s_cfg_pst == 0)
    {
        return -1;
    }
    if((s_cfg_pst->io_read == 0) || \
        (s_cfg_pst->io_write == 0) || \
        (s_cfg_pst->mem_read == 0) || \
        (s_cfg_pst->buffer == 0) || \
        (s_cfg_pst->getms == 0))
    {
        return -1;
    }

    switch(s_state_st.state)
    {
        case XMODEM_STATE_IDLE:
            res = -2;
        break;
        case XMODEM_STATE_TX_START_WAIT:
            buf = s_cfg_pst->buffer;
            if(0 != s_cfg_pst->io_read(buf,1))
            {
                if((buf[0] == 'C') || (buf[0] == NAK))
                {
                    s_cfg_pst->crccheck = (buf[0] == 'C') ? 1 : 0;
                    s_state_st.ms = t;
                    s_state_st.state = XMODEM_STATE_TX_DATA;
                }

                if(buf[0] == CAN)
                {
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -3;
                }
            }

            if((buf[0] != 'C') && (buf[0] != NAK) && (buf[0] != CAN))
            {
                /* 非法值或者没有收到数据,继续等待直到超时 */
                if((t - s_state_st.ms) >= s_cfg_pst->ack_timeout)
                {
                    /* 超时未收到响应,继续重复发送启动字符 
                     * 如果超过了设置的超时时间则退出
                    */
                    if(s_cfg_pst->start_timeout <= 1)
                    {
                        /* 超时退出 */
                        s_state_st.state = XMODEM_STATE_IDLE;
                        res = -4;
                    }
                    else
                    {
                        s_cfg_pst->start_timeout--;
                    }
                    s_state_st.ms = t;
                }
                else
                {
                    /* 未超时继续等待*/
                }
            }
        break;
        case XMODEM_STATE_TX_DATA:
            buf = s_cfg_pst->buffer;
            buf[0] = (s_cfg_pst->plen == 1024)? STX : SOH;
            buf[1] = s_state_st.pnum;
            buf[2] = ~buf[1];
            if((s_cfg_pst->xferlen + s_cfg_pst->plen) >= s_cfg_pst->totallen)
            {
                len = s_cfg_pst->mem_read(s_cfg_pst->addr+s_cfg_pst->xferlen, buf+3,s_cfg_pst->totallen-s_cfg_pst->xferlen);
            }
            else
            {
                len = s_cfg_pst->mem_read(s_cfg_pst->addr+s_cfg_pst->xferlen, buf+3,s_cfg_pst->plen);
            }
            if((len < s_cfg_pst->plen) && (len > 0))
            {
                memset(buf+3+len, CTRLZ, s_cfg_pst->plen - len);
            }
            if(len > 0)
            {
            xmodem_check_cal(s_cfg_pst->crccheck, buf+3, s_cfg_pst->plen);
            len = s_cfg_pst->plen + 3 + ((s_cfg_pst->crccheck == 0) ? 1 : 2);
            s_cfg_pst->io_write(buf,len);
            s_state_st.state = XMODEM_STATE_TX_ACK_WAIT;
            s_state_st.ms = t;
            }
            else
            {
                /* 读完 */
                tmp = EOT;
                s_cfg_pst->io_write(&tmp,1);
                s_state_st.state = XMODEM_STATE_IDLE;
                res = 1;
            }
        break;
        case XMODEM_STATE_TX_ACK_WAIT:
            buf = s_cfg_pst->buffer;
            if(0 != s_cfg_pst->io_read(buf,1))
            {
                if(buf[0] == ACK) 
                {
                    /* 判断是否传输完,未传输完则发送下一包 */
                    if((s_cfg_pst->xferlen + s_cfg_pst->plen) >= s_cfg_pst->totallen)
                    {
                        /* 发送完 */
                        s_state_st.state = XMODEM_STATE_IDLE;
                        s_state_st.pnum++;
                        s_cfg_pst->xferlen += s_cfg_pst->plen;
                        if(s_cfg_pst->xferlen >= s_cfg_pst->totallen)
                        {
                            s_cfg_pst->xferlen = s_cfg_pst->totallen;
                        }
                        tmp = EOT;
                        s_cfg_pst->io_write(&tmp,1);
                        res = 1;
                    }
                    else
                    {
                        /* 发送下一包 */
                        s_state_st.state = XMODEM_STATE_TX_DATA;
                        s_state_st.pnum++;
                        s_cfg_pst->xferlen += s_cfg_pst->plen;
                    }
                }

                if(buf[0] == NAK) 
                {
                    /* 重发 */
                    s_state_st.state = XMODEM_STATE_TX_DATA;
                }

                if(buf[0] == CAN)
                {
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -3;
                }
            }

            if((buf[0] != ACK) && (buf[0] != NAK) && (buf[0] != CAN))
            {
                /* 非法值或者没有收到数据,继续等待直到超时 */
                if((t - s_state_st.ms) >= s_cfg_pst->ack_timeout)
                {
                    /* 超时退出 */
                    s_state_st.state = XMODEM_STATE_IDLE;
                    res = -5;
                }
                else
                {
                    /* 未超时继续等待*/
                }
            }
        break;
    }
    return res;
}

/**
 * \fn xmodem_init_tx
 * 发送初始化
 * 以下成员必须初始化
 * \param[in] cfg \ref xmodem_cfg_st 配置信息
*/
void xmodem_init_tx(xmodem_cfg_st* cfg)
{
    s_cfg_pst = cfg;
    s_state_st.state = XMODEM_STATE_TX_START_WAIT;  /* 等待接收端发送启动 */
    s_state_st.getlen = 0;
    s_state_st.pnum = 1;
    if(s_cfg_pst != 0)
    {
        s_state_st.ms = s_cfg_pst->getms();
        s_cfg_pst->xferlen = 0;
        if(s_cfg_pst->io_read_flush != 0)
        {
            s_cfg_pst->io_read_flush();
        }
    }
}