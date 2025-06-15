#include "io_iic.h"

/**

 *                     _______________________   
 *    SCL ____________|                       |__
 *        ————————————————————————
 *    SDA                         |______________
 *        （1）      （2）        (4)         （6）
 *                        （3）           (5)
 *    其中(3) SDA低建立时间 (5) SDA高保持时间
 *    (1) 拉高SDA  （4）拉高SDA产生上升沿
 *    (2) SCL拉高 SCL高时SDA上升沿即停止信号
 */
void io_iic_start(io_iic_dev_st* dev)
{
    /* SCL高时,SDA下降沿 */
    dev->sda_write(1);               /* (1) SDA拉高以便后面产生下降沿  */
    dev->scl_write(1);               /* (2) 拉高SCL                   */
    if(dev->delay_pf != 0)           /* (3) SCL高保持*/
    {
        dev->delay_pf(dev->delayus);
    }
    dev->sda_write(0);              /* (4)SCL高时SDA下降沿 启动    */
    if(dev->delay_pf != 0)          /* (5)SCL高保持               */
    {
        dev->delay_pf(dev->delayus);
    }
    dev->scl_write(0);             /* (6)SCL恢复                 */
}

/**

 *                     _______________________   
 *    SCL ____________|                       |____
 *                                 ————————————————
 *    SDA ————————————————————————|
 *        （1）      （2）        (4)         （6）
 *                        （3）           (5)
 *    其中(3) SDA低建立时间 (5) SDA高保持时间
 *    (1) 拉低SDA  （4）拉高SDA产生上升沿
 *    (2) SCL拉高 SCL高时SDA上升沿即停止信号
 */
void io_iic_stop(io_iic_dev_st* dev)
{
    /* SCL高时,SDA上升沿 */
    dev->sda_write(0);               /* (1)SDA先输出低以便产生上升沿 */
    dev->scl_write(1);               /* (2)SCL高                   */
    if(dev->delay_pf != 0)           /* (3)SCL高保持               */
    {
        dev->delay_pf(dev->delayus);
    } 
    dev->sda_write(1);               /* (4)SCL高时SDA上升沿 停止    */
    if(dev->delay_pf != 0)           /* (5)SCL高保持               */
    {
        dev->delay_pf(dev->delayus);
    }
    //dev->scl_write(0);               /* (6)SCL恢复                 */
}

/**
 *   |       B0               | B1~B6|    B7               |           NACK/ACK      |
 *                 ___________      _            __________              ____________
 *    ____________|           |   x  |__________|          |____________|            |
 * (1)[2]      (4)                                      (6)[7]        (9)[10]       (12)
 *        (3)           (5)                                     (8)           (11) 
 * 其中(1)(6)(12)拉低SCL;(4)(9)拉高SCL；
 * [2]输出  [7]转为读 [10]读ACK；
 * (3)(8)低保持时间,(5)(11)高保持时间。
 */
int io_iic_write(io_iic_dev_st* dev, uint8_t val)
{
    uint8_t tmp = val;
    uint8_t ack = 0;
    if(dev == 0)
    {
        return -1;
    }
    if((dev->scl_write == 0) || (dev->sda_write == 0) || (dev->sda_read == 0) || (dev->sda_2read == 0))
    {
        return -1;
    }
    /* SCL下降沿后准备数据,对方上升沿采集数据,高位在前 */
    for(uint8_t i=0; i<8; i++)
    {
        dev->scl_write(0);               /* (1) SCL拉低以便修改数据    */
        if((tmp & 0x80) != 0)            /* [2] 准备SDA数据            */
        {
            dev->sda_write(1); 
        }
        else
        {
            dev->sda_write(0);  
        }
        if(dev->delay_pf != 0)
        {
            dev->delay_pf(dev->delayus); /* (3) SCL拉低时间即数据建立时间 */
        }
        dev->scl_write(1);                /*(4) SCL上升沿对方采样        */
        if(dev->delay_pf != 0)
        { 
            dev->delay_pf(dev->delayus); /* (5) SCL高保持时间,数据保持时间 */
        }
        tmp <<= 1;                       /* 处理下一位           */
    }
    dev->scl_write(0);                   /* (6)SCL归0  完成8个CLK */
    dev->sda_2read();                    /* [7]SDA转为读          */
    if(dev->delay_pf != 0)
    { 
        dev->delay_pf(dev->delayus);     /* (8)第九个时钟拉低时间  */
    }
    dev->scl_write(1);                   /* (9)SCL上升沿          */
    ack = dev->sda_read();               /* [10]上升沿后读         */
    if(dev->delay_pf != 0)
    { 
        dev->delay_pf(dev->delayus);     /* (11)第九个时钟高保持    */
    }
    dev->scl_write(0);                   /* (12)恢复SCL到低        */
    return (ack==0) ? 0 : -2;
}

/**
 *   |       B0               | B1~B6|    B7               |           NACK/ACK      |
 *                 ___________      _            __________              ____________
 *    ____________|           |   x  |__________|          |____________|            |
 * (1)[2]      (4)[5]                                    (7)[8]       (10)         (12)
 *        (3)           (6)                                     (9)           (11) 
 * 其中(1)(7)(12)拉低SCL;(4)(10)拉高SCL；
 * [2]转为读  [5]读 [8]输出ACK；
 * (3)(9)低保持时间,(6)(11)高保持时间。
 */
int io_iic_read(io_iic_dev_st* dev, uint8_t* val, uint8_t ack)
{
    uint8_t tmp = 0;
    if((dev == 0) || (val == 0))
    {
        return -1;
    }
    if((dev->scl_write == 0) || (dev->sda_write == 0) || (dev->sda_read == 0) || (dev->sda_2read == 0))
    {
        return -1;
    }
    /* SCL下降沿后对方准备数据,上升沿读数据,高位在前 */
    for(uint8_t i=0; i<8; i++)
    {
        tmp <<= 1;                      /* 处理下一位,先移动后读取             */
        dev->scl_write(0);              /* (1)                               */
        dev->sda_2read();               /* [2]转为读输入高阻,以便对方能输出     */
        if(dev->delay_pf != 0)           
        {
            dev->delay_pf(dev->delayus);/* (3)SCL低保持时间                    */
        }
        dev->scl_write(1);              /* (4)SCL上升沿                        */
        if(dev->sda_read())             /* (5)读数据(SCL低时对方已经准备好数据)  */
        { 
            tmp |= 0x01;                /* 高位在前,最后左移到高位              */
        }      
        if(dev->delay_pf != 0)          
        { 
            dev->delay_pf(dev->delayus);/* (6)SCL高保持时间                     */
        }
    }
    dev->scl_write(0);                  /* (7)恢复SCL时钟为低                   */
    dev->sda_write(ack);                /* [8]准备ACK信号(SCL低才能更行SDL)      */
    if(dev->delay_pf != 0)
    { 
        dev->delay_pf(dev->delayus);    /* (9)第九个SCL拉低时间                  */
    }
    dev->scl_write(1);                  /* (10)SCL上升沿发数据触发对方读          */
    if(dev->delay_pf != 0)
    { 
        dev->delay_pf(dev->delayus);    /* (11)第九个SCL拉高保持时间              */
    }
    dev->scl_write(0);                  /* (12)第九个SCL完成恢复低                */
    //dev->sda_write(1);                /* 这里无需驱动SDA,后面可能还是读),需要发送时再驱动 */
    *val = tmp;
    return 0;
}

void io_iic_init(io_iic_dev_st* dev)
{
    if((dev != 0) && (dev->init != 0))
    {
        dev->init();
    }
}

void io_iic_deinit(io_iic_dev_st* dev)
{
    if((dev != 0) && (dev->deinit != 0))
    { 
        dev->deinit();
    }
}