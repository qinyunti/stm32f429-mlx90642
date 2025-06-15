#include "stm32f4_regs.h"
#include "gpio.h"
#include "clock.h"
#include "ili9341v.h"
#include "lcd_itf.h"
#include "spi.h"

#define LCD_SPI  5
#define LCD_HSIZE ILI9341V_HSIZE
#define LCD_VSIZE ILI9341V_VSIZE

static void port_lcd_set_dcx(uint8_t val)
{
	if(val){
		gpio_write((void*)GPIOA_BASE, 'D', 13, 1);
	}else{
		gpio_write((void*)GPIOA_BASE, 'D', 13, 0);
	}
}                

static void port_lcd_set_bk(uint8_t val)
{

}

static void port_lcd_set_reset(uint8_t val)
{

}

static void port_lcd_spi_write(uint8_t* buffer, uint32_t len)
{
	spi_transfer(LCD_SPI,buffer,0,len,1);
}    

static void port_lcd_spi_enable(uint8_t val)
{
	(void)val;
}    

static void port_lcd_delay_ms(uint32_t t)
{
	clock_delay(t);
}

static void port_lcd_init(void)
{
	volatile uint32_t *RCC_AHB1ENR = (void *)(RCC_BASE + 0x30);
	/* 
     * PD13 数据/命令 GPIO
	 */
	*RCC_AHB1ENR |= (1u<<3); /* GPIOD */
	*RCC_AHB1ENR |= (1u<<2); /* GPIOC */

	gpio_set((void*)GPIOA_BASE, 'D', 13, 0, GPIOx_MODER_MODERy_GPOUTPUT,GPIOx_OSPEEDR_OSPEEDRy_HIGH, GPIOx_PUPDR_PULLUP);
	gpio_write((void*)GPIOA_BASE, 'D', 13, 1);
	
	spi_cfg_st cfg={
		.baud = 90000000ul,
		.databits = 8,
		.mode = 3,
		.msb = 1,
	};
	spi_init(LCD_SPI, &cfg);

	port_lcd_set_bk(0);
	port_lcd_set_dcx(1);
	
	port_lcd_set_reset(0);
	/* 延时 */
	port_lcd_set_reset(1);
	/* 延时 */
	port_lcd_set_bk(1);
}           

static void port_lcd_deinit(void)
{

}        
    
/******************************************************************************
 *                        以下是LCD设备实例
 * 
******************************************************************************/

/* 设备实例 */
static ili9341v_dev_st s_lcd_itf_dev =
{
    .set_dcx = port_lcd_set_dcx,
    .set_reset = port_lcd_set_reset,
    .write = port_lcd_spi_write,
    .enable = port_lcd_spi_enable,
    .delay = port_lcd_delay_ms,
    .init = port_lcd_init,
    .deinit = port_lcd_deinit,

    .buffer = (uint16_t*)0,
};

/******************************************************************************
 *                        以下是对外操作接口
 * 
******************************************************************************/


/**
 * \fn lcd_itf_init
 * 初始化
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_init(void)
{
    return ili9341v_init(&s_lcd_itf_dev);
}

/**
 * \fn lcd_itf_deinit
 * 解除初始化
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_deinit(void)
{
    return ili9341v_deinit(&s_lcd_itf_dev);
}

/**
 * \fn lcd_itf_sync
 * 刷新显示
 * \retval 0 成功
 * \retval 其他值 失败
*/
int lcd_itf_sync(void)
{
    return ili9341v_sync(&s_lcd_itf_dev, 0, LCD_HSIZE-1, 0, LCD_VSIZE-1, s_lcd_itf_dev.buffer, LCD_HSIZE*LCD_VSIZE*2);
}

/**
 * \fn lcd_itf_set_pixel
 * 写点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel(uint16_t x, uint16_t y, uint16_t rgb565)
{
    //if(x >= LCD_HSIZE)
    //{
    //    return -1;
    //}
    //if(y >= LCD_VSIZE)
    //{
    //    return -1;
    //}
    s_lcd_itf_dev.buffer[y*LCD_HSIZE + x] = (uint16_t)((rgb565>>8)&0xFF) | (uint16_t)((rgb565<<8) & 0xFF00);
}

/**
 * \fn lcd_itf_set_pixel_0
 * 写点
 * \param[in] offset 偏移位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel_0(uint32_t offset, uint16_t rgb565)
{
    s_lcd_itf_dev.buffer[offset] = (uint16_t)((rgb565>>8)&0xFF) | (uint16_t)((rgb565<<8) & 0xFF00);
}

/**
 * \fn lcd_itf_get_pixel
 * 读点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \return rgb565颜色
*/
uint16_t lcd_itf_get_pixel(uint16_t x, uint16_t y)
{
    uint16_t color = s_lcd_itf_dev.buffer[y*LCD_HSIZE + x]; 
    return ((uint16_t)(color>>8) | (uint16_t)(color<<8));
}

/**
 * \fn lcd_itf_fill_direct
 * 直接填充区域
 * \param[in] x x开始坐标位置
 * \param[in] w 宽度
 * \param[in] y y开始坐标位置
 * \param[in] h 高度
 * \param[in] buffer rgb565颜色缓存区
*/
void lcd_itf_fill_direct(uint16_t x, uint16_t w, uint16_t y, uint16_t h, uint16_t* buffer)
{
		ili9341v_sync(&s_lcd_itf_dev, x, x+w-1, y, y+h-1, buffer, w*h*2);
}

/**
 * \fn lcd_itf_set_pixel_direct
 * 直接写点
 * \param[in] x x坐标位置
 * \param[in] y y坐标位置
 * \param[in] rgb565 颜色
*/
void lcd_itf_set_pixel_direct(uint16_t x, uint16_t y, uint16_t rgb565)
{
		uint16_t tmp = rgb565;
		ili9341v_sync(&s_lcd_itf_dev, x, x, y, y, &tmp, 2);
}
