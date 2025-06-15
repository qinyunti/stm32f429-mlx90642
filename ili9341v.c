#include <string.h>
#include "ili9341v.h"

/**
 * \struct ili9341v_cmd_st
 * 命令结构体
*/
typedef struct
{
    uint8_t    cmd;         /**< 命令                  */
    uint8_t    data[5];     /**< 参数,最多5个参数       */
    uint8_t    datalen;     /**< 参数长度              */
    uint16_t   delay;       /**< 延时时间              */
} ili9341v_cmd_st;

static ili9341v_cmd_st s_ili9341v_cmd_init_list[]=
{
    {ILI9341V_CMD_SLPOUT,{0   },0,120},  /**< SLPOUT (11h): Sleep Out */
    {ILI9341V_CMD_MADCTL,{0x08   },1,0},    /**< MADCTL (36h): Memory Data Access Control */
    {ILI9341V_CMD_COLMOD,{0x55},1,0},    /**< 16bit/pixel 65K         */
    //{ILI9341V_CMD_PORCTRL,{0x0C,0x0C,0x00,0x33,0x33},5,0},  /**< 默认值 */
    //{ILI9341V_CMD_GCTRL,{0x35},1,0},                        /**< 默认值 */

    {ILI9341V_CMD_VCOMS,{0x28},1,0},                          /**< 0x28(1.1V) 默认值是0x20(0.9) */
    {ILI9341V_CMD_INVOFF,{0x00},0,0},
    //{ILI9341V_CMD_LCMCTRL,{0x2C},1,0},                      /**< 默认值       */
    //{ILI9341V_CMD_VDVVRHEN,{0x01,0xFF},2,0},                /**< 默认值       */
    //{ILI9341V_CMD_VRHS,{0x0B},1,0},                         /**< 默认值       */
    //{ILI9341V_CMD_VDVS,{0x20},1,0},                         /**< 默认值       */
    //{ILI9341V_CMD_FRCTRL2,{0x0F},1,0},                      /**< 默认值  (60Hz帧率)   */
    //{ILI9341V_CMD_PWCTRL1,{0xA4,0xA1},2,0},                 /**< 默认值       */

    //{ILI9341V_CMD_PVGAMCTRL,{0xD0,0xD1,0x08,0x0F,0x11,0x2A,0x36,0x55,0x44,0x3A,0x0B,0x06,0x11,0x20},2,0},
    //{ILI9341V_CMD_NVGAMCTRL,{0xD0,0x02,0x07,0x0A,0x0B,0x18,0x34,0x43,0x4A,0x2B,0x1B,0x1C,0x22,0x1F},2,0},

    //{0xE4,{0x1D,0,0},3,0},
    //{0x21,{0,0,0},0,0},
    //{ILI9341V_CMD_NORON, {0,0},0,0},  /**< NORON (13h): Normal Display Mode On */
    {ILI9341V_CMD_ITF_CTL,{0x01,0x00,0x00},3,0},
    {ILI9341V_CMD_DISPON,{0},0,0},    /**< DISPON (29h): Display On */
};

/**
 * \fn ili9341v_write_cmd
 * 写命令
 * \param[in] dev \ref ili9341v_dev_st
 * \param[in] cmd 命令字节
 * \retval 0 成功
 * \retval 其他值 失败
*/
static int ili9341v_write_cmd(ili9341v_dev_st* dev,uint8_t cmd)
{
    uint8_t tmp;
#if ILI9341V_CHECK_PARAM
    if(dev == (ili9341v_dev_st*)0)
    {
        return -1;
    }
    if(dev->set_dcx == (ili9341v_set_dcx_pf)0)
    {
        return -1;
    }
    if(dev->write == (ili9341v_spi_write_pf)0)
    {
        return -1;
    }
#endif
    tmp = cmd;
    dev->enable(1);
    dev->set_dcx(0);
    dev->write(&tmp,1);
    dev->enable(0);
    return 0;
}

/**
 * \fn ili9341v_write_data
 * 写数据
 * \param[in] dev \ref ili9341v_dev_st
 * \param[in] data 待写入数据
 * \param[in] len 待写入数据长度
 * \retval 0 成功
 * \retval 其他值 失败
*/
static int ili9341v_write_data(ili9341v_dev_st* dev,uint8_t* data, uint32_t len)
{
#if ILI9341V_CHECK_PARAM
    if(dev == (ili9341v_dev_st*)0)
    {
        return -1;
    }
    if(dev->set_dcx == (ili9341v_set_dcx_pf)0)
    {
        return -1;
    }
    if(dev->write == (ili9341v_spi_write_pf)0)
    {
        return -1;
    }
#endif
    dev->enable(1);
    dev->set_dcx(1);
    dev->write(data,len);
    dev->enable(0);
    return 0;
}

/**
 * \fn ili9341v_set_windows
 * 设置窗口范围(行列地址)
 * \param[in] dev \ref ili9341v_dev_st
 * \param[in] data 待写入数据
 * \param[in] len 待写入数据长度
 * \retval 0 成功
 * \retval 其他值 失败
*/
static int ili9341v_set_windows(ili9341v_dev_st* dev, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
    uint8_t data[4];
    ili9341v_write_cmd(dev, ILI9341V_CMD_CASET);
    data[0] = (x0>>8) & 0xFF;  /* 列开始地址 大端 */
    data[1] = x0 & 0xFF;
    data[2] = (x1>>8) & 0xFF;  /* 列结束地址 大端 */
    data[3] = x1 & 0xFF;
    ili9341v_write_data(dev, data, 4);

    ili9341v_write_cmd(dev, ILI9341V_CMD_RASET);
    data[0] = (y0>>8) & 0xFF;  /* 行开始地址 大端 */
    data[1] = y0 & 0xFF;
    data[2] = (y1>>8) & 0xFF;  /* 行结束地址 大端 */
    data[3] = y1 & 0xFF;
    ili9341v_write_data(dev, data, 4);

    return 0;
}

/**
 * \fn ili9341v_sync
 * 现存写入ili9341v
 * \param[in] dev \ref ili9341v_dev_st
 * \paran[in] x0 列开始地址
 * \paran[in] x1 列结束地址
 * \paran[in] y0 行开始地址
 * \paran[in] y1 行结束地址 
 * \paran[in] buffer 待写入数据 
 * \paran[in] len 待写入数据长度 
 * \retval 0 成功
 * \retval 其他值 失败
*/
int ili9341v_sync(ili9341v_dev_st* dev, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1, uint16_t* buffer, uint32_t len)
{
    (void)dev;
    ili9341v_set_windows(dev, x0, x1, y0, y1);
    ili9341v_write_cmd(dev,ILI9341V_CMD_RAMWR);
    ili9341v_write_data(dev, (uint8_t*)buffer, len);
    return 0;
}

/**
 * \fn ili9341v_init
 * 初始化
 * \param[in] dev \ref ili9341v_dev_st
 * \retval 0 成功
 * \retval 其他值 失败
*/
int ili9341v_init(ili9341v_dev_st* dev)
{
#if ILI9341V_CHECK_PARAM
    if(dev == (ili9341v_dev_st*)0)
    {
        return -1;
    }
#endif
    if(dev->init != 0)
    {
        dev->init();
    }
    dev->set_reset(1);
    dev->delay(10);
    dev->set_reset(0);
    dev->delay(10);
    dev->set_reset(1);
    dev->delay(120);

		if(dev->buffer != 0){
			memset(dev->buffer,0,sizeof(ILI9341V_HSIZE*ILI9341V_VSIZE*2));
		}
    /* 初始化序列 */
    for(uint32_t i=0; i<sizeof(s_ili9341v_cmd_init_list)/sizeof(s_ili9341v_cmd_init_list[0]); i++)
    {
        ili9341v_write_cmd(dev, s_ili9341v_cmd_init_list[i].cmd);
        if(s_ili9341v_cmd_init_list[i].datalen > 0)
        {
            ili9341v_write_data(dev, s_ili9341v_cmd_init_list[i].data,s_ili9341v_cmd_init_list[i].datalen);
            if(s_ili9341v_cmd_init_list[i].delay > 0)
            {
                dev->delay(s_ili9341v_cmd_init_list[i].delay);
            }
        }
    }

		uint16_t tmp = 0x00;
		ili9341v_set_windows(dev, 0, 240-1, 0, 320-1);
    ili9341v_write_cmd(dev,ILI9341V_CMD_RAMWR);
		for(int i=0; i<240*320; i++){
			ili9341v_write_data(dev, (uint8_t*)(&tmp), 2);
		}
    return 0;
}

/**
 * \fn ili9341v_deinit
 * 解除初始化
 * \param[in] dev \ref ili9341v_dev_st
 * \return 总是返回0
*/
int ili9341v_deinit(ili9341v_dev_st* dev)
{
#if ILI9341V_CHECK_PARAM
    if(dev == (ili9341v_dev_st*)0)
    {
        return -1;
    }
#endif

    /* @todo 添加IO等解除初始化配置 */

    if(dev->deinit != 0)
    {
        dev->deinit();
    }
    return 0;
}